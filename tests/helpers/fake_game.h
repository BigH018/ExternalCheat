#pragma once

// Hand-built game snapshots for the aimbot and triggerbot tests.
//
// You (T, controller 1, pawn entity 101) stand with your eyes at the world origin, looking along +x (view angles 0, 0).
// Players are added with their eyes at a given point; pawn entity index = 100 + controller index.

#include <cstdint>
#include <string>

#include "game/snapshot.h"

namespace test
{
inline constexpr float kEyeHeight = 64.0f;

inline game::PlayerSnapshot make_player(std::uint32_t index, game::Team team, maths::Vec3 eye)
{
    game::PlayerSnapshot player;
    player.index = index;
    player.controller = 0x4DF5A000000 + index * 0x1000;
    player.pawn = 0x4DF60000000 + index * 0x10000;
    player.pawn_index = 100 + index;
    player.name = "Bot" + std::to_string(index);
    player.team = team;
    player.alive = true;
    player.health = 100;
    player.view_offset = {0.0f, 0.0f, kEyeHeight};
    player.origin = eye - player.view_offset;
    player.flags = game::kFlagOnGround;
    player.weapon_id = 7; // AK-47, a rifle
    return player;
}

// A match with only you in it.
inline game::GameSnapshot make_match()
{
    game::GameSnapshot game;
    game.in_match = true;
    game::PlayerSnapshot local = make_player(1, game::Team::terrorist, {0.0f, 0.0f, 0.0f});
    local.is_local = true;
    game.players.push_back(local);
    game.local_state.view_angles = maths::Angles{0.0f, 0.0f};
    return game;
}

inline game::PlayerSnapshot& add_player(game::GameSnapshot& game, std::uint32_t index, game::Team team,
                                        maths::Vec3 eye)
{
    game.players.push_back(make_player(index, team, eye));
    return game.players.back();
}

inline game::PlayerSnapshot& local_of(game::GameSnapshot& game)
{
    return game.players.front();
}
} // namespace test
