#include "core/process_memory.h"

namespace core
{
// ReadProcessMemory/WriteProcessMemory report a bad *remote* address through their return value, not an exception.
// The __try guard covers our side of the copy (a bad local buffer), so a mistake here can never crash the tool.
// No C++ objects with destructors may live in these functions (C2712).

bool ProcessMemory::do_read(std::uintptr_t address, void* buffer, std::size_t size) const noexcept
{
    if (process_ == nullptr)
    {
        return false;
    }

    SIZE_T bytes_read = 0;
    BOOL ok = FALSE;
    __try
    {
        ok = ReadProcessMemory(process_, reinterpret_cast<LPCVOID>(address), buffer, size, &bytes_read);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    return ok != FALSE && bytes_read == size;
}

bool ProcessMemory::do_write(std::uintptr_t address, const void* buffer, std::size_t size) noexcept
{
    if (process_ == nullptr)
    {
        return false;
    }

    SIZE_T bytes_written = 0;
    BOOL ok = FALSE;
    __try
    {
        ok = WriteProcessMemory(process_, reinterpret_cast<LPVOID>(address), buffer, size, &bytes_written);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    return ok != FALSE && bytes_written == size;
}
} // namespace core
