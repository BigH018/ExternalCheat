#include "game/player.h"

#include <cmath>
#include <utility>

#include "config.h"
#include "game/bones.h"
#include "game/handle.h"
#include "game/observer.h"
#include "game/offsets.h"
#include "game/schema.h"
#include "game/view.h"
#include "game/visibility.h"
#include "game/weapon.h"
#include "game/writes.h"

namespace game
{
namespace
{
namespace base_entity = schema::C_BaseEntity;

// A bool field, read as a byte (any non-zero byte is true; a byte that isn't 0 or 1 isn't a valid C++ bool).
bool read_flag(const core::Memory& memory, std::uintptr_t address) noexcept
{
    return memory.read<std::uint8_t>(address).value_or(0) != 0;
}

Team to_team(std::uint8_t value) noexcept
{
    return value <= static_cast<std::uint8_t>(Team::counter_terrorist) ? static_cast<Team>(value) : Team::none;
}

// Fills the pawn part of `player`, or leaves `player` untouched if the pawn reads as garbage (health out of range,
// no scene node, non-finite position): then nothing it read should be trusted.
void read_pawn(const core::Memory& memory, std::uintptr_t entity_system, std::uintptr_t pawn, bool controller_alive,
               PlayerSnapshot& player)
{
    const auto health = memory.read<std::int32_t>(pawn + base_entity::m_iHealth);
    const auto life_state = memory.read<std::uint8_t>(pawn + base_entity::m_lifeState);
    const auto scene_node = memory.read<std::uintptr_t>(pawn + base_entity::m_pGameSceneNode);
    if (!health || !life_state || !scene_node || *health < 0 || *health > config::kMaxSaneHealth ||
        !core::is_plausible_pointer(*scene_node, alignof(std::uintptr_t)))
    {
        return;
    }
    const auto origin = memory.read<maths::Vec3>(*scene_node + schema::CGameSceneNode::m_vecAbsOrigin);
    if (!origin || !origin->is_finite())
    {
        return;
    }
    const auto view_offset = memory.read<maths::Vec3>(pawn + schema::C_BaseModelEntity::m_vecViewOffset);

    player.pawn = pawn;
    player.health = *health;
    player.alive = controller_alive && *life_state == 0 && *health > 0;
    player.origin = *origin;
    player.view_offset = view_offset && view_offset->is_finite() ? *view_offset : maths::Vec3{};
    if (const auto eyes = memory.read<maths::Angles>(pawn + schema::C_CSPlayerPawn::m_angEyeAngles);
        eyes && std::isfinite(eyes->pitch) && std::isfinite(eyes->yaw))
    {
        player.eye_angles = *eyes;
    }
    player.dormant = read_flag(memory, *scene_node + schema::CGameSceneNode::m_bDormant);
    player.armor = memory.read<std::int32_t>(pawn + schema::C_CSPlayerPawn::m_ArmorValue).value_or(0);
    player.flags = memory.read<std::uint32_t>(pawn + base_entity::m_fFlags).value_or(0);
    player.scoped = read_flag(memory, pawn + schema::C_CSPlayerPawn::m_bIsScoped);
    player.weapon_id = read_active_weapon_id(memory, entity_system, pawn);
    player.spotted_by_mask = read_spotted_by_mask(memory, pawn).value_or(0);
    player.bones = read_bones(memory, *scene_node, *origin);
}
} // namespace

std::optional<std::uintptr_t> read_local_pawn(const core::Memory& memory, std::uintptr_t client_base) noexcept
{
    return memory.read<std::uintptr_t>(client_base + offsets::client::dwLocalPlayerPawn);
}

std::optional<PlayerSnapshot> read_player(const core::Memory& memory, std::uintptr_t entity_system,
                                          const EntityRef& controller, std::uintptr_t local_controller)
{
    const auto team = memory.read<std::uint8_t>(controller.entity + base_entity::m_iTeamNum);
    const auto pawn_handle = memory.read<std::uint32_t>(controller.entity + schema::CCSPlayerController::m_hPlayerPawn);
    if (!team || !pawn_handle)
    {
        return std::nullopt;
    }

    PlayerSnapshot player;
    player.index = controller.index;
    player.controller = controller.entity;
    player.is_local = controller.entity == local_controller;
    player.team = to_team(*team);
    player.name = core::read_string(memory, controller.entity + schema::CBasePlayerController::m_iszPlayerName,
                                    config::kPlayerNameLength)
                      .value_or("");

    const bool controller_alive = read_flag(memory, controller.entity + schema::CCSPlayerController::m_bPawnIsAlive);
    // A garbage pawn leaves pawn == 0 and alive == false.
    if (const std::uintptr_t pawn = resolve_handle(memory, entity_system, *pawn_handle); pawn != 0)
    {
        read_pawn(memory, entity_system, pawn, controller_alive, player);
        player.pawn_index = player.pawn != 0 ? handle_index(*pawn_handle) : 0;
    }
    // Only a dead player spectates; a living one's observer mode and target are left over from earlier.
    if (!controller_alive)
    {
        if (const auto observer = read_observer(memory, entity_system, controller.entity))
        {
            player.observer_mode = observer->mode;
            player.observer_target = observer->target;
        }
    }
    return player;
}

GameSnapshot read_game(const core::Memory& memory, std::uintptr_t client_base)
{
    GameSnapshot snapshot;
    const auto entity_system = read_entity_system(memory, client_base);
    if (!entity_system)
    {
        return snapshot;
    }

    std::uint32_t max_clients = config::kMaxPlayers;
    if (auto globals = read_globals(memory, client_base); globals && globals->is_sane())
    {
        max_clients = static_cast<std::uint32_t>(globals->max_clients);
        snapshot.globals = std::move(*globals);
    }
    if (const auto view = read_view_matrix(memory, client_base); view && view->is_sane())
    {
        snapshot.view = *view;
    }

    const std::uintptr_t local_controller =
        memory.read<std::uintptr_t>(client_base + offsets::client::dwLocalPlayerController).value_or(0);
    for (const EntityRef& controller : find_player_controllers(memory, *entity_system, max_clients))
    {
        if (auto player = read_player(memory, *entity_system, controller, local_controller))
        {
            snapshot.in_match = snapshot.in_match || player->is_local;
            snapshot.players.push_back(std::move(*player));
        }
    }
    if (const PlayerSnapshot* local = snapshot.local(); local != nullptr && local->pawn != 0)
    {
        LocalState& state = snapshot.local_state;
        state.crosshair_entity =
            memory.read<std::int32_t>(local->pawn + schema::C_CSPlayerPawn::m_iIDEntIndex).value_or(-1);
        state.flash_alpha =
            memory.read<float>(local->pawn + schema::C_CSPlayerPawnBase::m_flFlashOverlayAlpha).value_or(0.0f);
        state.flash_max_alpha =
            memory.read<float>(local->pawn + schema::C_CSPlayerPawnBase::m_flFlashMaxAlpha).value_or(0.0f);
    }
    snapshot.local_state.view_angles = read_view_angles(memory, client_base);
    return snapshot;
}
} // namespace game
