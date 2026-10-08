#include "input/key_poll.h"

#include <cstddef>

#include <Windows.h>

namespace input
{
namespace
{
constexpr SHORT kDownBit = static_cast<SHORT>(0x8000); // GetAsyncKeyState: the key is down right now
} // namespace

bool is_key_down(std::uint32_t vk) noexcept
{
    return (GetAsyncKeyState(static_cast<int>(vk)) & kDownBit) != 0;
}

KeySet poll_keys() noexcept
{
    KeySet down;
    for (const KeyInfo& key : bindable_keys())
    {
        if (is_key_down(key.vk))
        {
            down.set(static_cast<std::size_t>(key.vk));
        }
    }
    if (is_key_down(kVkEscape))
    {
        down.set(static_cast<std::size_t>(kVkEscape)); // for the bind capture ("Escape clears")
    }
    return down;
}
} // namespace input
