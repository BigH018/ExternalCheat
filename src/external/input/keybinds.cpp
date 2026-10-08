#include "input/keybinds.h"

#include <cstddef>
#include <map>
#include <utility>

namespace input
{
KeySet KeyFrame::down_or_pressed() const noexcept
{
    KeySet keys = down;
    for (std::size_t vk = 0; vk < presses.size(); ++vk)
    {
        if (presses[vk] > 0)
        {
            keys.set(vk);
        }
    }
    return keys;
}

PressCounts presses_from_edges(const KeySet& before, const KeySet& now) noexcept
{
    PressCounts presses{};
    const KeySet edges = now & ~before;
    for (std::size_t vk = 0; vk < presses.size(); ++vk)
    {
        presses[vk] = static_cast<std::uint8_t>(edges[vk] ? 1 : 0);
    }
    return presses;
}

ActionStates KeybindEngine::update(const KeyFrame& keys, const Binds& binds, Suspension suspension)
{
    ActionStates states;
    if (!primed_)
    {
        primed_ = true;
        states.active = toggled_on_;
        return states;
    }
    for (std::size_t i = 0; i < kActionCount; ++i)
    {
        const Bind& bind = binds[i];
        const bool allowed = suspension == Suspension::none ||
                             (suspension == Suspension::menu_open && action(static_cast<ActionId>(i)).works_in_menu);
        if (!allowed || bind.key == kUnbound || bind.key >= keys.down.size())
        {
            states.active[i] = toggled_on_[i] && bind.mode == BindMode::toggle; // toggles keep their state
            continue;
        }
        const std::size_t key = bind.key;
        const std::uint8_t presses = keys.presses[key];
        states.presses[i] = presses;
        switch (bind.mode)
        {
        case BindMode::hold:
            states.active[i] = keys.down[key];
            break;
        case BindMode::toggle:
            if (presses % 2 == 1)
            {
                toggled_on_.flip(i); // two presses inside one frame: on and off again
            }
            states.active[i] = toggled_on_[i];
            break;
        case BindMode::press:
            break;
        }
    }
    return states;
}

std::vector<Conflict> find_conflicts(const Binds& binds)
{
    std::map<std::uint32_t, std::vector<ActionId>> by_key;
    for (std::size_t i = 0; i < kActionCount; ++i)
    {
        if (binds[i].key != kUnbound)
        {
            by_key[binds[i].key].push_back(static_cast<ActionId>(i));
        }
    }
    std::vector<Conflict> conflicts;
    for (auto& [key, ids] : by_key)
    {
        if (ids.size() > 1)
        {
            conflicts.push_back(Conflict{key, std::move(ids)});
        }
    }
    return conflicts;
}
} // namespace input
