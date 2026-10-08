#pragma once

// Walking the entity list remotely: entity designer names, and the player controllers.
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "core/memory.h"

namespace game
{
// An entity found in the list.
struct EntityRef
{
    std::uint32_t index = 0;    // its entity index (for a controller: player slot + 1)
    std::uintptr_t entity = 0;  // the entity itself
    std::uint32_t handle = 0;   // its own handle (index + serial), as its identity holds it
};

// The designer name of entity `index` ("cs_player_controller", "weapon_ak47"), or nullopt.
[[nodiscard]] std::optional<std::string> designer_name(const core::Memory& memory, std::uintptr_t entity_system,
                                                       std::uint32_t index);

// Every player controller: indices 1..max_clients whose designer name is "cs_player_controller", in index order.
// `max_clients` comes from CGlobalVars (64 on a local server); it is clamped to what the entity list can hold.
[[nodiscard]] std::vector<EntityRef> find_player_controllers(const core::Memory& memory, std::uintptr_t entity_system,
                                                             std::uint32_t max_clients);
} // namespace game
