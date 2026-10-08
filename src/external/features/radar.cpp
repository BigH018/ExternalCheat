#include "features/radar.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>

#include "config.h"
#include "features/esp.h"
#include "features/targeting.h"

namespace features
{
namespace
{
constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;
constexpr Color kBorder = Color::rgb(0x8CC4CF, 0.55f);
constexpr Color kGuide = Color::rgb(0xFFFFFF, 0.10f);  // the cross through you and the half-range ring
constexpr Color kOutline{0.0f, 0.0f, 0.0f, 0.75f};    // under each dot and your arrow
constexpr Color kRangeText = Color::rgb(0xEDEBF7, 0.6f);
constexpr Color kNameText = Color::rgb(0xEDEBF7);
constexpr float kBorderThickness = 1.5f;
constexpr float kFacingThickness = 1.5f;
constexpr float kEdgePadding = 2.0f; // pixels between a dot on the edge and the border
constexpr float kTextInset = 4.0f;   // the range label, from the bottom-right corner

// The yaw the radar's "up" points along: yours with rotation on, the world's +y with it off.
float radar_yaw(const game::GameSnapshot& game, const game::PlayerSnapshot& local, bool rotate) noexcept
{
    if (!rotate)
    {
        return config::kRadarNorthUpYaw;
    }
    return game.local_state.view_angles ? game.local_state.view_angles->yaw : local.eye_angles.yaw;
}

Color dot_colour(const game::PlayerSnapshot& player, const game::PlayerSnapshot& local, bool enemy,
                 const settings::RadarSettings& settings) noexcept
{
    const settings::RadarColours& colours = settings.colours;
    if (!enemy)
    {
        return colours.team;
    }
    const bool visible = !settings.visibility_colours || is_visible_to(player, local);
    return visible ? colours.enemy_visible : colours.enemy_hidden;
}

// You: an arrow in the middle pointing where you look.
void add_you(std::vector<render::Primitive>& out, maths::Vec2 centre, maths::Vec2 facing, float dot_size,
             Color colour)
{
    const float size = dot_size * config::kRadarYouSize;
    const maths::Vec2 side{-facing.y, facing.x};
    for (const bool outline : {true, false})
    {
        const float s = outline ? size + 1.5f : size;
        const maths::Vec2 tip = centre + facing * (s * 1.4f);
        const maths::Vec2 back = centre - facing * (s * 0.8f);
        out.push_back(render::FilledTriangle{tip, back + side * s, back - side * s, outline ? kOutline : colour});
    }
}
} // namespace

RadarPanel radar_panel(settings::RadarCorner corner, float size, maths::Vec2 screen, float margin) noexcept
{
    const bool right = corner == settings::RadarCorner::top_right || corner == settings::RadarCorner::bottom_right;
    const bool bottom = corner == settings::RadarCorner::bottom_left || corner == settings::RadarCorner::bottom_right;
    return RadarPanel{{right ? screen.x - margin - size : margin, bottom ? screen.y - margin - size : margin}, size};
}

maths::Vec2 radar_offset(maths::Vec3 you, maths::Vec3 point, float radar_yaw) noexcept
{
    // Forward = (cos b, sin b); right = forward turned 90 degrees clockwise seen from above = (sin b, -cos b).
    const float b = radar_yaw * kDegToRad;
    const float dx = point.x - you.x;
    const float dy = point.y - you.y;
    const float right = dx * std::sin(b) - dy * std::cos(b);
    const float ahead = dx * std::cos(b) + dy * std::sin(b);
    return {right, -ahead};
}

maths::Vec2 radar_direction(float yaw, float radar_yaw) noexcept
{
    const float a = yaw * kDegToRad;
    return radar_offset({}, {std::cos(a), std::sin(a), 0.0f}, radar_yaw);
}

maths::Vec2 clamp_to_square(maths::Vec2 point) noexcept
{
    const float reach = std::max(std::abs(point.x), std::abs(point.y));
    return reach > 1.0f ? point * (1.0f / reach) : point;
}

std::vector<render::Primitive> build_radar(const game::GameSnapshot& game, const settings::RadarSettings& settings,
                                           settings::TeamMode team_mode, maths::Vec2 screen)
{
    std::vector<render::Primitive> out;
    const game::PlayerSnapshot* local = game.local();
    if (!settings.enabled || !game.in_match || local == nullptr || local->pawn == 0)
    {
        return out;
    }

    const float size = config::kRadarSize.clamp(settings.size);
    const float dot = config::kRadarDotSize.clamp(settings.dot_size);
    const float range_units = config::kRadarRange.clamp(settings.range) * maths::kUnitsPerMetre;
    const RadarPanel panel = radar_panel(settings.corner, size, screen, config::kRadarMargin);
    const maths::Vec2 centre = panel.centre();
    const float reach = size * 0.5f - dot - kEdgePadding; // pixels from the centre to a dot on the edge
    const float yaw = radar_yaw(game, *local, settings.rotate);

    out.push_back(render::FilledRect{panel.min, panel.max(), settings.colours.background});
    out.push_back(render::Line{{centre.x, panel.min.y}, {centre.x, panel.max().y}, kGuide, 1.0f});
    out.push_back(render::Line{{panel.min.x, centre.y}, {panel.max().x, centre.y}, kGuide, 1.0f});
    out.push_back(render::Circle{centre, reach * 0.5f, kGuide, 1.0f});
    out.push_back(render::Rect{panel.min, panel.max(), kBorder, kBorderThickness});
    out.push_back(render::Text{{panel.max().x - kTextInset, panel.max().y - kTextInset},
                               std::format("{:.0f} m", config::kRadarRange.clamp(settings.range)), kRangeText,
                               render::TextAnchor::bottom_right});

    // Teammates first, enemies on top of them.
    for (const bool enemies : {false, true})
    {
        for (const game::PlayerSnapshot& player : game.players)
        {
            if (!is_live_target(player))
            {
                continue;
            }
            const bool enemy = is_enemy(player, *local, team_mode);
            if (enemy != enemies ||
                (!enemy && (team_mode != settings::TeamMode::teams || !settings.show_teammates)))
            {
                continue;
            }
            maths::Vec2 at = radar_offset(local->origin, player.origin, yaw) * (1.0f / range_units);
            const bool outside = std::max(std::abs(at.x), std::abs(at.y)) > 1.0f;
            if (outside && !settings.clamp_to_edge)
            {
                continue;
            }
            at = centre + clamp_to_square(at) * reach;
            Color colour = dot_colour(player, *local, enemy, settings);
            if (outside)
            {
                colour = colour.faded(config::kRadarEdgeFade);
            }
            if (settings.facing)
            {
                const maths::Vec2 tip =
                    at + radar_direction(player.eye_angles.yaw, yaw) * (dot * config::kRadarFacingLength);
                out.push_back(render::Line{at, tip, kOutline, kFacingThickness + 2.0f});
                out.push_back(render::Line{at, tip, colour, kFacingThickness});
            }
            out.push_back(render::FilledCircle{at, dot + 1.0f, kOutline});
            out.push_back(render::FilledCircle{at, dot, colour});
            if (settings.names)
            {
                out.push_back(render::Text{{at.x, at.y + dot + 1.0f}, display_name(player.name), kNameText,
                                           render::TextAnchor::top_centre});
            }
        }
    }

    add_you(out, centre, radar_direction(radar_yaw(game, *local, true), yaw), dot, settings.colours.you);
    return out;
}
} // namespace features
