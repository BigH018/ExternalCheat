#include "settings/themes.h"

#include <algorithm>
#include <cmath>

namespace settings
{
namespace
{
// sRGB channel -> linear light (WCAG 2.x).
float linear(float channel) noexcept
{
    return channel <= 0.04045f ? channel / 12.92f : std::pow((channel + 0.055f) / 1.055f, 2.4f);
}

float luminance(const Color& c) noexcept
{
    return 0.2126f * linear(c.r) + 0.7152f * linear(c.g) + 0.0722f * linear(c.b);
}
} // namespace

std::string_view theme_name(MenuTheme theme) noexcept
{
    switch (theme)
    {
    case MenuTheme::midnight: return "Midnight";
    case MenuTheme::black: return "Black";
    case MenuTheme::graphite: return "Graphite";
    case MenuTheme::violet: return "Violet";
    case MenuTheme::ice: return "Ice";
    }
    return "Midnight";
}

float contrast_ratio(const Color& a, const Color& b) noexcept
{
    const float la = luminance(a);
    const float lb = luminance(b);
    return (std::max(la, lb) + 0.05f) / (std::min(la, lb) + 0.05f);
}

Color text_on(const Color& fill, const ThemeColours& theme) noexcept
{
    return contrast_ratio(fill, theme.text) >= contrast_ratio(fill, theme.window) ? theme.text : theme.window;
}
} // namespace settings
