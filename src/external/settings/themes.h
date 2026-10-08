#pragma once

// The menu's colour themes (Settings page, Appearance). Every theme is dark and easy on the eyes at night: near-black
// and grey surfaces, off-white text (never pure white), muted status colours. The accent colour (switches, the
// selected tab, sliders) is a setting of its own (OverlaySettings::accent); a theme only suggests one.
//
// PURE: no <Windows.h>, no ImGui.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "color.h"

namespace settings
{
enum class MenuTheme : std::uint8_t
{
    midnight, // neutral graphite, soft white accent (the default)
    black,    // pure black for OLED screens
    graphite, // a lighter grey, steel-blue accent
    violet,   // the logo's turtleneck and gold sparkles
    ice,      // the logo's teal background
};

inline constexpr std::array kMenuThemes = {MenuTheme::midnight, MenuTheme::black, MenuTheme::graphite,
                                           MenuTheme::violet, MenuTheme::ice};

struct ThemeColours
{
    Color window;        // the page area behind the panels
    Color chrome;        // header and tab bar
    Color panel;         // panels
    Color control;       // inputs, buttons, a switch that's off
    Color control_hover; // the same, hovered
    Color border;        // panel edges, separators
    Color text;          // labels and values
    Color text_dim;      // explanations, less important values
    Color text_faint;    // disabled text, icons of tabs that aren't selected
    Color accent;        // the accent this theme suggests
    Color ok;
    Color warn;
    Color danger;
};

// clang-format off
inline constexpr std::array<ThemeColours, kMenuThemes.size()> kThemeColours = {{
    // window              chrome                panel                 control               control_hover
    // border              text                  text_dim              text_faint            accent
    // ok                  warn                  danger
    {Color::rgb(0x121214), Color::rgb(0x0C0C0E), Color::rgb(0x19191C), Color::rgb(0x242428), Color::rgb(0x2E2E33),
     Color::rgb(0x29292E), Color::rgb(0xDCDCE0), Color::rgb(0x9A9AA3), Color::rgb(0x64646C), Color::rgb(0xD2D2D8),
     Color::rgb(0x7DBF95), Color::rgb(0xD8B36A), Color::rgb(0xD9777A)},
    {Color::rgb(0x050505), Color::rgb(0x000000), Color::rgb(0x0D0D0E), Color::rgb(0x19191B), Color::rgb(0x232326),
     Color::rgb(0x1E1E21), Color::rgb(0xD4D4D6), Color::rgb(0x8E8E92), Color::rgb(0x57575B), Color::rgb(0xBDBDC2),
     Color::rgb(0x74B48B), Color::rgb(0xCFA962), Color::rgb(0xD06E71)},
    {Color::rgb(0x1D1E21), Color::rgb(0x17181A), Color::rgb(0x25262A), Color::rgb(0x303135), Color::rgb(0x3A3B40),
     Color::rgb(0x34353A), Color::rgb(0xE1E1E4), Color::rgb(0xA3A4AB), Color::rgb(0x6E6F76), Color::rgb(0x9DB4D3),
     Color::rgb(0x84C49C), Color::rgb(0xDDB873), Color::rgb(0xDE7E81)},
    {Color::rgb(0x15141C), Color::rgb(0x100F16), Color::rgb(0x1C1B26), Color::rgb(0x272636), Color::rgb(0x312F43),
     Color::rgb(0x2B2A3A), Color::rgb(0xE3E0F0), Color::rgb(0xA19DB9), Color::rgb(0x6A6684), Color::rgb(0xE2C46A),
     Color::rgb(0x82C29A), Color::rgb(0xE2C46A), Color::rgb(0xDE7F86)},
    {Color::rgb(0x101416), Color::rgb(0x0B0E10), Color::rgb(0x161C1F), Color::rgb(0x1F272B), Color::rgb(0x283236),
     Color::rgb(0x232C30), Color::rgb(0xDCE5E7), Color::rgb(0x92A2A7), Color::rgb(0x5B6A6F), Color::rgb(0x9CC9D1),
     Color::rgb(0x7DC29E), Color::rgb(0xD9B56E), Color::rgb(0xD87C7E)},
}};
// clang-format on

[[nodiscard]] constexpr const ThemeColours& theme_colours(MenuTheme theme) noexcept
{
    const auto index = static_cast<std::size_t>(theme);
    return kThemeColours[index < kThemeColours.size() ? index : 0];
}

[[nodiscard]] std::string_view theme_name(MenuTheme theme) noexcept;

// WCAG contrast ratio of two opaque colours: 1 = none, 21 = black on white (alpha is ignored). The tests keep every
// theme's text readable with it.
[[nodiscard]] float contrast_ratio(const Color& a, const Color& b) noexcept;

// Text for on top of a `fill` (a button or switch in the accent colour): the theme's text colour or its window colour,
// whichever stands out more.
[[nodiscard]] Color text_on(const Color& fill, const ThemeColours& theme) noexcept;
} // namespace settings
