#include "features/esp.h"

#include <algorithm>
#include <cmath>
#include <format>

#include "config.h"
#include "features/targeting.h"
#include "game/weapon.h"
#include "maths/skeleton.h"

namespace features
{
namespace
{
constexpr Color kShadow{0.0f, 0.0f, 0.0f, 0.75f};
constexpr Color kHealthBarBack{0.0f, 0.0f, 0.0f, 0.6f};
constexpr Color kHealthFull = Color::rgb(0x5CD65C);
constexpr Color kHealthHalf = Color::rgb(0xF2D45C);
constexpr Color kHealthEmpty = Color::rgb(0xF25C5C);
constexpr float kMinBoxHeight = 2.0f; // pixels; smaller boxes aren't drawn
constexpr int kMaxHealth = 100;

bool off_screen(const ScreenBox& box, maths::Vec2 screen) noexcept
{
    return box.max.x < 0.0f || box.min.x > screen.x || box.max.y < 0.0f || box.min.y > screen.y;
}

maths::Vec2 snapline_origin(settings::SnaplineOrigin origin, maths::Vec2 screen) noexcept
{
    switch (origin)
    {
    case settings::SnaplineOrigin::centre: return {screen.x * 0.5f, screen.y * 0.5f};
    case settings::SnaplineOrigin::top: return {screen.x * 0.5f, 0.0f};
    case settings::SnaplineOrigin::bottom: break;
    }
    return {screen.x * 0.5f, screen.y};
}

class PlayerPainter
{
public:
    PlayerPainter(const settings::EspSettings& settings, std::vector<render::Primitive>& out, float line_height)
        : settings_(settings), out_(out), line_height_(line_height)
    {
    }

    void box(const ScreenBox& box, Color colour)
    {
        const float thickness = settings_.thickness;
        if (settings_.box_style == settings::BoxStyle::full)
        {
            if (settings_.outline)
            {
                out_.push_back(render::Rect{box.min, box.max, kShadow, thickness + 2.0f});
            }
            out_.push_back(render::Rect{box.min, box.max, colour, thickness});
            return;
        }
        // Corners: two arms per corner, each a fraction of the side.
        const float arm_x = box.width() * config::kEspCornerFraction;
        const float arm_y = box.height() * config::kEspCornerFraction;
        const maths::Vec2 corners[4] = {box.min, {box.max.x, box.min.y}, box.max, {box.min.x, box.max.y}};
        const float dir_x[4] = {1.0f, -1.0f, -1.0f, 1.0f};
        const float dir_y[4] = {1.0f, 1.0f, -1.0f, -1.0f};
        for (const bool shadow : {true, false})
        {
            if (shadow && !settings_.outline)
            {
                continue;
            }
            const Color c = shadow ? kShadow : colour;
            const float t = shadow ? thickness + 2.0f : thickness;
            for (int i = 0; i < 4; ++i)
            {
                out_.push_back(render::Line{corners[i], {corners[i].x + arm_x * dir_x[i], corners[i].y}, c, t});
                out_.push_back(render::Line{corners[i], {corners[i].x, corners[i].y + arm_y * dir_y[i]}, c, t});
            }
        }
    }

    // A vertical bar left of the box, filled from the bottom; the number (if on) beside its top.
    void health(const ScreenBox& box, int health)
    {
        const float clamped = static_cast<float>(std::clamp(health, 0, kMaxHealth));
        const float fill_top = box.max.y - box.height() * clamped / static_cast<float>(kMaxHealth);
        float label_right = box.min.x - config::kEspLabelGap;
        if (settings_.health_bar)
        {
            const float right = box.min.x - config::kEspHealthBarGap;
            const float left = right - config::kEspHealthBarWidth;
            out_.push_back(render::FilledRect{{left - 1.0f, box.min.y - 1.0f}, {right + 1.0f, box.max.y + 1.0f},
                                              kHealthBarBack});
            out_.push_back(render::FilledRect{{left, fill_top}, {right, box.max.y}, health_colour(health)});
            label_right = left - config::kEspLabelGap;
        }
        if (settings_.health_number)
        {
            const float y = settings_.health_bar ? std::max(box.min.y, fill_top - line_height_ * 0.5f) : box.min.y;
            out_.push_back(render::Text{{label_right, y}, std::to_string(health), health_colour(health),
                                        render::TextAnchor::top_right});
        }
    }

    // Labels above the box (scoped tag over the name), then below it (weapon, distance), stacked by line height.
    void labels(const ScreenBox& box, const game::PlayerSnapshot& player, float metres)
    {
        const float cx = box.centre_x();
        float above = box.min.y - config::kEspLabelGap;
        if (settings_.name)
        {
            out_.push_back(render::Text{{cx, above}, display_name(player.name), settings_.colours.text,
                                        render::TextAnchor::bottom_centre});
            above -= line_height_;
        }
        if (settings_.scoped_indicator && player.scoped)
        {
            out_.push_back(
                render::Text{{cx, above}, "SCOPED", settings_.colours.scoped, render::TextAnchor::bottom_centre});
        }

        float below = box.max.y + config::kEspLabelGap;
        if (settings_.weapon && player.weapon_id)
        {
            const std::string_view weapon = game::weapon_info(*player.weapon_id).name;
            if (!weapon.empty())
            {
                out_.push_back(render::Text{{cx, below}, std::string(weapon), settings_.colours.text,
                                            render::TextAnchor::top_centre});
                below += line_height_;
            }
        }
        if (settings_.distance)
        {
            out_.push_back(render::Text{{cx, below}, distance_text(metres), settings_.colours.text,
                                        render::TextAnchor::top_centre});
        }
    }

