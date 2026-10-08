#include <cmath>
#include <limits>

#include <doctest.h>

#include "maths/angles.h"

using doctest::Approx;
using maths::Angles;
using maths::Vec3;

TEST_CASE("normalize_yaw wraps into [-180, 180)")
{
    CHECK(maths::normalize_yaw(0.0f) == Approx(0.0f));
    CHECK(maths::normalize_yaw(190.0f) == Approx(-170.0f));
    CHECK(maths::normalize_yaw(-190.0f) == Approx(170.0f));
    CHECK(maths::normalize_yaw(720.0f + 45.0f) == Approx(45.0f));
    CHECK(maths::normalize_yaw(180.0f) == Approx(-180.0f));
    CHECK(maths::normalize_yaw(std::numeric_limits<float>::quiet_NaN()) == 0.0f);
}

TEST_CASE("normalize clamps pitch to +-89 and wraps yaw")
{
    CHECK(maths::normalize({120.0f, 0.0f}).pitch == Approx(89.0f));
    CHECK(maths::normalize({-120.0f, 0.0f}).pitch == Approx(-89.0f));
    const Angles wrapped = maths::normalize({10.0f, 370.0f});
    CHECK(wrapped.pitch == Approx(10.0f));
    CHECK(wrapped.yaw == Approx(10.0f));
    CHECK(maths::normalize({std::numeric_limits<float>::infinity(), 0.0f}).pitch == 0.0f);
}

TEST_CASE("calc_aim_angles: the game's convention (pitch positive = down, yaw from +x towards +y)")
{
    const Vec3 eye{0.0f, 0.0f, 0.0f};
    const Angles ahead = maths::calc_aim_angles(eye, {100.0f, 0.0f, 0.0f});
    CHECK(ahead.pitch == Approx(0.0f));
    CHECK(ahead.yaw == Approx(0.0f));
    CHECK(maths::calc_aim_angles(eye, {0.0f, 100.0f, 0.0f}).yaw == Approx(90.0f));
    CHECK(maths::calc_aim_angles(eye, {0.0f, -100.0f, 0.0f}).yaw == Approx(-90.0f));
    CHECK(maths::calc_aim_angles(eye, {100.0f, 0.0f, -100.0f}).pitch == Approx(45.0f)); // below = positive
    CHECK(maths::calc_aim_angles(eye, {100.0f, 0.0f, 100.0f}).pitch == Approx(-45.0f));
}

TEST_CASE("calc_aim_angles and forward agree")
{
    const Vec3 eye{10.0f, -20.0f, 64.0f};
    const Vec3 target{-300.0f, 450.0f, 20.0f};
    const Vec3 direction = target - eye;
    const Vec3 f = maths::forward(maths::calc_aim_angles(eye, target));
    CHECK(f.length() == Approx(1.0f));
    CHECK(f.x == Approx(direction.x / direction.length()));
    CHECK(f.y == Approx(direction.y / direction.length()));
    CHECK(f.z == Approx(direction.z / direction.length()));
}

TEST_CASE("angle_delta and angular_distance take the short way round")
{
    const Angles d = maths::angle_delta({0.0f, 170.0f}, {0.0f, -170.0f});
    CHECK(d.yaw == Approx(20.0f));
    CHECK(maths::angular_distance({0.0f, 170.0f}, {0.0f, -170.0f}) == Approx(20.0f));
    CHECK(maths::angular_distance({3.0f, 0.0f}, {0.0f, 4.0f}) == Approx(5.0f));
    CHECK(maths::is_within_fov({0.0f, 0.0f}, {3.0f, 4.0f}, 5.0f));
    CHECK_FALSE(maths::is_within_fov({0.0f, 0.0f}, {3.0f, 4.0f}, 4.9f));
}

TEST_CASE("smoothing_fraction: 1 snaps, higher is slower, the same speed at any frame rate")
{
    CHECK(maths::smoothing_fraction(1.0f, 0.016f) == 1.0f);
    CHECK(maths::smoothing_fraction(0.5f, 0.016f) == 1.0f); // below the range: snap
    // One 60 Hz frame with smoothing 5 covers a fifth.
    CHECK(maths::smoothing_fraction(5.0f, 1.0f / 60.0f) == Approx(0.2f));
    // Two 120 Hz frames cover what one 60 Hz frame does.
    const float half = maths::smoothing_fraction(5.0f, 1.0f / 120.0f);
    CHECK(1.0f - (1.0f - half) * (1.0f - half) == Approx(0.2f));
    CHECK(maths::smoothing_fraction(5.0f, 0.0f) == Approx(0.0f));
    // A stall is clamped: never more than config::kMaxFrameSeconds of progress.
    CHECK(maths::smoothing_fraction(5.0f, 10.0f) == Approx(maths::smoothing_fraction(5.0f, 0.1f)));
}

TEST_CASE("step_towards: a fraction of the way, the short way round")
{
    const Angles half = maths::step_towards({0.0f, 170.0f}, {10.0f, -170.0f}, 0.5f);
    CHECK(half.pitch == Approx(5.0f));
    CHECK(std::abs(half.yaw) == Approx(180.0f)); // halfway across the +-180 seam, not back through 0
    CHECK(maths::step_towards({0.0f, 0.0f}, {10.0f, 20.0f}, 1.0f) == Angles{10.0f, 20.0f});
    CHECK(maths::step_towards({0.0f, 0.0f}, {10.0f, 20.0f}, 0.0f) == Angles{0.0f, 0.0f});
    CHECK(maths::step_towards({0.0f, 0.0f}, {10.0f, 20.0f}, 7.0f) == Angles{10.0f, 20.0f}); // clamped
}

TEST_CASE("distance_to_ray")
{
    const Vec3 origin{0.0f, 0.0f, 0.0f};
    const Vec3 along_x{1.0f, 0.0f, 0.0f};
    CHECK(maths::distance_to_ray({100.0f, 3.0f, 4.0f}, origin, along_x) == Approx(5.0f));
    CHECK(maths::distance_to_ray({100.0f, 0.0f, 0.0f}, origin, along_x) == Approx(0.0f));
    CHECK(maths::distance_to_ray({-3.0f, 4.0f, 0.0f}, origin, along_x) == Approx(5.0f)); // behind: to the origin
}
