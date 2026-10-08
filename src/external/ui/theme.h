#pragma once

// The menu's look. Colours come from the theme and accent the user picked (settings::OverlaySettings, themes in
// settings/themes.h); use_theme() turns them into ui::palette() and ImGui's style colours once per frame. Pages and
// widgets refer to palette() names, never to inline hex values.

#include <imgui.h>

#include "color.h"
#include "settings/settings.h"

namespace ui
{
// The colours the menu and the watermark draw with right now.
struct Palette
{
    ImVec4 window;
    ImVec4 chrome;
    ImVec4 panel;
    ImVec4 control;
    ImVec4 control_hover;
    ImVec4 border;
    ImVec4 text;
    ImVec4 text_dim;
    ImVec4 text_faint;
    ImVec4 accent;
    ImVec4 accent_hover; // the accent, a little brighter
    ImVec4 accent_soft;  // the accent, faint: selections, hovered rows
    ImVec4 on_accent;    // text on an accent fill
    ImVec4 ok;
    ImVec4 warn;
    ImVec4 danger;
};

[[nodiscard]] const Palette& palette() noexcept;

// Rebuilds palette() and ImGui's style colours if the theme or accent changed since the last call (cheap otherwise).
// Call once per frame, after ImGui's NewFrame and before anything is drawn.
void use_theme(const settings::OverlaySettings& overlay);

// Sizes, spacing and rounding (once, at start-up).
void apply_style(ImGuiStyle& style);

[[nodiscard]] ImVec4 to_imvec4(const Color& c) noexcept;
[[nodiscard]] ImVec4 with_alpha(ImVec4 c, float alpha) noexcept;
[[nodiscard]] ImU32 u32(const ImVec4& c) noexcept;

// Fonts loaded once by ImGuiLayer. `semibold` and `bold` fall back to `regular` if the face isn't available.
struct Fonts
{
    ImFont* regular = nullptr;
    ImFont* semibold = nullptr; // panel titles, tabs
    ImFont* bold = nullptr;     // the app name
};

// A size in pixels at UI scale 1.0 (follows ImGui's font scale).
float scaled(float pixels);
} // namespace ui
