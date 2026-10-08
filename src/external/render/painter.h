#pragma once

// Draws render primitives with ImGui, on a draw list (the overlay uses the background draw list, under the menu).

#include <span>

#include <imgui.h>

#include "render/primitives.h"

namespace render
{
void paint(ImDrawList& draw, std::span<const Primitive> primitives, ImFont* font, float font_size);
} // namespace render
