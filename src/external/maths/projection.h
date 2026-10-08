#pragma once

// The game's view matrix (world -> clip space) and world-to-screen.
//
// Source 2 keeps the matrix as 16 floats, row-major: clip.x = m[0]x + m[1]y + m[2]z + m[3], ..., clip.w = m[12]x +
// m[13]y + m[14]z + m[15]. Proven 2026-10-06 in a bot match (build 14189): a point straight along the camera's view
// direction projected to the exact centre of the 1920x1080 screen at 100, 1000 and 5000 units, its w equal to its
// distance; a point above it landed higher on the screen, one to the right further right, one behind had w < 0.
//
// PURE: no <Windows.h>.

#include <array>
#include <cmath>
#include <optional>

#include "maths/vec.h"

namespace maths
{
struct ViewMatrix
{
    std::array<float, 16> m{};

    [[nodiscard]] constexpr float at(int row, int column) const noexcept { return m[row * 4 + column]; }

    // A matrix worth projecting with: every value finite, and not all zero (the game leaves it zeroed until the first
    // frame of a match is rendered).
    [[nodiscard]] bool is_sane() const noexcept
    {
        bool any_nonzero = false;
        for (const float value : m)
        {
            if (!std::isfinite(value))
            {
                return false;
            }
            any_nonzero = any_nonzero || value != 0.0f;
        }
        return any_nonzero;
    }
};

static_assert(sizeof(ViewMatrix) == 64, "ViewMatrix must match the game's 4x4 float layout");

// Points closer to the camera plane than this (in w, i.e. world units along the view direction) are not projected:
// behind the camera, or so close that the division blows up.
inline constexpr float kMinClipW = 0.01f;

// Where `world` appears on a screen of `screen` pixels (origin top-left, y down), or nullopt if it is behind the
// camera. A point in front of the camera but outside the screen still projects (off-screen coordinates).
[[nodiscard]] std::optional<Vec2> world_to_screen(const ViewMatrix& view, Vec3 world, Vec2 screen) noexcept;
} // namespace maths
