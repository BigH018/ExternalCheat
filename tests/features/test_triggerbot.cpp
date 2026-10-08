#include <cstdint>
#include <vector>

#include <doctest.h>

#include "config.h"
#include "features/triggerbot.h"
#include "helpers/fake_game.h"
#include "maths/skeleton.h"

using features::TriggerBlock;
using game::Team;

namespace
{
constexpr settings::TeamMode kTeams = settings::TeamMode::teams;

// You with an AK-47; an enemy CT 500 units ahead, under the crosshair.
game::GameSnapshot on_target()
{
    game::GameSnapshot game = test::make_match();
    const game::PlayerSnapshot& enemy = test::add_player(game, 2, Team::counter_terrorist, {500.0f, 0.0f, 0.0f});
    game.local_state.crosshair_entity = static_cast<std::int32_t>(enemy.pawn_index);
    return game;
}

settings::TriggerbotSettings defaults()
{
    settings::TriggerbotSettings trigger;
    trigger.enabled = true;
    return trigger;
}

// Runs the state machine from t = 0 to `end_ms` in 1 ms frames; returns the times attack went down.
std::vector<std::uint64_t> presses(features::Triggerbot& bot, const settings::TriggerbotSettings& trigger,
                                   std::uint64_t end_ms, bool target = true, std::uint64_t lose_target_at = ~0ull)
{
    std::vector<std::uint64_t> down_at;
    bool was_down = false;
    for (std::uint64_t t = 0; t <= end_ms; ++t)
    {
        const bool down = bot.update(t, true, target && t < lose_target_at, trigger);
        if (down && !was_down)
        {
            down_at.push_back(t);
        }
        was_down = down;
    }
    return down_at;
}
} // namespace

TEST_CASE("trigger_target: the player whose pawn is under the crosshair, if the filters allow")
{
    game::GameSnapshot game = on_target();
    settings::TriggerbotSettings trigger = defaults();
    REQUIRE(features::trigger_target(game, trigger, kTeams) != nullptr);
    CHECK(features::trigger_target(game, trigger, kTeams)->index == 2);

    SUBCASE("nothing under the crosshair, or a non-player entity")
    {
        game.local_state.crosshair_entity = -1;
        CHECK(features::trigger_target(game, trigger, kTeams) == nullptr);
        game.local_state.crosshair_entity = 999;
        CHECK(features::trigger_target(game, trigger, kTeams) == nullptr);
    }
    SUBCASE("a teammate: only with team check off or in free for all")
    {
        game.players[1].team = Team::terrorist;
        CHECK(features::trigger_target(game, trigger, kTeams) == nullptr);
        CHECK(features::trigger_target(game, trigger, settings::TeamMode::free_for_all) != nullptr);
        trigger.team_check = false;
        CHECK(features::trigger_target(game, trigger, kTeams) != nullptr);
    }
    SUBCASE("dead, max distance, visible only")
    {
        trigger.max_distance = 10.0f; // 500 units = 12.7 m
        CHECK(features::trigger_target(game, trigger, kTeams) == nullptr);
        trigger.max_distance = 0.0f;
        trigger.visible_only = true;
        CHECK(features::trigger_target(game, trigger, kTeams) == nullptr);
        game.players[1].spotted_by_mask = 1; // your slot (0)
        CHECK(features::trigger_target(game, trigger, kTeams) != nullptr);
        game.players[1].alive = false;
        CHECK(features::trigger_target(game, trigger, kTeams) == nullptr);
    }
    SUBCASE("head only: the crosshair ray within the head radius of the middle of the head")
    {
        trigger.head_only = true;
        maths::Bones bones{};
        bones.fill(game.players[1].origin);
        bones[maths::bone::kHeadCentre] = {500.0f, 0.0f, 0.0f};
        game.players[1].bones = bones;
        CHECK(features::trigger_target(game, trigger, kTeams) != nullptr); // looking straight at it
        game.local_state.view_angles = maths::Angles{2.0f, 0.0f};         // 17 units lower: the chest
        CHECK(features::trigger_target(game, trigger, kTeams) == nullptr);
        game.local_state.view_angles.reset();
        CHECK(features::trigger_target(game, trigger, kTeams) == nullptr);
    }
}

