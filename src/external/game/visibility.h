#pragma once

// The visibility heuristic (option 2 of the three in CLAUDE.md §13 Phase 4): the spotted-by mask.
//
// C_CSPlayerPawn::m_entitySpottedState.m_bSpottedByMask is uint32[2], a bit per player slot, set by the game when
// that player has line of sight to this pawn (it feeds the radar). Read as one little-endian uint64, bit n is slot n.
// A player's slot is its controller's entity index - 1. It is "spotted", not a per-frame ray: it lags a little, and
// an exact line-of-sight test would need the game's own trace (internal, out of scope).
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <optional>

#include "core/memory.h"

namespace game
{
// The slot of the player whose controller has entity index `controller_index` (1..64), as the mask numbers them.
[[nodiscard]] constexpr std::uint32_t player_slot(std::uint32_t controller_index) noexcept
{
    return controller_index - 1;
}

// True if `mask` says player slot `slot` has spotted the pawn.
[[nodiscard]] constexpr bool is_spotted_by(std::uint64_t mask, std::uint32_t slot) noexcept
{
    return slot < 64 && ((mask >> slot) & 1u) != 0;
}

// The pawn's spotted-by mask (both uint32 halves in one read), or nullopt if the read fails.
[[nodiscard]] std::optional<std::uint64_t> read_spotted_by_mask(const core::Memory& memory,
                                                                std::uintptr_t pawn) noexcept;
} // namespace game
