#pragma once

// Which bindable keys (and Escape) are physically down right now, via GetAsyncKeyState. Main thread, once per frame.
//
// GetAsyncKeyState is global: it sees keys pressed in other programs too. app/frame only polls while the game or the
// overlay is the foreground window, and passes an empty set otherwise (so HOLD binds release on Alt+Tab). It reports
// the PHYSICAL mouse buttons, ignoring "swap left/right buttons" in Windows settings.

#include <cstdint>

#include "input/keys.h"

namespace input
{
[[nodiscard]] KeySet poll_keys() noexcept;

// One key, right now.
[[nodiscard]] bool is_key_down(std::uint32_t vk) noexcept;
} // namespace input
