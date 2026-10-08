#include <limits>

#include <doctest.h>

#include "maths/projection.h"

namespace
{
constexpr maths::Vec2 kScreen{1920.0f, 1080.0f};

// A camera at the world origin looking along +x (Source: x forward, y left, z up), row-major: clip.x = -y (right),
// clip.y = z (up), clip.w = x (distance ahead).
constexpr maths::ViewMatrix kCamera{{0.0f, -1.0f, 0.0f, 0.0f, //
                                     0.0f, 0.0f, 1.0f, 0.0f,  //
                                     0.0f, 0.0f, 0.0f, 0.0f,  //
                                     1.0f, 0.0f, 0.0f, 0.0f}};

// Read live 2026-10-06 (build 14189, de_mirage, 1920x1080), with the local player's eye position and view angles.
constexpr maths::ViewMatrix kLive{{-0.45355620980262756f, -0.5973165035247803f, 3.4865588194321617e-09f,
                                   -348.3659362792969f, 0.11293353140354156f, -0.08575303107500076f,
                                   1.3257718086242676f, 238.3029327392578f, 0.7921051383018494f, -0.6014636754989624f,
                                   -0.10637769848108292f, 1815.416259765625f, 0.7919051051139832f,
                                   -0.6013118028640747f, -0.10635083168745041f, 1821.457763671875f}};
} // namespace

TEST_CASE("world_to_screen: straight ahead is the centre of the screen")
{
    const auto centre = maths::world_to_screen(kCamera, {100.0f, 0.0f, 0.0f}, kScreen);
    REQUIRE(centre.has_value());
    CHECK(centre->x == doctest::Approx(960.0f));
    CHECK(centre->y == doctest::Approx(540.0f));
}

TEST_CASE("world_to_screen: right is +x on screen, up is -y (screen y grows downwards)")
{
    const auto right = maths::world_to_screen(kCamera, {100.0f, -50.0f, 0.0f}, kScreen); // y negative = right
    REQUIRE(right.has_value());
    CHECK(right->x == doctest::Approx(1440.0f)); // ndc 0.5
    CHECK(right->y == doctest::Approx(540.0f));

    const auto up = maths::world_to_screen(kCamera, {100.0f, 0.0f, 25.0f}, kScreen);
    REQUIRE(up.has_value());
    CHECK(up->x == doctest::Approx(960.0f));
    CHECK(up->y == doctest::Approx(405.0f)); // ndc 0.25 -> 540 * 0.75
}

TEST_CASE("world_to_screen: off-screen points still project, points behind the camera don't")
{
    const auto far_left = maths::world_to_screen(kCamera, {10.0f, 100.0f, 0.0f}, kScreen);
    REQUIRE(far_left.has_value());
    CHECK(far_left->x < 0.0f);

    CHECK_FALSE(maths::world_to_screen(kCamera, {-100.0f, 0.0f, 0.0f}, kScreen).has_value());
    CHECK_FALSE(maths::world_to_screen(kCamera, {0.0f, 10.0f, 0.0f}, kScreen).has_value()); // on the camera plane
    const float nan = std::numeric_limits<float>::quiet_NaN();
    CHECK_FALSE(maths::world_to_screen(kCamera, {nan, 0.0f, 0.0f}, kScreen).has_value());
}

TEST_CASE("world_to_screen with the live matrix: the view direction lands on the crosshair (row-major)")
{
    // Eye (-1738.45, 736.83, 16.0) + 1000 units along view angles (6.105, -37.210).
    const auto ahead = maths::world_to_screen(kLive, {-946.5497538f, 135.5187435f, -90.3508767f}, kScreen);
    REQUIRE(ahead.has_value());
    CHECK(ahead->x == doctest::Approx(960.0f).epsilon(0.001));
    CHECK(ahead->y == doctest::Approx(540.0f).epsilon(0.001));

    // A bot's feet (Kev), as the probe projected them.
    const auto kev = maths::world_to_screen(kLive, {278.49994f, -874.87079f, -163.18805f}, kScreen);
    REQUIRE(kev.has_value());
    CHECK(kev->x == doctest::Approx(977.78f).epsilon(0.001));
    CHECK(kev->y == doctest::Approx(513.18f).epsilon(0.001));

    // Read as column-major (the transpose), the translation lands in w: points collapse towards the centre or end up
    // "behind" the camera. Kev's feet don't land where they should. The layout matters.
    maths::ViewMatrix transposed{};
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            transposed.m[c * 4 + r] = kLive.at(r, c);
        }
    }
    const auto wrong = maths::world_to_screen(transposed, {278.49994f, -874.87079f, -163.18805f}, kScreen);
    CHECK_FALSE((wrong && wrong->x == doctest::Approx(977.78f).epsilon(0.001)));
}
