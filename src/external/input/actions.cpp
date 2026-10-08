#include "input/actions.h"

#include <algorithm>

namespace input
{
namespace
{
constexpr std::array<BindMode, 2> kHoldOrToggle = {BindMode::hold, BindMode::toggle};
constexpr std::array<BindMode, 1> kPressOnly = {BindMode::press};

// A PRESS action with no default key that only works while you play.
constexpr ActionDef press(ActionId id, std::string_view key, std::string_view label, Category category)
{
    return ActionDef{id, key, label, category, false, Bind{kUnbound, BindMode::press}, false, false};
}

// Defaults: INSERT menu, END panic, DELETE exit (the final layout, CLAUDE.md Phase 9); the aimbot aims while Mouse 1
// is held (so it aims while you shoot), the triggerbot fires while Mouse 4 is held.
constexpr std::array<ActionDef, kActionCount> kActions = {{
    {ActionId::menu_toggle, "menu_toggle", "Show / hide the menu", Category::general, false,
     Bind{kVkInsert, BindMode::press}, true, true},
    {ActionId::panic, "panic", "Panic (every feature off)", Category::general, false, Bind{kVkEnd, BindMode::press},
     true, false},
    {ActionId::exit, "exit", "Exit the tool", Category::general, false, Bind{kVkDelete, BindMode::press}, true, false},
    {ActionId::aimbot_activate, "aimbot", "Aim key", Category::aimbot, true, Bind{kVkMouse1, BindMode::hold}, false,
     false},
    press(ActionId::aimbot_enable, "aimbot_enable_toggle", "Aimbot on / off", Category::aimbot),
    {ActionId::triggerbot_activate, "triggerbot", "Trigger key (activation: key)", Category::triggerbot, true,
     Bind{kVkMouse4, BindMode::hold}, false, false},
    press(ActionId::triggerbot_enable, "triggerbot_enable_toggle", "Triggerbot on / off", Category::triggerbot),
    press(ActionId::esp_enable, "esp_enable_toggle", "ESP on / off", Category::esp),
    press(ActionId::radar_enable, "radar_enable_toggle", "Radar on / off", Category::misc),
    press(ActionId::bomb_timer_enable, "bomb_timer_enable_toggle", "Bomb timer on / off", Category::misc),
    press(ActionId::spectators_enable, "spectators_enable_toggle", "Spectator list on / off", Category::misc),
    press(ActionId::preset_off, "preset_off", "Preset: Off", Category::presets),
    press(ActionId::preset_chill, "preset_chill", "Preset: Chill", Category::presets),
    press(ActionId::preset_medium, "preset_medium", "Preset: Medium", Category::presets),
    press(ActionId::preset_rage, "preset_rage", "Preset: Rage", Category::presets),
}};

constexpr bool in_id_order()
{
    for (std::size_t i = 0; i < kActions.size(); ++i)
    {
        if (index_of(kActions[i].id) != i)
        {
            return false;
        }
    }
    return true;
}
static_assert(in_id_order(), "kActions must list every action in ActionId order");
} // namespace

std::string_view category_name(Category category) noexcept
{
    switch (category)
    {
    case Category::general: return "General";
    case Category::aimbot: return "Aimbot";
    case Category::triggerbot: return "Triggerbot";
    case Category::esp: return "ESP";
    case Category::misc: return "Misc";
    case Category::presets: return "Presets";
    }
    return "";
}

std::span<const ActionDef> actions() noexcept
{
    return kActions;
}

const ActionDef& action(ActionId id) noexcept
{
    return kActions[index_of(id)];
}

std::span<const BindMode> allowed_modes(ActionId id) noexcept
{
    if (action(id).hold_or_toggle)
    {
        return kHoldOrToggle;
    }
    return kPressOnly;
}

bool mode_allowed(ActionId id, BindMode mode) noexcept
{
    const std::span<const BindMode> modes = allowed_modes(id);
    return std::find(modes.begin(), modes.end(), mode) != modes.end();
}

std::string_view mode_name(BindMode mode) noexcept
{
    switch (mode)
    {
    case BindMode::hold: return "Hold";
    case BindMode::toggle: return "Toggle";
    case BindMode::press: return "Press";
    }
    return "";
}

bool key_allowed(ActionId id, std::uint32_t key)
{
    if (action(id).hotkey)
    {
        return can_be_hotkey(key);
    }
    return key == kUnbound || is_bindable(key);
}

Binds default_binds() noexcept
{
    Binds binds{};
    for (const ActionDef& def : kActions)
    {
        binds[index_of(def.id)] = def.default_bind;
    }
    return binds;
}
} // namespace input
