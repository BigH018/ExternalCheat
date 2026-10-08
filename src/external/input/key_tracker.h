#pragma once

// Counts key presses from raw input events (WM_INPUT), so every press counts however slow a frame is: a tap shorter
// than a frame, or two taps inside one frame, still arrive (the overlay's frames can be slow while the game takes the
// GPU, and polling once per frame missed those taps).
//
// The overlay window registers for keyboard and mouse raw input in the background (RIDEV_INPUTSINK) and feeds every
// event here; app/frame takes the counts once per frame. That's Windows telling us about input it delivers anyway: not
// a hook, nothing in the game changes, the game gets every key as before. Held state still comes from GetAsyncKeyState
// (input/key_poll); this only counts presses.
//
// PURE: no <Windows.h> (the RAWKEYBOARD / RAWMOUSE fields arrive as plain integers).

#include <cstdint>

#include "input/keys.h"

namespace input
{
// RAWKEYBOARD::Flags and RAWMOUSE::usButtonFlags bits (winuser.h).
inline constexpr std::uint16_t kRawKeyBreak = 0x0001; // RI_KEY_BREAK: the key went up
inline constexpr std::uint16_t kRawKeyE0 = 0x0002;    // RI_KEY_E0: the extended (right-hand) variant
inline constexpr std::uint16_t kRawMouse1Down = 0x0001; // RI_MOUSE_BUTTON_1_DOWN; each button's "up" bit is down << 1
inline constexpr std::uint16_t kRawMouse2Down = 0x0004;
inline constexpr std::uint16_t kRawMouse3Down = 0x0010;
inline constexpr std::uint16_t kRawMouse4Down = 0x0040;
inline constexpr std::uint16_t kRawMouse5Down = 0x0100;

// The virtual key a raw keyboard event is for. Shift, Ctrl and Alt resolve to their left / right variants (as the
// bindable keys are); kUnbound for the fake keys Windows inserts into some sequences (VKey 0xFF).
[[nodiscard]] std::uint32_t raw_keyboard_vk(std::uint16_t vkey, std::uint16_t make_code,
                                            std::uint16_t flags) noexcept;

class KeyTracker
{
public:
    // One key went down or up. A "down" for a key that is already down is auto-repeat and isn't a press.
    void on_key(std::uint32_t vk, bool down) noexcept;
    // One raw keyboard event (RAWKEYBOARD's VKey, MakeCode and Flags).
    void on_keyboard(std::uint16_t vkey, std::uint16_t make_code, std::uint16_t flags) noexcept;
    // One raw mouse event's button transitions (RAWMOUSE::usButtonFlags); the wheel and movement are ignored.
    void on_mouse_buttons(std::uint16_t button_flags) noexcept;

    // The presses since the last call, then counting starts again.
    [[nodiscard]] PressCounts take_presses() noexcept;

private:
    KeySet down_;
    PressCounts presses_{};
};
} // namespace input
