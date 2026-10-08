#include "features/triggerbot.h"

#include <optional>

#include "config.h"
#include "features/targeting.h"
#include "game/weapon.h"
#include "maths/angles.h"

namespace features
{
namespace
{
bool weapon_allowed(game::WeaponClass weapon_class, const settings::WeaponFilter& filter) noexcept
{
    switch (weapon_class)
    {
    case game::WeaponClass::pistol: return filter.pistol;
    case game::WeaponClass::smg: return filter.smg;
    case game::WeaponClass::rifle: return filter.rifle;
    case game::WeaponClass::sniper: return filter.sniper;
    case game::WeaponClass::shotgun: return filter.shotgun;
    case game::WeaponClass::heavy: return filter.heavy;
    case game::WeaponClass::unknown:
    case game::WeaponClass::knife:
    case game::WeaponClass::grenade:
    case game::WeaponClass::other: break;
    }
    return false;
}

game::WeaponClass weapon_class_of(const game::PlayerSnapshot& player) noexcept
{
    return player.weapon_id ? game::weapon_info(*player.weapon_id).weapon_class : game::WeaponClass::unknown;
}

// The crosshair ray passes within config::kTriggerHeadRadius of the middle of the head.
bool crosshair_on_head(const game::PlayerSnapshot& target, const game::PlayerSnapshot& local,
                       const std::optional<maths::Angles>& view) noexcept
{
    if (!view)
    {
        return false;
    }
    return maths::distance_to_ray(target.head_position(), local.eye_position(), maths::forward(*view)) <=
           config::kTriggerHeadRadius;
}
} // namespace

const game::PlayerSnapshot* trigger_target(const game::GameSnapshot& game,
                                           const settings::TriggerbotSettings& settings,
                                           settings::TeamMode team_mode) noexcept
{
    const game::PlayerSnapshot* local = game.local();
    const std::int32_t under_crosshair = game.local_state.crosshair_entity;
    if (local == nullptr || under_crosshair <= 0)
    {
        return nullptr;
    }
    for (const game::PlayerSnapshot& player : game.players)
    {
        if (player.pawn_index != static_cast<std::uint32_t>(under_crosshair))
        {
            continue;
        }
        if (!is_live_target(player) || (settings.team_check && !is_enemy(player, *local, team_mode)) ||
            !within_distance(distance_metres(*local, player), settings.max_distance) ||
            (settings.visible_only && !is_visible_to(player, *local)) ||
            (settings.head_only && !crosshair_on_head(player, *local, game.local_state.view_angles)))
        {
            return nullptr;
        }
        return &player;
    }
    return nullptr;
}

TriggerBlock trigger_block(const game::GameSnapshot& game, const settings::TriggerbotSettings& settings,
                           settings::TeamMode team_mode) noexcept
{
    const game::PlayerSnapshot* local = game.local();
    if (!game.in_match || local == nullptr || !local->alive || local->pawn == 0)
    {
        return TriggerBlock::not_in_match;
    }
    const game::WeaponClass weapon = weapon_class_of(*local);
    if (!weapon_allowed(weapon, settings.weapons))
    {
        return TriggerBlock::weapon;
    }
    if (settings.snipers_scoped_only && weapon == game::WeaponClass::sniper && !local->scoped)
    {
        return TriggerBlock::not_scoped;
    }
    if (settings.not_flashed && game.local_state.flashed(config::kFlashedFraction))
    {
        return TriggerBlock::flashed;
    }
    if (settings.not_in_air && !local->on_ground())
    {
        return TriggerBlock::in_air;
    }
    return trigger_target(game, settings, team_mode) != nullptr ? TriggerBlock::none : TriggerBlock::no_target;
}

bool Triggerbot::update(std::uint64_t now_ms, bool active, bool can_fire,
                        const settings::TriggerbotSettings& settings)
{
    const bool target = active && can_fire;
    switch (state_)
    {
    case State::idle:
        if (target)
        {
            state_ = State::reacting;
            until_ms_ = now_ms + static_cast<std::uint64_t>(config::kTriggerReaction.clamp(settings.reaction_ms));
            return update(now_ms, active, can_fire, settings); // a 0 ms reaction fires this frame
        }
        break;
    case State::reacting:
        if (!target)
        {
            state_ = State::idle;
        }
        else if (now_ms >= until_ms_)
        {
            start_volley(now_ms, settings);
        }
        break;
    case State::pressed:
        if (now_ms >= until_ms_)
        {
            if (shots_left_ > 0 && target)
            {
                state_ = State::between;
            }
            else
            {
                state_ = State::cooldown;
            }
            until_ms_ = now_ms + static_cast<std::uint64_t>(shot_delay_ms_);
        }
        break;
    case State::between:
        if (!target)
        {
            state_ = State::idle;
        }
        else if (now_ms >= until_ms_)
        {
            --shots_left_;
            press(now_ms);
        }
        break;
    case State::cooldown:
        if (now_ms >= until_ms_)
        {
            state_ = State::idle;
            if (target)
            {
                start_volley(now_ms, settings); // still on target: the next volley needs no new reaction
            }
        }
        break;
    case State::holding:
        if (!target)
        {
            state_ = State::idle;
        }
        break;
    }
    return attack_down();
}

void Triggerbot::reset() noexcept
{
    state_ = State::idle;
    until_ms_ = 0;
    shots_left_ = 0;
}

void Triggerbot::start_volley(std::uint64_t now_ms, const settings::TriggerbotSettings& settings)
{
    shot_delay_ms_ = config::kTriggerShotDelay.clamp(settings.shot_delay_ms);
    switch (settings.fire_mode)
    {
    case settings::FireMode::hold:
        state_ = State::holding;
        return;
    case settings::FireMode::burst:
        shots_left_ = config::kTriggerBurst.clamp(settings.burst_shots) - 1;
        break;
    case settings::FireMode::single:
        shots_left_ = 0;
        break;
    }
    press(now_ms);
}

void Triggerbot::press(std::uint64_t now_ms)
{
    state_ = State::pressed;
    until_ms_ = now_ms + config::kTriggerTapMs;
}
} // namespace features
