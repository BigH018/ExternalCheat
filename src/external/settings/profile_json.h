#pragma once

// Settings <-> JSON text, for profiles (settings/profile_store). Same design as the AC project's profile_json.
//
// Format (profiles/<name>.json):
//   { "schema_version": 1,
//     "overlay": {...}, "general": {...}, "esp": {..., "colours": {...}}, "aimbot": {...},
//     "triggerbot": {..., "weapons": {...}}, "radar": {..., "colours": {...}}, "bomb_timer": {...},
//     "spectators": {...},
//     "keybinds": { "menu_toggle": {"key": "INSERT", "mode": "press"}, "aimbot": {"key": "Mouse 1", ...}, ... } }
// Colours are "#RRGGBBAA", enums are lower-case names ("free_for_all"), keys are input/keys names, null = unbound.
// Keybinds are keyed by input::ActionDef::key.
//
// Loading never fails on bad CONTENT: unknown keys are ignored, missing keys keep their defaults, wrong types / bad
// enums / bad colours / keys an action can't take fall back to the default, numbers are clamped to their config::Range.
// Every problem becomes a warning. Only text that isn't valid JSON fails.
//
// PURE: no <Windows.h>, no ImGui.

#include <string>
#include <string_view>
#include <vector>

#include "settings/settings.h"

namespace settings
{
// Pretty-printed JSON (2-space indent, trailing newline), fields in the order the menu shows them.
[[nodiscard]] std::string to_json_text(const Settings& settings);

struct ParseResult
{
    bool ok = false;                   // false = not valid JSON (settings are then the defaults)
    std::string error;                 // why, when !ok
    Settings settings;                 // what was loaded (defaults where something was wrong or missing)
    std::vector<std::string> warnings; // every content problem, e.g. "aimbot.fov: 99 adjusted to 30 (range/rounding)"
};

[[nodiscard]] ParseResult from_json_text(std::string_view text);
} // namespace settings
