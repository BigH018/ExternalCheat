#include <algorithm>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <doctest.h>

#include "features/esp.h"
#include "features/targeting.h"
#include "game/snapshot.h"
#include "settings/settings.h"

namespace
{
using render::Primitive;

constexpr maths::Vec2 kScreen{1920.0f, 1080.0f};
constexpr float kLineHeight = 15.0f;
constexpr settings::TeamMode kTeams = settings::TeamMode::teams;

// A camera at the world origin looking along +x: clip.x = -y, clip.y = z, w = x.
constexpr maths::ViewMatrix kCamera{{0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                     0.0f, 0.0f, 0.0f}};

game::PlayerSnapshot make_player(std::uint32_t index, game::Team team, maths::Vec3 origin)
{
    game::PlayerSnapshot player;
    player.index = index;
    player.controller = 0x4DF5A000000 + index * 0x1000;
    player.pawn = 0x4DF60000000 + index * 0x10000;
    player.name = "Bot" + std::to_string(index);
    player.team = team;
    player.alive = true;
    player.health = 100;
    player.origin = origin;
    player.view_offset = {0.0f, 0.0f, 64.0f};
    player.flags = game::kFlagOnGround;
    player.weapon_id = 7;
    return player;
}

// You (T, index 1) at the camera; an enemy CT (2) 500 units ahead, a teammate T (3) ahead and to the right.
game::GameSnapshot make_game()
{
    game::GameSnapshot game;
    game.in_match = true;
    game.view = kCamera;
    game::PlayerSnapshot local = make_player(1, game::Team::terrorist, {0.0f, 0.0f, -64.0f});
    local.is_local = true;
    game.players.push_back(local);
    game.players.push_back(make_player(2, game::Team::counter_terrorist, {500.0f, 0.0f, -32.0f}));
    game.players.push_back(make_player(3, game::Team::terrorist, {500.0f, -200.0f, -32.0f}));
    return game;
}

settings::EspSettings enabled()
{
    settings::EspSettings esp;
    esp.enabled = true;
    return esp;
}

template <class T>
std::vector<T> all_of_type(const std::vector<Primitive>& primitives)
{
    std::vector<T> out;
    for (const Primitive& p : primitives)
    {
        if (const T* typed = std::get_if<T>(&p))
        {
            out.push_back(*typed);
        }
    }
    return out;
}

// Boxes drawn in a given colour (the shadow outline is a different colour, so this counts real boxes).
std::size_t boxes_in(const std::vector<Primitive>& primitives, const Color& colour)
{
    const auto rects = all_of_type<render::Rect>(primitives);
    const auto same = [&](const render::Rect& r) { return r.colour == colour; };
    return static_cast<std::size_t>(std::ranges::count_if(rects, same));
}

bool has_text(const std::vector<Primitive>& primitives, const std::string& text)
{
    const auto texts = all_of_type<render::Text>(primitives);
    return std::ranges::any_of(texts, [&](const render::Text& t) { return t.text == text; });
}
} // namespace

TEST_CASE("player_box: feet to above the eyes, half as wide as tall, centred")
{
    const game::PlayerSnapshot bot = make_player(2, game::Team::counter_terrorist, {500.0f, 0.0f, -32.0f});
    const auto box = features::player_box(kCamera, bot, kScreen);
    REQUIRE(box.has_value());
    // Feet z -32 -> ndc -0.064 -> y 574.56. Top z = -32 + 64 + 8 = 40 -> ndc 0.08 -> y 496.8.
    CHECK(box->max.y == doctest::Approx(574.56f));
    CHECK(box->min.y == doctest::Approx(496.8f));
    CHECK(box->width() == doctest::Approx(box->height() * 0.5f));
    CHECK(box->centre_x() == doctest::Approx(960.0f));

    const game::PlayerSnapshot behind = make_player(2, game::Team::counter_terrorist, {-500.0f, 0.0f, -32.0f});
    CHECK_FALSE(features::player_box(kCamera, behind, kScreen).has_value());
}

TEST_CASE("health_colour: green, yellow, red, clamped")
{
    const Color full = features::health_colour(100);
    const Color half = features::health_colour(50);
    const Color empty = features::health_colour(0);
    CHECK(full.g > full.r);
    CHECK(half.r > 0.9f);
    CHECK(half.g > 0.8f);
    CHECK(empty.r > empty.g);
    CHECK(features::health_colour(150) == full);
    CHECK(features::health_colour(-5) == empty);
}

TEST_CASE("is_enemy: teams vs free for all, never yourself")
{
    const game::GameSnapshot game = make_game();
    const game::PlayerSnapshot& local = game.players[0];
    CHECK(features::is_enemy(game.players[1], local, settings::TeamMode::teams));
    CHECK_FALSE(features::is_enemy(game.players[2], local, settings::TeamMode::teams));
    CHECK(features::is_enemy(game.players[2], local, settings::TeamMode::free_for_all));
    CHECK_FALSE(features::is_enemy(local, local, settings::TeamMode::free_for_all));
}

