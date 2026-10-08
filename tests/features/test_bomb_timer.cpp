#include <string>
#include <variant>
#include <vector>

#include <doctest.h>

#include "features/bomb_timer.h"
#include "helpers/fake_game.h"
#include "maths/vec.h"
#include "settings/settings.h"

namespace
{
using features::BombPhase;
using features::DefuseVerdict;

constexpr maths::Vec2 kScreen{1920.0f, 1080.0f};
constexpr float kLineHeight = 15.0f;
constexpr float kNow = 1000.0f;

// You at the origin, a bomb on A planted 10 m away along +x with `left` seconds of 40 to go.
game::GameSnapshot with_bomb(float left)
{
    game::GameSnapshot game = test::make_match();
    game.globals.curtime = kNow;
    game::PlantedBomb bomb;
    bomb.entity = 0x4DF70000000;
    bomb.site = 0;
    bomb.position = test::local_of(game).origin + maths::Vec3{10.0f * maths::kUnitsPerMetre, 0.0f, 0.0f};
    bomb.ticking = true;
    bomb.blow_time = kNow + left;
    bomb.timer_length = 40.0f;
    game.bomb = bomb;
    return game;
}

std::vector<std::string> texts(const std::vector<render::Primitive>& primitives)
{
    std::vector<std::string> out;
    for (const render::Primitive& primitive : primitives)
    {
        if (const auto* text = std::get_if<render::Text>(&primitive))
        {
            out.push_back(text->text);
        }
    }
    return out;
}

bool contains(const std::vector<std::string>& lines, const std::string& wanted)
{
    for (const std::string& line : lines)
    {
        if (line == wanted)
        {
            return true;
        }
    }
    return false;
}
} // namespace

TEST_CASE("defuse_verdict: over 10 s no kit needed, over 5 s with a kit, otherwise too late")
{
    CHECK(features::defuse_verdict(31.0f) == DefuseVerdict::no_kit_needed);
    CHECK(features::defuse_verdict(10.01f) == DefuseVerdict::no_kit_needed);
    CHECK(features::defuse_verdict(10.0f) == DefuseVerdict::kit_needed);
    CHECK(features::defuse_verdict(5.01f) == DefuseVerdict::kit_needed);
    CHECK(features::defuse_verdict(5.0f) == DefuseVerdict::too_late);
    CHECK(features::defuse_verdict(0.0f) == DefuseVerdict::too_late);
}

TEST_CASE("bomb_timer_info: none without a bomb; a ticking bomb's time, fraction, site and distance")
{
    CHECK_FALSE(features::bomb_timer_info(test::make_match()).has_value());

    const auto info = features::bomb_timer_info(with_bomb(30.0f));
    REQUIRE(info.has_value());
    CHECK(info->phase == BombPhase::ticking);
    CHECK(info->site == 'A');
    CHECK(info->seconds_left == doctest::Approx(30.0f));
    CHECK(info->fraction_left == doctest::Approx(0.75f));
    CHECK(info->verdict == DefuseVerdict::no_kit_needed);
    REQUIRE(info->distance_metres.has_value());
    CHECK(*info->distance_metres == doctest::Approx(10.0f));
}

TEST_CASE("bomb_timer_info: never below zero; site B and unknown")
{
    game::GameSnapshot game = with_bomb(-3.0f);
    game.bomb->site = 1;
    auto info = features::bomb_timer_info(game);
    REQUIRE(info.has_value());
    CHECK(info->seconds_left == 0.0f);
    CHECK(info->fraction_left == 0.0f);
    CHECK(info->site == 'B');

    game.bomb->site = 7;
    CHECK(features::bomb_timer_info(game)->site == '?');
}

TEST_CASE("bomb_timer_info: defusing in time / too late, with the defuser's name")
{
    game::GameSnapshot game = with_bomb(8.0f);
    game::PlayerSnapshot& defuser = test::add_player(game, 2, game::Team::counter_terrorist, {50.0f, 0.0f, 0.0f});
    defuser.name = "Kev";
    game.bomb->being_defused = true;
    game.bomb->defuser_pawn = defuser.pawn;
    game.bomb->defuse_length = 5.0f;
    game.bomb->defuse_end = kNow + 4.0f; // done before the bomb's 8 s

    auto info = features::bomb_timer_info(game);
    REQUIRE(info.has_value());
    CHECK(info->phase == BombPhase::defusing);
    CHECK(info->defuse_left == doctest::Approx(4.0f));
    CHECK(info->defuse_fraction_left == doctest::Approx(0.8f));
    CHECK(info->defuse_in_time);
    CHECK(info->defuser == "Kev");

    game.bomb->defuse_end = kNow + 9.0f; // a no-kit defuse started with 8 s left
    game.bomb->defuse_length = 10.0f;
    info = features::bomb_timer_info(game);
    CHECK_FALSE(info->defuse_in_time);
}

