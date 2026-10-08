#pragma once

// 2D and 3D float vectors. Vec3 has the game's own layout (Source 2 `Vector`: three floats, 12 bytes), so it can be
// read straight out of game memory.
//
// PURE: no <Windows.h>.

#include <cmath>

namespace maths
{
struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;

    friend constexpr Vec2 operator+(Vec2 a, Vec2 b) noexcept { return {a.x + b.x, a.y + b.y}; }
    friend constexpr Vec2 operator-(Vec2 a, Vec2 b) noexcept { return {a.x - b.x, a.y - b.y}; }
    friend constexpr Vec2 operator*(Vec2 a, float s) noexcept { return {a.x * s, a.y * s}; }
    friend constexpr bool operator==(Vec2, Vec2) noexcept = default;

    [[nodiscard]] float length() const noexcept { return std::sqrt(x * x + y * y); }
};

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    friend constexpr Vec3 operator+(Vec3 a, Vec3 b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
    friend constexpr Vec3 operator-(Vec3 a, Vec3 b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
    friend constexpr Vec3 operator*(Vec3 a, float s) noexcept { return {a.x * s, a.y * s, a.z * s}; }
    friend constexpr bool operator==(Vec3, Vec3) noexcept = default;

    [[nodiscard]] constexpr float dot(Vec3 other) const noexcept { return x * other.x + y * other.y + z * other.z; }
    [[nodiscard]] float length() const noexcept { return std::sqrt(dot(*this)); }
    [[nodiscard]] float distance_to(Vec3 other) const noexcept { return (other - *this).length(); }

    // False for NaN or infinity in any component: a torn or garbage read.
    [[nodiscard]] bool is_finite() const noexcept
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }
};

static_assert(sizeof(Vec3) == 12, "Vec3 must match the game's Vector layout");

// Source 2 world units are inches: 1 unit = 0.0254 m.
inline constexpr float kUnitsPerMetre = 1.0f / 0.0254f;

[[nodiscard]] constexpr float units_to_metres(float units) noexcept
{
    return units / kUnitsPerMetre;
}
} // namespace maths
