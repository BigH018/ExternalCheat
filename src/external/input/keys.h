#pragma once

// Virtual-key codes <-> readable names: every keyboard key and mouse button a bind can use.
//
// Binds are virtual-key codes in memory; profiles (settings/profile_json) store the names ("INSERT", "Mouse 4", "F5"),
// so profiles stay readable.
// - The scroll wheel can't be polled with GetAsyncKeyState, so it isn't bindable.
// - Generic Shift/Ctrl/Alt (0x10-0x12) are left out on purpose: they report "down" together with the left/right
//   variants, so a capture would pick the wrong one. Use LSHIFT/RSHIFT etc.
// - Escape is reserved: in the bind capture it means "clear this bind".
//
// PURE: no <Windows.h> (the codes are the documented VK_* values).

#include <array>
#include <bitset>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace input
{
inline constexpr std::uint32_t kUnbound = 0; // "no key" (VK 0 doesn't exist)
inline constexpr std::uint32_t kVkMouse1 = 0x01; // left button
inline constexpr std::uint32_t kVkMouse4 = 0x05; // XBUTTON1, the back side button
inline constexpr std::uint32_t kVkEscape = 0x1B;
inline constexpr std::uint32_t kVkEnd = 0x23;
inline constexpr std::uint32_t kVkInsert = 0x2D;
inline constexpr std::uint32_t kVkDelete = 0x2E;
inline constexpr std::string_view kUnboundName = "Unbound";

// Every virtual key that is down right now (index = VK code).
using KeySet = std::bitset<256>;
// How often each virtual key went down since the last frame (index = VK code).
using PressCounts = std::array<std::uint8_t, 256>;

struct KeyInfo
{
    std::uint32_t vk;
    std::string_view name;
};

// Every bindable key, sorted by VK (Escape is not in it).
[[nodiscard]] std::span<const KeyInfo> bindable_keys();
[[nodiscard]] bool is_bindable(std::uint32_t vk);

// Display name: "Unbound" for kUnbound, "VK 0x.." for a code that isn't in the table.
[[nodiscard]] std::string key_name(std::uint32_t vk);

// VK code for a name (case-insensitive), or nullopt if unknown or "Unbound".
[[nodiscard]] std::optional<std::uint32_t> vk_from_name(std::string_view name);

// Mouse 1-5 (they never arrive as WM_KEYDOWN). 0x03 is VK_CANCEL, not a mouse button.
[[nodiscard]] constexpr bool is_mouse_button(std::uint32_t vk) noexcept
{
    return vk >= 0x01 && vk <= 0x06 && vk != 0x03;
}

// Shift, Ctrl, Alt (generic and left/right) and the Windows keys.
[[nodiscard]] constexpr bool is_modifier(std::uint32_t vk) noexcept
{
    return (vk >= 0x10 && vk <= 0x12) || (vk >= 0xA0 && vk <= 0xA5) || vk == 0x5B || vk == 0x5C;
}

// Whether RegisterHotKey can take this key on its own (the menu key): a bindable keyboard key, not a modifier.
[[nodiscard]] inline bool can_be_hotkey(std::uint32_t vk)
{
    return is_bindable(vk) && !is_mouse_button(vk) && !is_modifier(vk);
}
} // namespace input
