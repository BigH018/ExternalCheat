#pragma once

// View angles and the maths the aimbot and triggerbot need.
//
// The convention, proven 2026-10-06 in a bot match (build 14189): angles are degrees, (pitch, yaw, roll) as in
// m_angEyeAngles and dwViewAngles. Pitch positive = looking down (the game clamps it to +-89). Yaw is measured from +x
// towards +y and wraps at +-180. Forward = (cos p * cos y, cos p * sin y, -sin p): the point 1000 units along it
// projected exactly onto the crosshair.
//
// PURE: no <Windows.h>.

#include "maths/vec.h"

namespace maths
{
struct Angles
{
    float pitch = 0.0f;
    float yaw = 0.0f;

    friend constexpr bool operator==(Angles, Angles) noexcept = default;
};

inline constexpr float kMaxPitch = 89.0f;

// `yaw` wrapped into [-180, 180).
[[nodiscard]] float normalize_yaw(float yaw) noexcept;

// Yaw wrapped, pitch clamped to +-kMaxPitch: angles the game accepts.
[[nodiscard]] Angles normalize(Angles angles) noexcept;

// The angles that look from `from` straight at `to`.
[[nodiscard]] Angles calc_aim_angles(Vec3 from, Vec3 to) noexcept;

// `to - from`, with the yaw difference taken the short way round (so 170 -> -170 is +20, not -340).
[[nodiscard]] Angles angle_delta(Angles from, Angles to) noexcept;

// How far apart two view directions are, in degrees (the length of angle_delta).
[[nodiscard]] float angular_distance(Angles a, Angles b) noexcept;

// True if `target` is within `fov` degrees of `view`.
[[nodiscard]] bool is_within_fov(Angles view, Angles target, float fov) noexcept;

// The fraction of the remaining angle to cover this frame. `smoothing` 1 = all of it (snap); s > 1 = 1/s per 60 Hz
// frame, compounded over `frame_seconds` so the speed doesn't depend on the frame rate.
[[nodiscard]] float smoothing_fraction(float smoothing, float frame_seconds) noexcept;

// `fraction` (0..1) of the way from `current` to `target`, the short way round, normalized.
[[nodiscard]] Angles step_towards(Angles current, Angles target, float fraction) noexcept;

// The unit vector the angles look along.
[[nodiscard]] Vec3 forward(Angles angles) noexcept;

// The shortest distance from `point` to the ray that starts at `origin` and runs along `direction` (a unit vector).
// Points behind the origin measure to the origin itself.
[[nodiscard]] float distance_to_ray(Vec3 point, Vec3 origin, Vec3 direction) noexcept;
} // namespace maths
