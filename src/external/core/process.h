#pragma once

// Finding cs2.exe, opening a handle to it, and locating its modules (Toolhelp32).

#include <cstdint>
#include <optional>
#include <string_view>

#include <Windows.h>

namespace core
{
// --diag and --live only read. The overlay adds write access (Phase 5) for its only game writes: the view angles
// (aimbot) and the attack button (triggerbot), each a single field (game/writes).
inline constexpr DWORD kReadOnlyAccess = PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION;
inline constexpr DWORD kReadWriteAccess = kReadOnlyAccess | PROCESS_VM_WRITE | PROCESS_VM_OPERATION;

// RAII owner of a kernel HANDLE closed with CloseHandle. Holds nullptr when empty (INVALID_HANDLE_VALUE is normalised
// to nullptr on construction).
class UniqueHandle
{
public:
    UniqueHandle() noexcept = default;
    explicit UniqueHandle(HANDLE handle) noexcept;
    ~UniqueHandle();

    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;
    UniqueHandle(UniqueHandle&& other) noexcept;
    UniqueHandle& operator=(UniqueHandle&& other) noexcept;

    [[nodiscard]] HANDLE get() const noexcept { return handle_; }
    [[nodiscard]] explicit operator bool() const noexcept { return handle_ != nullptr; }

private:
    void reset() noexcept;

    HANDLE handle_ = nullptr;
};

struct ModuleInfo
{
    std::uintptr_t base = 0;
    std::uint32_t size = 0;
};

struct OpenResult
{
    UniqueHandle handle;
    DWORD error = ERROR_SUCCESS; // GetLastError() from OpenProcess when `handle` is empty
};

// PID of the first running process whose exe name matches `exe_name` (case-insensitive), if any.
[[nodiscard]] std::optional<DWORD> find_process(std::wstring_view exe_name);

// OpenProcess with exactly `access`.
[[nodiscard]] OpenResult open_handle(DWORD pid, DWORD access);

// Base address and size of `module_name` (case-insensitive) in process `pid`, if it is loaded.
[[nodiscard]] std::optional<ModuleInfo> module_base(DWORD pid, std::wstring_view module_name);

// The process's main window: its largest visible, unowned top-level window. nullptr while it has none (still loading).
[[nodiscard]] HWND find_main_window(DWORD pid) noexcept;

// True while the process behind `process` (opened with PROCESS_QUERY_LIMITED_INFORMATION) is still running.
[[nodiscard]] bool is_running(HANDLE process) noexcept;
} // namespace core
