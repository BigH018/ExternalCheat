#include "render/painter.h"

#include <cfloat>

namespace render
{
namespace
{
constexpr ImU32 kTextShadow = IM_COL32(0, 0, 0, 200);

ImU32 to_u32(const Color& colour)
{
    return ImGui::ColorConvertFloat4ToU32(ImVec4(colour.r, colour.g, colour.b, colour.a));
}

ImVec2 to_imvec(maths::Vec2 point)
{
    return ImVec2(point.x, point.y);
}

class Painter
{
public:
    Painter(ImDrawList& draw, ImFont* font, float font_size) : draw_(draw), font_(font), font_size_(font_size) {}

    void operator()(const Line& line) const
    {
        draw_.AddLine(to_imvec(line.from), to_imvec(line.to), to_u32(line.colour), line.thickness);
    }

    void operator()(const Rect& rect) const
    {
        draw_.AddRect(to_imvec(rect.min), to_imvec(rect.max), to_u32(rect.colour), 0.0f, rect.thickness);
    }

    void operator()(const FilledRect& rect) const
    {
        draw_.AddRectFilled(to_imvec(rect.min), to_imvec(rect.max), to_u32(rect.colour));
    }

    void operator()(const Circle& circle) const
    {
        draw_.AddCircle(to_imvec(circle.centre), circle.radius, to_u32(circle.colour), 0, circle.thickness);
    }

    void operator()(const FilledCircle& circle) const
    {
        draw_.AddCircleFilled(to_imvec(circle.centre), circle.radius, to_u32(circle.colour));
    }

    void operator()(const FilledTriangle& triangle) const
    {
        draw_.AddTriangleFilled(to_imvec(triangle.a), to_imvec(triangle.b), to_imvec(triangle.c),
                                to_u32(triangle.colour));
    }

    void operator()(const Text& text) const
    {
        const ImVec2 size = font_->CalcTextSizeA(font_size_, FLT_MAX, 0.0f, text.text.c_str());
        ImVec2 at = to_imvec(text.position);
        switch (text.anchor)
        {
        case TextAnchor::top_right: at.x -= size.x; break;
        case TextAnchor::top_centre: at.x -= size.x * 0.5f; break;
        case TextAnchor::bottom_centre:
            at.x -= size.x * 0.5f;
            at.y -= size.y;
            break;
        case TextAnchor::bottom_right:
            at.x -= size.x;
            at.y -= size.y;
            break;
        case TextAnchor::top_left: break;
        }
        // Whole pixels keep the text sharp.
        at = ImVec2(static_cast<float>(static_cast<int>(at.x)), static_cast<float>(static_cast<int>(at.y)));
        draw_.AddText(font_, font_size_, ImVec2(at.x + 1.0f, at.y + 1.0f), kTextShadow, text.text.c_str());
        draw_.AddText(font_, font_size_, at, to_u32(text.colour), text.text.c_str());
    }

private:
    ImDrawList& draw_;
    ImFont* font_;
    float font_size_;
};
} // namespace

void paint(ImDrawList& draw, std::span<const Primitive> primitives, ImFont* font, float font_size)
{
    const Painter painter(draw, font, font_size);
    for (const Primitive& primitive : primitives)
    {
        std::visit(painter, primitive);
    }
}
} // namespace render
