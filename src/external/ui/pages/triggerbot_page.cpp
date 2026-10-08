#include <imgui.h>

#include "config.h"
#include "features/triggerbot.h"
#include "settings/settings.h"
#include "ui/keybind_widgets.h"
#include "ui/pages/controls.h"
#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui::pages
{
namespace
{
const char* block_text(features::TriggerBlock block)
{
    switch (block)
    {
    case features::TriggerBlock::none: return "on target";
    case features::TriggerBlock::not_in_match: return "not in a match (or dead)";
    case features::TriggerBlock::weapon: return "this weapon is filtered out";
    case features::TriggerBlock::not_scoped: return "sniper not scoped";
    case features::TriggerBlock::flashed: return "flashed";
    case features::TriggerBlock::in_air: return "in the air";
    case features::TriggerBlock::no_target: return "waiting for a target";
    }
    return "";
}

void draw_main(PageContext& ctx, settings::TriggerbotSettings& trigger)
{
    widgets::panel_begin(ctx.fonts, "Triggerbot", &trigger.enabled,
                         "Fires for you while an enemy is under your crosshair (the game's own \"what am I aiming at\" "
                         "value).");
    static constexpr const char* kActivations[] = {"Always on", "Trigger key"};
    choice("Activation", trigger.activation, kActivations,
           "Always on: whenever it's enabled. Trigger key: while the key below is held, or toggled on (its mode).");
    if (trigger.activation == settings::TriggerActivation::key)
    {
        keybind::bind_row(ctx.app, input::ActionId::triggerbot_activate, "Trigger key",
                          "Hold: fires while the key is down. Toggle: each press switches it on or off.");
    }
    keybind::bind_row(ctx.app, input::ActionId::triggerbot_enable, "On / off key");
    if (trigger.enabled)
    {
        const auto& status = ctx.app.trigger_status;
        const Palette& p = palette();
        const bool on_target = status.active && status.block == features::TriggerBlock::none;
        widgets::info_row("Status", !status.active ? "waiting for the key" : block_text(status.block),
                          on_target ? p.ok : p.text_dim);
    }
    widgets::panel_end();
}

void draw_firing(PageContext& ctx, settings::TriggerbotSettings& trigger)
{
    widgets::panel_begin(ctx.fonts, "Firing");
    widgets::slider_row("Reaction delay", &trigger.reaction_ms, config::kTriggerReaction.min,
                        config::kTriggerReaction.max, "%d ms",
                        "How long a target has to be under the crosshair before the first shot.");
    static constexpr const char* kModes[] = {"Single", "Burst", "Hold"};
    choice("Fire mode", trigger.fire_mode, kModes,
           "Single: one shot, then the delay below. Burst: N shots the delay apart. Hold: attack stays down while the "
           "target stays under the crosshair.");
    if (trigger.fire_mode == settings::FireMode::burst)
    {
        widgets::slider_row("Shots per burst", &trigger.burst_shots, config::kTriggerBurst.min,
                            config::kTriggerBurst.max, "%d");
    }
    if (trigger.fire_mode != settings::FireMode::hold)
    {
        widgets::slider_row("Delay between shots", &trigger.shot_delay_ms, config::kTriggerShotDelay.min,
                            config::kTriggerShotDelay.max, "%d ms",
                            "Between the shots of a burst, and before the next tap or burst.");
    }
    widgets::panel_end();
}

void draw_filters(PageContext& ctx, settings::TriggerbotSettings& trigger)
{
    widgets::panel_begin(ctx.fonts, "Filters");
    team_mode_row(ctx.app.settings.general.team_mode);
    widgets::switch_row("Team check", &trigger.team_check,
                        "Only fire at enemies (who is an enemy: the team mode above).");
    widgets::switch_row("Visible only", &trigger.visible_only,
                        "Only fire at bots the game marks as spotted by you (the radar's line-of-sight flag).");
    widgets::switch_row("Head only", &trigger.head_only,
                        "Only fire while the crosshair is on the head (from the bones).");
    max_distance_row(trigger.max_distance, "Bots further away than this are ignored. All the way left = no limit.");
    widgets::subheading("HOLD FIRE");
    widgets::switch_row("Snipers: only when scoped", &trigger.snipers_scoped_only,
                        "With an AWP, SSG 08, SCAR-20 or G3SG1, only fire while zoomed in.");
    widgets::switch_row("While flashed", &trigger.not_flashed,
                        "Hold fire while more than half of a flash is still on screen.");
    widgets::switch_row("While in the air", &trigger.not_in_air,
                        "Hold fire while you're jumping or falling (shots go wide).");
    widgets::panel_end();
}

void draw_weapons(PageContext& ctx, settings::TriggerbotSettings& trigger)
{
    widgets::panel_begin(ctx.fonts, "Weapons", nullptr,
                         "Fire only with these weapon classes. Knives, grenades and the C4 never fire.");
    settings::WeaponFilter& w = trigger.weapons;
    widgets::chips({{"Pistols", &w.pistol},
                    {"SMGs", &w.smg},
                    {"Rifles", &w.rifle},
                    {"Snipers", &w.sniper},
                    {"Shotguns", &w.shotgun},
                    {"Heavy", &w.heavy}});
    widgets::panel_end();
}
} // namespace

void draw_triggerbot(PageContext& ctx)
{
    widgets::page_intro("Fires when a living enemy bot is under the crosshair.");
    settings::TriggerbotSettings& trigger = ctx.app.settings.triggerbot;
    widgets::Columns columns;
    draw_main(ctx, trigger);
    draw_firing(ctx, trigger);
    draw_weapons(ctx, trigger);
    columns.next();
    draw_filters(ctx, trigger);
}
} // namespace ui::pages
