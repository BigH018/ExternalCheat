#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <set>
#include <string>

#include <doctest.h>

#include "input/actions.h"
#include "input/keybinds.h"
#include "input/keys.h"

using input::ActionId;
using input::BindMode;
using input::Suspension;

namespace
{
constexpr std::uint32_t kF = 0x46;
constexpr std::uint32_t kG = 0x47;

// Keys held at the end of the frame, and keys pressed once during it.
input::KeyFrame frame(std::initializer_list<std::uint32_t> down = {},
                      std::initializer_list<std::uint32_t> pressed = {})
{
    input::KeyFrame keys;
    for (const std::uint32_t vk : down)
    {
        keys.down.set(vk);
    }
    for (const std::uint32_t vk : pressed)
    {
        ++keys.presses[vk];
    }
    return keys;
}

// `vk` pressed this frame and still held.
input::KeyFrame press(std::uint32_t vk)
{
    return frame({vk}, {vk});
}

// Every action unbound except `id` on F with `mode`.
input::Binds only(ActionId id, BindMode mode)
{
    input::Binds binds{};
    binds[input::index_of(id)] = input::Bind{kF, mode};
    return binds;
}

// An engine past its priming frame.
input::KeybindEngine primed()
{
    input::KeybindEngine engine;
    engine.update(frame(), input::Binds{}, Suspension::none);
    return engine;
}
} // namespace

TEST_CASE("actions: the registry lists every action once, in id order, with valid defaults")
{
    const auto actions = input::actions();
    REQUIRE(actions.size() == input::kActionCount);
    std::set<std::string> ids;
    for (std::size_t i = 0; i < actions.size(); ++i)
    {
        const input::ActionDef& def = actions[i];
        CHECK(input::index_of(def.id) == i);
        CHECK(&input::action(def.id) == &def);
        CHECK(ids.insert(std::string(def.key)).second); // profile ids are unique
        CHECK_FALSE(def.label.empty());
        CHECK(input::mode_allowed(def.id, def.default_bind.mode));
        CHECK(input::key_allowed(def.id, def.default_bind.key));
    }
    CHECK(input::action(ActionId::menu_toggle).hotkey);
    CHECK(input::action(ActionId::panic).works_in_menu);
    CHECK(input::action(ActionId::exit).works_in_menu);
    CHECK_FALSE(input::action(ActionId::esp_enable).works_in_menu);
}

TEST_CASE("actions: modes and keys an action allows")
{
    CHECK(input::allowed_modes(ActionId::aimbot_activate).size() == 2);
    CHECK(input::mode_allowed(ActionId::aimbot_activate, BindMode::toggle));
    CHECK_FALSE(input::mode_allowed(ActionId::aimbot_activate, BindMode::press));
    CHECK(input::allowed_modes(ActionId::esp_enable).size() == 1);
    CHECK(input::mode_allowed(ActionId::esp_enable, BindMode::press));
    CHECK_FALSE(input::mode_allowed(ActionId::esp_enable, BindMode::hold));
    CHECK(input::mode_name(BindMode::hold) == "Hold");

    CHECK(input::key_allowed(ActionId::esp_enable, input::kUnbound));
    CHECK(input::key_allowed(ActionId::esp_enable, input::kVkMouse4));
    CHECK_FALSE(input::key_allowed(ActionId::esp_enable, input::kVkEscape));
    // The menu key is a hotkey: a keyboard key, never unbound.
    CHECK(input::key_allowed(ActionId::menu_toggle, kF));
    CHECK_FALSE(input::key_allowed(ActionId::menu_toggle, input::kUnbound));
    CHECK_FALSE(input::key_allowed(ActionId::menu_toggle, input::kVkMouse4));
    CHECK_FALSE(input::key_allowed(ActionId::menu_toggle, 0xA0)); // LSHIFT
}

TEST_CASE("keybind engine: the first update only takes the toggle states")
{
    input::KeybindEngine engine;
    const input::Binds binds = only(ActionId::panic, BindMode::press);
    CHECK_FALSE(engine.update(press(kF), binds, Suspension::none).did_fire(ActionId::panic));
    CHECK_FALSE(engine.update(frame({kF}), binds, Suspension::none).did_fire(ActionId::panic)); // still held
    CHECK(engine.update(press(kF), binds, Suspension::none).did_fire(ActionId::panic));
}

TEST_CASE("keybind engine: press fires once per press, quick taps and several taps in one frame included")
{
    input::KeybindEngine engine = primed();
    const input::Binds binds = only(ActionId::esp_enable, BindMode::press);
    CHECK(engine.update(press(kF), binds, Suspension::none).press_count(ActionId::esp_enable) == 1);
    CHECK_FALSE(engine.update(frame({kF}), binds, Suspension::none).did_fire(ActionId::esp_enable)); // held
    CHECK_FALSE(engine.update(frame(), binds, Suspension::none).did_fire(ActionId::esp_enable));
    CHECK(engine.update(frame({}, {kF}), binds, Suspension::none).did_fire(ActionId::esp_enable)); // tap within a frame
    input::KeyFrame three = frame();
    three.presses[kF] = 3;
    CHECK(engine.update(three, binds, Suspension::none).press_count(ActionId::esp_enable) == 3);
    CHECK_FALSE(engine.update(press(kF), binds, Suspension::none).is_active(ActionId::esp_enable));
}

