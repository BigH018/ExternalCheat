#pragma once

// Signature scanning: IDA-style byte patterns ("48 8B 05 ? ? ? ? 41 89 BE") matched against a local copy of a remote
// module. Copying first and scanning locally keeps it to a few dozen large reads instead of millions of small ones.
//
// PURE: no <Windows.h>. Remote reads go through core::Memory.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "core/memory.h"

namespace core
{
class Pattern
{
public:
    // Hex bytes and wildcards (? or ??) separated by whitespace. nullopt if a token is neither, or the pattern is empty
    // or only wildcards.
    [[nodiscard]] static std::optional<Pattern> parse(std::string_view text);

    [[nodiscard]] std::size_t size() const noexcept { return bytes_.size(); }

    // True if the pattern matches `data` starting at `position` (false if it would run past the end).
    [[nodiscard]] bool matches_at(std::span<const std::byte> data, std::size_t position) const noexcept;

    // Every position in `data` where the pattern matches, in order, at most `max_hits` of them.
    [[nodiscard]] std::vector<std::size_t> find_all(std::span<const std::byte> data, std::size_t max_hits) const;

private:
    static constexpr std::int16_t kWildcard = -1;

    std::vector<std::int16_t> bytes_; // 0..255, or kWildcard
    std::size_t anchor_ = 0;          // the first non-wildcard byte: candidates are found with memchr on it
};

// A local copy of a remote address range.
struct RemoteCopy
{
    std::uintptr_t base = 0;
    std::vector<std::byte> bytes;
    std::size_t unreadable_pages = 0; // pages that couldn't be read; zero-filled in `bytes`

    [[nodiscard]] bool contains(std::uintptr_t address, std::size_t size) const noexcept
    {
        return address >= base && address - base <= bytes.size() && size <= bytes.size() - (address - base);
    }

    // The copied bytes of [address, address + size), or an empty span if that isn't inside the copy.
    [[nodiscard]] std::span<const std::byte> view(std::uintptr_t address, std::size_t size) const noexcept
    {
        if (!contains(address, size))
        {
            return {};
        }
        return std::span<const std::byte>(bytes).subspan(address - base, size);
    }
};

// Copies [base, base + size) in `chunk_size` reads. A chunk that can't be read is retried page by page; pages that
// still fail are zero-filled and counted in `unreadable_pages`.
[[nodiscard]] RemoteCopy copy_remote(const Memory& memory, std::uintptr_t base, std::size_t size,
                                     std::size_t chunk_size = config::kRemoteCopyChunk);

// Where a RIP-relative operand points: the instruction at `instruction` is `instruction_size` bytes long, so RIP is
// instruction + instruction_size, and the operand is RIP + disp32. Wraps like the CPU would.
[[nodiscard]] constexpr std::uintptr_t rip_relative(std::uintptr_t instruction, std::uint32_t instruction_size,
                                                    std::int32_t disp32) noexcept
{
    return instruction + instruction_size + static_cast<std::uintptr_t>(static_cast<std::intptr_t>(disp32));
}

// rip_relative for an instruction inside `copy`, reading its disp32 at instruction + disp_offset from the copy.
// nullopt if the displacement isn't inside the copy.
[[nodiscard]] std::optional<std::uintptr_t> rip_relative_target(const RemoteCopy& copy, std::uintptr_t instruction,
                                                                std::uint32_t disp_offset,
                                                                std::uint32_t instruction_size) noexcept;
} // namespace core
