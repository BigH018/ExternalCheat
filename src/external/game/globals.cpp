#include "game/globals.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>

#include "config.h"
#include "game/offsets.h"

namespace game
{
namespace layout = offsets::layout;

namespace
{
template <core::RemoteValue T>
T field(const std::array<std::byte, layout::kGlobalsReadSize>& block, std::uintptr_t offset) noexcept
{
    T value{};
    std::memcpy(&value, block.data() + offset, sizeof(T));
    return value;
}
} // namespace

bool GlobalVars::is_sane() const noexcept
{
    return max_clients > 0 && max_clients <= static_cast<std::int32_t>(config::kMaxPlayers) &&
           std::isfinite(interval_per_tick) && interval_per_tick > 0.0f && interval_per_tick < 1.0f &&
           std::isfinite(curtime);
}

std::optional<GlobalVars> read_globals(const core::Memory& memory, std::uintptr_t client_base)
{
    const auto pointer = memory.read<std::uintptr_t>(client_base + offsets::client::dwGlobalVars);
    if (!pointer || !core::is_plausible_pointer(*pointer, alignof(float)))
    {
        return std::nullopt;
    }
    // One read for the whole struct, so the fields belong to the same moment.
    std::array<std::byte, layout::kGlobalsReadSize> block{};
    if (!memory.read_bytes(*pointer, block.data(), block.size()))
    {
        return std::nullopt;
    }
    GlobalVars globals;
    globals.realtime = field<float>(block, layout::kGlobalsRealtime);
    globals.frame_count = field<std::int32_t>(block, layout::kGlobalsFrameCount);
    globals.max_clients = field<std::int32_t>(block, layout::kGlobalsMaxClients);
    globals.interval_per_tick = field<float>(block, layout::kGlobalsIntervalPerTick);
    globals.curtime = field<float>(block, layout::kGlobalsCurtime);
    globals.tick_count = field<std::int32_t>(block, layout::kGlobalsTickCount);
    const auto map_name = field<std::uintptr_t>(block, layout::kGlobalsMapName);
    if (core::is_plausible_pointer(map_name))
    {
        globals.map_name = core::read_string(memory, map_name, config::kMaxNameLength).value_or("");
    }
    return globals;
}
} // namespace game
