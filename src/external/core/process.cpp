#include "core/process.h"

#include <utility>

#include <TlHelp32.h>

namespace core
{
namespace
{
// CreateToolhelp32Snapshot(TH32CS_SNAPMODULE) can fail with ERROR_BAD_LENGTH while the target is loading modules.
// Retrying a few times is the documented fix.
constexpr int kSnapshotAttempts = 5;

bool names_equal(const wchar_t* a, std::wstring_view b) noexcept
{
    return CompareStringOrdinal(a, -1, b.data(), static_cast<int>(b.size()), TRUE) == CSTR_EQUAL;
}

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

std::optional<DWORD> find_process(std::wstring_view exe_name)
{
    const UniqueHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!snapshot)
    {
        return std::nullopt;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    for (BOOL ok = Process32FirstW(snapshot.get(), &entry); ok; ok = Process32NextW(snapshot.get(), &entry))
    {
        if (names_equal(entry.szExeFile, exe_name))
        {
            return entry.th32ProcessID;
        }
    }
    return std::nullopt;
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

std::optional<ModuleInfo> module_base(DWORD pid, std::wstring_view module_name)
{
    UniqueHandle snapshot;
    for (int attempt = 0; attempt < kSnapshotAttempts && !snapshot; ++attempt)
    {
        snapshot = UniqueHandle(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
        if (!snapshot && GetLastError() != ERROR_BAD_LENGTH)
        {
            break;
        }
    }
    if (!snapshot)
    {
        return std::nullopt;
    }

    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    for (BOOL ok = Module32FirstW(snapshot.get(), &entry); ok; ok = Module32NextW(snapshot.get(), &entry))
    {
        if (names_equal(entry.szModule, module_name))
        {
            return ModuleInfo{reinterpret_cast<std::uintptr_t>(entry.modBaseAddr), entry.modBaseSize};
        }
    }
    return std::nullopt;
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
