// settings/presets: what each preset switches, what none of them touch, and that every value stays in its range.

#include <doctest.h>

#include "color.h"
#include "config.h"
#include "input/actions.h"
#include "settings/presets.h"

namespace
{
using settings::Preset;

// Everything a preset must leave alone, customised away from the defaults.
settings::Settings customised()
{
    settings::Settings s;
    s.overlay.watermark = false;
    s.overlay.theme = settings::MenuTheme::ice;
    s.overlay.accent = Color::rgba(0x11AA22FF);
    s.general.team_mode = settings::TeamMode::free_for_all;
    s.esp.show_teammates = true;
    s.esp.max_distance = 50.0f;
    s.esp.thickness = 3.0f;
    s.esp.visibility_colours = false;
    s.esp.colours.enemy_visible = Color::rgba(0x12345678);
    s.aimbot.team_check = false;
    s.aimbot.max_distance = 80.0f;
    s.aimbot.draw_fov = false;
    s.aimbot.fov_colour = Color::rgba(0x01020304);
    s.triggerbot.team_check = false;
    s.triggerbot.max_distance = 40.0f;
    s.triggerbot.shot_delay_ms = 400;
    s.triggerbot.burst_shots = 5;
    s.radar.corner = settings::RadarCorner::bottom_left;
    s.radar.size = 400.0f;
    s.radar.colours.you = Color::rgba(0xFF00FFFF);
    s.bomb_timer.top = 10.0f;
    s.spectators.side = settings::PanelSide::left;
    s.keybinds.bind(input::ActionId::aimbot_activate) = {0x06, input::BindMode::toggle};
    return s;
}

void check_untouched(const settings::Settings& before, const settings::Settings& after)
{
    CHECK(after.overlay == before.overlay);
    CHECK(after.general == before.general);
    CHECK(after.keybinds == before.keybinds);
    CHECK(after.esp.show_teammates == before.esp.show_teammates);
    CHECK(after.esp.max_distance == before.esp.max_distance);
    CHECK(after.esp.thickness == before.esp.thickness);
    CHECK(after.esp.visibility_colours == before.esp.visibility_colours);
    CHECK(after.esp.colours == before.esp.colours);
    CHECK(after.aimbot.team_check == before.aimbot.team_check);
    CHECK(after.aimbot.max_distance == before.aimbot.max_distance);
    CHECK(after.aimbot.draw_fov == before.aimbot.draw_fov);
    CHECK(after.aimbot.fov_colour == before.aimbot.fov_colour);
    CHECK(after.triggerbot.team_check == before.triggerbot.team_check);
    CHECK(after.triggerbot.max_distance == before.triggerbot.max_distance);
    CHECK(after.triggerbot.shot_delay_ms == before.triggerbot.shot_delay_ms);
    CHECK(after.triggerbot.burst_shots == before.triggerbot.burst_shots);
    CHECK(after.radar.corner == before.radar.corner);
    CHECK(after.radar.size == before.radar.size);
    CHECK(after.radar.colours == before.radar.colours);
    CHECK(after.bomb_timer.top == before.bomb_timer.top);
    CHECK(after.spectators.side == before.spectators.side);
}

void check_in_range(const settings::Settings& s)
{
    CHECK(config::kAimFov.contains(s.aimbot.fov));
    CHECK(config::kAimSmoothing.contains(s.aimbot.smoothing));
    CHECK(config::kTriggerReaction.contains(s.triggerbot.reaction_ms));
    CHECK(config::kEspThickness.contains(s.esp.thickness));
}

settings::Settings applied(Preset preset, settings::Settings s = settings::Settings{})
{
    settings::apply_preset(s, preset);
    return s;
}
} // namespace

TEST_CASE("presets: names and descriptions")
{
    CHECK(settings::kPresets.size() == 4);
    CHECK(settings::preset_name(Preset::off) == "Off");
    CHECK(settings::preset_name(Preset::chill) == "Chill");
    CHECK(settings::preset_name(Preset::medium) == "Medium");
    CHECK(settings::preset_name(Preset::rage) == "Rage");
    for (const Preset preset : settings::kPresets)
    {
        CHECK_FALSE(settings::preset_description(preset).empty());
    }
}

