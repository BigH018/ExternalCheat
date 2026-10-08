#include <string>
#include <variant>
#include <vector>

#include <doctest.h>

#include "config.h"
#include "features/spectators.h"
#include "helpers/fake_game.h"
#include "settings/settings.h"

namespace
{
using game::ObserverMode;
using game::Team;

constexpr maths::Vec2 kScreen{1920.0f, 1080.0f};
constexpr float kLineHeight = 15.0f;

// A dead player watching `target` (a pawn address; 0 = nobody) in `mode`.
game::PlayerSnapshot& add_dead(game::GameSnapshot& game, std::uint32_t index, Team team, ObserverMode mode,
                               std::uintptr_t target)
{
    game::PlayerSnapshot& player = test::add_player(game, index, team, {100.0f * index, 0.0f, 0.0f});
    player.alive = false;
    player.health = 0;
    player.observer_mode = mode;
    player.observer_target = target;
    return player;
}

settings::SpectatorSettings enabled()
{
    settings::SpectatorSettings list;
    list.enabled = true;
    return list;
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
} // namespace

TEST_CASE("spectator_info: dead players watching you in first or third person, nobody else")
{
    game::GameSnapshot game = test::make_match();
    const std::uintptr_t you = test::local_of(game).pawn;
    add_dead(game, 2, Team::counter_terrorist, ObserverMode::in_eye, you);
    add_dead(game, 3, Team::terrorist, ObserverMode::chase, you);
    add_dead(game, 4, Team::terrorist, ObserverMode::roaming, you);   // free camera: not watching anyone
    add_dead(game, 5, Team::terrorist, ObserverMode::fixed, you);
    add_dead(game, 6, Team::terrorist, ObserverMode::in_eye, 0);      // death cam over, target not resolved
    test::add_player(game, 7, Team::counter_terrorist, {500.0f, 0.0f, 0.0f});
    const std::uintptr_t bot7 = game.players.back().pawn;
    add_dead(game, 8, Team::counter_terrorist, ObserverMode::in_eye, bot7); // watching someone else
    // A living player's left-over camera never counts (game/player doesn't even fill it, but be safe).
    game::PlayerSnapshot& living = test::add_player(game, 9, Team::terrorist, {900.0f, 0.0f, 0.0f});
    living.observer_mode = ObserverMode::in_eye;
    living.observer_target = you;

    const auto info = features::spectator_info(game, settings::TeamMode::teams);
    REQUIRE(info.has_value());
    CHECK(info->watched.empty());
    REQUIRE(info->spectators.size() == 2);
    CHECK(info->spectators[0].name == "Bot2");
    CHECK(info->spectators[0].mode == ObserverMode::in_eye);
    CHECK(info->spectators[0].enemy);
    CHECK(info->spectators[1].name == "Bot3");
    CHECK(info->spectators[1].mode == ObserverMode::chase);
    CHECK_FALSE(info->spectators[1].enemy);

    // Free for all: everyone is an enemy.
    const auto ffa = features::spectator_info(game, settings::TeamMode::free_for_all);
    REQUIRE(ffa.has_value());
    CHECK(ffa->spectators[1].enemy);
}

TEST_CASE("spectator_info: while you're dead, the player you watch and who else watches them")
{
    game::GameSnapshot game = test::make_match();
    test::add_player(game, 2, Team::terrorist, {300.0f, 0.0f, 0.0f});
    const std::uintptr_t kev = game.players.back().pawn;
    game.players.back().name = "Kev";
    add_dead(game, 3, Team::terrorist, ObserverMode::in_eye, kev);

    game::PlayerSnapshot& local = test::local_of(game);
    local.alive = false;
    local.observer_mode = ObserverMode::chase;
    local.observer_target = kev;
    CHECK(features::watched_pawn(game) == kev);
    const auto info = features::spectator_info(game, settings::TeamMode::teams);
    REQUIRE(info.has_value());
    CHECK(info->watched == "Kev");
    REQUIRE(info->spectators.size() == 1); // not you
    CHECK(info->spectators[0].name == "Bot3");

    // Your death cam or a free camera: nobody to list.
    test::local_of(game).observer_mode = ObserverMode::roaming;
    CHECK(features::watched_pawn(game) == 0);
    CHECK_FALSE(features::spectator_info(game, settings::TeamMode::teams).has_value());
}

TEST_CASE("spectator_info: none outside a match")
{
    CHECK_FALSE(features::spectator_info(game::GameSnapshot{}, settings::TeamMode::teams).has_value());
}

TEST_CASE("observer_mode_name")
{
    CHECK(std::string(features::observer_mode_name(ObserverMode::in_eye)) == "1st person");
    CHECK(std::string(features::observer_mode_name(ObserverMode::chase)) == "3rd person");
    CHECK(std::string(features::observer_mode_name(ObserverMode::roaming)).empty());
}

TEST_CASE("build_spectator_list: off by default, rows, modes, empty panel, side")
{
    game::GameSnapshot game = test::make_match();
    const std::uintptr_t you = test::local_of(game).pawn;

    CHECK(features::build_spectator_list(game, settings::SpectatorSettings{}, settings::TeamMode::teams, kScreen,
                                         kLineHeight)
              .empty());

    // Nobody watching: "Nobody", or no panel at all with hide_when_empty.
    settings::SpectatorSettings list = enabled();
    auto primitives = features::build_spectator_list(game, list, settings::TeamMode::teams, kScreen, kLineHeight);
    auto lines = texts(primitives);
    REQUIRE(lines.size() == 3);
    CHECK(lines[0] == "Spectators");
    CHECK(lines[1] == "0");
    CHECK(lines[2] == "Nobody");
    list.hide_when_empty = true;
    CHECK(features::build_spectator_list(game, list, settings::TeamMode::teams, kScreen, kLineHeight).empty());

    add_dead(game, 2, Team::counter_terrorist, ObserverMode::in_eye, you);
    add_dead(game, 3, Team::terrorist, ObserverMode::chase, you);
    primitives = features::build_spectator_list(game, list, settings::TeamMode::teams, kScreen, kLineHeight);
    lines = texts(primitives);
    CHECK(lines == std::vector<std::string>{"Spectators", "2", "Bot2", "1st person", "Bot3", "3rd person"});

    // Background first, on the right edge by default; on the left when asked.
    const auto* background = std::get_if<render::FilledRect>(&primitives.front());
    REQUIRE(background != nullptr);
    CHECK(background->max.x == doctest::Approx(kScreen.x - config::kSpectatorMargin));
    CHECK(background->min.y == doctest::Approx(list.top));
    CHECK(background->max.y > background->min.y + 3.0f * kLineHeight);
    list.side = settings::PanelSide::left;
    primitives = features::build_spectator_list(game, list, settings::TeamMode::teams, kScreen, kLineHeight);
    background = std::get_if<render::FilledRect>(&primitives.front());
    REQUIRE(background != nullptr);
    CHECK(background->min.x == doctest::Approx(config::kSpectatorMargin));

    list.show_mode = false;
    lines = texts(features::build_spectator_list(game, list, settings::TeamMode::teams, kScreen, kLineHeight));
    CHECK(lines == std::vector<std::string>{"Spectators", "2", "Bot2", "Bot3"});
}

TEST_CASE("build_spectator_list: while dead, the title names the player you watch")
{
    game::GameSnapshot game = test::make_match();
    test::add_player(game, 2, Team::terrorist, {300.0f, 0.0f, 0.0f}).name = "Kev";
    const std::uintptr_t kev = game.players.back().pawn;
    game::PlayerSnapshot& local = test::local_of(game);
    local.alive = false;
    local.observer_mode = ObserverMode::in_eye;
    local.observer_target = kev;
    const auto lines =
        texts(features::build_spectator_list(game, enabled(), settings::TeamMode::teams, kScreen, kLineHeight));
    REQUIRE_FALSE(lines.empty());
    CHECK(lines[0] == "Watching Kev");
}
