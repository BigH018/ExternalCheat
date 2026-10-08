#include <cstdint>
#include <initializer_list>

#include <doctest.h>

#include "config.h"
#include "input/actions.h"
#include "input/bind_capture.h"
#include "input/keys.h"

using input::ActionId;

namespace
{
constexpr std::uint32_t kF = 0x46;
constexpr std::uint32_t kG = 0x47;
constexpr std::uint32_t kLShift = 0xA0;

input::KeySet keys(std::initializer_list<std::uint32_t> down = {})
{
    input::KeySet set;
    for (const std::uint32_t vk : down)
    {
        set.set(vk);
    }
    return set;
}

// A capture for `id` that has seen everything released (ready for the next key).
input::BindCapture ready(ActionId id)
{
    input::BindCapture capture;
    capture.start(id, 0);
    REQUIRE_FALSE(capture.update(keys(), 1).has_value());
    return capture;
}
} // namespace

TEST_CASE("bind capture: waits for every key to be released first")
{
    input::BindCapture capture;
    CHECK_FALSE(capture.active());
    capture.start(ActionId::esp_enable, 0);
    CHECK(capture.active());
    CHECK(capture.capturing(ActionId::esp_enable));
    CHECK_FALSE(capture.capturing(ActionId::panic));
    CHECK_FALSE(capture.update(keys({input::kVkMouse1}), 10).has_value()); // the click that started it
    CHECK_FALSE(capture.update(keys({input::kVkMouse1}), 20).has_value());
    CHECK_FALSE(capture.update(keys(), 30).has_value()); // released
    const auto result = capture.update(keys({kF}), 40);
    REQUIRE(result.has_value());
    CHECK(result->action == ActionId::esp_enable);
    CHECK(result->key == kF);
    CHECK_FALSE(capture.active());
    CHECK_FALSE(capture.update(keys({kG}), 50).has_value()); // over
}

TEST_CASE("bind capture: mouse buttons bind, the lowest code wins")
{
    input::BindCapture capture = ready(ActionId::aimbot_activate);
    const auto result = capture.update(keys({kG, input::kVkMouse4, kF}), 5);
    REQUIRE(result.has_value());
    CHECK(result->key == input::kVkMouse4);
}

TEST_CASE("bind capture: Escape clears the bind, even with another key in the same frame")
{
    input::BindCapture capture = ready(ActionId::esp_enable);
    const auto result = capture.update(keys({input::kVkEscape, kF}), 5);
    REQUIRE(result.has_value());
    CHECK(result->key == input::kUnbound);
}

TEST_CASE("bind capture: the menu key takes keyboard keys only, and Escape cancels it")
{
    input::BindCapture capture = ready(ActionId::menu_toggle);
    CHECK_FALSE(capture.update(keys({input::kVkMouse4}), 5).has_value()); // not a hotkey: ignored
    CHECK_FALSE(capture.update(keys({kLShift}), 6).has_value());
    CHECK(capture.active());
    const auto result = capture.update(keys({input::kVkMouse4, kLShift, kF}), 7);
    REQUIRE(result.has_value());
    CHECK(result->key == kF);

    capture = ready(ActionId::menu_toggle);
    CHECK_FALSE(capture.update(keys({input::kVkEscape}), 5).has_value()); // can't be unbound
    CHECK_FALSE(capture.active());
}

TEST_CASE("bind capture: keys it doesn't listen to don't block the release wait")
{
    input::BindCapture capture;
    capture.start(ActionId::menu_toggle, 0);
    CHECK_FALSE(capture.update(keys({input::kVkMouse1}), 1).has_value()); // Mouse 1 still down: not a menu key
    const auto result = capture.update(keys({input::kVkMouse1, kF}), 2);
    REQUIRE(result.has_value());
    CHECK(result->key == kF);
}

TEST_CASE("bind capture: cancel, restart on another action, timeout")
{
    input::BindCapture capture = ready(ActionId::esp_enable);
    capture.cancel();
    CHECK_FALSE(capture.active());
    CHECK_FALSE(capture.update(keys({kF}), 5).has_value());

    capture.start(ActionId::esp_enable, 0);
    capture.start(ActionId::radar_enable, 0); // another key button clicked: the capture moves
    CHECK(capture.capturing(ActionId::radar_enable));
    CHECK_FALSE(capture.capturing(ActionId::esp_enable));

    capture = ready(ActionId::esp_enable);
    CHECK_FALSE(capture.update(keys(), config::kBindCaptureTimeoutMs - 1).has_value());
    CHECK(capture.active());
    CHECK_FALSE(capture.update(keys({kF}), config::kBindCaptureTimeoutMs).has_value()); // too late
    CHECK_FALSE(capture.active());
}
