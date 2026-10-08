#pragma once

// Access to another process's memory, behind an interface so game/ code can be unit-tested against a fake.
//
// The real implementation is core/process_memory (ReadProcessMemory / WriteProcessMemory). The tests use
// tests/helpers/fake_memory.h. Code that only reads takes `const Memory&`; only code that writes takes `Memory&`.
//
// PURE: no <Windows.h>.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "config.h"

namespace core
{
// True if `address` looks like an x64 user-space address with the given alignment (1 = any).
// A cheap first filter against null and garbage; it can't tell whether the memory is actually mapped.
[[nodiscard]] constexpr bool is_plausible_pointer(std::uintptr_t address, std::size_t alignment = 1) noexcept
{
    return address >= config::kMinValidPointer && address < config::kMaxValidPointer && alignment != 0 &&
           address % alignment == 0;
}

// True if [address, address + size) is non-empty and lies inside user space.
[[nodiscard]] constexpr bool is_plausible_range(std::uintptr_t address, std::size_t size) noexcept
{
    return size != 0 && is_plausible_pointer(address) && size <= config::kMaxValidPointer - address;
}

// Anything we copy byte-for-byte out of (or into) another process.
template <class T>
concept RemoteValue = std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T>;

class Memory
{
public:
    virtual ~Memory() = default;

    // Copies `size` bytes at `address` into `buffer`. True only if every byte was copied; on false the contents of
    // `buffer` are unspecified (use safe_read / read, which leave the caller's value untouched).
    [[nodiscard]] bool read_bytes(std::uintptr_t address, void* buffer, std::size_t size) const noexcept
    {
        return buffer != nullptr && is_plausible_range(address, size) && do_read(address, buffer, size);
    }

    // Copies `size` bytes from `buffer` to `address`. True only if every byte was written.
    [[nodiscard]] bool write_bytes(std::uintptr_t address, const void* buffer, std::size_t size) noexcept
    {
        return buffer != nullptr && is_plausible_range(address, size) && do_write(address, buffer, size);
    }

    // Reads a T at `address`. On failure returns false and leaves `out` untouched.
    template <RemoteValue T>
    [[nodiscard]] bool safe_read(std::uintptr_t address, T& out) const noexcept
    {
        T value{};
        if (!read_bytes(address, &value, sizeof(T)))
        {
            return false;
        }
        out = value;
        return true;
    }

    // Reads a T at `address`, or nullopt if the read failed.
    template <RemoteValue T>
    [[nodiscard]] std::optional<T> read(std::uintptr_t address) const noexcept
    {
        T value{};
        if (!read_bytes(address, &value, sizeof(T)))
        {
            return std::nullopt;
        }
        return value;
    }

    // Writes `value` at `address`.
    template <RemoteValue T>
    [[nodiscard]] bool safe_write(std::uintptr_t address, const T& value) noexcept
    {
        return write_bytes(address, &value, sizeof(T));
    }

protected:
    Memory() = default;
    Memory(const Memory&) = default;
    Memory& operator=(const Memory&) = default;

private:
    // Called only with a non-null buffer and a plausible, non-empty range.
    [[nodiscard]] virtual bool do_read(std::uintptr_t address, void* buffer, std::size_t size) const noexcept = 0;
    [[nodiscard]] virtual bool do_write(std::uintptr_t address, const void* buffer, std::size_t size) noexcept = 0;
};

// Reads a NUL-terminated string of at most `max_length` characters at `address`. Reads never cross a page boundary
// in one go, so a short string at the very end of a mapped page still reads. nullopt if a read fails or there is no
// terminator within `max_length`.
[[nodiscard]] inline std::optional<std::string> read_string(const Memory& memory, std::uintptr_t address,
                                                            std::size_t max_length)
{
    std::string text;
    char chunk[config::kPageSize];
    while (text.size() < max_length)
    {
        const std::uintptr_t at = address + text.size();
        const std::size_t to_page_end = config::kPageSize - at % config::kPageSize;
        const std::size_t want = std::min({to_page_end, max_length - text.size(), sizeof(chunk)});
        if (!memory.read_bytes(at, chunk, want))
        {
            return std::nullopt;
        }
        const std::string_view got(chunk, want);
        const std::size_t end = got.find('\0');
        text.append(got.substr(0, end));
        if (end != std::string_view::npos)
        {
            return text;
        }
    }
    return std::nullopt;
}
} // namespace core
