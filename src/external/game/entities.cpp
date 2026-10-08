#include "game/entities.h"

#include <algorithm>

#include "config.h"
#include "game/handle.h"
#include "game/offsets.h"

namespace game
{
namespace layout = offsets::layout;

namespace
{
std::optional<std::string> identity_designer_name(const core::Memory& memory, std::uintptr_t identity)
{
    const auto name = memory.read<std::uintptr_t>(identity + layout::kIdentityDesignerName);
    if (!name || !core::is_plausible_pointer(*name))
    {
        return std::nullopt;
    }
    return core::read_string(memory, *name, config::kMaxNameLength);
}
} // namespace

std::optional<std::string> designer_name(const core::Memory& memory, std::uintptr_t entity_system,
                                         std::uint32_t index)
{
    const auto identity = identity_address(memory, entity_system, index);
    return identity ? identity_designer_name(memory, *identity) : std::nullopt;
}

std::vector<EntityRef> find_player_controllers(const core::Memory& memory, std::uintptr_t entity_system,
                                               std::uint32_t max_clients)
{
    std::vector<EntityRef> controllers;
    const std::uint32_t last = std::min(max_clients, config::kMaxPlayers);
    for (std::uint32_t index = 1; index <= last; ++index)
    {
        const auto identity = identity_address(memory, entity_system, index);
        if (!identity)
        {
            continue;
        }
        const auto entity = memory.read<std::uintptr_t>(*identity + layout::kIdentityEntity);
        if (!entity || !core::is_plausible_pointer(*entity, alignof(std::uintptr_t)))
        {
            continue; // an empty player slot
        }
        if (identity_designer_name(memory, *identity) != layout::kPlayerControllerDesignerName)
        {
            continue;
        }
        const auto handle = memory.read<std::uint32_t>(*identity + layout::kIdentityHandle);
        controllers.push_back(EntityRef{index, *entity, handle.value_or(layout::kInvalidHandle)});
    }
    return controllers;
}
} // namespace game
