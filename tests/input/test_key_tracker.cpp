#include <cstdint>

#include <doctest.h>

#include "input/key_tracker.h"
#include "input/keys.h"

using input::KeyTracker;

namespace
{
constexpr std::uint32_t kF = 0x46;
constexpr std::uint16_t kDown = 0;
constexpr std::uint16_t kUp = input::kRawKeyBreak;
} // namespace

TEST_CASE("raw input: keyboard events map to the bindable virtual keys")
{
    CHECK(input::raw_keyboard_vk(0x46, 0x21, kDown) == kF);
    CHECK(input::raw_keyboard_vk(0x2D, 0x52, input::kRawKeyE0) == input::kVkInsert);
    CHECK(input::raw_keyboard_vk(0x10, 0x2A, kDown) == 0xA0); // left Shift
    CHECK(input::raw_keyboard_vk(0x10, 0x36, kDown) == 0xA1); // right Shift: same VK, its own scan code
    CHECK(input::raw_keyboard_vk(0x11, 0x1D, kDown) == 0xA2); // left Ctrl
    CHECK(input::raw_keyboard_vk(0x11, 0x1D, input::kRawKeyE0) == 0xA3);
    CHECK(input::raw_keyboard_vk(0x12, 0x38, kDown) == 0xA4); // left Alt
    CHECK(input::raw_keyboard_vk(0x12, 0x38, input::kRawKeyE0) == 0xA5);
    CHECK(input::raw_keyboard_vk(0xFF, 0x2A, kDown) == input::kUnbound); // fake key in an escape sequence
}

TEST_CASE("key tracker: one press per key-down, auto-repeat and releases don't count")
{
    KeyTracker tracker;
    tracker.on_keyboard(0x46, 0x21, kDown);
    tracker.on_keyboard(0x46, 0x21, kDown); // held: Windows repeats the key-down
    tracker.on_keyboard(0x46, 0x21, kDown);
    input::PressCounts presses = tracker.take_presses();
    CHECK(presses[kF] == 1);
    CHECK(tracker.take_presses()[kF] == 0); // taken: counting starts again

    tracker.on_keyboard(0x46, 0x21, kUp);
    tracker.on_keyboard(0x46, 0x21, kDown); // three quick taps before the next frame
    tracker.on_keyboard(0x46, 0x21, kUp);
    tracker.on_keyboard(0x46, 0x21, kDown);
    tracker.on_keyboard(0x46, 0x21, kUp);
    tracker.on_keyboard(0x46, 0x21, kDown);
    tracker.on_keyboard(0x46, 0x21, kUp);
    presses = tracker.take_presses();
    CHECK(presses[kF] == 3);
    CHECK(presses[0x47] == 0);
}

TEST_CASE("key tracker: mouse buttons, a click inside one event, and the wheel ignored")
{
    KeyTracker tracker;
    tracker.on_mouse_buttons(input::kRawMouse4Down);
    tracker.on_mouse_buttons(0); // movement
    tracker.on_mouse_buttons(static_cast<std::uint16_t>(input::kRawMouse4Down << 1)); // up
    tracker.on_mouse_buttons(static_cast<std::uint16_t>(input::kRawMouse1Down | (input::kRawMouse1Down << 1)));
    tracker.on_mouse_buttons(0x0400); // RI_MOUSE_WHEEL
    const input::PressCounts presses = tracker.take_presses();
    CHECK(presses[input::kVkMouse4] == 1);
    CHECK(presses[input::kVkMouse1] == 1); // down and up in the same event: still a press
    CHECK(presses[0x02] == 0);
    CHECK(presses[0x06] == 0);

    tracker.on_mouse_buttons(input::kRawMouse5Down);
    CHECK(tracker.take_presses()[0x06] == 1);
}

TEST_CASE("key tracker: counts stop at 255, unknown keys are ignored")
{
    KeyTracker tracker;
    for (int i = 0; i < 300; ++i)
    {
        tracker.on_key(kF, true);
        tracker.on_key(kF, false);
    }
    tracker.on_key(input::kUnbound, true);
    tracker.on_key(400, true);
    const input::PressCounts presses = tracker.take_presses();
    CHECK(presses[kF] == 255);
    CHECK(presses[input::kUnbound] == 0);
}
