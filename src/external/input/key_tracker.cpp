#include "input/key_tracker.h"

#include <array>
#include <cstddef>
#include <limits>

namespace input
{
namespace
{
constexpr std::uint16_t kFakeKey = 0xFF;
constexpr std::uint16_t kVkShift = 0x10;
constexpr std::uint16_t kVkControl = 0x11;
constexpr std::uint16_t kVkMenu = 0x12; // Alt
constexpr std::uint32_t kVkLShift = 0xA0;
constexpr std::uint32_t kVkRShift = 0xA1;
constexpr std::uint32_t kVkLControl = 0xA2;
constexpr std::uint32_t kVkRControl = 0xA3;
constexpr std::uint32_t kVkLMenu = 0xA4;
constexpr std::uint32_t kVkRMenu = 0xA5;
constexpr std::uint16_t kRightShiftScanCode = 0x36; // both Shifts share the VK; only the scan code differs

struct MouseButton
{
    std::uint16_t down_flag;
    std::uint32_t vk;
};
constexpr std::array<MouseButton, 5> kMouseButtons = {{
    {kRawMouse1Down, 0x01},
    {kRawMouse2Down, 0x02},
    {kRawMouse3Down, 0x04},
    {kRawMouse4Down, 0x05},
    {kRawMouse5Down, 0x06},
}};
} // namespace

std::uint32_t raw_keyboard_vk(std::uint16_t vkey, std::uint16_t make_code, std::uint16_t flags) noexcept
{
    const bool extended = (flags & kRawKeyE0) != 0;
    switch (vkey)
    {
    case kFakeKey: return kUnbound;
    case kVkShift: return make_code == kRightShiftScanCode ? kVkRShift : kVkLShift;
    case kVkControl: return extended ? kVkRControl : kVkLControl;
    case kVkMenu: return extended ? kVkRMenu : kVkLMenu;
    default: return vkey < KeySet().size() ? vkey : kUnbound;
    }
}

void KeyTracker::on_key(std::uint32_t vk, bool down) noexcept
{
    if (vk == kUnbound || vk >= down_.size())
    {
        return;
    }
    if (down && !down_[vk] && presses_[vk] < std::numeric_limits<std::uint8_t>::max())
    {
        ++presses_[vk];
    }
    down_[vk] = down;
}

void KeyTracker::on_keyboard(std::uint16_t vkey, std::uint16_t make_code, std::uint16_t flags) noexcept
{
    on_key(raw_keyboard_vk(vkey, make_code, flags), (flags & kRawKeyBreak) == 0);
}

void KeyTracker::on_mouse_buttons(std::uint16_t button_flags) noexcept
{
    for (const MouseButton& button : kMouseButtons)
    {
        if ((button_flags & button.down_flag) != 0)
        {
            on_key(button.vk, true);
        }
        if ((button_flags & (button.down_flag << 1)) != 0)
        {
            on_key(button.vk, false);
        }
    }
}

PressCounts KeyTracker::take_presses() noexcept
{
    const PressCounts presses = presses_;
    presses_.fill(0);
    return presses;
}
} // namespace input
