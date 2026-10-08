#pragma once

// "Press a key to bind" capture.
//
// The menu starts a capture for one action; every frame app/frame feeds it the keys that are down:
//   1. it first waits until every key it listens to is released (the click that started the capture may still be
//      held)
//   2. then the next key or mouse button that goes down is the new bind (the lowest VK if several in one frame)
//   3. Escape clears the bind (for the menu key, which can't be unbound, it cancels); clicking the button again or the
//      timeout (config::kBindCaptureTimeoutMs) cancels without changing anything
// The menu key only listens to keys RegisterHotKey can take (input::can_be_hotkey): no mouse buttons, no modifiers.
// While a capture runs, no keybind fires (app/frame suspends the engine and the menu hotkey). app/frame leaves Mouse 1
// out while the cursor is over the menu, so clicks in the menu stay clicks: Mouse 1 is bound by clicking outside it.
//
// PURE: no <Windows.h>, no ImGui.

#include <cstdint>
#include <optional>

#include "input/actions.h"
#include "input/keys.h"

namespace input
{
struct CaptureResult
{
    ActionId action;
    std::uint32_t key; // kUnbound = cleared with Escape
};

class BindCapture
{
public:
    // Starts (or moves) the capture to `action`.
    void start(ActionId action, std::uint64_t now_ms) noexcept;
    void cancel() noexcept { active_ = false; }

    [[nodiscard]] bool active() const noexcept { return active_; }
    [[nodiscard]] bool capturing(ActionId action) const noexcept { return active_ && action_ == action; }

    // Feed this frame's keys. Returns the result on the frame a key is accepted (the capture then ends). A timeout,
    // or Escape on the menu key, ends it with nullopt.
    std::optional<CaptureResult> update(const KeySet& down, std::uint64_t now_ms);

private:
    ActionId action_ = ActionId::menu_toggle;
    std::uint64_t started_ms_ = 0;
    bool active_ = false;
    bool waiting_for_release_ = false;
};
} // namespace input