TEST_CASE("display_name and distance_text")
{
    CHECK(features::display_name("Kev") == "Kev");
    CHECK(features::display_name("") == "?");
    const std::string cut = features::display_name("AVeryLongPlayerNameThatGoesOn");
    CHECK(cut.size() == 20);
    CHECK(cut.ends_with("..."));
    CHECK(features::distance_text(24.4f) == "24 m");
}

TEST_CASE("build_esp draws nothing when off, outside a match, or without a view matrix")
{
    game::GameSnapshot game = make_game();
    CHECK(features::build_esp(game, settings::EspSettings{}, kTeams, kScreen, kLineHeight).empty());

    game.view.reset();
    CHECK(features::build_esp(game, enabled(), kTeams, kScreen, kLineHeight).empty());

    game = make_game();
    game.in_match = false;
    CHECK(features::build_esp(game, enabled(), kTeams, kScreen, kLineHeight).empty());
}

TEST_CASE("build_esp: enemies only by default; teammates with the option; everyone in free for all")
{
    const game::GameSnapshot game = make_game();
    settings::EspSettings esp = enabled();
    settings::TeamMode team_mode = kTeams;
    esp.visibility_colours = false; // everyone in the "visible" colours
    const settings::EspColours& c = esp.colours;

    auto out = features::build_esp(game, esp, team_mode, kScreen, kLineHeight);
    CHECK(boxes_in(out, c.enemy_visible) == 1);
    CHECK(boxes_in(out, c.team_visible) == 0);
    CHECK(has_text(out, "Bot2"));
    CHECK_FALSE(has_text(out, "Bot3"));
    CHECK_FALSE(has_text(out, "Bot1")); // never yourself

    esp.show_teammates = true;
    out = features::build_esp(game, esp, team_mode, kScreen, kLineHeight);
    CHECK(boxes_in(out, c.enemy_visible) == 1);
    CHECK(boxes_in(out, c.team_visible) == 1);

    esp.show_teammates = false;
    team_mode = settings::TeamMode::free_for_all;
    out = features::build_esp(game, esp, team_mode, kScreen, kLineHeight);
    CHECK(boxes_in(out, c.enemy_visible) == 2);
}

TEST_CASE("build_esp skips the dead, the dormant, pawnless, far and behind-the-camera players")
{
    settings::EspSettings esp = enabled();
    settings::TeamMode team_mode = kTeams;
    team_mode = settings::TeamMode::free_for_all;
    esp.visibility_colours = false;
    const Color colour = esp.colours.enemy_visible;

    SUBCASE("dead")
    {
        game::GameSnapshot game = make_game();
        game.players[1].alive = false;
        CHECK(boxes_in(features::build_esp(game, esp, team_mode, kScreen, kLineHeight), colour) == 1);
    }
    SUBCASE("dormant")
    {
        game::GameSnapshot game = make_game();
        game.players[1].dormant = true;
        CHECK(boxes_in(features::build_esp(game, esp, team_mode, kScreen, kLineHeight), colour) == 1);
    }
    SUBCASE("no pawn")
    {
        game::GameSnapshot game = make_game();
        game.players[1].pawn = 0;
        CHECK(boxes_in(features::build_esp(game, esp, team_mode, kScreen, kLineHeight), colour) == 1);
    }
    SUBCASE("behind the camera")
    {
        game::GameSnapshot game = make_game();
        game.players[1].origin.x = -500.0f;
        CHECK(boxes_in(features::build_esp(game, esp, team_mode, kScreen, kLineHeight), colour) == 1);
    }
    SUBCASE("beyond the max distance")
    {
        game::GameSnapshot game = make_game();
        game.players[2].origin.x = 5000.0f; // ~127 m; Bot2 is ~12.7 m away
        esp.max_distance = 50.0f;
        const auto out = features::build_esp(game, esp, team_mode, kScreen, kLineHeight);
        CHECK(boxes_in(out, colour) == 1);
        CHECK(has_text(out, "Bot2"));
    }
}

TEST_CASE("build_esp: visible / hidden colours follow the spotted-by bit of your slot")
{
    game::GameSnapshot game = make_game();
    settings::EspSettings esp = enabled();
    settings::TeamMode team_mode = kTeams;
    const settings::EspColours& c = esp.colours;

    CHECK(boxes_in(features::build_esp(game, esp, team_mode, kScreen, kLineHeight), c.enemy_hidden) == 1);

    game.players[1].spotted_by_mask = std::uint64_t{1} << 0; // slot 0 = you (controller index 1)
    CHECK(boxes_in(features::build_esp(game, esp, team_mode, kScreen, kLineHeight), c.enemy_visible) == 1);

    game.players[1].spotted_by_mask = std::uint64_t{1} << 4; // someone else spotted it
    CHECK(boxes_in(features::build_esp(game, esp, team_mode, kScreen, kLineHeight), c.enemy_hidden) == 1);
}

