#include <imgui.h>

#include "config.h"
#include "settings/settings.h"
#include "ui/keybind_widgets.h"
#include "ui/pages/controls.h"
#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui::pages
{
namespace
{
void draw_main(PageContext& ctx, settings::AimbotSettings& aim)
{
    widgets::panel_begin(ctx.fonts, "Aimbot", &aim.enabled,
                         "Turns your view towards a bot while the aim key is held (or toggled on). It writes the game's "
                         "view angles from outside, so the camera visibly moves: there is no silent aim externally.");
    keybind::bind_row(ctx.app, input::ActionId::aimbot_activate, "Aim key",
                      "Hold: aims while the key is down. Toggle: each press switches aiming on or off.");
    keybind::bind_row(ctx.app, input::ActionId::aimbot_enable, "On / off key");
    if (aim.enabled)
    {
        const auto& status = ctx.app.aim_status;
        const Palette& p = palette();
        widgets::info_row("Status",
                          !status.active      ? "waiting for the aim key"
                          : status.has_target ? "aiming"
                                              : "nobody in the FOV",
                          status.has_target ? p.ok : p.text_dim);
    }
    widgets::panel_end();
}

void draw_targeting(PageContext& ctx, settings::AimbotSettings& aim)
{
    widgets::panel_begin(ctx.fonts, "Targeting");
    static constexpr const char* kTargets[] = {"Head", "Body", "Nearest"};
    choice("Aim at", aim.target, kTargets,
           "Head: the middle of the head. Body: the chest. Nearest: whichever of head, neck, chest, stomach and pelvis "
           "is closest to your crosshair.");
    static constexpr const char* kPriorities[] = {"Crosshair", "Distance", "Health"};
    choice("Priority", aim.priority, kPriorities,
           "Which bot to pick when several are inside the FOV: the one closest to your crosshair, the closest to you, "
           "or the one with the lowest health.");
    widgets::panel_end();
}

void draw_feel(PageContext& ctx, settings::AimbotSettings& aim)
{
    widgets::panel_begin(ctx.fonts, "Feel");
    widgets::slider_row("FOV", &aim.fov, config::kAimFov.min, config::kAimFov.max, "%.1f°",
                        "How far from your crosshair a bot may be, in degrees, for the aimbot to take it.");
    widgets::slider_row("Smoothing", &aim.smoothing, config::kAimSmoothing.min, config::kAimSmoothing.max,
                        aim.smoothing <= 1.0f ? "snap" : "%.1f",
                        "1 = snap straight onto the target. Higher = a slower, smoother turn. It feels the same at any "
                        "frame rate.");
    widgets::switch_row("Show FOV circle", &aim.draw_fov, "The FOV as a circle around the crosshair.");
    if (aim.draw_fov)
    {
        widgets::colour_row("Circle colour", aim.fov_colour);
    }
    widgets::panel_end();
}

void draw_filters(PageContext& ctx, settings::AimbotSettings& aim)
{
    widgets::panel_begin(ctx.fonts, "Filters");
    team_mode_row(ctx.app.settings.general.team_mode);
    widgets::switch_row("Team check", &aim.team_check, "Only aim at enemies (who is an enemy: the team mode above).");
    widgets::switch_row("Visible only", &aim.visible_only,
                        "Only aim at bots the game marks as spotted by you (the radar's line-of-sight flag). It lags a "
                        "little behind what you see.");
    max_distance_row(aim.max_distance, "Bots further away than this are ignored. All the way left = no limit.");
    widgets::panel_end();
}
} // namespace

void draw_aimbot(PageContext& ctx)
{
    widgets::page_intro("Turns your view towards a bot while the aim key is held.");
    settings::AimbotSettings& aim = ctx.app.settings.aimbot;
    widgets::Columns columns;
    draw_main(ctx, aim);
    draw_targeting(ctx, aim);
    columns.next();
    draw_feel(ctx, aim);
    draw_filters(ctx, aim);
}
} // namespace ui::pages
