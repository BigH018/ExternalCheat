#include <cmath>
#include <numbers>
#include <vector>

#include <doctest.h>

#include "features/aimbot.h"
#include "features/targeting.h"
#include "helpers/fake_game.h"
#include "maths/skeleton.h"

using doctest::Approx;
using game::Team;

namespace
{
constexpr settings::TeamMode kTeams = settings::TeamMode::teams;

settings::AimbotSettings snap_settings()
{
    settings::AimbotSettings aim;
    aim.enabled = true;
    aim.smoothing = 1.0f;
    aim.fov = 10.0f;
    return aim;
}

float degrees(float radians)
{
    return radians * 180.0f / std::numbers::pi_v<float>;
}
} // namespace

TEST_CASE("targeting: live targets, enemies, distance")
{
    game::GameSnapshot game = test::make_match();
    game::PlayerSnapshot& enemy = test::add_player(game, 2, Team::counter_terrorist, {500.0f, 0.0f, 0.0f});
    const game::PlayerSnapshot& local = test::local_of(game);
    CHECK(features::is_live_target(enemy));
    CHECK_FALSE(features::is_live_target(local));
    CHECK(features::distance_metres(local, enemy) == Approx(500.0f * 0.0254f));
    CHECK(features::within_distance(10.0f, 0.0f)); // 0 = no limit
    CHECK(features::within_distance(10.0f, 10.0f));
    CHECK_FALSE(features::within_distance(10.5f, 10.0f));
    enemy.dormant = true;
    CHECK_FALSE(features::is_live_target(enemy));
    enemy.dormant = false;
    enemy.alive = false;
    CHECK_FALSE(features::is_live_target(enemy));
}

TEST_CASE("aim_point: head, body, nearest; with and without bones")
{
    game::GameSnapshot game = test::make_match();
    game::PlayerSnapshot& enemy = test::add_player(game, 2, Team::counter_terrorist, {500.0f, 0.0f, 0.0f});
    const maths::Vec3 eye{0.0f, 0.0f, 0.0f};
    const maths::Angles view{0.0f, 0.0f};

    SUBCASE("no bones: head = eyes, body lower")
    {
        CHECK(features::aim_point(enemy, settings::AimTarget::head, eye, view) == enemy.eye_position());
        const maths::Vec3 body = features::aim_point(enemy, settings::AimTarget::body, eye, view);
        CHECK(body.z == Approx(enemy.origin.z + 0.7f * test::kEyeHeight));
        // Looking slightly up: the head is nearer the crosshair than the body.
        CHECK(features::aim_point(enemy, settings::AimTarget::nearest, eye, {-1.0f, 0.0f}) == enemy.eye_position());
    }
    SUBCASE("bones: the middle of the head (bone 7), the chest (bone 4), the nearest of five")
    {
        maths::Bones bones{};
        bones.fill(enemy.origin);
        bones[maths::bone::kHead] = {500.0f, 0.0f, -4.0f};
        bones[maths::bone::kHeadCentre] = {502.0f, 0.0f, 0.0f};
        bones[maths::bone::kSpine3] = {500.0f, 0.0f, -18.0f};
        bones[maths::bone::kPelvis] = {500.0f, 0.0f, -28.0f};
        enemy.bones = bones;
        CHECK(features::aim_point(enemy, settings::AimTarget::head, eye, view) == bones[maths::bone::kHeadCentre]);
        CHECK(features::aim_point(enemy, settings::AimTarget::body, eye, view) == bones[maths::bone::kSpine3]);
        // Looking down at the pelvis: nearest picks it.
        const maths::Angles at_pelvis = maths::calc_aim_angles(eye, bones[maths::bone::kPelvis]);
        CHECK(features::aim_point(enemy, settings::AimTarget::nearest, eye, at_pelvis) == bones[maths::bone::kPelvis]);
    }
}

