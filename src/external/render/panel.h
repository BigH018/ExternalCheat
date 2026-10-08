#pragma once

// A see-through panel of text rows and bars (the bomb timer, the spectator list): rows are written top to bottom, and
// the background and border go under them once their height is known.
//
// PURE: no <Windows.h>, no ImGui.

#include <cstddef>
#include <initializer_list>
#include <string>
#include <vector>

#include "color.h"
#include "maths/vec.h"
#include "render/primitives.h"

namespace render
{
inline constexpr Color kPanelBackground = Color::rgb(0x0E1220, 0.78f);
inline constexpr Color kPanelBorder = Color::rgb(0x8CC4CF, 0.55f);
inline constexpr Color kPanelText = Color::rgb(0xEDEBF7);
inline constexpr Color kPanelDim = Color::rgb(0xEDEBF7, 0.65f);

// A vertical line across a bar at `fraction` of its width (the bomb timer's latest-defuse marks).
struct BarMark
{
    float fraction;
    Color colour;
};

class PanelWriter
{
public:
    // Rows are appended to `out`; the panel's left edge, top and width are in pixels, `line_height` is the font's.
    PanelWriter(std::vector<Primitive>& out, maths::Vec2 top_left, float width, float line_height);

    // A row: `left` at the left edge, `right` (if any) at the right edge.
    void text(const std::string& left, Color left_colour, const std::string& right = {},
              Color right_colour = kPanelText);
    // A bar filled to `filled` (0..1) of the panel's inner width, with optional marks.
    void bar(float filled, Color colour, std::initializer_list<BarMark> marks = {});

    // The bottom of the last row plus the padding.
    [[nodiscard]] float bottom() const noexcept;

    // Puts the background and the border under everything written since the constructor. Call once, at the end.
    void finish(Color background = kPanelBackground, Color border = kPanelBorder);

private:
    std::vector<Primitive>& out_;
    std::size_t first_;
    maths::Vec2 top_left_;
    float width_;
    float left_;
    float right_;
    float y_;
    float line_height_;
};
} // namespace render