TEST_CASE("bomb_timer_info: defused and exploded win over everything else")
{
    game::GameSnapshot game = with_bomb(12.0f);
    game.bomb->being_defused = true;
    game.bomb->defused = true;
    CHECK(features::bomb_timer_info(game)->phase == BombPhase::defused);
    game.bomb->exploded = true;
    CHECK(features::bomb_timer_info(game)->phase == BombPhase::exploded);
}

TEST_CASE("build_bomb_timer: nothing when off or without a bomb; a centred panel with the countdown")
{
    settings::BombTimerSettings timer;
    CHECK(features::build_bomb_timer(with_bomb(30.0f), timer, kScreen, kLineHeight).empty()); // off by default
    timer.enabled = true;
    CHECK(features::build_bomb_timer(test::make_match(), timer, kScreen, kLineHeight).empty());

    const auto primitives = features::build_bomb_timer(with_bomb(30.0f), timer, kScreen, kLineHeight);
    REQUIRE_FALSE(primitives.empty());
    const auto* background = std::get_if<render::FilledRect>(&primitives.front());
    REQUIRE(background != nullptr);
    CHECK((background->min.x + background->max.x) * 0.5f == doctest::Approx(kScreen.x * 0.5f));
    CHECK(background->min.y == timer.top);
    CHECK(background->max.y > background->min.y + 2.0f * kLineHeight);

    const auto lines = texts(primitives);
    CHECK(contains(lines, "BOMB A"));
    CHECK(contains(lines, "30.0 s"));
    CHECK(contains(lines, "Time to defuse without a kit"));
    CHECK(contains(lines, "10 m from you"));

    timer.defuse_hint = false;
    timer.distance = false;
    const auto bare = texts(features::build_bomb_timer(with_bomb(30.0f), timer, kScreen, kLineHeight));
    CHECK(bare.size() == 2); // "BOMB A" and the time
}

TEST_CASE("build_bomb_timer: defusing, defused and exploded rows")
{
    settings::BombTimerSettings timer;
    timer.enabled = true;
    game::GameSnapshot game = with_bomb(7.0f);
    game.bomb->being_defused = true;
    game.bomb->defuse_length = 5.0f;
    game.bomb->defuse_end = kNow + 2.5f;
    auto lines = texts(features::build_bomb_timer(game, timer, kScreen, kLineHeight));
    CHECK(contains(lines, "Defusing"));
    CHECK(contains(lines, "2.5 s"));
    CHECK(contains(lines, "Defuse will make it"));
    CHECK_FALSE(contains(lines, "Defuse only with a kit")); // the hint gives way to the defuse in progress

    game.bomb->defused = true;
    lines = texts(features::build_bomb_timer(game, timer, kScreen, kLineHeight));
    CHECK(contains(lines, "BOMB A DEFUSED"));

    game.bomb->exploded = true;
    lines = texts(features::build_bomb_timer(game, timer, kScreen, kLineHeight));
    CHECK(contains(lines, "BOMB A EXPLODED"));
}

TEST_CASE("build_bomb_timer: the countdown bar marks the latest defuse without a kit (10 s) and with one (5 s)")
{
    settings::BombTimerSettings timer;
    timer.enabled = true;
    std::vector<float> marks;
    for (const render::Primitive& primitive : features::build_bomb_timer(with_bomb(30.0f), timer, kScreen, kLineHeight))
    {
        if (const auto* line = std::get_if<render::Line>(&primitive))
        {
            CHECK(line->from.x == line->to.x); // vertical
            marks.push_back(line->from.x);
        }
    }
    // The bar runs inside the panel's padding: (1920 - 250) / 2 + 8 = 843 to 843 + 234 = 1077; 40 s fuse.
    constexpr float kBarLeft = 843.0f;
    constexpr float kBarWidth = 234.0f;
    REQUIRE(marks.size() == 4); // each mark with its dark outline
    CHECK(marks[0] == doctest::Approx(kBarLeft + kBarWidth * 10.0f / 40.0f));
    CHECK(marks[1] == doctest::Approx(kBarLeft + kBarWidth * 10.0f / 40.0f));
    CHECK(marks[2] == doctest::Approx(kBarLeft + kBarWidth * 5.0f / 40.0f));
    CHECK(marks[3] == doctest::Approx(kBarLeft + kBarWidth * 5.0f / 40.0f));

    // Defused: no bar, no marks.
    game::GameSnapshot game = with_bomb(30.0f);
    game.bomb->defused = true;
    for (const render::Primitive& primitive : features::build_bomb_timer(game, timer, kScreen, kLineHeight))
    {
        CHECK_FALSE(std::holds_alternative<render::Line>(primitive));
    }
}

