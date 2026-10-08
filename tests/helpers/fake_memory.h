#pragma once

// A core::Memory backed by local buffers, so game/ code can be tested without the game.
//
// Map regions at made-up "remote" addresses, put values in them, then hand the FakeMemory to the code under test.
// A read or write succeeds only if the whole range lies inside one mapped region, like RPM on unmapped pages.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string_view>
#include <utility>
#include <vector>

#include <doctest.h>

#include "core/memory.h"

namespace test
{
class FakeMemory final : public core::Memory
{
public:
    // Maps `size` zeroed bytes at `base`. Regions must not overlap.
    void map(std::uintptr_t base, std::size_t size) { regions_[base] = std::vector<std::byte>(size); }

    // Puts `value` at `address` (which must already be mapped). Not counted as a write.
    template <core::RemoteValue T>
    void put(std::uintptr_t address, const T& value)
    {
        std::byte* target = find(address, sizeof(T));
        REQUIRE_MESSAGE(target != nullptr, "FakeMemory::put outside every mapped region");
        std::memcpy(target, &value, sizeof(T));
    }

    // Puts `size` raw bytes at `address` (which must already be mapped). Not counted as a write.
    void put_bytes(std::uintptr_t address, const void* data, std::size_t size)
    {
        std::byte* target = find(address, size);
        REQUIRE_MESSAGE(target != nullptr, "FakeMemory::put_bytes outside every mapped region");
        std::memcpy(target, data, size);
    }

    // Puts `text` and its NUL terminator at `address`.
    void put_string(std::uintptr_t address, std::string_view text)
    {
        put_bytes(address, text.data(), text.size());
        put<char>(address + text.size(), '\0');
    }

    // Maps a region just big enough for `value` and puts it there.
    template <core::RemoteValue T>
    void map_value(std::uintptr_t address, const T& value)
    {
        map(address, sizeof(T));
        put(address, value);
    }

    [[nodiscard]] std::size_t read_count() const noexcept { return read_count_; }
    [[nodiscard]] std::size_t write_count() const noexcept { return write_count_; }

private:
    bool do_read(std::uintptr_t address, void* buffer, std::size_t size) const noexcept override
    {
        ++read_count_;
        const std::byte* source = find(address, size);
        if (source == nullptr)
        {
            return false;
        }
        std::memcpy(buffer, source, size);
        return true;
    }

    bool do_write(std::uintptr_t address, const void* buffer, std::size_t size) noexcept override
    {
        ++write_count_;
        std::byte* target = find(address, size);
        if (target == nullptr)
        {
            return false;
        }
        std::memcpy(target, buffer, size);
        return true;
    }

    // Pointer to the local bytes backing [address, address + size), or nullptr if that range isn't fully mapped.
    const std::byte* find(std::uintptr_t address, std::size_t size) const noexcept
    {
        auto it = regions_.upper_bound(address);
        if (it == regions_.begin())
        {
            return nullptr;
        }
        --it;
        const std::uintptr_t offset = address - it->first;
        const std::vector<std::byte>& bytes = it->second;
        if (offset > bytes.size() || size > bytes.size() - offset)
        {
            return nullptr;
        }
        return bytes.data() + offset;
    }

    std::byte* find(std::uintptr_t address, std::size_t size) noexcept
    {
        return const_cast<std::byte*>(std::as_const(*this).find(address, size));
    }

    std::map<std::uintptr_t, std::vector<std::byte>> regions_;
    mutable std::size_t read_count_ = 0;
    std::size_t write_count_ = 0;
};
} // namespace test
