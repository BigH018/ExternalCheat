#pragma once

// The triggerbot: fires while a living enemy is under the crosshair. Pure: a snapshot, the settings and the time in,
// "attack down or up" out; app/frame turns changes into `attack` button writes (game/writes).
//
// The crosshair target is the game's own m_iIDEntIndex (the entity index under the crosshair), which equals the
// target pawn's entity index (proven live 2026-10-07: 211 on a bot's head, -1 fifteen degrees off it).
//
// PURE: no <Windows.h>, no ImGui.

#include <cstdint>

#include "game/snapshot.h"
#include "settings/settings.h"

namespace features
{
// Why the triggerbot won't fire right now (none = it may). The first reason that applies, in this order.
enum class TriggerBlock : std::uint8_t
{
    none,
    not_in_match, // no match, no local pawn, or you're dead
    weapon,       // the weapon's class is filtered out, or it can't shoot (knife, grenade, C4)
    not_scoped,   // a sniper rifle without the scope
    flashed,
    in_air,
    no_target,    // nothing (or nobody the filters allow) under the crosshair
};

// The player under the crosshair if the triggerbot may shoot them: a live target, an enemy (if team_check), within
// max distance, visible (if visible_only), crosshair on the head (if head_only). nullptr otherwise.
[[nodiscard]] const game::PlayerSnapshot* trigger_target(const game::GameSnapshot& game,
                                                         const settings::TriggerbotSettings& settings,
                                                         settings::TeamMode team_mode) noexcept;

// Everything that has to hold for a shot, about you and your weapon first, then the target.
[[nodiscard]] TriggerBlock trigger_block(const game::GameSnapshot& game, const settings::TriggerbotSettings& settings,
                                         settings::TeamMode team_mode) noexcept;

// The firing state machine:
//   idle --target--> reacting --reaction delay--> pressed (one tap, config::kTriggerTapMs) --> between shots (burst)
//   --> pressed ... --> cooldown (the shot delay) --> pressed again if the target is still there, else idle.
// Hold mode: reacting --> holding (attack down) until the target leaves. Losing the target cancels a pending reaction
// or burst; a tap in progress always finishes, so the game never sees a half press.
class Triggerbot
{
public:
    // One frame. `now_ms`: a monotonic clock. `active`: enabled and allowed (activation key, menu closed, game
    // focused). `can_fire`: trigger_block(...) == none. Returns whether attack should be down.
    bool update(std::uint64_t now_ms, bool active, bool can_fire, const settings::TriggerbotSettings& settings);

    // Back to idle, attack up.
    void reset() noexcept;

    [[nodiscard]] bool attack_down() const noexcept { return state_ == State::pressed || state_ == State::holding; }

private:
    enum class State : std::uint8_t
    {
        idle,
        reacting,
        pressed,
        between,
        cooldown,
        holding,
    };

    void start_volley(std::uint64_t now_ms, const settings::TriggerbotSettings& settings);
    void press(std::uint64_t now_ms);

    State state_ = State::idle;
    std::uint64_t until_ms_ = 0; // when the current state's wait ends
    int shots_left_ = 0;         // in this volley, after the current tap
    int shot_delay_ms_ = 0;      // captured at the start of a volley
};
} // namespace features
