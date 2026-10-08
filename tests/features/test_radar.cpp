#include <cmath>
#include <variant>
#include <vector>

#include <doctest.h>

#include "config.h"
#include "features/radar.h"
#include "helpers/fake_game.h"
#include "settings/settings.h"

namespace
{
using render::Primitive;

constexpr maths::Vec2 kScreen{1920.0f, 1080.0f};
constexpr settings::TeamMode kTeams = settings::TeamMode::teams;
constexpr float kMetre = maths::kUnitsPerMetre;

// 200 px, 20 m, dots of radius 4 in the top-right corner: the centre is at (1810, 110) and a dot on the edge is
// 100 - 4 - 2 = 94 px from it.
constexpr maths::Vec2 kCentre{1920.0f - 10.0f - 100.0f, 10.0f + 100.0f};
constexpr float kReach = 94.0f;

settings::RadarSettings radar_on()
{
    settings::RadarSettings radar;
    radar.enabled = true;
    radar.size = 200.0f;
    radar.range = 20.0f;
    radar.dot_size = 4.0f;
    radar.facing = false;
    return radar;
}

bool near(maths::Vec2 a, maths::Vec2 b, float tolerance = 0.01f)
{
    return std::abs(a.x - b.x) < tolerance && std::abs(a.y - b.y) < tolerance;
}

// The centres of the dots drawn in `colour` (the dark outline under each dot has another colour).
std::vector<maths::Vec2> dots(const std::vector<Primitive>& primitives, const Color& colour)
{
    std::vector<maths::Vec2> out;
    for (const Primitive& primitive : primitives)
    {
        if (const auto* dot = std::get_if<render::FilledCircle>(&primitive); dot != nullptr && dot->colour == colour)
        {
            out.push_back(dot->centre);
        }
    }
    return out;
}

template <class T>
int count(const std::vector<Primitive>& primitives)
{
    int n = 0;
    for (const Primitive& primitive : primitives)
    {
        n += std::holds_alternative<T>(primitive) ? 1 : 0;
    }
    return n;
}
} // namespace

TEST_CASE("radar_panel: each corner, margin from both edges")
{
    using settings::RadarCorner;
    CHECK(features::radar_panel(RadarCorner::top_left, 200.0f, kScreen, 10.0f).min == maths::Vec2{10.0f, 10.0f});
    CHECK(features::radar_panel(RadarCorner::top_right, 200.0f, kScreen, 10.0f).min == maths::Vec2{1710.0f, 10.0f});
    CHECK(features::radar_panel(RadarCorner::bottom_left, 200.0f, kScreen, 10.0f).min == maths::Vec2{10.0f, 870.0f});
    const features::RadarPanel panel = features::radar_panel(RadarCorner::bottom_right, 200.0f, kScreen, 10.0f);
    CHECK(panel.min == maths::Vec2{1710.0f, 870.0f});
    CHECK(panel.max() == maths::Vec2{1910.0f, 1070.0f});
    CHECK(panel.centre() == maths::Vec2{1810.0f, 970.0f});
}

TEST_CASE("radar_offset: looking along +x, ahead is up, +y is left, behind is down; height is ignored")
{
    CHECK(near(features::radar_offset({}, {100.0f, 0.0f, 50.0f}, 0.0f), {0.0f, -100.0f}));
    CHECK(near(features::radar_offset({}, {0.0f, 100.0f, 0.0f}, 0.0f), {-100.0f, 0.0f}));
    CHECK(near(features::radar_offset({}, {0.0f, -100.0f, 0.0f}, 0.0f), {100.0f, 0.0f}));
    CHECK(near(features::radar_offset({}, {-100.0f, 0.0f, -80.0f}, 0.0f), {0.0f, 100.0f}));
    // Measured from you, not from the world origin.
    CHECK(near(features::radar_offset({500.0f, 500.0f, 0.0f}, {600.0f, 500.0f, 0.0f}, 0.0f), {0.0f, -100.0f}));
}

