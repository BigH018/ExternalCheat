#include <cstdint>
#include <limits>

#include <doctest.h>

#include "game/offsets.h"
#include "game/view.h"
#include "helpers/fake_memory.h"
#include "maths/projection.h"

namespace
{
constexpr std::uintptr_t kClientBase = 0x7FFD62930000;
constexpr std::uintptr_t kMatrix = kClientBase + game::offsets::client::dwViewMatrix;

// Read live 2026-10-06 (build 14189, de_mirage), rounded.
constexpr maths::ViewMatrix kLive{{-0.4536f, -0.5973f, 0.0f, -348.3659f, 0.1129f, -0.0858f, 1.3258f, 238.3029f, 0.7921f,
                                   -0.6015f, -0.1064f, 1815.4163f, 0.7919f, -0.6013f, -0.1064f, 1821.4578f}};
} // namespace

TEST_CASE("read_view_matrix reads 16 floats at client.dll + dwViewMatrix, row-major")
{
    test::FakeMemory memory;
    memory.map_value(kMatrix, kLive);
    const auto view = game::read_view_matrix(memory, kClientBase);
    REQUIRE(view.has_value());
    CHECK(view->at(0, 3) == doctest::Approx(-348.3659f));
    CHECK(view->at(3, 0) == doctest::Approx(0.7919f));
    CHECK(view->at(3, 3) == doctest::Approx(1821.4578f));
    CHECK(view->is_sane());

    const test::FakeMemory nothing;
    CHECK_FALSE(game::read_view_matrix(nothing, kClientBase).has_value());
}

TEST_CASE("ViewMatrix::is_sane: all zeros or non-finite values are not")
{
    CHECK_FALSE(maths::ViewMatrix{}.is_sane());

    maths::ViewMatrix nan = kLive;
    nan.m[5] = std::numeric_limits<float>::quiet_NaN();
    CHECK_FALSE(nan.is_sane());

    maths::ViewMatrix infinite = kLive;
    infinite.m[15] = std::numeric_limits<float>::infinity();
    CHECK_FALSE(infinite.is_sane());

    maths::ViewMatrix one{};
    one.m[15] = 1.0f;
    CHECK(one.is_sane());
}
