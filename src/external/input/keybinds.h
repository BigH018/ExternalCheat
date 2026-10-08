#pragma once

// The keybind engine: "which keys are down, and how often each was pressed, this frame" -> action states.
//
// Per frame (app/frame):
//   states = engine.update(keys, settings.keybinds.binds, suspension);
//   states.is_active(ActionId::aimbot_activate) // HOLD: held right now / TOGGLE: toggled on
//   states.press_count(ActionId::esp_enable)    // presses this frame (any mode); did_fire = at least one
// Presses come from raw input (input/key_tracker), not from comparing frames, so they don't depend on the frame rate:
// a tap shorter than a frame counts, and two taps inside one frame flip a toggle twice.
//
// PURE: no <Windows.h>, no ImGui.

#include <array>
#include <bitset>
#include <cstdint>
#include <vector>

#include "input/actions.h"
#include "input/keys.h"

namespace input
{
// This frame's keys.
struct KeyFrame
{
    KeySet down;           // held right now (GetAsyncKeyState)
    PressCounts presses{}; // how often each key went down since the last frame (raw input)

    // Held now or pressed during the frame (a quick tap): what the bind capture looks at.
    [[nodiscard]] KeySet down_or_pressed() const noexcept;
};

// Presses from two polls of the held keys (a rising edge = one press), for when raw input isn't available. Misses taps
// shorter than a frame.
[[nodiscard]] PressCounts presses_from_edges(const KeySet& before, const KeySet& now) noexcept;

// Why actions are (partly) held back this frame.
enum class Suspension : std::uint8_t
{
    none,
    menu_open, // only actions with works_in_menu fire (panic, exit); HOLD reads released, toggles keep their state
    capture,   // a bind is being captured: nothing fires, toggles keep their state
};

struct ActionStates
{
    std::bitset<kActionCount> active;             // HOLD held / TOGGLE on
    std::array<std::uint8_t, kActionCount> presses{}; // presses of the action's key this frame (0 while held back)

    [[nodiscard]] bool is_active(ActionId id) const { return active.test(index_of(id)); }
    [[nodiscard]] int press_count(ActionId id) const { return presses[index_of(id)]; }
    [[nodiscard]] bool did_fire(ActionId id) const { return press_count(id) > 0; }
};

class KeybindEngine
{
public:
    // Advance one frame. Presses while an action is held back are dropped, not saved for later. The very first update
    // only takes the toggle states: a key pressed while the tool starts must not fire.
    ActionStates update(const KeyFrame& keys, const Binds& binds, Suspension suspension);

    // Turn every TOGGLE action off (panic).
    void reset_toggles() noexcept { toggled_on_.reset(); }

private:
    std::bitset<kActionCount> toggled_on_;
    bool primed_ = false;
};

// One key bound to more than one action.
struct Conflict
{
    std::uint32_t key;
    std::vector<ActionId> actions; // in ActionId order
};

// Every conflict, by key code. Unbound actions are ignored.
[[nodiscard]] std::vector<Conflict> find_conflicts(const Binds& binds);
} // namespace input
