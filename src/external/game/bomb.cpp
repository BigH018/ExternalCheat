#include "game/bomb.h"

#include <cmath>

#include "config.h"
#include "game/handle.h"
#include "game/offsets.h"
#include "game/schema.h"

namespace game
{
namespace
{
namespace c4 = schema::C_PlantedC4;

bool read_flag(const core::Memory& memory, std::uintptr_t address) noexcept
{
    return memory.read<std::uint8_t>(address).value_or(0) != 0;
}
} // namespace

std::optional<bool> read_bomb_planted(const core::Memory& memory, std::uintptr_t client_base) noexcept
{
    const auto rules = memory.read<std::uintptr_t>(client_base + offsets::client::dwGameRules);
    if (!rules || !core::is_plausible_pointer(*rules, alignof(std::uintptr_t)))
    {
        return std::nullopt;
    }
    const auto planted = memory.read<std::uint8_t>(*rules + schema::C_CSGameRules::m_bBombPlanted);
    if (!planted)
    {
        return std::nullopt;
    }
    return *planted != 0;
}

std::optional<PlantedBomb> read_planted_bomb(const core::Memory& memory, std::uintptr_t entity_system,
                                             std::uintptr_t entity)
{
    const auto blow_time = memory.read<float>(entity + c4::m_flC4Blow);
    const auto timer_length = memory.read<float>(entity + c4::m_flTimerLength);
    const auto scene_node = memory.read<std::uintptr_t>(entity + schema::C_BaseEntity::m_pGameSceneNode);
    if (!blow_time || !timer_length || !scene_node || !std::isfinite(*blow_time) || !std::isfinite(*timer_length) ||
        *timer_length <= 0.0f || *timer_length > config::kMaxBombTimer ||
        !core::is_plausible_pointer(*scene_node, alignof(std::uintptr_t)))
    {
        return std::nullopt;
    }
    const auto position = memory.read<maths::Vec3>(*scene_node + schema::CGameSceneNode::m_vecAbsOrigin);
    if (!position || !position->is_finite())
    {
        return std::nullopt;
    }

    PlantedBomb bomb;
    bomb.entity = entity;
    bomb.site = memory.read<std::int32_t>(entity + c4::m_nBombSite).value_or(-1);
    bomb.position = *position;
    bomb.ticking = read_flag(memory, entity + c4::m_bBombTicking);
    bomb.exploded = read_flag(memory, entity + c4::m_bHasExploded);
    bomb.defused = read_flag(memory, entity + c4::m_bBombDefused);
    bomb.being_defused = read_flag(memory, entity + c4::m_bBeingDefused);
    bomb.blow_time = *blow_time;
    bomb.timer_length = *timer_length;
    bomb.defuse_end = memory.read<float>(entity + c4::m_flDefuseCountDown).value_or(0.0f);
    bomb.defuse_length = memory.read<float>(entity + c4::m_flDefuseLength).value_or(0.0f);
    if (!std::isfinite(bomb.defuse_end) || !std::isfinite(bomb.defuse_length))
    {
        bomb.being_defused = false;
        bomb.defuse_end = 0.0f;
        bomb.defuse_length = 0.0f;
    }
    if (const auto defuser = memory.read<std::uint32_t>(entity + c4::m_hBombDefuser);
        defuser && is_valid_handle(*defuser))
    {
        bomb.defuser_pawn = resolve_handle(memory, entity_system, *defuser);
    }
    return bomb;
}

std::optional<PlantedBomb> read_bomb(const core::Memory& memory, std::uintptr_t client_base,
                                     std::uintptr_t entity_system)
{
    const auto entity = memory.read<std::uintptr_t>(client_base + offsets::client::dwPlantedC4);
    if (!entity || !core::is_plausible_pointer(*entity, alignof(std::uintptr_t)))
    {
        return std::nullopt; // 0: no bomb this round
    }
    // Stale-pointer guard: the bomb's identity holds its handle, and the handle must lead back to the same entity.
    const auto identity = memory.read<std::uintptr_t>(*entity + schema::CEntityInstance::m_pEntity);
    const auto handle = identity && core::is_plausible_pointer(*identity, alignof(std::uintptr_t))
                            ? memory.read<std::uint32_t>(*identity + offsets::layout::kIdentityHandle)
                            : std::nullopt;
    if (!handle || !is_valid_handle(*handle) || resolve_handle(memory, entity_system, *handle) != *entity)
    {
        return std::nullopt;
    }
    return read_planted_bomb(memory, entity_system, *entity);
}
} // namespace game
