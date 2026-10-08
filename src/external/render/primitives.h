#pragma once

// What the overlay draws on the game, as plain data: features build a list of these, render/painter draws them.
// Coordinates are pixels in the overlay (= the game's client area), origin top-left.
//
// PURE: no <Windows.h>, no ImGui.

#include <cstdint>
#include <string>
#include <variant>

#include "color.h"
#include "maths/vec.h"

namespace render
{
// Which point of the text `Text::position` is.
enum class TextAnchor : std::uint8_t
{
    top_left,
    top_right,
    top_centre,
    bottom_centre,
    bottom_right,
};

struct Line
{
    maths::Vec2 from;
    maths::Vec2 to;
    Color colour;
    float thickness = 1.0f;
};

// An outlined rectangle.
struct Rect
{
    maths::Vec2 min;
    maths::Vec2 max;
    Color colour;
    float thickness = 1.0f;
};

struct FilledRect
{
    maths::Vec2 min;
    maths::Vec2 max;
    Color colour;
};

// An outlined circle.
struct Circle
{
    maths::Vec2 centre;
    float radius = 0.0f;
    Color colour;
    float thickness = 1.0f;
};

struct FilledCircle
{
    maths::Vec2 centre;
    float radius = 0.0f;
    Color colour;
};

struct FilledTriangle
{
    maths::Vec2 a;
    maths::Vec2 b;
    maths::Vec2 c;
    Color colour;
};

// One line of text with a dark shadow under it.
struct Text
{
    maths::Vec2 position;
    std::string text;
    Color colour;
    TextAnchor anchor = TextAnchor::top_left;
};

using Primitive = std::variant<Line, Rect, FilledRect, Circle, FilledCircle, FilledTriangle, Text>;
} // namespace render
