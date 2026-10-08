#pragma once

// Who a feature may act on: the checks the ESP, the aimbot and the triggerbot share.
//
// PURE: no <Windows.h>, no ImGui.

#include "game/snapshot.h"
#include "settings/settings.h"

namespace features
{
// True if `player` counts as an enemy of `local` under `mode` (never `local` itself).
[[nodiscard]] bool is_enemy(const game::PlayerSnapshot& player, const game::PlayerSnapshot& local,
                            settings::TeamMode mode) noexcept;

// Someone a feature can act on at all: not you, has a pawn, alive, not dormant.
[[nodiscard]] bool is_live_target(const game::PlayerSnapshot& player) noexcept;

// The spotted-by heuristic: `local`'s slot bit is set in `player`'s mask.
[[nodiscard]] bool is_visible_to(const game::PlayerSnapshot& player, const game::PlayerSnapshot& local) noexcept;

// Distance between two players' feet, in metres.
[[nodiscard]] float distance_metres(const game::PlayerSnapshot& a, const game::PlayerSnapshot& b) noexcept;

// `max_metres` <= 0 means no limit.
[[nodiscard]] constexpr bool within_distance(float metres, float max_metres) noexcept
{
    return max_metres <= 0.0f || metres <= max_metres;
}
} // namespace features
