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
void draw_radar(PageContext& ctx, settings::RadarSettings& radar)
{
    widgets::panel_begin(ctx.fonts, "Radar", &radar.enabled,
                         "Our own radar, drawn by the overlay: you in the middle, every player as a dot, through "
                         "walls. The game's radar isn't touched.");
    keybind::bind_row(ctx.app, input::ActionId::radar_enable, "On / off key");
    static constexpr const char* kCorners[] = {"Top left", "Top right", "Bottom left", "Bottom right"};
    choice("Corner", radar.corner, kCorners, "Where the radar sits. Top left covers the game's own radar and the "
                                             "watermark.");
    widgets::slider_row("Size", &radar.size, config::kRadarSize.min, config::kRadarSize.max, "%.0f px");
    widgets::slider_row("Range", &radar.range, config::kRadarRange.min, config::kRadarRange.max, "%.0f m",
                        "How far the radar reaches, from you to its edge. Smaller = zoomed in.");
    widgets::slider_row("Dot size", &radar.dot_size, config::kRadarDotSize.min, config::kRadarDotSize.max, "%.1f px");
    widgets::switch_row("Rotate with view", &radar.rotate,
                        "On: where you look is always up. Off: the map stays still (north up, like the game's radar) "
                        "and your arrow turns.");
    settings::TeamMode& team_mode = ctx.app.settings.general.team_mode;
    team_mode_row(team_mode);
    if (team_mode == settings::TeamMode::teams)
    {
        widgets::switch_row("Show teammates", &radar.show_teammates, "Draw your own team too, in the team colour.");
    }
    widgets::switch_row("Facing", &radar.facing, "A short line from each dot, the way that player is looking.");
    widgets::switch_row("Names", &radar.names, "Each player's name under their dot.");
    widgets::switch_row("Out of range on the edge", &radar.clamp_to_edge,
                        "Players further away than the range sit on the edge in their direction, faded. Off: they're "
                        "hidden.");
    widgets::panel_end();
}

void draw_radar_colours(PageContext& ctx, settings::RadarSettings& radar)
{
    settings::RadarColours& colours = radar.colours;
    widgets::panel_begin(ctx.fonts, "Radar colours");
    widgets::switch_row("Visible / hidden colours", &radar.visibility_colours,
                        "Enemies you can see (the game's \"spotted\" flag, which lags a little) in the visible colour, "
                        "the rest in the hidden one. Off: every enemy in the visible colour.");
    widgets::colour_row("Enemy, visible", colours.enemy_visible);
    widgets::colour_row("Enemy, hidden", colours.enemy_hidden);
    widgets::colour_row("Team", colours.team);
    widgets::colour_row("You", colours.you);
    widgets::colour_row("Background", colours.background);
    if (widgets::button("Reset colours"))
    {
        colours = settings::RadarColours{};
    }
    widgets::panel_end();
}

void draw_bomb_timer(PageContext& ctx, settings::BombTimerSettings& timer)
{
    widgets::panel_begin(ctx.fonts, "Bomb timer", &timer.enabled,
                         "While a bomb is planted: a countdown at the top of the screen with the site, a bar, and who "
                         "is defusing. Only in modes with a bomb (Casual, Competitive), not Deathmatch.");
    keybind::bind_row(ctx.app, input::ActionId::bomb_timer_enable, "On / off key");
    widgets::slider_row("Height", &timer.top, config::kBombTimerTop.min, config::kBombTimerTop.max, "%.0f px",
                        "Distance from the top of the screen.");
    widgets::switch_row("Defuse hint", &timer.defuse_hint,
                        "Whether a defuse started now would make it: green = even without a kit (over 10 s left), "
                        "yellow = only with a kit (over 5 s), red = too late. While someone defuses: whether their "
                        "defuse finishes in time.");
    widgets::switch_row("Distance", &timer.distance, "How far you are from the bomb.");
    widgets::panel_end();
}

void draw_spectators(PageContext& ctx, settings::SpectatorSettings& list)
{
    widgets::panel_begin(ctx.fonts, "Spectator list", &list.enabled,
                         "Dead players watching you in first or third person. While you're dead and watch someone, it "
                         "lists who else watches them. Bots spectate after their ~5 s death cam, so you'll see them in "
                         "Casual or Competitive (in Deathmatch everyone respawns first).");
    keybind::bind_row(ctx.app, input::ActionId::spectators_enable, "On / off key");
    static constexpr const char* kSides[] = {"Left", "Right"};
    choice("Side", list.side, kSides);
    widgets::slider_row("Height", &list.top, config::kSpectatorTop.min, config::kSpectatorTop.max, "%.0f px",
                        "Distance from the top of the screen. The default sits under the radar (top right).");
    widgets::switch_row("Show the camera", &list.show_mode, "\"1st person\" or \"3rd person\" next to each name.");
    widgets::switch_row("Hide when nobody watches", &list.hide_when_empty, "Off: the panel says \"Nobody\" instead.");
    widgets::panel_end();
}
} // namespace

void draw_misc(PageContext& ctx)
{
    widgets::page_intro("Radar and match helpers, drawn by the overlay. Nothing here changes the game.");
    settings::RadarSettings& radar = ctx.app.settings.radar;
    widgets::Columns columns;
    draw_radar(ctx, radar);
    draw_radar_colours(ctx, radar);
    columns.next();
    draw_bomb_timer(ctx, ctx.app.settings.bomb_timer);
    draw_spectators(ctx, ctx.app.settings.spectators);
}
} // namespace ui::pages