TEST_CASE("presets: none touch keybinds, colours, team options, distances or positions; values stay in range")
{
    const settings::Settings before = customised();
    for (const Preset preset : settings::kPresets)
    {
        CAPTURE(settings::preset_name(preset));
        const settings::Settings after = applied(preset, before);
        check_untouched(before, after);
        check_in_range(after);
    }
}

TEST_CASE("presets: Off turns every feature off and changes nothing else")
{
    settings::Settings everything_on = applied(Preset::rage);
    settings::Settings expected = everything_on;
    expected.esp.enabled = false;
    expected.aimbot.enabled = false;
    expected.triggerbot.enabled = false;
    expected.radar.enabled = false;
    expected.bomb_timer.enabled = false;
    expected.spectators.enabled = false;
    CHECK(applied(Preset::off, everything_on) == expected);
}

TEST_CASE("presets: Chill: basic ESP, slow narrow body aimbot on visible bots, no triggerbot, misc on")
{
    const settings::Settings s = applied(Preset::chill, applied(Preset::rage));
    CHECK(s.esp.enabled);
    CHECK(s.esp.box_style == settings::BoxStyle::corners);
    CHECK_FALSE(s.esp.skeleton);
    CHECK_FALSE(s.esp.snaplines);
    CHECK(s.esp.name);
    CHECK(s.esp.health_bar);
    CHECK(s.aimbot.enabled);
    CHECK(s.aimbot.target == settings::AimTarget::body);
    CHECK(s.aimbot.fov == doctest::Approx(3.0f));
    CHECK(s.aimbot.smoothing == doctest::Approx(12.0f));
    CHECK(s.aimbot.visible_only);
    CHECK_FALSE(s.triggerbot.enabled);
    CHECK(s.radar.enabled);
    CHECK(s.bomb_timer.enabled);
    CHECK(s.spectators.enabled);
}

TEST_CASE("presets: Medium: fuller ESP, faster head aimbot, key triggerbot with its limits")
{
    const settings::Settings s = applied(Preset::medium, applied(Preset::rage));
    CHECK(s.esp.box_style == settings::BoxStyle::full);
    CHECK(s.esp.skeleton);
    CHECK(s.esp.head_circle);
    CHECK_FALSE(s.esp.snaplines);
    CHECK(s.aimbot.target == settings::AimTarget::head);
    CHECK(s.aimbot.fov == doctest::Approx(6.0f));
    CHECK(s.aimbot.smoothing == doctest::Approx(6.0f));
    CHECK(s.aimbot.visible_only);
    CHECK(s.triggerbot.enabled);
    CHECK(s.triggerbot.activation == settings::TriggerActivation::key);
    CHECK(s.triggerbot.reaction_ms == 80);
    CHECK(s.triggerbot.fire_mode == settings::FireMode::single);
    CHECK(s.triggerbot.visible_only);
    CHECK(s.triggerbot.not_flashed);
    CHECK(s.triggerbot.not_in_air);
    CHECK(s.triggerbot.snipers_scoped_only);
}

TEST_CASE("presets: Rage: everything, snap aimbot, always-on triggerbot without limits")
{
    settings::Settings start;
    start.triggerbot.weapons.pistol = false;
    start.triggerbot.head_only = true;
    const settings::Settings s = applied(Preset::rage, start);
    CHECK(s.esp.enabled);
    CHECK(s.esp.snaplines);
    CHECK(s.esp.health_number);
    CHECK(s.aimbot.enabled);
    CHECK(s.aimbot.fov == config::kAimFov.max);
    CHECK(s.aimbot.smoothing == config::kAimSmoothing.min);
    CHECK_FALSE(s.aimbot.visible_only);
    CHECK(s.triggerbot.enabled);
    CHECK(s.triggerbot.activation == settings::TriggerActivation::always);
    CHECK(s.triggerbot.reaction_ms == 0);
    CHECK(s.triggerbot.fire_mode == settings::FireMode::hold);
    CHECK(s.triggerbot.weapons == settings::WeaponFilter{});
    CHECK_FALSE(s.triggerbot.head_only);
    CHECK_FALSE(s.triggerbot.not_flashed);
    CHECK_FALSE(s.triggerbot.not_in_air);
    CHECK_FALSE(s.triggerbot.snipers_scoped_only);
    CHECK(s.radar.enabled);
}
