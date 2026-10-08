#include "input/bind_capture.h"

#include <cstddef>

#include "config.h"

namespace input
{
namespace
{
// The keys a capture for `action` listens to: what it may be bound to, plus Escape.
bool watched(ActionId action, std::uint32_t vk)
{
    return vk == kVkEscape || (vk != kUnbound && key_allowed(action, vk));
}

// The lowest watched VK that is down, or nullopt.
std::optional<std::uint32_t> first_down(ActionId action, const KeySet& down)
{
    for (std::size_t vk = 0; vk < down.size(); ++vk)
    {
        if (down[vk] && watched(action, static_cast<std::uint32_t>(vk)))
        {
            return static_cast<std::uint32_t>(vk);
        }
    }
    return std::nullopt;
}
} // namespace

void BindCapture::start(ActionId action, std::uint64_t now_ms) noexcept
{
    action_ = action;
    started_ms_ = now_ms;
    active_ = true;
    waiting_for_release_ = true;
}

std::optional<CaptureResult> BindCapture::update(const KeySet& down, std::uint64_t now_ms)
{
    if (!active_)
    {
        return std::nullopt;
    }
    if (now_ms >= started_ms_ + config::kBindCaptureTimeoutMs)
    {
        active_ = false;
        return std::nullopt;
    }
    const std::optional<std::uint32_t> pressed = first_down(action_, down);
    if (waiting_for_release_)
    {
        if (!pressed)
        {
            waiting_for_release_ = false; // everything released: the next press counts
        }
        return std::nullopt;
    }
    if (!pressed)
    {
        return std::nullopt;
    }
    active_ = false;
    // Escape wins even if another key went down in the same frame: it's the explicit "clear".
    if (down[kVkEscape])
    {
        if (action(action_).hotkey)
        {
            return std::nullopt; // the menu key can't be unbound: Escape cancels
        }
        return CaptureResult{action_, kUnbound};
    }
    return CaptureResult{action_, *pressed};
}
} // namespace input