TEST_CASE("radar_offset: north up (yaw 90) is the game's radar: +x right, +y up")
{
    constexpr float kNorth = config::kRadarNorthUpYaw;
    CHECK(near(features::radar_offset({}, {100.0f, 0.0f, 0.0f}, kNorth), {100.0f, 0.0f}));
    CHECK(near(features::radar_offset({}, {0.0f, 100.0f, 0.0f}, kNorth), {0.0f, -100.0f}));
}

TEST_CASE("radar_direction: your own yaw points up, turning right points right")
{
    CHECK(near(features::radar_direction(37.0f, 37.0f), {0.0f, -1.0f}));
    CHECK(near(features::radar_direction(37.0f - 90.0f, 37.0f), {1.0f, 0.0f})); // yaw decreases turning right
    CHECK(near(features::radar_direction(37.0f + 180.0f, 37.0f), {0.0f, 1.0f}));
    CHECK(features::radar_direction(-123.0f, 10.0f).length() == doctest::Approx(1.0f));
}

TEST_CASE("clamp_to_square: inside unchanged, outside pulled onto the edge in the same direction")
{
    CHECK(features::clamp_to_square({0.5f, -0.25f}) == maths::Vec2{0.5f, -0.25f});
    CHECK(features::clamp_to_square({1.0f, 1.0f}) == maths::Vec2{1.0f, 1.0f});
    CHECK(near(features::clamp_to_square({2.0f, 1.0f}), {1.0f, 0.5f}));
    CHECK(near(features::clamp_to_square({-3.0f, -3.0f}), {-1.0f, -1.0f}));
}

TEST_CASE("build_radar: nothing when off, outside a match, or without your pawn")
{
    game::GameSnapshot game = test::make_match();
    test::add_player(game, 2, game::Team::counter_terrorist, {200.0f, 0.0f, 0.0f});
    settings::RadarSettings radar = radar_on();
    CHECK_FALSE(features::build_radar(game, radar, kTeams, kScreen).empty());

    radar.enabled = false;
    CHECK(features::build_radar(game, radar, kTeams, kScreen).empty());

    radar.enabled = true;
    game.in_match = false;
    CHECK(features::build_radar(game, radar, kTeams, kScreen).empty());

    game.in_match = true;
    test::local_of(game).pawn = 0;
    CHECK(features::build_radar(game, radar, kTeams, kScreen).empty());
}

TEST_CASE("build_radar: background, guides, your arrow; an enemy 10 m ahead sits halfway up")
{
    game::GameSnapshot game = test::make_match(); // you look along +x
    test::add_player(game, 2, game::Team::counter_terrorist, {10.0f * kMetre, 0.0f, 0.0f});
    const settings::RadarSettings radar = radar_on();
    const std::vector<Primitive> primitives = features::build_radar(game, radar, kTeams, kScreen);

    CHECK(count<render::FilledRect>(primitives) == 1);
    CHECK(count<render::FilledTriangle>(primitives) == 2); // your arrow and its outline
    CHECK(count<render::Text>(primitives) == 1);           // "20 m"
    const auto enemy = dots(primitives, radar.colours.enemy_hidden); // nobody spotted it
    REQUIRE(enemy.size() == 1);
    CHECK(near(enemy[0], {kCentre.x, kCentre.y - kReach * 0.5f}));
}

TEST_CASE("build_radar: rotation off keeps north up, so an enemy ahead along +x is to the right")
{
    game::GameSnapshot game = test::make_match();
    test::add_player(game, 2, game::Team::counter_terrorist, {10.0f * kMetre, 0.0f, 0.0f});
    settings::RadarSettings radar = radar_on();
    radar.rotate = false;
    const auto enemy = dots(features::build_radar(game, radar, kTeams, kScreen), radar.colours.enemy_hidden);
    REQUIRE(enemy.size() == 1);
    CHECK(near(enemy[0], {kCentre.x + kReach * 0.5f, kCentre.y}));
}

