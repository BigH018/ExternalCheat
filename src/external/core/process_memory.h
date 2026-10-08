#pragma once

// core::Memory for a real process: Uses a kernel driver for memory access.
// The only code in the project that calls the driver's IOCTLs.

#include <cstddef>
#include <cstdint>

#include <Windows.h>

#include "core/memory.h"
#include "core/kernel_interface.h"

namespace core
{
    class ProcessMemory final : public Memory
    {
    public:
        // `kernel` must outlive this object.
        explicit ProcessMemory(KernelInterface& kernel) noexcept : kernel_(kernel) {}

    private:
        [[nodiscard]] bool do_read(std::uintptr_t address, void* buffer, std::size_t size) const noexcept override;
        [[nodiscard]] bool do_write(std::uintptr_t address, const void* buffer, std::size_t size) noexcept override;

        KernelInterface& kernel_;
    };
} // namespace core