TEST_CASE("build_esp: labels")
{
    game::GameSnapshot game = make_game();
    game.players[1].health = 57;
    game.players[1].scoped = true;
    settings::EspSettings esp = enabled();
    settings::TeamMode team_mode = kTeams;
    esp.health_number = true;

    auto out = features::build_esp(game, esp, team_mode, kScreen, kLineHeight);
    CHECK(has_text(out, "Bot2"));
    CHECK(has_text(out, "AK-47"));
    CHECK(has_text(out, "13 m")); // 500 units = 12.7 m
    CHECK(has_text(out, "SCOPED"));
    CHECK(has_text(out, "57"));
    CHECK(all_of_type<render::FilledRect>(out).size() == 2); // health bar back + fill

    esp.name = esp.weapon = esp.distance = esp.scoped_indicator = esp.health_bar = esp.health_number = false;
    out = features::build_esp(game, esp, team_mode, kScreen, kLineHeight);
    CHECK(all_of_type<render::Text>(out).empty());
    CHECK(all_of_type<render::FilledRect>(out).empty());
}

TEST_CASE("build_esp: box styles, outline, head circle, skeleton, snaplines")
{
    game::GameSnapshot game = make_game();
    settings::EspSettings esp = enabled();
    settings::TeamMode team_mode = kTeams;
    esp.name = esp.weapon = esp.distance = esp.health_bar = false;

    SUBCASE("full box with outline: shadow rect + rect")
    {
        const auto out = features::build_esp(game, esp, team_mode, kScreen, kLineHeight);
        CHECK(all_of_type<render::Rect>(out).size() == 2);
        esp.outline = false;
        CHECK(all_of_type<render::Rect>(features::build_esp(game, esp, team_mode, kScreen, kLineHeight)).size() == 1);
    }
    SUBCASE("corners: 8 lines, 16 with the outline")
    {
        esp.box_style = settings::BoxStyle::corners;
        auto out = features::build_esp(game, esp, team_mode, kScreen, kLineHeight);
        CHECK(all_of_type<render::Rect>(out).empty());
        CHECK(all_of_type<render::Line>(out).size() == 16);
        esp.outline = false;
        out = features::build_esp(game, esp, team_mode, kScreen, kLineHeight);
        CHECK(all_of_type<render::Line>(out).size() == 8);
    }
    SUBCASE("head circle around the eyes (no bones)")
    {
        esp.head_circle = true;
        const auto circles =
            all_of_type<render::Circle>(features::build_esp(game, esp, team_mode, kScreen, kLineHeight));
        REQUIRE(circles.size() == 1);
        CHECK(circles[0].centre.x == doctest::Approx(960.0f));
        CHECK(circles[0].radius >= 2.0f);
    }
    SUBCASE("head circle on the middle of the head (bone 7), not the head joint (bone 6, the jaw)")
    {
        esp.head_circle = true;
        maths::Bones bones{};
        bones.fill(game.players[1].origin);
        bones[maths::bone::kHead] = {500.0f, 0.0f, 20.0f};
        bones[maths::bone::kHeadCentre] = {500.0f, 0.0f, 30.0f};
        game.players[1].bones = bones;
        const auto circles =
            all_of_type<render::Circle>(features::build_esp(game, esp, team_mode, kScreen, kLineHeight));
        REQUIRE(circles.size() == 1);
        CHECK(circles[0].centre.y == doctest::Approx(540.0f - 540.0f * 30.0f / 500.0f));
        // 6.5 units at 500 units away: 540 * 6.5 / 500 pixels.
        CHECK(circles[0].radius == doctest::Approx(540.0f * 6.5f / 500.0f).epsilon(0.01));
    }
    SUBCASE("skeleton only with bones")
    {
        esp.box = false;
        esp.skeleton = true;
        CHECK(all_of_type<render::Line>(features::build_esp(game, esp, team_mode, kScreen, kLineHeight)).empty());
        maths::Bones bones{};
        for (std::size_t i = 0; i < bones.size(); ++i)
        {
            bones[i] = game.players[1].origin + maths::Vec3{0.0f, 0.0f, static_cast<float>(i) * 3.0f};
        }
        game.players[1].bones = bones;
        CHECK(all_of_type<render::Line>(features::build_esp(game, esp, team_mode, kScreen, kLineHeight)).size() ==
              maths::kSkeletonLinks.size());
    }
    SUBCASE("snaplines from the chosen origin")
    {
        esp.box = false;
        esp.snaplines = true;
        auto lines = all_of_type<render::Line>(features::build_esp(game, esp, team_mode, kScreen, kLineHeight));
        REQUIRE(lines.size() == 1);
        CHECK(lines[0].from == maths::Vec2{960.0f, 1080.0f});
        esp.snapline_origin = settings::SnaplineOrigin::top;
        lines = all_of_type<render::Line>(features::build_esp(game, esp, team_mode, kScreen, kLineHeight));
        REQUIRE(lines.size() == 1);
        CHECK(lines[0].from == maths::Vec2{960.0f, 0.0f});
    }
}