TEST_CASE("build_radar: visible / hidden colours from the spotted-by mask")
{
    game::GameSnapshot game = test::make_match();
    test::add_player(game, 2, game::Team::counter_terrorist, {200.0f, 0.0f, 0.0f}).spotted_by_mask = 1; // your slot
    test::add_player(game, 3, game::Team::counter_terrorist, {-200.0f, 0.0f, 0.0f});
    settings::RadarSettings radar = radar_on();
    std::vector<Primitive> primitives = features::build_radar(game, radar, kTeams, kScreen);
    CHECK(dots(primitives, radar.colours.enemy_visible).size() == 1);
    CHECK(dots(primitives, radar.colours.enemy_hidden).size() == 1);

    radar.visibility_colours = false;
    primitives = features::build_radar(game, radar, kTeams, kScreen);
    CHECK(dots(primitives, radar.colours.enemy_visible).size() == 2);
}

TEST_CASE("build_radar: teammates follow the team mode and the setting; dead and dormant players are skipped")
{
    game::GameSnapshot game = test::make_match(); // you are T
    test::add_player(game, 2, game::Team::terrorist, {200.0f, 0.0f, 0.0f});
    test::add_player(game, 3, game::Team::counter_terrorist, {0.0f, 200.0f, 0.0f}).alive = false;
    test::add_player(game, 4, game::Team::counter_terrorist, {0.0f, -200.0f, 0.0f}).dormant = true;
    settings::RadarSettings radar = radar_on();
    const Color enemy = radar.colours.enemy_hidden;

    std::vector<Primitive> primitives = features::build_radar(game, radar, kTeams, kScreen);
    CHECK(dots(primitives, radar.colours.team).size() == 1);
    CHECK(dots(primitives, enemy).empty());

    radar.show_teammates = false;
    primitives = features::build_radar(game, radar, kTeams, kScreen);
    CHECK(dots(primitives, radar.colours.team).empty());

    // Free for all: the teammate is an enemy too.
    primitives = features::build_radar(game, radar, settings::TeamMode::free_for_all, kScreen);
    CHECK(dots(primitives, enemy).size() == 1);
}

TEST_CASE("build_radar: out of range is clamped to the edge and faded, or hidden")
{
    game::GameSnapshot game = test::make_match();
    test::add_player(game, 2, game::Team::counter_terrorist, {80.0f * kMetre, 0.0f, 0.0f}); // 4x the range, ahead
    settings::RadarSettings radar = radar_on();
    const Color faded = radar.colours.enemy_hidden.faded(config::kRadarEdgeFade);
    const auto edge = dots(features::build_radar(game, radar, kTeams, kScreen), faded);
    REQUIRE(edge.size() == 1);
    CHECK(near(edge[0], {kCentre.x, kCentre.y - kReach}));

    radar.clamp_to_edge = false;
    const std::vector<Primitive> primitives = features::build_radar(game, radar, kTeams, kScreen);
    CHECK(dots(primitives, faded).empty());
    CHECK(dots(primitives, radar.colours.enemy_hidden).empty());
}

TEST_CASE("build_radar: facing lines and names are optional")
{
    game::GameSnapshot game = test::make_match();
    test::add_player(game, 2, game::Team::counter_terrorist, {200.0f, 0.0f, 0.0f}).eye_angles = {0.0f, 180.0f};
    settings::RadarSettings radar = radar_on();
    const int guides = count<render::Line>(features::build_radar(game, radar, kTeams, kScreen)); // the cross
    const int texts = count<render::Text>(features::build_radar(game, radar, kTeams, kScreen));

    radar.facing = true;
    radar.names = true;
    const std::vector<Primitive> primitives = features::build_radar(game, radar, kTeams, kScreen);
    CHECK(count<render::Line>(primitives) == guides + 2); // the line and its outline
    CHECK(count<render::Text>(primitives) == texts + 1);

    // The bot looks back at you (yaw 180 while you look along 0): its line points down the radar.
    for (const Primitive& primitive : primitives)
    {
        if (const auto* line = std::get_if<render::Line>(&primitive);
            line != nullptr && line->colour == radar.colours.enemy_hidden)
        {
            CHECK(line->to.y > line->from.y);
            CHECK(line->to.x == doctest::Approx(line->from.x).epsilon(0.001));
        }
    }
}
