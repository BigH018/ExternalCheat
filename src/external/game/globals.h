#pragma once

// CGlobalVars (client.dll + dwGlobalVars -> CGlobalVars*): the game's clock and the player count limit.
// Layout in game/offsets.h (offsets::layout::kGlobals*), proven live in build 14189.
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <optional>
#include <string>

#include "core/memory.h"

namespace game
{
struct GlobalVars
{
    float realtime = 0.0f;          // seconds since the game started
    std::int32_t frame_count = 0;
    std::int32_t max_clients = 0;   // 64 on a local server
    float interval_per_tick = 0.0f; // 1/64
    float curtime = 0.0f;           // game time (tick_count * interval_per_tick)
    std::int32_t tick_count = 0;
    std::string map_name;           // "de_mirage"; empty if it couldn't be read

    // The values a running match has: a sane player limit and tick interval.
    [[nodiscard]] bool is_sane() const noexcept;
};

// Reads the globals. nullopt if the pointer or the struct can't be read.
[[nodiscard]] std::optional<GlobalVars> read_globals(const core::Memory& memory, std::uintptr_t client_base);
} // namespace game
