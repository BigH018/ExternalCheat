#pragma once
#include <Windows.h>
#include <winioctl.h>
#include <string>
#include <cstdint>

// IOCTL Definitions (Must match your driver)
constexpr ULONG init_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9A1, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
constexpr ULONG read_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9A2, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
constexpr ULONG write_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9A3, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
constexpr ULONG get_pid_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9A4, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
constexpr ULONG get_module_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9A5, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

struct info_t {
    HANDLE target_pid = 0;
    void* target_address = 0x0;
    void* buffer_address = 0x0;
    SIZE_T size = 0;
    SIZE_T return_size = 0;
    wchar_t process_name[256] = { 0 };
    wchar_t module_name[256] = { 0 };
    void* module_base = 0x0;
    SIZE_T module_size = 0;
};

class KernelInterface {
private:
    HANDLE hDriver;

public:
    KernelInterface() : hDriver(INVALID_HANDLE_VALUE) {}

    ~KernelInterface() {
        if (hDriver != INVALID_HANDLE_VALUE) {
            CloseHandle(hDriver);
        }
    }

    bool Connect() {
        hDriver = CreateFileA(
            "\\\\.\\x9f2a3b",
            GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
        );
        return hDriver != INVALID_HANDLE_VALUE;
    }

    bool Attach(DWORD pid) const {
        if (hDriver == INVALID_HANDLE_VALUE) return false;
        info_t request = { 0 };
        request.target_pid = (HANDLE)(ULONG_PTR)pid;
        DWORD bytesReturned = 0;
        return DeviceIoControl(hDriver, init_code, &request, sizeof(request), &request, sizeof(request), &bytesReturned, NULL) != 0;
    }

    bool ReadMemory(void* address, void* buffer, size_t size) const {
        if (hDriver == INVALID_HANDLE_VALUE) return false;
        info_t request = { 0 };
        request.target_address = address;
        request.buffer_address = buffer;
        request.size = size;
        DWORD bytesReturned = 0;
        return DeviceIoControl(hDriver, read_code, &request, sizeof(request), &request, sizeof(request), &bytesReturned, NULL) != 0;
    }

    bool WriteMemory(void* address, const void* buffer, size_t size) const {
        if (hDriver == INVALID_HANDLE_VALUE) return false;
        info_t request = { 0 };
        request.target_address = address;
        request.buffer_address = const_cast<void*>(buffer);
        request.size = size;
        DWORD bytesReturned = 0;
        return DeviceIoControl(hDriver, write_code, &request, sizeof(request), &request, sizeof(request), &bytesReturned, NULL) != 0;
    }

    DWORD GetPid(const wchar_t* process_name) const {
        if (hDriver == INVALID_HANDLE_VALUE) return 0;
        info_t request = { 0 };
        wcsncpy_s(request.process_name, process_name, 255);
        DWORD bytesReturned = 0;
        if (DeviceIoControl(hDriver, get_pid_code, &request, sizeof(request), &request, sizeof(request), &bytesReturned, NULL)) {
            return (DWORD)(ULONG_PTR)request.target_pid;
        }
        return 0;
    }

    void* GetModuleBase(DWORD pid, const wchar_t* module_name, SIZE_T& out_size) const {
        if (hDriver == INVALID_HANDLE_VALUE) return nullptr;
        info_t request = { 0 };
        request.target_pid = (HANDLE)(ULONG_PTR)pid;
        wcsncpy_s(request.module_name, module_name, 255);
        DWORD bytesReturned = 0;
        if (DeviceIoControl(hDriver, get_module_code, &request, sizeof(request), &request, sizeof(request), &bytesReturned, NULL)) {
            out_size = request.module_size;
            return request.module_base;
        }
        out_size = 0;
        return nullptr;
    }
};