#include <doctest.h>

#include "color.h"

TEST_CASE("Color::rgb unpacks 0xRRGGBB")
{
    const Color c = Color::rgb(0xFF8000, 0.5f);
    CHECK(c.r == doctest::Approx(1.0f));
    CHECK(c.g == doctest::Approx(128.0f / 255.0f));
    CHECK(c.b == doctest::Approx(0.0f));
    CHECK(c.a == doctest::Approx(0.5f));
}

TEST_CASE("Color::lerp clamps t")
{
    const Color black{0.0f, 0.0f, 0.0f, 1.0f};
    const Color white{1.0f, 1.0f, 1.0f, 0.0f};
    CHECK(black.lerp(white, 0.0f) == black);
    CHECK(black.lerp(white, 1.0f) == white);
    const Color mid = black.lerp(white, 0.5f);
    CHECK(mid.r == doctest::Approx(0.5f));
    CHECK(mid.a == doctest::Approx(0.5f));
    CHECK(black.lerp(white, 2.0f) == white);
    CHECK(black.lerp(white, -1.0f) == black);
}

TEST_CASE("Color::faded multiplies alpha and stays in 0..1")
{
    CHECK(Color::rgb(0xFFFFFF, 0.8f).faded(0.5f).a == doctest::Approx(0.4f));
    CHECK(Color::rgb(0xFFFFFF, 0.8f).faded(2.0f).a == doctest::Approx(1.0f));
    CHECK(Color::rgb(0xFFFFFF, 0.8f).faded(0.5f).r == doctest::Approx(1.0f));
}
