#include "core/process_memory.h"

namespace core
{
    // The kernel driver validates all remote addresses internally, so we no longer
    // need __try/__except guards. A bad address simply returns false from the driver.

    bool ProcessMemory::do_read(std::uintptr_t address, void* buffer, std::size_t size) const noexcept
    {
        if (buffer == nullptr || size == 0)
        {
            return false;
        }

        return kernel_.ReadMemory(reinterpret_cast<void*>(address), buffer, size);
    }

    bool ProcessMemory::do_write(std::uintptr_t address, const void* buffer, std::size_t size) noexcept
    {
        if (buffer == nullptr || size == 0)
        {
            return false;
        }

        return kernel_.WriteMemory(reinterpret_cast<void*>(address), buffer, size);
    }
} // namespace core