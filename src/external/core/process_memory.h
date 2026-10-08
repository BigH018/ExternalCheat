#pragma once

// core::Memory for a real process: ReadProcessMemory / WriteProcessMemory on a handle someone else owns
// (core::UniqueHandle in main.cpp). The only code in the project that calls RPM/WPM.

#include <cstddef>
#include <cstdint>

#include <Windows.h>

#include "core/memory.h"

namespace core
{
class ProcessMemory final : public Memory
{
public:
    // `process` must outlive this object. Reads need PROCESS_VM_READ; writes need PROCESS_VM_WRITE |
    // PROCESS_VM_OPERATION.
    explicit ProcessMemory(HANDLE process) noexcept : process_(process) {}

private:
    [[nodiscard]] bool do_read(std::uintptr_t address, void* buffer, std::size_t size) const noexcept override;
    [[nodiscard]] bool do_write(std::uintptr_t address, const void* buffer, std::size_t size) noexcept override;

    HANDLE process_ = nullptr;
};
} // namespace core
