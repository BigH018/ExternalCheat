#pragma once

// The bomb timer: how long the planted bomb has left, whether a defuse can still make it, and a panel to draw.
// Pure: the snapshot (its bomb and the game time) in, primitives out.
//
// PURE: no <Windows.h>, no ImGui.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "game/snapshot.h"
#include "maths/vec.h"
#include "render/primitives.h"
#include "settings/settings.h"

namespace features
{
enum class BombPhase : std::uint8_t
{
    ticking,
    defusing,
    defused,
    exploded,
};

// Whether a defuse started right now finishes before the bomb explodes.
enum class DefuseVerdict : std::uint8_t
{
    no_kit_needed, // more than config::kDefuseSecondsNoKit left
    kit_needed,    // more than config::kDefuseSecondsKit left
    too_late,
};

struct BombTimerInfo
{
    BombPhase phase = BombPhase::ticking;
    char site = '?';              // 'A', 'B' or '?'
    float seconds_left = 0.0f;    // until it explodes, never below 0
    float fraction_left = 0.0f;   // of the whole fuse, 0..1
    float timer_length = 0.0f;    // the whole fuse, seconds (where the bar's defuse marks go)
    DefuseVerdict verdict = DefuseVerdict::no_kit_needed;
    float defuse_left = 0.0f;     // defusing: seconds until the defuse completes
    float defuse_fraction_left = 0.0f;
    bool defuse_in_time = false;  // defusing: it completes before the bomb explodes
    std::string defuser;          // defusing: the defuser's name, empty if unknown
    std::optional<float> distance_metres; // from you to the bomb, if you're in the match with a pawn
};

[[nodiscard]] DefuseVerdict defuse_verdict(float seconds_left) noexcept;

// The bomb's state against the game time. nullopt if no bomb is planted.
[[nodiscard]] std::optional<BombTimerInfo> bomb_timer_info(const game::GameSnapshot& game);

// The panel at the top-centre of the screen. Nothing with the timer off or no bomb planted. `line_height` is the font's
// height in pixels (rows are stacked by it).
[[nodiscard]] std::vector<render::Primitive> build_bomb_timer(const game::GameSnapshot& game,
                                                              const settings::BombTimerSettings& settings,
                                                              maths::Vec2 screen, float line_height);
} // namespace features
