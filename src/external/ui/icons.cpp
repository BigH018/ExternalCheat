#include "ui/icons.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace ui
{
namespace
{
// Points are given in units of the icon's half size (-1..1), y down.
struct Pen
{
    ImDrawList* draw;
    ImVec2 centre;
    float r; // half the icon's size
    ImU32 colour;
    float thickness;

    [[nodiscard]] ImVec2 at(float x, float y) const { return ImVec2(centre.x + x * r, centre.y + y * r); }

    void line(float x1, float y1, float x2, float y2) const
    {
        draw->AddLine(at(x1, y1), at(x2, y2), colour, thickness);
    }
    void circle(float x, float y, float radius) const { draw->AddCircle(at(x, y), radius * r, colour, 0, thickness); }
    void dot(float x, float y, float radius) const { draw->AddCircleFilled(at(x, y), radius * r, colour); }
    void rect(float x1, float y1, float x2, float y2, float rounding) const
    {
        draw->AddRect(at(x1, y1), at(x2, y2), colour, rounding * r, thickness);
    }
    template <std::size_t N>
    void polyline(const std::array<ImVec2, N>& points, bool closed) const
    {
        std::array<ImVec2, N> screen{};
        for (std::size_t i = 0; i < N; ++i)
        {
            screen[i] = at(points[i].x, points[i].y);
        }
        draw->AddPolyline(screen.data(), static_cast<int>(N), colour, thickness,
                          closed ? ImDrawFlags_Closed : ImDrawFlags_None);
    }
};

void home(const Pen& p)
{
    p.polyline(std::array{ImVec2(-0.85f, -0.05f), ImVec2(0.0f, -0.8f), ImVec2(0.85f, -0.05f)}, false);
    p.polyline(std::array{ImVec2(-0.6f, -0.25f), ImVec2(-0.6f, 0.75f), ImVec2(0.6f, 0.75f), ImVec2(0.6f, -0.25f)},
               false);
    p.line(-0.15f, 0.75f, -0.15f, 0.3f);
    p.line(-0.15f, 0.3f, 0.15f, 0.3f);
    p.line(0.15f, 0.3f, 0.15f, 0.75f);
}

void aimbot(const Pen& p)
{
    p.circle(0.0f, 0.0f, 0.6f);
    p.line(0.0f, -0.95f, 0.0f, -0.3f);
    p.line(0.0f, 0.3f, 0.0f, 0.95f);
    p.line(-0.95f, 0.0f, -0.3f, 0.0f);
    p.line(0.3f, 0.0f, 0.95f, 0.0f);
    p.dot(0.0f, 0.0f, 0.1f);
}

void triggerbot(const Pen& p)
{
    p.polyline(std::array{ImVec2(0.2f, -0.95f), ImVec2(-0.5f, 0.12f), ImVec2(-0.02f, 0.12f), ImVec2(-0.2f, 0.95f),
                          ImVec2(0.5f, -0.12f), ImVec2(0.02f, -0.12f)},
               true);
}

void esp(const Pen& p)
{
    constexpr float kEdge = 0.85f;
    constexpr float kArm = 0.35f;
    for (const float sx : {-1.0f, 1.0f})
    {
        for (const float sy : {-1.0f, 1.0f})
        {
            p.line(sx * kEdge, sy * kEdge, sx * (kEdge - kArm), sy * kEdge);
            p.line(sx * kEdge, sy * kEdge, sx * kEdge, sy * (kEdge - kArm));
        }
    }
    p.circle(0.0f, -0.25f, 0.2f);
    p.polyline(std::array{ImVec2(-0.38f, 0.5f), ImVec2(-0.3f, 0.12f), ImVec2(0.3f, 0.12f), ImVec2(0.38f, 0.5f)},
               false);
}

void misc(const Pen& p)
{
    p.circle(0.0f, 0.0f, 0.85f);
    p.circle(0.0f, 0.0f, 0.42f);
    p.line(0.0f, 0.0f, 0.6f, -0.6f);
    p.dot(0.0f, 0.0f, 0.1f);
    p.dot(-0.35f, 0.45f, 0.11f);
}

void keybinds(const Pen& p)
{
    p.rect(-0.95f, -0.55f, 0.95f, 0.55f, 0.15f);
    for (const float x : {-0.55f, -0.18f, 0.18f, 0.55f})
    {
        p.dot(x, -0.18f, 0.08f);
    }
    p.line(-0.45f, 0.22f, 0.45f, 0.22f);
}

void settings(const Pen& p)
{
    constexpr int kTeeth = 8;
    p.circle(0.0f, 0.0f, 0.58f);
    p.circle(0.0f, 0.0f, 0.22f);
    for (int i = 0; i < kTeeth; ++i)
    {
        const float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / kTeeth;
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        p.line(c * 0.58f, s * 0.58f, c * 0.92f, s * 0.92f);
    }
}
} // namespace

void draw_icon(ImDrawList* draw, Icon icon, ImVec2 centre, float size, ImU32 colour)
{
    const Pen pen{draw, centre, size * 0.5f, colour, std::max(1.5f, size * 0.09f)};
    switch (icon)
    {
    case Icon::home: home(pen); break;
    case Icon::aimbot: aimbot(pen); break;
    case Icon::triggerbot: triggerbot(pen); break;
    case Icon::esp: esp(pen); break;
    case Icon::misc: misc(pen); break;
    case Icon::keybinds: keybinds(pen); break;
    case Icon::settings: settings(pen); break;
    }
}
} // namespace ui