TEST_CASE("trigger_block: you and your weapon first, then the target")
{
    game::GameSnapshot game = on_target();
    settings::TriggerbotSettings trigger = defaults();
    game::PlayerSnapshot& local = test::local_of(game);
    CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::none);

    SUBCASE("not in a match / dead")
    {
        local.alive = false;
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::not_in_match);
    }
    SUBCASE("weapon filter; knives and grenades never fire")
    {
        trigger.weapons.rifle = false;
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::weapon);
        trigger.weapons.rifle = true;
        local.weapon_id = 42; // knife
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::weapon);
        local.weapon_id = 43; // flashbang
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::weapon);
        local.weapon_id.reset();
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::weapon);
        local.weapon_id = 4; // Glock-18
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::none);
    }
    SUBCASE("snipers only when scoped")
    {
        local.weapon_id = 9; // AWP
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::not_scoped);
        local.scoped = true;
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::none);
        local.scoped = false;
        trigger.snipers_scoped_only = false;
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::none);
    }
    SUBCASE("flashed: more than half the flash's peak on screen")
    {
        game.local_state.flash_max_alpha = 255.0f;
        game.local_state.flash_alpha = 200.0f;
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::flashed);
        game.local_state.flash_alpha = 100.0f;
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::none);
        game.local_state.flash_alpha = 200.0f;
        trigger.not_flashed = false;
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::none);
    }
    SUBCASE("in the air")
    {
        local.flags = 0;
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::in_air);
        trigger.not_in_air = false;
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::none);
    }
    SUBCASE("no target")
    {
        game.local_state.crosshair_entity = -1;
        CHECK(features::trigger_block(game, trigger, kTeams) == TriggerBlock::no_target);
    }
}

TEST_CASE("Triggerbot: single tap after the reaction delay, then one tap per shot delay")
{
    features::Triggerbot bot;
    settings::TriggerbotSettings trigger = defaults();
    trigger.reaction_ms = 40;
    trigger.shot_delay_ms = 150;
    const auto down_at = presses(bot, trigger, 500);
    // Tap at 40, held kTriggerTapMs, then 150 ms of cooldown before the next tap (no new reaction).
    const std::uint64_t period = config::kTriggerTapMs + 150;
    REQUIRE(down_at.size() == 3);
    CHECK(down_at[0] == 40);
    CHECK(down_at[1] == 40 + period);
    CHECK(down_at[2] == 40 + 2 * period);
}

TEST_CASE("Triggerbot: a tap lasts kTriggerTapMs")
{
    features::Triggerbot bot;
    settings::TriggerbotSettings trigger = defaults();
    trigger.reaction_ms = 0;
    CHECK(bot.update(0, true, true, trigger)); // 0 ms reaction: fires on the first frame
    CHECK(bot.update(config::kTriggerTapMs - 1, true, true, trigger));
    CHECK_FALSE(bot.update(config::kTriggerTapMs, true, true, trigger));
}

TEST_CASE("Triggerbot: losing the target during the reaction delay cancels the shot")
{
    features::Triggerbot bot;
    settings::TriggerbotSettings trigger = defaults();
    trigger.reaction_ms = 100;
    CHECK(presses(bot, trigger, 300, true, 50).empty());
}

TEST_CASE("Triggerbot: burst fires N shots the delay apart, and stops when the target leaves")
{
    settings::TriggerbotSettings trigger = defaults();
    trigger.fire_mode = settings::FireMode::burst;
    trigger.burst_shots = 3;
    trigger.reaction_ms = 0;
    trigger.shot_delay_ms = 50;
    const std::uint64_t step = config::kTriggerTapMs + 50;
    {
        features::Triggerbot bot;
        const auto down_at = presses(bot, trigger, 3 * step - 1);
        REQUIRE(down_at.size() == 3);
        CHECK(down_at[1] == step);
        CHECK(down_at[2] == 2 * step);
    }
    {
        features::Triggerbot bot;
        CHECK(presses(bot, trigger, 1000, true, step - 10).size() == 1); // gone before the second shot
    }
}

TEST_CASE("Triggerbot: hold mode keeps attack down while on target")
{
    features::Triggerbot bot;
    settings::TriggerbotSettings trigger = defaults();
    trigger.fire_mode = settings::FireMode::hold;
    trigger.reaction_ms = 10;
    CHECK_FALSE(bot.update(0, true, true, trigger));
    CHECK(bot.update(10, true, true, trigger));
    CHECK(bot.update(900, true, true, trigger));
    CHECK_FALSE(bot.update(901, true, false, trigger));
}

TEST_CASE("Triggerbot: inactive never fires; a tap in progress finishes; reset lets go")
{
    settings::TriggerbotSettings trigger = defaults();
    trigger.reaction_ms = 0;
    {
        features::Triggerbot bot;
        for (std::uint64_t t = 0; t < 200; ++t)
        {
            CHECK_FALSE(bot.update(t, false, true, trigger));
        }
    }
    {
        features::Triggerbot bot;
        CHECK(bot.update(0, true, true, trigger));
        CHECK(bot.update(1, false, true, trigger)); // still the same tap
        CHECK_FALSE(bot.update(config::kTriggerTapMs, false, true, trigger));
    }
    {
        features::Triggerbot bot;
        trigger.fire_mode = settings::FireMode::hold;
        CHECK(bot.update(0, true, true, trigger));
        bot.reset();
        CHECK_FALSE(bot.attack_down());
    }
}
