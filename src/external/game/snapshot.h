#pragma once

// Plain copies of game state, taken once per read. Features and the UI only ever see these, never game pointers
// (the pointers here are for game/ code and the debug views, not to be dereferenced elsewhere).
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "game/globals.h"
#include "game/visibility.h"
#include "maths/angles.h"
#include "maths/projection.h"
#include "maths/skeleton.h"
#include "maths/vec.h"

namespace game
{
enum class Team : std::uint8_t
{
    none = 0,
    spectator = 1,
    terrorist = 2,
    counter_terrorist = 3,
};

// m_fFlags bits (Source engine player flags).
inline constexpr std::uint32_t kFlagOnGround = 1u << 0;

// How a dead player spectates (ObserverMode_t, from the server.dll dump; the client field is a uint8). Proven live
// 2026-10-07: a bot that dies is `roaming` without a target for ~5 s (the death cam), then `in_eye` on a player pawn.
enum class ObserverMode : std::uint8_t
{
    none = 0,
    fixed = 1,
    in_eye = 2,  // first person
    chase = 3,   // third person
    roaming = 4, // free camera
};

// First or third person: the modes in which the target is the one being watched.
[[nodiscard]] constexpr bool watches_target(ObserverMode mode) noexcept
{
    return mode == ObserverMode::in_eye || mode == ObserverMode::chase;
}

struct PlayerSnapshot
{
    std::uint32_t index = 0;       // controller entity index; player slot = index - 1
    std::uintptr_t controller = 0;
    std::uintptr_t pawn = 0;       // 0 if the controller has no pawn right now
    std::uint32_t pawn_index = 0;  // the pawn's entity index (what m_iIDEntIndex names), 0 without a pawn
    bool is_local = false;

    std::string name;
    Team team = Team::none;

    // From the pawn (zero / false if there is none).
    bool alive = false;            // controller says so, life state 0, health > 0
    std::int32_t health = 0;
    std::int32_t armor = 0;
    maths::Vec3 origin;            // feet
    maths::Vec3 view_offset;       // eye height above the feet
    maths::Angles eye_angles;      // m_angEyeAngles: where the player looks (the radar's facing lines)
    bool dormant = false;
    std::uint32_t flags = 0;       // m_fFlags (bit 0 = on the ground)
    bool scoped = false;
    std::optional<std::uint16_t> weapon_id;
    std::uint64_t spotted_by_mask = 0;  // a bit per player slot that can see this pawn (game/visibility)
    std::optional<maths::Bones> bones;  // world positions (maths/skeleton.h), if the bone array read sanely

    // While dead (the controller says so): how this player spectates and whom (game/observer). Living players: none, 0.
    ObserverMode observer_mode = ObserverMode::none;
    std::uintptr_t observer_target = 0; // the pawn being watched, 0 = none (or a stale handle)

    [[nodiscard]] maths::Vec3 eye_position() const noexcept { return origin + view_offset; }
    // The middle of the head: bone::kHeadCentre if the bones were read, otherwise the eye position.
    [[nodiscard]] maths::Vec3 head_position() const noexcept
    {
        return bones ? (*bones)[maths::bone::kHeadCentre] : eye_position();
    }
    [[nodiscard]] std::uint32_t slot() const noexcept { return player_slot(index); }
    [[nodiscard]] bool on_ground() const noexcept { return (flags & kFlagOnGround) != 0; }
};

// What only the local player has: the entity under the crosshair, the flash, where the camera looks.
struct LocalState
{
    std::int32_t crosshair_entity = -1; // m_iIDEntIndex: entity index under the crosshair, -1 = none
    float flash_alpha = 0.0f;           // m_flFlashOverlayAlpha: the white overlay right now
    float flash_max_alpha = 0.0f;       // m_flFlashMaxAlpha: its peak for the current flash
    std::optional<maths::Angles> view_angles; // dwViewAngles

    // More than config::kFlashedFraction of the flash's peak is still on screen.
    [[nodiscard]] bool flashed(float fraction) const noexcept
    {
        return flash_max_alpha > 0.0f && flash_alpha > flash_max_alpha * fraction;
    }
};

// The planted bomb (game/bomb). Times are game times, like GlobalVars::curtime.
struct PlantedBomb
{
    std::uintptr_t entity = 0;
    std::int32_t site = -1;          // m_nBombSite: 0 = A, 1 = B
    maths::Vec3 position;
    bool ticking = false;
    bool exploded = false;
    bool defused = false;
    bool being_defused = false;
    float blow_time = 0.0f;          // m_flC4Blow: when it explodes
    float timer_length = 0.0f;       // m_flTimerLength: the whole fuse (40 s by default)
    float defuse_end = 0.0f;         // m_flDefuseCountDown: when the defuse in progress completes
    float defuse_length = 0.0f;      // m_flDefuseLength: 10 s, 5 s with a kit
    std::uintptr_t defuser_pawn = 0; // the pawn defusing it, 0 = nobody
};

// Everything one read of the game produced.
struct GameSnapshot
{
    bool in_match = false;              // the local player's controller exists
    LocalState local_state;             // only meaningful in a match
    GlobalVars globals;
    std::optional<maths::ViewMatrix> view; // only when is_sane()
    std::vector<PlayerSnapshot> players; // every controller, the local player included, in index order
    std::optional<PlantedBomb> bomb;     // a bomb is planted; only read while the bomb timer is on (game/bomb)

    // The local player, or nullptr if they aren't in the list.
    [[nodiscard]] const PlayerSnapshot* local() const noexcept
    {
        for (const PlayerSnapshot& player : players)
        {
            if (player.is_local)
            {
                return &player;
            }
        }
        return nullptr;
    }
};

// "T", "CT", "SPEC", "-".
[[nodiscard]] constexpr const char* team_short_name(Team team) noexcept
{
    switch (team)
    {
    case Team::terrorist: return "T";
    case Team::counter_terrorist: return "CT";
    case Team::spectator: return "SPEC";
    case Team::none: break;
    }
    return "-";
}
} // namespace game
