// settings/profile_json: round trips, the file format, the forgiving load, and profiles/default.json == the code
// defaults.

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <Windows.h>

#include <doctest.h>

#include "color.h"
#include "config.h"
#include "input/actions.h"
#include "input/keys.h"
#include "settings/profile_json.h"

namespace
{
using input::ActionId;

bool has_warning(const std::vector<std::string>& warnings, const std::string& part)
{
    return std::any_of(warnings.begin(), warnings.end(),
                       [&](const std::string& w) { return w.find(part) != std::string::npos; });
}

settings::ParseResult parse(const std::string& text)
{
    return settings::from_json_text(text);
}

// The repo root, from tests.exe's own path (<repo>\bin\<Configuration>\tests.exe).
std::filesystem::path repo_root()
{
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path().parent_path().parent_path();
}
} // namespace

TEST_CASE("profile json: the defaults round-trip with no warnings")
{
    const settings::Settings defaults;
    const settings::ParseResult result = parse(settings::to_json_text(defaults));
    REQUIRE(result.ok);
    CHECK(result.warnings.empty());
    CHECK(result.settings == defaults);
}

TEST_CASE("profile json: a fully customised profile round-trips exactly")
{
    settings::Settings s;
    s.overlay.theme = settings::MenuTheme::violet;
    s.overlay.accent = Color::rgba(0x4080C0FF);
    s.overlay.watermark = false;
    s.overlay.frame_outline = true;
    s.general.team_mode = settings::TeamMode::free_for_all;
    s.esp.enabled = true;
    s.esp.max_distance = 75.5f;
    s.esp.box_style = settings::BoxStyle::corners;
    s.esp.thickness = 2.25f;
    s.esp.snapline_origin = settings::SnaplineOrigin::top;
    s.esp.colours.enemy_hidden = Color::rgba(0x11223344);
    s.esp.colours.scoped = Color::rgba(0xABCDEF01);
    s.aimbot.enabled = true;
    s.aimbot.target = settings::AimTarget::nearest;
    s.aimbot.priority = settings::AimPriority::lowest_health;
    s.aimbot.fov = 12.5f;
    s.aimbot.smoothing = 4.25f;
    s.aimbot.fov_colour = Color::rgba(0x01020304);
    s.triggerbot.enabled = true;
    s.triggerbot.activation = settings::TriggerActivation::always;
    s.triggerbot.reaction_ms = 123;
    s.triggerbot.fire_mode = settings::FireMode::burst;
    s.triggerbot.burst_shots = 7;
    s.triggerbot.shot_delay_ms = 999;
    s.triggerbot.weapons.sniper = false;
    s.triggerbot.weapons.heavy = false;
    s.triggerbot.head_only = true;
    s.radar.enabled = true;
    s.radar.corner = settings::RadarCorner::bottom_left;
    s.radar.size = 333.0f;
    s.radar.range = 99.75f;
    s.radar.rotate = false;
    s.radar.colours.background = Color::rgba(0x00000080);
    s.bomb_timer.enabled = true;
    s.bomb_timer.top = 42.0f;
    s.spectators.enabled = true;
    s.spectators.side = settings::PanelSide::left;
    s.spectators.hide_when_empty = true;
    s.keybinds.bind(ActionId::menu_toggle) = {0x74, input::BindMode::press}; // F5
    s.keybinds.bind(ActionId::aimbot_activate) = {input::kVkMouse4, input::BindMode::toggle};
    s.keybinds.bind(ActionId::panic) = {input::kUnbound, input::BindMode::press};
    s.keybinds.bind(ActionId::preset_rage) = {0x52, input::BindMode::press}; // R

    const settings::ParseResult result = parse(settings::to_json_text(s));
    REQUIRE(result.ok);
    CHECK(result.warnings.empty());
    CHECK(result.settings == s);
}

TEST_CASE("profile json: the file format (schema, sections, names, colours, keys)")
{
    settings::Settings s;
    s.keybinds.bind(ActionId::panic) = {input::kUnbound, input::BindMode::press};
    const std::string text = settings::to_json_text(s);
    CHECK(text.rfind("{\n  \"schema_version\": 1,\n  \"overlay\": {", 0) == 0);
    CHECK(text.find("\"team_mode\": \"teams\"") != std::string::npos);
    CHECK(text.find("\"enemy_visible\": \"#F25C5CFF\"") != std::string::npos);
    CHECK(text.find("\"fov_colour\": \"#EDEBF759\"") != std::string::npos);
    CHECK(text.find("\"thickness\": 1.5,") != std::string::npos);
    CHECK(text.find("\"reaction_ms\": 40,") != std::string::npos);
    CHECK(text.find("\"weapons\": {\n      \"pistol\": true") != std::string::npos);
    CHECK(text.find("\"menu_toggle\": {\n      \"key\": \"INSERT\",\n      \"mode\": \"press\"") != std::string::npos);
    CHECK(text.find("\"aimbot\": {\n      \"key\": \"Mouse 1\",\n      \"mode\": \"hold\"") != std::string::npos);
    CHECK(text.find("\"panic\": {\n      \"key\": null") != std::string::npos);
    CHECK(text.find("\"preset_rage\": {\n      \"key\": null") != std::string::npos);
    CHECK(text.back() == '\n');
}

