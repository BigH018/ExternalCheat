#pragma once

// The aimbot: which target to aim at, where on it, and the view angles to write this frame. Pure: a snapshot and the
// settings in, angles out; app/frame writes them (game/writes).
//
// External limits (CLAUDE.md §3): the angles are written to dwViewAngles from our own loop, not inside the game's
// CreateMove, so there is no silent aim and the camera visibly moves.
//
// PURE: no <Windows.h>, no ImGui.

#include <optional>
#include <span>
#include <vector>

#include "game/snapshot.h"
#include "maths/angles.h"
#include "maths/projection.h"
#include "maths/vec.h"
#include "render/primitives.h"
#include "settings/settings.h"

namespace features
{
struct AimCandidate
{
    const game::PlayerSnapshot* player = nullptr;
    maths::Vec3 point;          // where on the player
    maths::Angles angles;       // the view angles that look at `point`
    float fov_distance = 0.0f;  // degrees from the current view
    float metres = 0.0f;        // distance from you
};

// The point to aim at on `target`: its head bone, chest bone or (nearest) whichever of head, neck, chest, stomach and
// pelvis is closest to `view` as seen from `eye`. Without bones: the eyes for the head, lower for the body.
[[nodiscard]] maths::Vec3 aim_point(const game::PlayerSnapshot& target, settings::AimTarget where, maths::Vec3 eye,
                                    maths::Angles view);

// Every player the aimbot may aim at right now: live, enemy (if team_check), within max distance, visible (if
// visible_only) and inside the FOV. Empty outside a match or without view angles.
[[nodiscard]] std::vector<AimCandidate> find_candidates(const game::GameSnapshot& game,
                                                        const settings::AimbotSettings& settings,
                                                        settings::TeamMode team_mode);

// The candidate `priority` prefers (ties: the one closest to the crosshair).
[[nodiscard]] std::optional<AimCandidate> select_target(std::span<const AimCandidate> candidates,
                                                        settings::AimPriority priority);

// The view angles to write this frame (one smoothing step towards the chosen target), or nullopt if there is nothing
// to aim at. Doesn't look at keys or focus: the caller decides whether the aimbot is active.
[[nodiscard]] std::optional<maths::Angles> compute_aim(const game::GameSnapshot& game,
                                                       const settings::AimbotSettings& settings,
                                                       settings::TeamMode team_mode, float frame_seconds);

// The FOV as a circle around the crosshair (radius from the live view matrix), or nullopt if it can't be drawn.
[[nodiscard]] std::optional<render::Circle> fov_circle(const game::GameSnapshot& game,
                                                       const settings::AimbotSettings& settings, maths::Vec2 screen);
} // namespace features
