#pragma once

// Small line icons for the tab bar, drawn with ImGui's draw list (no icon font, nothing to vendor).

#include <imgui.h>

namespace ui
{
enum class Icon
{
    home,
    aimbot,     // a crosshair
    triggerbot, // a lightning bolt
    esp,        // a corner box around a figure
    misc,       // a radar
    keybinds,   // a keyboard
    settings,   // a gear
};

// The icon centred on `centre`, fitting a `size` x `size` square.
void draw_icon(ImDrawList* draw, Icon icon, ImVec2 centre, float size, ImU32 colour);
} // namespace ui
