#pragma once

// An RGBA colour as floats in 0..1, the way the settings store colours (profiles save them as "#RRGGBBAA").
//
// PURE: no <Windows.h>, no ImGui.

#include <algorithm>
#include <cstdint>

struct Color
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    // 0xRRGGBB and an alpha.
    [[nodiscard]] static constexpr Color rgb(std::uint32_t hex, float alpha = 1.0f) noexcept
    {
        return Color{static_cast<float>((hex >> 16) & 0xFF) / 255.0f, static_cast<float>((hex >> 8) & 0xFF) / 255.0f,
                     static_cast<float>(hex & 0xFF) / 255.0f, alpha};
    }

    // 0xRRGGBBAA. Profiles store colours this way, so a default meant to survive a save and load unchanged uses it
    // (a byte-exact alpha) instead of rgb(hex, 0.35f).
    [[nodiscard]] static constexpr Color rgba(std::uint32_t hex) noexcept
    {
        return Color{static_cast<float>((hex >> 24) & 0xFF) / 255.0f, static_cast<float>((hex >> 16) & 0xFF) / 255.0f,
                     static_cast<float>((hex >> 8) & 0xFF) / 255.0f, static_cast<float>(hex & 0xFF) / 255.0f};
    }

    // Back to 0xRRGGBBAA: each channel clamped to 0..1 and rounded to the nearest byte.
    [[nodiscard]] constexpr std::uint32_t to_rgba() const noexcept
    {
        const auto byte = [](float v) { return static_cast<std::uint32_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
        return (byte(r) << 24) | (byte(g) << 16) | (byte(b) << 8) | byte(a);
    }

    // The same colour with its alpha multiplied by `factor` (fading a colour out).
    [[nodiscard]] constexpr Color faded(float factor) const noexcept
    {
        return Color{r, g, b, std::clamp(a * factor, 0.0f, 1.0f)};
    }

    // Straight interpolation from this colour (t = 0) to `to` (t = 1); t is clamped to 0..1.
    [[nodiscard]] constexpr Color lerp(const Color& to, float t) const noexcept
    {
        const float k = std::clamp(t, 0.0f, 1.0f);
        return Color{r + (to.r - r) * k, g + (to.g - g) * k, b + (to.b - b) * k, a + (to.a - a) * k};
    }

    friend constexpr bool operator==(const Color&, const Color&) noexcept = default;
};