TEST_CASE("find_candidates: FOV, team check, distance, visible only, nothing outside a match")
{
    game::GameSnapshot game = test::make_match();
    test::add_player(game, 2, Team::counter_terrorist, {500.0f, 0.0f, 0.0f});  // on the crosshair
    test::add_player(game, 3, Team::counter_terrorist, {500.0f, 50.0f, 0.0f}); // 5.7 degrees left
    test::add_player(game, 4, Team::terrorist, {500.0f, -20.0f, 0.0f});        // teammate, 2.3 degrees right
    settings::AimbotSettings aim = snap_settings();

    SUBCASE("FOV")
    {
        aim.fov = 5.0f;
        const auto candidates = features::find_candidates(game, aim, kTeams);
        REQUIRE(candidates.size() == 1);
        CHECK(candidates[0].player->index == 2);
        CHECK(candidates[0].fov_distance == Approx(0.0f));
        aim.fov = 10.0f;
        CHECK(features::find_candidates(game, aim, kTeams).size() == 2);
    }
    SUBCASE("team check off and free for all both take the teammate")
    {
        aim.team_check = false;
        CHECK(features::find_candidates(game, aim, kTeams).size() == 3);
        aim.team_check = true;
        CHECK(features::find_candidates(game, aim, settings::TeamMode::free_for_all).size() == 3);
    }
    SUBCASE("max distance")
    {
        aim.max_distance = 10.0f; // 500 units = 12.7 m
        CHECK(features::find_candidates(game, aim, kTeams).empty());
        aim.max_distance = 13.0f;
        CHECK(features::find_candidates(game, aim, kTeams).size() == 2);
    }
    SUBCASE("visible only: your slot's bit (slot 0)")
    {
        aim.visible_only = true;
        CHECK(features::find_candidates(game, aim, kTeams).empty());
        game.players[2].spotted_by_mask = 1;
        const auto candidates = features::find_candidates(game, aim, kTeams);
        REQUIRE(candidates.size() == 1);
        CHECK(candidates[0].player->index == 3);
    }
    SUBCASE("nothing outside a match, without view angles, or while you're dead")
    {
        game::GameSnapshot copy = game;
        copy.in_match = false;
        CHECK(features::find_candidates(copy, aim, kTeams).empty());
        copy = game;
        copy.local_state.view_angles.reset();
        CHECK(features::find_candidates(copy, aim, kTeams).empty());
        copy = game;
        test::local_of(copy).alive = false;
        CHECK(features::find_candidates(copy, aim, kTeams).empty());
    }
}

TEST_CASE("select_target: each priority, ties by crosshair")
{
    game::GameSnapshot game = test::make_match();
    test::add_player(game, 2, Team::counter_terrorist, {800.0f, 0.0f, 0.0f}).health = 90;  // on the crosshair, far
    test::add_player(game, 3, Team::counter_terrorist, {300.0f, 40.0f, 0.0f}).health = 20; // off it, close, weak
    test::add_player(game, 4, Team::counter_terrorist, {300.0f, -45.0f, 0.0f}).health = 20; // same health, further off
    settings::AimbotSettings aim = snap_settings();
    aim.fov = 20.0f;
    const auto candidates = features::find_candidates(game, aim, kTeams);
    REQUIRE(candidates.size() == 3);

    CHECK(features::select_target(candidates, settings::AimPriority::crosshair)->player->index == 2);
    CHECK(features::select_target(candidates, settings::AimPriority::distance)->player->index == 3);
    CHECK(features::select_target(candidates, settings::AimPriority::lowest_health)->player->index == 3);
    CHECK_FALSE(features::select_target({}, settings::AimPriority::crosshair).has_value());
}

TEST_CASE("compute_aim: snap, smoothed step, nothing to aim at")
{
    game::GameSnapshot game = test::make_match();
    test::add_player(game, 2, Team::counter_terrorist, {500.0f, 50.0f, 0.0f});
    const float target_yaw = degrees(std::atan2(50.0f, 500.0f));
    settings::AimbotSettings aim = snap_settings();

    const auto snapped = features::compute_aim(game, aim, kTeams, 1.0f / 60.0f);
    REQUIRE(snapped.has_value());
    CHECK(snapped->yaw == Approx(target_yaw));
    CHECK(snapped->pitch == Approx(0.0f));

    aim.smoothing = 5.0f;
    const auto step = features::compute_aim(game, aim, kTeams, 1.0f / 60.0f);
    REQUIRE(step.has_value());
    CHECK(step->yaw == Approx(target_yaw / 5.0f));

    aim.fov = 1.0f;
    CHECK_FALSE(features::compute_aim(game, aim, kTeams, 1.0f / 60.0f).has_value());
}

TEST_CASE("fov_circle: radius from the view matrix, nothing without one")
{
    game::GameSnapshot game = test::make_match();
    // A camera at the origin looking along +x: clip.x = -y, clip.y = z, w = x.
    game.view = maths::ViewMatrix{{0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                   0.0f, 0.0f, 0.0f}};
    settings::AimbotSettings aim = snap_settings();
    aim.fov = 5.0f;
    const maths::Vec2 screen{1920.0f, 1080.0f};
    const auto circle = features::fov_circle(game, aim, screen);
    REQUIRE(circle.has_value());
    CHECK(circle->centre.x == Approx(960.0f));
    CHECK(circle->centre.y == Approx(540.0f));
    CHECK(circle->radius == Approx(540.0f * std::tan(5.0f * std::numbers::pi_v<float> / 180.0f)));
    game.view.reset();
    CHECK_FALSE(features::fov_circle(game, aim, screen).has_value());
}