    void head_circle(const maths::ViewMatrix& view, const game::PlayerSnapshot& player, maths::Vec2 screen,
                     Color colour)
    {
        const maths::Vec3 head = player.head_position();
        const auto centre = maths::world_to_screen(view, head, screen);
        const auto edge = maths::world_to_screen(view, head + maths::Vec3{0.0f, 0.0f, config::kEspHeadRadius}, screen);
        if (!centre || !edge)
        {
            return;
        }
        const float radius = std::max((*edge - *centre).length(), 2.0f);
        out_.push_back(render::Circle{*centre, radius, colour, settings_.thickness});
    }

    void skeleton(const maths::ViewMatrix& view, const maths::Bones& bones, maths::Vec2 screen)
    {
        for (const maths::ScreenLine& line : maths::project_skeleton(view, bones, screen))
        {
            out_.push_back(render::Line{line.from, line.to, settings_.colours.skeleton, settings_.thickness});
        }
    }

    void snapline(const ScreenBox& box, maths::Vec2 screen, Color colour)
    {
        const maths::Vec2 from = snapline_origin(settings_.snapline_origin, screen);
        const bool from_top = settings_.snapline_origin == settings::SnaplineOrigin::top;
        const maths::Vec2 to{box.centre_x(), from_top ? box.min.y : box.max.y};
        out_.push_back(render::Line{from, to, colour.faded(0.8f), settings_.thickness});
    }

private:
    const settings::EspSettings& settings_;
    std::vector<render::Primitive>& out_;
    float line_height_;
};
} // namespace

std::optional<ScreenBox> player_box(const maths::ViewMatrix& view, const game::PlayerSnapshot& player,
                                    maths::Vec2 screen)
{
    const maths::Vec3 top_world{player.origin.x, player.origin.y,
                                player.eye_position().z + config::kEspBoxTopAboveEye};
    const auto feet = maths::world_to_screen(view, player.origin, screen);
    const auto top = maths::world_to_screen(view, top_world, screen);
    if (!feet || !top)
    {
        return std::nullopt;
    }
    const float height = feet->y - top->y;
    if (!(height >= kMinBoxHeight))
    {
        return std::nullopt;
    }
    const float half_width = height * config::kEspBoxAspect * 0.5f;
    const float centre_x = (feet->x + top->x) * 0.5f;
    return ScreenBox{{centre_x - half_width, top->y}, {centre_x + half_width, feet->y}};
}

Color health_colour(int health) noexcept
{
    const float fraction = static_cast<float>(std::clamp(health, 0, kMaxHealth)) / static_cast<float>(kMaxHealth);
    return fraction >= 0.5f ? kHealthHalf.lerp(kHealthFull, (fraction - 0.5f) * 2.0f)
                            : kHealthEmpty.lerp(kHealthHalf, fraction * 2.0f);
}

std::string display_name(std::string_view name)
{
    if (name.empty())
    {
        return "?";
    }
    if (name.size() <= config::kEspMaxNameLength)
    {
        return std::string(name);
    }
    return std::string(name.substr(0, config::kEspMaxNameLength - 3)) + "...";
}

std::string distance_text(float metres)
{
    return std::format("{:.0f} m", metres);
}

std::vector<render::Primitive> build_esp(const game::GameSnapshot& game, const settings::EspSettings& settings,
                                         settings::TeamMode team_mode, maths::Vec2 screen, float line_height)
{
    std::vector<render::Primitive> out;
    const game::PlayerSnapshot* local = game.local();
    if (!settings.enabled || !game.in_match || !game.view || local == nullptr)
    {
        return out;
    }
    const maths::ViewMatrix& view = *game.view;
    PlayerPainter painter(settings, out, line_height);
    for (const game::PlayerSnapshot& player : game.players)
    {
        if (!is_live_target(player))
        {
            continue;
        }
        const bool enemy = is_enemy(player, *local, team_mode);
        if (!enemy && (team_mode == settings::TeamMode::free_for_all || !settings.show_teammates))
        {
            continue;
        }
        const float metres = distance_metres(*local, player);
        if (!within_distance(metres, settings.max_distance))
        {
            continue;
        }
        const auto box = player_box(view, player, screen);
        if (!box || off_screen(*box, screen))
        {
            continue;
        }

        const bool visible = !settings.visibility_colours || is_visible_to(player, *local);
        const settings::EspColours& colours = settings.colours;
        const Color colour = enemy ? (visible ? colours.enemy_visible : colours.enemy_hidden)
                                   : (visible ? colours.team_visible : colours.team_hidden);

        if (settings.snaplines)
        {
            painter.snapline(*box, screen, colour);
        }
        if (settings.box)
        {
            painter.box(*box, colour);
        }
        if (settings.skeleton && player.bones)
        {
            painter.skeleton(view, *player.bones, screen);
        }
        if (settings.head_circle)
        {
            painter.head_circle(view, player, screen, colour);
        }
        if (settings.health_bar || settings.health_number)
        {
            painter.health(*box, player.health);
        }
        painter.labels(*box, player, metres);
    }
    return out;
}
} // namespace features
