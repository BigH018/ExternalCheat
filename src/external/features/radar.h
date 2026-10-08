#pragma once

// Our own radar, drawn by the overlay: a square in a corner of the game window with you in the middle and every
// player as a dot. It only reads the snapshot (positions, facing, spotted-by mask); nothing is written to the game.
//
// Radar coordinates: x right, y down (screen directions), measured from you. With rotation on, up is where you look;
// with it off, up is the world's +y, like the game's own radar.
//
// PURE: no <Windows.h>, no ImGui.

#include <vector>

#include "game/snapshot.h"
#include "maths/vec.h"
#include "render/primitives.h"
#include "settings/settings.h"

namespace features
{
// The radar's square on screen.
struct RadarPanel
{
    maths::Vec2 min; // top-left
    float size = 0.0f;

    [[nodiscard]] maths::Vec2 max() const noexcept { return {min.x + size, min.y + size}; }
    [[nodiscard]] maths::Vec2 centre() const noexcept { return {min.x + size * 0.5f, min.y + size * 0.5f}; }
};

// The square of side `size` in `corner` of a `screen`-sized window, `margin` pixels from both edges.
[[nodiscard]] RadarPanel radar_panel(settings::RadarCorner corner, float size, maths::Vec2 screen,
                                     float margin) noexcept;

// Where `point` lands relative to `you` on a radar whose "up" is the world direction `radar_yaw` (degrees, the game's
// yaw convention): x right, y down, in world units. Height is ignored.
[[nodiscard]] maths::Vec2 radar_offset(maths::Vec3 you, maths::Vec3 point, float radar_yaw) noexcept;

// The unit vector a player looking along `yaw` points on the same radar (x right, y down).
[[nodiscard]] maths::Vec2 radar_direction(float yaw, float radar_yaw) noexcept;

// `point` in edge units (1 = the edge of the square) pulled back onto the square if it lies outside, keeping its
// direction. Points inside are unchanged.
[[nodiscard]] maths::Vec2 clamp_to_square(maths::Vec2 point) noexcept;

// Everything the radar draws this frame. Nothing outside a match, with the radar off, or without your pawn.
// `team_mode` decides who is an enemy.
[[nodiscard]] std::vector<render::Primitive> build_radar(const game::GameSnapshot& game,
                                                         const settings::RadarSettings& settings,
                                                         settings::TeamMode team_mode, maths::Vec2 screen);
} // namespace features