TEST_CASE("profile json: invalid JSON fails; a non-object loads the defaults with a warning")
{
    CHECK_FALSE(parse("{ not json").ok);
    CHECK_FALSE(parse("").ok);
    const settings::ParseResult array = parse("[1, 2]");
    REQUIRE(array.ok);
    CHECK(array.settings == settings::Settings{});
    CHECK(has_warning(array.warnings, "not a JSON object"));
}

TEST_CASE("profile json: missing sections and fields keep their defaults")
{
    const settings::ParseResult result = parse(R"({"aimbot": {"enabled": true}, "esp": {"colours": {}}})");
    REQUIRE(result.ok);
    CHECK(result.warnings.empty());
    settings::Settings expected;
    expected.aimbot.enabled = true;
    CHECK(result.settings == expected);
}

TEST_CASE("profile json: unknown sections and fields are ignored with a warning")
{
    const settings::ParseResult result =
        parse(R"({"turbo": {}, "esp": {"enabled": true, "x_ray": true, "colours": {"glow": "#FFFFFF"}}})");
    REQUIRE(result.ok);
    CHECK(result.settings.esp.enabled);
    CHECK(has_warning(result.warnings, "turbo: unknown setting"));
    CHECK(has_warning(result.warnings, "esp.x_ray: unknown setting"));
    CHECK(has_warning(result.warnings, "esp.colours.glow: unknown setting"));
}

TEST_CASE("profile json: wrong types, bad enums and bad colours fall back to the default with a warning")
{
    const settings::ParseResult result = parse(R"({
        "aimbot": {"enabled": "yes", "target": "feet", "fov": "wide", "fov_colour": "red"},
        "esp": {"thickness": true, "colours": {"enemy_visible": "#12345"}},
        "general": {"team_mode": 1},
        "radar": {"colours": 5},
        "spectators": 5
    })");
    REQUIRE(result.ok);
    const settings::Settings defaults;
    CHECK(result.settings == defaults);
    CHECK(has_warning(result.warnings, "aimbot.enabled: expected true/false"));
    CHECK(has_warning(result.warnings, "aimbot.target: expected one of head, body, nearest"));
    CHECK(has_warning(result.warnings, "aimbot.fov: expected a number"));
    CHECK(has_warning(result.warnings, "aimbot.fov_colour: expected #RRGGBB"));
    CHECK(has_warning(result.warnings, "esp.thickness: expected a number")); // a bool is not a number
    CHECK(has_warning(result.warnings, "esp.colours.enemy_visible: expected #RRGGBB"));
    CHECK(has_warning(result.warnings, "general.team_mode: expected one of teams, free_for_all"));
    CHECK(has_warning(result.warnings, "radar.colours: expected an object"));
    CHECK(has_warning(result.warnings, "spectators: expected an object"));
}

TEST_CASE("profile json: numbers are clamped to their range and whole numbers rounded, with a warning")
{
    const settings::ParseResult result = parse(R"({
        "aimbot": {"fov": 999, "smoothing": 0.2},
        "triggerbot": {"reaction_ms": 12.6, "burst_shots": -4, "shot_delay_ms": 1e30},
        "radar": {"size": 300.5},
        "esp": {"max_distance": -1}
    })");
    REQUIRE(result.ok);
    CHECK(result.settings.aimbot.fov == config::kAimFov.max);
    CHECK(result.settings.aimbot.smoothing == config::kAimSmoothing.min);
    CHECK(result.settings.triggerbot.reaction_ms == 13);
    CHECK(result.settings.triggerbot.burst_shots == config::kTriggerBurst.min);
    CHECK(result.settings.triggerbot.shot_delay_ms == config::kTriggerShotDelay.max);
    CHECK(result.settings.radar.size == 300.5f);
    CHECK(result.settings.esp.max_distance == 0.0f);
    CHECK(has_warning(result.warnings, "aimbot.fov: 999 adjusted to 30"));
    CHECK(has_warning(result.warnings, "aimbot.smoothing: 0.2 adjusted to 1"));
    CHECK(has_warning(result.warnings, "triggerbot.reaction_ms: 12.6 adjusted to 13"));
    CHECK(has_warning(result.warnings, "triggerbot.burst_shots: -4 adjusted to 2"));
    CHECK(has_warning(result.warnings, "triggerbot.shot_delay_ms"));
    CHECK(has_warning(result.warnings, "esp.max_distance: -1 adjusted to 0"));
    CHECK_FALSE(has_warning(result.warnings, "radar.size")); // in range: no warning for a float
}

