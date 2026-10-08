#include "render/panel.h"

#include <algorithm>
#include <cstddef>

#include "config.h"

namespace render
{
namespace
{
constexpr Color kBarBack{0.0f, 0.0f, 0.0f, 0.6f};
} // namespace

PanelWriter::PanelWriter(std::vector<Primitive>& out, maths::Vec2 top_left, float width, float line_height)
    : out_(out), first_(out.size()), top_left_(top_left), width_(width), left_(top_left.x + config::kPanelPadding),
      right_(top_left.x + width - config::kPanelPadding), y_(top_left.y + config::kPanelPadding),
      line_height_(line_height)
{
}

void PanelWriter::text(const std::string& left, Color left_colour, const std::string& right, Color right_colour)
{
    out_.push_back(Text{{left_, y_}, left, left_colour, TextAnchor::top_left});
    if (!right.empty())
    {
        out_.push_back(Text{{right_, y_}, right, right_colour, TextAnchor::top_right});
    }
    y_ += line_height_ + config::kPanelRowGap;
}

void PanelWriter::bar(float filled, Color colour, std::initializer_list<BarMark> marks)
{
    const float end = left_ + (right_ - left_) * std::clamp(filled, 0.0f, 1.0f);
    out_.push_back(FilledRect{{left_, y_}, {right_, y_ + config::kPanelBarHeight}, kBarBack});
    out_.push_back(FilledRect{{left_, y_}, {end, y_ + config::kPanelBarHeight}, colour});
    for (const BarMark& mark : marks)
    {
        if (mark.fraction <= 0.0f || mark.fraction >= 1.0f)
        {
            continue;
        }
        const float x = left_ + (right_ - left_) * mark.fraction;
        const maths::Vec2 top{x, y_ - config::kPanelMarkOverhang};
        const maths::Vec2 bottom{x, y_ + config::kPanelBarHeight + config::kPanelMarkOverhang};
        out_.push_back(Line{top, bottom, kBarBack, config::kPanelMarkThickness + 2.0f});
        out_.push_back(Line{top, bottom, mark.colour, config::kPanelMarkThickness});
    }
    y_ += config::kPanelBarHeight + config::kPanelRowGap;
}

// The gap after the last row stands in for part of the padding.
float PanelWriter::bottom() const noexcept
{
    return y_ - config::kPanelRowGap + config::kPanelPadding;
}

void PanelWriter::finish(Color background, Color border)
{
    const maths::Vec2 max{top_left_.x + width_, bottom()};
    const auto at = out_.begin() + static_cast<std::ptrdiff_t>(first_);
    out_.insert(at, {Primitive{FilledRect{top_left_, max, background}}, Primitive{Rect{top_left_, max, border, 1.5f}}});
}
} // namespace render
