#pragma once

// Every bindable action, plus the Bind / BindMode types.
//
// To add a bindable action: add it to ActionId (before count) and to kActions in actions.cpp, then handle it in
// app/frame (handle_actions). The Keybinds page and the default binds pick it up automatically.
//
// PURE: no <Windows.h>, no ImGui.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "input/keys.h"

namespace input
{
// How a bind turns key presses into an action state.
enum class BindMode : std::uint8_t
{
    hold,   // active while the key is held
    toggle, // each press switches it on or off
    press,  // fires once, on the frame the key goes down
};

struct Bind
{
    std::uint32_t key = kUnbound; // virtual-key code
    BindMode mode = BindMode::press;

    friend constexpr bool operator==(const Bind&, const Bind&) = default;
};

enum class ActionId : std::size_t
{
    // General
    menu_toggle,
    panic,
    exit,
    // Aimbot
    aimbot_activate,
    aimbot_enable,
    // Triggerbot
    triggerbot_activate,
    triggerbot_enable,
    // ESP
    esp_enable,
    // Misc
    radar_enable,
    bomb_timer_enable,
    spectators_enable,
    // Presets (settings/presets)
    preset_off,
    preset_chill,
    preset_medium,
    preset_rage,

    count,
};
inline constexpr std::size_t kActionCount = static_cast<std::size_t>(ActionId::count);

[[nodiscard]] constexpr std::size_t index_of(ActionId id) noexcept
{
    return static_cast<std::size_t>(id);
}

enum class Category : std::uint8_t
{
    general,
    aimbot,
    triggerbot,
    esp,
    misc,
    presets,
};
inline constexpr std::array kCategories{Category::general, Category::aimbot, Category::triggerbot, Category::esp,
                                        Category::misc, Category::presets};
[[nodiscard]] std::string_view category_name(Category category) noexcept;

struct ActionDef
{
    ActionId id;
    std::string_view key;   // stable id in profiles (settings/profile_json), e.g. "menu_toggle": never rename one
    std::string_view label; // shown on the Keybinds page, e.g. "Show / hide the menu"
    Category category;
    bool hold_or_toggle; // true: HOLD or TOGGLE; false: PRESS only
    Bind default_bind;
    bool works_in_menu; // also fires while the menu is open (panic, exit)
    // The menu key: a RegisterHotKey (receiving it is what lets the menu take focus from the game), so it must be a
    // keyboard key that isn't a modifier (input::can_be_hotkey), and it can't be unbound. app/frame handles it through
    // the overlay's hotkey, not through the keybind engine.
    bool hotkey;
};

// The registry, in ActionId order (so actions()[index_of(id)] is that action).
[[nodiscard]] std::span<const ActionDef> actions() noexcept;
[[nodiscard]] const ActionDef& action(ActionId id) noexcept;

// Allowed modes for an action (HOLD + TOGGLE, or PRESS only), and whether a mode is one of them.
[[nodiscard]] std::span<const BindMode> allowed_modes(ActionId id) noexcept;
[[nodiscard]] bool mode_allowed(ActionId id, BindMode mode) noexcept;
[[nodiscard]] std::string_view mode_name(BindMode mode) noexcept;

// Whether `key` may be bound to `id` (kUnbound included, except for the menu key).
[[nodiscard]] bool key_allowed(ActionId id, std::uint32_t key);

using Binds = std::array<Bind, kActionCount>;

// Every action's default bind.
[[nodiscard]] Binds default_binds() noexcept;
} // namespace input
