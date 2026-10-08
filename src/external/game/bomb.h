#pragma once

// The planted bomb, read from outside: client.dll + dwPlantedC4 points straight at the C_PlantedC4 entity from the
// plant until the next round starts (0 otherwise), proven live 2026-10-07 (docs/offsets.md "Planted bomb").
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <optional>

#include "core/memory.h"
#include "game/snapshot.h"

namespace game
{
// C_CSGameRules::m_bBombPlanted (client.dll + dwGameRules -> rules). nullopt if the rules can't be read. Only the
// diagnostic uses it (as a cross-check of dwPlantedC4).
[[nodiscard]] std::optional<bool> read_bomb_planted(const core::Memory& memory, std::uintptr_t client_base) noexcept;

// The C_PlantedC4 fields of `entity`. nullopt if they read as garbage (non-finite times, a timer outside
// 0..config::kMaxBombTimer, no position).
[[nodiscard]] std::optional<PlantedBomb> read_planted_bomb(const core::Memory& memory, std::uintptr_t entity_system,
                                                           std::uintptr_t entity);

// The planted bomb, or nullopt when there is none. A pointer whose identity no longer resolves to it through the
// entity list (the entity was freed) counts as none.
[[nodiscard]] std::optional<PlantedBomb> read_bomb(const core::Memory& memory, std::uintptr_t client_base,
                                                   std::uintptr_t entity_system);
} // namespace game
