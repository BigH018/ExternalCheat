#include "core/process.h"

#include <string>
#include <utility>

namespace core
{
    namespace
    {
        struct WindowSearch
        {
            DWORD pid = 0;
            HWND best = nullptr;
            LONG best_area = 0;
        };

        BOOL CALLBACK consider_window(HWND window, LPARAM param) noexcept
        {
            auto& search = *reinterpret_cast<WindowSearch*>(param);
            DWORD owner_pid = 0;
            GetWindowThreadProcessId(window, &owner_pid);
            if (owner_pid != search.pid || !IsWindowVisible(window) || GetWindow(window, GW_OWNER) != nullptr)
            {
                return TRUE;
            }
            RECT client{};
            if (!GetClientRect(window, &client))
            {
                return TRUE;
            }
            const LONG area = (client.right - client.left) * (client.bottom - client.top);
            if (search.best == nullptr || area > search.best_area)
            {
                search.best = window;
                search.best_area = area;
            }
            return TRUE;
        }
    } // namespace

    UniqueHandle::UniqueHandle(HANDLE handle) noexcept : handle_(handle == INVALID_HANDLE_VALUE ? nullptr : handle)
    {
    }

    UniqueHandle::~UniqueHandle()
    {
        reset();
    }

    UniqueHandle::UniqueHandle(UniqueHandle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr))
    {
    }

    UniqueHandle& UniqueHandle::operator=(UniqueHandle&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    void UniqueHandle::reset() noexcept
    {
        if (handle_ != nullptr)
        {
            CloseHandle(handle_);
            handle_ = nullptr;
        }
    }

    std::optional<DWORD> find_process(KernelInterface& kernel, std::wstring_view exe_name)
    {
        // The driver uses partial (substring) matching against the full image path.
        // Passing "cs2.exe" matches the full kernel path that contains that name.
        std::wstring name(exe_name);
        const DWORD pid = kernel.GetPid(name.c_str());
        if (pid == 0)
        {
            return std::nullopt;
        }
        return pid;
    }

    OpenResult open_handle(DWORD pid, DWORD access)
    {
        OpenResult result;
        result.handle = UniqueHandle(OpenProcess(access, FALSE, pid));
        if (!result.handle)
        {
            result.error = GetLastError();
        }
        return result;
    }

    std::optional<ModuleInfo> module_base(KernelInterface& kernel, DWORD pid, std::wstring_view module_name)
    {
        std::wstring name(module_name);
        SIZE_T mod_size = 0;
        void* base = kernel.GetModuleBase(pid, name.c_str(), mod_size);
        if (base == nullptr)
        {
            return std::nullopt;
        }

        // The driver's SizeOfImage can be unreliable on Windows 11 25H2.
        // Read the PE headers directly from the target process to get the real size.
        IMAGE_DOS_HEADER dos = { 0 };
        if (kernel.ReadMemory(base, &dos, sizeof(dos)) && dos.e_magic == IMAGE_DOS_SIGNATURE)
        {
            IMAGE_NT_HEADERS nt = { 0 };
            void* nt_addr = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(base) + dos.e_lfanew);
            if (kernel.ReadMemory(nt_addr, &nt, sizeof(nt)) && nt.Signature == IMAGE_NT_SIGNATURE)
            {
                mod_size = nt.OptionalHeader.SizeOfImage;
            }
        }

        return ModuleInfo{ reinterpret_cast<std::uintptr_t>(base), static_cast<std::uint32_t>(mod_size) };
    }

    HWND find_main_window(DWORD pid) noexcept
    {
        WindowSearch search;
        search.pid = pid;
        EnumWindows(&consider_window, reinterpret_cast<LPARAM>(&search));
        return search.best;
    }

    bool is_running(HANDLE process) noexcept
    {
        DWORD exit_code = 0;
        return GetExitCodeProcess(process, &exit_code) != FALSE && exit_code == STILL_ACTIVE;
    }
} // namespace core