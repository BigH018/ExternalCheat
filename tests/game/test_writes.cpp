#include <array>
#include <cstdint>
#include <limits>

#include <doctest.h>

#include "game/offsets.h"
#include "game/writes.h"
#include "helpers/fake_memory.h"

namespace
{
constexpr std::uintptr_t kClientBase = 0x7FFA10000000;
namespace offsets = game::offsets;

test::FakeMemory make_client()
{
    test::FakeMemory memory;
    memory.map(kClientBase + offsets::buttons::attack, sizeof(std::uint32_t));
    memory.map(kClientBase + offsets::client::dwViewAngles, 3 * sizeof(float));
    return memory;
}
} // namespace

TEST_CASE("set_button writes the proven pressed / released values")
{
    test::FakeMemory memory = make_client();
    const std::uintptr_t attack = kClientBase + offsets::buttons::attack;
    REQUIRE(game::set_button(memory, kClientBase, offsets::buttons::attack, true));
    CHECK(memory.read<std::uint32_t>(attack) == 65537u);
    REQUIRE(game::set_button(memory, kClientBase, offsets::buttons::attack, false));
    CHECK(memory.read<std::uint32_t>(attack) == 256u);
    CHECK(memory.write_count() == 2);
    CHECK_FALSE(game::set_button(memory, kClientBase, offsets::buttons::jump, true)); // not mapped here
}

TEST_CASE("read_view_angles: pitch and yaw; nullopt when unreadable or not finite")
{
    test::FakeMemory memory = make_client();
    const std::uintptr_t angles = kClientBase + offsets::client::dwViewAngles;
    memory.put(angles, std::array<float, 3>{-3.1f, -111.9f, 0.0f});
    const auto read = game::read_view_angles(memory, kClientBase);
    REQUIRE(read.has_value());
    CHECK(read->pitch == doctest::Approx(-3.1f));
    CHECK(read->yaw == doctest::Approx(-111.9f));

    memory.put(angles, std::numeric_limits<float>::quiet_NaN());
    CHECK_FALSE(game::read_view_angles(memory, kClientBase).has_value());
    CHECK_FALSE(game::read_view_angles(memory, 0x7FFA20000000).has_value());
}

TEST_CASE("write_view_angles: normalized pitch and yaw, roll left alone")
{
    test::FakeMemory memory = make_client();
    const std::uintptr_t angles = kClientBase + offsets::client::dwViewAngles;
    memory.put(angles, std::array<float, 3>{0.0f, 0.0f, 7.0f});
    REQUIRE(game::write_view_angles(memory, kClientBase, maths::Angles{120.0f, 190.0f}));
    const auto raw = memory.read<std::array<float, 3>>(angles);
    REQUIRE(raw.has_value());
    CHECK((*raw)[0] == doctest::Approx(89.0f));   // clamped
    CHECK((*raw)[1] == doctest::Approx(-170.0f)); // wrapped
    CHECK((*raw)[2] == doctest::Approx(7.0f));    // roll untouched
}
