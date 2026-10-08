#pragma once

// Player reads: the local pawn pointer (Phase 0), and snapshots of every player (Phase 3).
//
// A player is two entities: the controller (name, team, a handle to the pawn) and the pawn (health, position,
// weapon). The controller lives as long as the player is in the match; the pawn can be missing or dead.
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <optional>

#include "core/memory.h"
#include "game/entities.h"
#include "game/snapshot.h"

namespace game
{
// Reads the local C_CSPlayerPawn pointer at client.dll + dwLocalPlayerPawn.
// nullopt = the read itself failed; 0 = no local pawn (main menu, loading screen).
[[nodiscard]] std::optional<std::uintptr_t> read_local_pawn(const core::Memory& memory,
                                                            std::uintptr_t client_base) noexcept;

// One player, read from its controller (and the pawn its m_hPlayerPawn handle resolves to). `local_controller` marks
// the local player. nullopt if the controller can't be read. A pawn that reads as garbage (health out of range,
// non-finite position) is dropped: the snapshot then has pawn == 0 and alive == false.
[[nodiscard]] std::optional<PlayerSnapshot> read_player(const core::Memory& memory, std::uintptr_t entity_system,
                                                        const EntityRef& controller, std::uintptr_t local_controller);

// The whole game as it is right now: globals, view matrix (if sane) and every player. Outside a match (main menu,
// loading) this is an empty snapshot with in_match == false; it never fails.
[[nodiscard]] GameSnapshot read_game(const core::Memory& memory, std::uintptr_t client_base);
} // namespace game