TEST_CASE("profile json: colours: #RRGGBB means opaque, case doesn't matter")
{
    const settings::ParseResult result =
        parse(R"({"esp": {"colours": {"enemy_visible": "#abcdef", "team_hidden": "#AbCdEf80"}}})");
    REQUIRE(result.ok);
    CHECK(result.warnings.empty());
    CHECK(result.settings.esp.colours.enemy_visible == Color::rgba(0xABCDEFFF));
    CHECK(result.settings.esp.colours.team_hidden == Color::rgba(0xABCDEF80));
}

TEST_CASE("profile json: keybinds: names, null = unbound, bad keys, keys and modes an action can't take")
{
    const settings::ParseResult result = parse(R"({"keybinds": {
        "esp_enable_toggle": {"key": "g", "mode": "press"},
        "panic": {"key": null},
        "radar_enable_toggle": {"key": "BANANA"},
        "aimbot": {"key": "mouse 5", "mode": "toggle"},
        "triggerbot": {"key": 5},
        "menu_toggle": {"key": "Mouse 1"},
        "exit": {"key": null, "mode": "hold"},
        "preset_chill": {"mode": "sideways"},
        "preset_off": {"key": "F9", "colour": "red"},
        "teleport": {"key": "T"},
        "preset_rage": "R"
    }})");
    REQUIRE(result.ok);
    const settings::KeybindSettings& k = result.settings.keybinds;
    const settings::KeybindSettings defaults;
    CHECK(k.bind(ActionId::esp_enable).key == 0x47);
    CHECK(k.bind(ActionId::panic).key == input::kUnbound);
    CHECK(k.bind(ActionId::radar_enable) == defaults.bind(ActionId::radar_enable));
    CHECK(k.bind(ActionId::aimbot_activate) == input::Bind{0x06, input::BindMode::toggle});
    CHECK(k.bind(ActionId::triggerbot_activate) == defaults.bind(ActionId::triggerbot_activate));
    CHECK(k.bind(ActionId::menu_toggle).key == input::kVkInsert); // a mouse button can't be the menu hotkey
    CHECK(k.bind(ActionId::exit) == input::Bind{input::kUnbound, input::BindMode::press}); // hold not allowed
    CHECK(k.bind(ActionId::preset_chill) == defaults.bind(ActionId::preset_chill));
    CHECK(k.bind(ActionId::preset_off).key == 0x78); // F9
    CHECK(has_warning(result.warnings, "keybinds.radar_enable_toggle: unknown key \"BANANA\""));
    CHECK(has_warning(result.warnings, "keybinds.triggerbot: unknown key 5"));
    CHECK(has_warning(result.warnings, "keybinds.menu_toggle: key \"Mouse 1\" can't be used"));
    CHECK(has_warning(result.warnings, "keybinds.exit: mode \"hold\" not allowed"));
    CHECK(has_warning(result.warnings, "keybinds.preset_chill: bad mode \"sideways\""));
    CHECK(has_warning(result.warnings, "keybinds.preset_off.colour: unknown setting"));
    CHECK(has_warning(result.warnings, "keybinds.teleport: unknown setting"));
    CHECK(has_warning(result.warnings, "keybinds.preset_rage: expected an object"));
    CHECK(result.warnings.size() == 8);
}

TEST_CASE("profile json: schema version: newer is loaded with a warning, invalid is assumed 1")
{
    const settings::ParseResult newer = parse(R"({"schema_version": 99, "esp": {"enabled": true}})");
    REQUIRE(newer.ok);
    CHECK(newer.settings.esp.enabled);
    CHECK(has_warning(newer.warnings, "newer version (99)"));
    const settings::ParseResult invalid = parse(R"({"schema_version": "one"})");
    REQUIRE(invalid.ok);
    CHECK(has_warning(invalid.warnings, "schema_version: invalid"));
    const settings::ParseResult current = parse(R"({"schema_version": 1})");
    REQUIRE(current.ok);
    CHECK(current.warnings.empty());
}

TEST_CASE("profile json: profiles/default.json in the repo equals the code defaults")
{
    const std::filesystem::path file = repo_root() / "profiles" / "default.json";
    std::ifstream in(file, std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    std::string actual = text.str();
    std::erase(actual, '\r'); // git may check the file out with CRLF line endings
    const std::string expected = settings::to_json_text(settings::Settings{});
    if (actual != expected)
    {
        // Leave the expected text next to it, so updating the file after a deliberate change is a copy.
        std::filesystem::path out_file = file;
        out_file += ".expected";
        std::ofstream(out_file, std::ios::binary) << expected;
        FAIL("profiles/default.json differs from the code defaults; expected text written to " << out_file.string());
    }
}
