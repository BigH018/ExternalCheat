#include <limits>

#include <doctest.h>

#include "maths/vec.h"

TEST_CASE("Vec3 arithmetic, length and distance")
{
    const maths::Vec3 a{1.0f, 2.0f, 3.0f};
    const maths::Vec3 b{4.0f, 6.0f, 3.0f};
    CHECK(a + b == maths::Vec3{5.0f, 8.0f, 6.0f});
    CHECK(b - a == maths::Vec3{3.0f, 4.0f, 0.0f});
    CHECK(a * 2.0f == maths::Vec3{2.0f, 4.0f, 6.0f});
    CHECK(a.dot(b) == doctest::Approx(25.0f));
    CHECK((b - a).length() == doctest::Approx(5.0f));
    CHECK(a.distance_to(b) == doctest::Approx(5.0f));
    CHECK(b.distance_to(a) == doctest::Approx(5.0f));
}

TEST_CASE("Vec3::is_finite")
{
    CHECK(maths::Vec3{-1738.4f, 736.8f, -47.9f}.is_finite());
    CHECK_FALSE(maths::Vec3{std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f}.is_finite());
    CHECK_FALSE(maths::Vec3{0.0f, 0.0f, std::numeric_limits<float>::infinity()}.is_finite());
}

TEST_CASE("Vec2 arithmetic and length")
{
    const maths::Vec2 a{3.0f, 4.0f};
    CHECK(a.length() == doctest::Approx(5.0f));
    CHECK(a + maths::Vec2{1.0f, 1.0f} == maths::Vec2{4.0f, 5.0f});
    CHECK(a - maths::Vec2{1.0f, 1.0f} == maths::Vec2{2.0f, 3.0f});
    CHECK(a * 0.5f == maths::Vec2{1.5f, 2.0f});
}

TEST_CASE("units_to_metres: 1 unit = 1 inch")
{
    CHECK(maths::units_to_metres(100.0f) == doctest::Approx(2.54f));
    CHECK(maths::units_to_metres(0.0f) == 0.0f);
}
