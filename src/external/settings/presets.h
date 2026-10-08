#pragma once

// Presets: one-click strength levels for the features (Settings page, and bindable keys in the Presets category).
//
// A preset switches FEATURES on or off and sets their strength (ESP elements, aimbot FOV / smoothing / aim point,
// triggerbot activation / delay / fire mode / filters). It never touches keybinds, colours, team mode, team checks,
// max distances, the overlay options or where the radar and panels sit, so the user's own setup around it stays.
// The result is an unsaved change: save it as a profile to keep it.
//   Off    - every feature off (the switches only)
//   Chill  - subtle: a basic ESP, a slow narrow aimbot on the body that only takes bots you can see, no triggerbot
//   Medium - a fuller ESP, a faster head aimbot, a triggerbot on the trigger key
//   Rage   - everything: the full ESP with snaplines, a snap aimbot over a wide FOV through walls, a triggerbot that
//            always fires straight away and holds attack, with no flash / air / scope limits
// Chill, Medium and Rage also turn the radar, the bomb timer and the spectator list on.
//
// PURE: no <Windows.h>, no ImGui.

#include <array>
#include <cstdint>
#include <string_view>

#include "settings/settings.h"

namespace settings
{
enum class Preset : std::uint8_t
{
    off,
    chill,
    medium,
    rage,
};
inline constexpr std::array kPresets{Preset::off, Preset::chill, Preset::medium, Preset::rage};

[[nodiscard]] std::string_view preset_name(Preset preset) noexcept;        // "Chill"
[[nodiscard]] std::string_view preset_description(Preset preset) noexcept; // one line for the Settings page

void apply_preset(Settings& settings, Preset preset) noexcept;
} // namespace settings
