#include <doctest.h>

#include "config.h"
#include "input/actions.h"
#include "input/keybinds.h"
#include "settings/settings.h"

TEST_CASE("Settings defaults: ESP off, sensible options, every number inside its range")
{
    const settings::Settings defaults;
    CHECK(defaults.overlay.watermark);
    CHECK_FALSE(defaults.overlay.frame_outline);

    CHECK(defaults.general.team_mode == settings::TeamMode::teams);

    const settings::EspSettings& esp = defaults.esp;
    CHECK_FALSE(esp.enabled); // nothing draws until the user turns it on
    CHECK(esp.box);
    CHECK(esp.name);
    CHECK(esp.health_bar);
    CHECK(esp.visibility_colours);
    CHECK(config::kEspThickness.contains(esp.thickness));
    CHECK(config::kMaxDistance.contains(esp.max_distance));
    CHECK(esp.colours.enemy_visible.a == doctest::Approx(1.0f));
}

TEST_CASE("Settings defaults: aimbot and triggerbot off, every number inside its range")
{
    const settings::Settings defaults;
    const settings::AimbotSettings& aim = defaults.aimbot;
    CHECK_FALSE(aim.enabled); // nothing writes to the game until the user turns it on
    CHECK(aim.team_check);
    CHECK(config::kAimFov.contains(aim.fov));
    CHECK(config::kAimSmoothing.contains(aim.smoothing));
    CHECK(config::kMaxDistance.contains(aim.max_distance));

    const settings::TriggerbotSettings& trigger = defaults.triggerbot;
    CHECK_FALSE(trigger.enabled);
    CHECK(trigger.activation == settings::TriggerActivation::key);
    CHECK(trigger.team_check);
    CHECK(config::kTriggerReaction.contains(trigger.reaction_ms));
    CHECK(config::kTriggerShotDelay.contains(trigger.shot_delay_ms));
    CHECK(config::kTriggerBurst.contains(trigger.burst_shots));
    CHECK(config::kMaxDistance.contains(trigger.max_distance));
}

TEST_CASE("Settings defaults: radar off, every number inside its range")
{
    const settings::RadarSettings radar = settings::Settings{}.radar;
    CHECK_FALSE(radar.enabled);
    CHECK(radar.corner == settings::RadarCorner::top_right); // clear of the watermark and the game's radar
    CHECK(config::kRadarSize.contains(radar.size));
    CHECK(config::kRadarRange.contains(radar.range));
    CHECK(config::kRadarDotSize.contains(radar.dot_size));
    CHECK(radar.colours.background.a < 1.0f); // see-through
}

TEST_CASE("Settings defaults: bomb timer off, inside its range")
{
    const settings::BombTimerSettings timer = settings::Settings{}.bomb_timer;
    CHECK_FALSE(timer.enabled);
    CHECK(config::kBombTimerTop.contains(timer.top));
    CHECK(timer.defuse_hint);
    CHECK(timer.distance);
}

TEST_CASE("Settings defaults: spectator list off, on the right under the radar, inside its range")
{
    const settings::Settings defaults;
    const settings::SpectatorSettings& list = defaults.spectators;
    CHECK_FALSE(list.enabled);
    CHECK(list.side == settings::PanelSide::right);
    CHECK(config::kSpectatorTop.contains(list.top));
    CHECK(list.top > config::kRadarMargin + defaults.radar.size); // clear of the default radar (top right)
    CHECK(list.show_mode);
    CHECK_FALSE(list.hide_when_empty);
}

TEST_CASE("Settings defaults: keybinds are the registry's defaults, with no conflicts")
{
    const settings::KeybindSettings keybinds = settings::Settings{}.keybinds;
    CHECK(keybinds.binds == input::default_binds());
    CHECK(keybinds.bind(input::ActionId::menu_toggle).key == input::kVkInsert);
    CHECK(keybinds.bind(input::ActionId::panic).key == input::kVkEnd);
    CHECK(keybinds.bind(input::ActionId::exit).key == input::kVkDelete);
    CHECK(keybinds.bind(input::ActionId::aimbot_activate) == input::Bind{input::kVkMouse1, input::BindMode::hold});
    CHECK(keybinds.bind(input::ActionId::triggerbot_activate) == input::Bind{input::kVkMouse4, input::BindMode::hold});
    CHECK(input::find_conflicts(keybinds.binds).empty());
}

TEST_CASE("config::Range")
{
    constexpr config::Range<float> range{1.0f, 4.0f};
    CHECK(range.contains(1.0f));
    CHECK(range.contains(4.0f));
    CHECK_FALSE(range.contains(4.5f));
    CHECK(range.clamp(9.0f) == 4.0f);
    CHECK(range.clamp(-1.0f) == 1.0f);
    CHECK(range.clamp(2.5f) == 2.5f);
}
