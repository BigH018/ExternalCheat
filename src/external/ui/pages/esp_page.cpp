#include <imgui.h>

#include "color.h"
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
void draw_main(PageContext& ctx, settings::EspSettings& esp)
{
    widgets::panel_begin(ctx.fonts, "ESP", &esp.enabled,
                         "Draws boxes and labels over the bots, through walls. Everything here applies live.");
    keybind::bind_row(ctx.app, input::ActionId::esp_enable, "On / off key");
    settings::TeamMode& team_mode = ctx.app.settings.general.team_mode;
    team_mode_row(team_mode);
    if (team_mode == settings::TeamMode::teams)
    {
        widgets::switch_row("Show teammates", &esp.show_teammates, "Draw your own team too, in the team colours.");
    }
    max_distance_row(esp.max_distance, "Players further away than this aren't drawn. All the way left = no limit.");
    widgets::panel_end();
}

void draw_shapes(PageContext& ctx, settings::EspSettings& esp)
{
    widgets::panel_begin(ctx.fonts, "Shapes");
    widgets::switch_row("Box", &esp.box, "A box from the feet to just above the head.");
    if (esp.box)
    {
        static constexpr const char* kStyles[] = {"Full", "Corners"};
        choice("Box style", esp.box_style, kStyles);
    }
    widgets::switch_row("Outline", &esp.outline, "A dark edge around boxes, so they stand out on bright walls.");
    widgets::switch_row("Head circle", &esp.head_circle,
                        "A circle around the head (the middle of the head from the bones, or the eyes if bones can't "
                        "be read).");
    widgets::switch_row("Skeleton", &esp.skeleton, "Lines along the spine, arms and legs, from the bone positions.");
    widgets::switch_row("Snaplines", &esp.snaplines, "A line from the edge or centre of the screen to each player.");
    if (esp.snaplines)
    {
        static constexpr const char* kOrigins[] = {"Bottom", "Centre", "Top"};
        choice("Snaplines from", esp.snapline_origin, kOrigins);
    }
    widgets::slider_row("Thickness", &esp.thickness, config::kEspThickness.min, config::kEspThickness.max, "%.1f px");
    widgets::panel_end();
}

void draw_labels(PageContext& ctx, settings::EspSettings& esp)
{
    widgets::panel_begin(ctx.fonts, "Labels");
    widgets::switch_row("Name", &esp.name, "The player's name above the box.");
    widgets::switch_row("Health bar", &esp.health_bar, "A bar left of the box: green when healthy, red when nearly dead.");
    widgets::switch_row("Health number", &esp.health_number, "The health as a number beside the bar.");
    widgets::switch_row("Weapon", &esp.weapon, "The weapon in the player's hands, under the box.");
    widgets::switch_row("Distance", &esp.distance, "How far away the player is, in metres.");
    widgets::switch_row("Scoped tag", &esp.scoped_indicator,
                        "\"SCOPED\" above the name while the player is zoomed in (AWP, SSG 08, SCAR-20, G3SG1, AUG, "
                        "SG 553).");
    widgets::panel_end();
}

void draw_colours(PageContext& ctx, settings::EspSettings& esp)
{
    settings::EspColours& colours = esp.colours;
    widgets::panel_begin(ctx.fonts, "Colours");
    widgets::switch_row("Visible / hidden colours", &esp.visibility_colours,
                        "Uses the game's own \"spotted\" flag (the one that puts players on the radar): visible while "
                        "you have line of sight, hidden otherwise. It lags a little behind what you see. Off: always "
                        "the visible colour.");
    widgets::colour_row("Enemy, visible", colours.enemy_visible);
    widgets::colour_row("Enemy, hidden", colours.enemy_hidden);
    widgets::colour_row("Team, visible", colours.team_visible);
    widgets::colour_row("Team, hidden", colours.team_hidden);
    widgets::colour_row("Text", colours.text);
    widgets::colour_row("Skeleton", colours.skeleton);
    widgets::colour_row("Scoped tag", colours.scoped);
    if (widgets::button("Reset colours"))
    {
        colours = settings::EspColours{};
    }
    widgets::panel_end();
}
} // namespace

void draw_esp(PageContext& ctx)
{
    widgets::page_intro("Draws boxes and labels over the bots, through walls.");
    settings::EspSettings& esp = ctx.app.settings.esp;
    widgets::Columns columns;
    draw_main(ctx, esp);
    draw_shapes(ctx, esp);
    columns.next();
    draw_labels(ctx, esp);
    draw_colours(ctx, esp);
}
} // namespace ui::pages