TEST_CASE("keybind engine: hold is active while the key is down")
{
    input::KeybindEngine engine = primed();
    const input::Binds binds = only(ActionId::aimbot_activate, BindMode::hold);
    CHECK_FALSE(engine.update(frame(), binds, Suspension::none).is_active(ActionId::aimbot_activate));
    CHECK(engine.update(press(kF), binds, Suspension::none).is_active(ActionId::aimbot_activate));
    CHECK(engine.update(frame({kF, kG}), binds, Suspension::none).is_active(ActionId::aimbot_activate));
    CHECK_FALSE(engine.update(frame({kG}), binds, Suspension::none).is_active(ActionId::aimbot_activate));
}

TEST_CASE("keybind engine: toggle flips on each press and stays")
{
    input::KeybindEngine engine = primed();
    const input::Binds binds = only(ActionId::aimbot_activate, BindMode::toggle);
    const auto active = [&](const input::KeyFrame& keys) {
        return engine.update(keys, binds, Suspension::none).is_active(ActionId::aimbot_activate);
    };
    CHECK(active(press(kF)));           // press: on
    CHECK(active(frame({kF})));         // held: stays on
    CHECK(active(frame()));             // released: stays on
    CHECK_FALSE(active(frame({}, {kF}))); // a tap shorter than a frame: off
    input::KeyFrame two = frame();
    two.presses[kF] = 2;
    CHECK_FALSE(active(two)); // two taps inside one frame: on and off again
    two.presses[kF] = 3;
    CHECK(active(two));

    engine.reset_toggles(); // panic
    CHECK_FALSE(active(frame()));
}

TEST_CASE("keybind engine: with the menu open only panic and exit fire; toggles keep their state")
{
    input::KeybindEngine engine = primed();
    input::Binds binds{};
    binds[input::index_of(ActionId::panic)] = input::Bind{kF, BindMode::press};
    binds[input::index_of(ActionId::esp_enable)] = input::Bind{kG, BindMode::press};
    auto states = engine.update(frame({kF, kG}, {kF, kG}), binds, Suspension::menu_open);
    CHECK(states.did_fire(ActionId::panic));
    CHECK_FALSE(states.did_fire(ActionId::esp_enable));

    engine = primed();
    const input::Binds toggle = only(ActionId::aimbot_activate, BindMode::toggle);
    engine.update(press(kF), toggle, Suspension::none); // on
    engine.update(frame(), toggle, Suspension::none);
    CHECK(engine.update(press(kF), toggle, Suspension::menu_open).is_active(ActionId::aimbot_activate)); // no flip
    CHECK(engine.update(frame(), toggle, Suspension::none).is_active(ActionId::aimbot_activate));

    engine = primed();
    const input::Binds hold = only(ActionId::aimbot_activate, BindMode::hold);
    CHECK_FALSE(engine.update(press(kF), hold, Suspension::menu_open).is_active(ActionId::aimbot_activate));
}

TEST_CASE("keybind engine: a press during a capture is dropped, not saved for later")
{
    input::KeybindEngine engine = primed();
    const input::Binds binds = only(ActionId::panic, BindMode::press);
    CHECK_FALSE(engine.update(press(kF), binds, Suspension::capture).did_fire(ActionId::panic));
    CHECK_FALSE(engine.update(frame({kF}), binds, Suspension::none).did_fire(ActionId::panic)); // still that press
    CHECK(engine.update(press(kF), binds, Suspension::none).did_fire(ActionId::panic));
}

TEST_CASE("keybind engine: unbound actions never fire")
{
    input::KeybindEngine engine = primed();
    input::KeyFrame all;
    all.down.set();
    all.presses.fill(1);
    const auto states = engine.update(all, input::Binds{}, Suspension::none);
    for (const input::ActionDef& def : input::actions())
    {
        CHECK_FALSE(states.did_fire(def.id));
    }
    CHECK(states.active.none());
}

TEST_CASE("key frames: presses from polling edges, and what the capture sees")
{
    input::KeySet before;
    before.set(kF);
    input::KeySet now;
    now.set(kF);
    now.set(kG);
    const input::PressCounts presses = input::presses_from_edges(before, now);
    CHECK(presses[kG] == 1);
    CHECK(presses[kF] == 0); // held in both: no press

    const input::KeyFrame keys = frame({kF}, {kG}); // F held, G tapped within the frame
    const input::KeySet seen = keys.down_or_pressed();
    CHECK(seen[kF]);
    CHECK(seen[kG]);
    CHECK(seen.count() == 2);
}

TEST_CASE("keybind conflicts: one key on several actions, by key")
{
    CHECK(input::find_conflicts(input::default_binds()).empty());
    input::Binds binds{};
    binds[input::index_of(ActionId::esp_enable)] = input::Bind{kF, BindMode::press};
    binds[input::index_of(ActionId::panic)] = input::Bind{kF, BindMode::press};
    binds[input::index_of(ActionId::radar_enable)] = input::Bind{kG, BindMode::press};
    const auto conflicts = input::find_conflicts(binds);
    REQUIRE(conflicts.size() == 1);
    CHECK(conflicts[0].key == kF);
    REQUIRE(conflicts[0].actions.size() == 2);
    CHECK(conflicts[0].actions[0] == ActionId::panic); // ActionId order
    CHECK(conflicts[0].actions[1] == ActionId::esp_enable);
}
