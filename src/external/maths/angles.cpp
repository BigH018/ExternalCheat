#include "maths/angles.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "config.h"

namespace maths
{
namespace
{
constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;
constexpr float kRadToDeg = 180.0f / std::numbers::pi_v<float>;
} // namespace

float normalize_yaw(float yaw) noexcept
{
    if (!std::isfinite(yaw))
    {
        return 0.0f;
    }
    float wrapped = std::fmod(yaw + 180.0f, 360.0f);
    if (wrapped < 0.0f)
    {
        wrapped += 360.0f;
    }
    return wrapped - 180.0f;
}

Angles normalize(Angles angles) noexcept
{
    const float pitch = std::isfinite(angles.pitch) ? std::clamp(angles.pitch, -kMaxPitch, kMaxPitch) : 0.0f;
    return Angles{pitch, normalize_yaw(angles.yaw)};
}

Angles calc_aim_angles(Vec3 from, Vec3 to) noexcept
{
    const Vec3 d = to - from;
    const float horizontal = std::sqrt(d.x * d.x + d.y * d.y);
    return normalize(Angles{-std::atan2(d.z, horizontal) * kRadToDeg, std::atan2(d.y, d.x) * kRadToDeg});
}

Angles angle_delta(Angles from, Angles to) noexcept
{
    return Angles{to.pitch - from.pitch, normalize_yaw(to.yaw - from.yaw)};
}

float angular_distance(Angles a, Angles b) noexcept
{
    const Angles d = angle_delta(a, b);
    return std::sqrt(d.pitch * d.pitch + d.yaw * d.yaw);
}

bool is_within_fov(Angles view, Angles target, float fov) noexcept
{
    return angular_distance(view, target) <= fov;
}

float smoothing_fraction(float smoothing, float frame_seconds) noexcept
{
    if (!(smoothing > 1.0f))
    {
        return 1.0f;
    }
    const float seconds = std::clamp(frame_seconds, 0.0f, config::kMaxFrameSeconds);
    const float per_reference_frame = 1.0f / smoothing;
    return 1.0f - std::pow(1.0f - per_reference_frame, seconds * config::kSmoothingReferenceHz);
}

Angles step_towards(Angles current, Angles target, float fraction) noexcept
{
    const float k = std::clamp(fraction, 0.0f, 1.0f);
    const Angles d = angle_delta(current, target);
    return normalize(Angles{current.pitch + d.pitch * k, current.yaw + d.yaw * k});
}

Vec3 forward(Angles angles) noexcept
{
    const float pitch = angles.pitch * kDegToRad;
    const float yaw = angles.yaw * kDegToRad;
    return Vec3{std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), -std::sin(pitch)};
}

float distance_to_ray(Vec3 point, Vec3 origin, Vec3 direction) noexcept
{
    const Vec3 to_point = point - origin;
    const float along = to_point.dot(direction);
    if (along <= 0.0f)
    {
        return to_point.length();
    }
    return (to_point - direction * along).length();
}
} // namespace maths
