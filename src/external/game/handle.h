#pragma once

// The chunked entity list and CHandle resolution (layout in game/offsets.h, offsets::layout).
//
//   chunk    = read(entity_system + 0x10 + (index >> 9) * 8)
//   identity = chunk + (index & 0x1FF) * 0x70                  (an address, not a read)
//   entity   = read(identity + 0x0)
//   a handle is current only if read(identity + 0x10) == handle (same index and serial)
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <optional>

#include "core/memory.h"
#include "game/offsets.h"

namespace game
{
[[nodiscard]] constexpr std::uint32_t handle_index(std::uint32_t handle) noexcept
{
    return handle & offsets::layout::kHandleIndexMask;
}

// False for 0xFFFFFFFF (the game's "no entity") and anything whose index is out of the list's range.
[[nodiscard]] constexpr bool is_valid_handle(std::uint32_t handle) noexcept
{
    return handle != offsets::layout::kInvalidHandle && handle_index(handle) != offsets::layout::kHandleIndexMask;
}

// The entity system: client.dll + dwEntityList. nullopt if the read fails or the pointer isn't plausible.
[[nodiscard]] std::optional<std::uintptr_t> read_entity_system(const core::Memory& memory,
                                                               std::uintptr_t client_base) noexcept;

// The address of entity `index`'s identity, or nullopt if its chunk isn't allocated.
[[nodiscard]] std::optional<std::uintptr_t> identity_address(const core::Memory& memory, std::uintptr_t entity_system,
                                                             std::uint32_t index) noexcept;

// The entity at `index`, or 0 if there is none (empty slot, unallocated chunk, failed read).
[[nodiscard]] std::uintptr_t entity_at(const core::Memory& memory, std::uintptr_t entity_system,
                                       std::uint32_t index) noexcept;

// The entity `handle` refers to, or 0 if the handle is invalid or stale (its slot now holds a different entity).
[[nodiscard]] std::uintptr_t resolve_handle(const core::Memory& memory, std::uintptr_t entity_system,
                                            std::uint32_t handle) noexcept;
} // namespace game
