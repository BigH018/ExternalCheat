#include <cstdint>
#include <limits>

#include <doctest.h>

#include "game/globals.h"
#include "game/offsets.h"
#include "helpers/fake_memory.h"

namespace
{
namespace layout = game::offsets::layout;

constexpr std::uintptr_t kClientBase = 0x7FFD62930000;
constexpr std::uintptr_t kSlot = kClientBase + game::offsets::client::dwGlobalVars;
constexpr std::uintptr_t kGlobals = 0x4DF87480090;
constexpr std::uintptr_t kMapName = 0x4DF87490000;

// The values read live in build 14189 (2026-10-06, de_mirage, a bot match).
test::FakeMemory make_globals()
{
    test::FakeMemory memory;
    memory.map_value<std::uintptr_t>(kSlot, kGlobals);
    memory.map(kGlobals, layout::kGlobalsReadSize);
    memory.put<float>(kGlobals + layout::kGlobalsRealtime, 6237.1733f);
    memory.put<std::int32_t>(kGlobals + layout::kGlobalsFrameCount, 528497);
    memory.put<std::int32_t>(kGlobals + layout::kGlobalsMaxClients, 64);
    memory.put<float>(kGlobals + layout::kGlobalsIntervalPerTick, 0.015625f);
    memory.put<float>(kGlobals + layout::kGlobalsCurtime, 6242.3823f);
    memory.put<std::int32_t>(kGlobals + layout::kGlobalsTickCount, 399512);
    memory.put<std::uintptr_t>(kGlobals + layout::kGlobalsMapName, kMapName);
    memory.map(kMapName, 0x1000);
    memory.put_string(kMapName, "de_mirage");
    return memory;
}
} // namespace

TEST_CASE("read_globals reads every field in one go")
{
    const test::FakeMemory memory = make_globals();
    const auto globals = game::read_globals(memory, kClientBase);
    REQUIRE(globals.has_value());
    CHECK(globals->realtime == doctest::Approx(6237.1733f));
    CHECK(globals->frame_count == 528497);
    CHECK(globals->max_clients == 64);
    CHECK(globals->interval_per_tick == doctest::Approx(1.0f / 64.0f));
    CHECK(globals->curtime == doctest::Approx(6242.3823f));
    CHECK(globals->tick_count == 399512);
    CHECK(globals->map_name == "de_mirage");
    CHECK(globals->is_sane());
    CHECK(memory.read_count() <= 4); // pointer + struct + map name (page-bounded string)
}

TEST_CASE("read_globals: no map name, no pointer")
{
    test::FakeMemory memory = make_globals();
    memory.put<std::uintptr_t>(kGlobals + layout::kGlobalsMapName, 0);
    const auto globals = game::read_globals(memory, kClientBase);
    REQUIRE(globals.has_value());
    CHECK(globals->map_name.empty());

    memory.put<std::uintptr_t>(kSlot, 0);
    CHECK_FALSE(game::read_globals(memory, kClientBase).has_value());

    const test::FakeMemory nothing;
    CHECK_FALSE(game::read_globals(nothing, kClientBase).has_value());
}

TEST_CASE("GlobalVars::is_sane")
{
    game::GlobalVars globals{.max_clients = 64, .interval_per_tick = 1.0f / 64.0f, .curtime = 10.0f};
    CHECK(globals.is_sane());

    game::GlobalVars bad = globals;
    bad.max_clients = 0;
    CHECK_FALSE(bad.is_sane());
    bad = globals;
    bad.max_clients = 65;
    CHECK_FALSE(bad.is_sane());
    bad = globals;
    bad.interval_per_tick = 0.0f;
    CHECK_FALSE(bad.is_sane());
    bad = globals;
    bad.curtime = std::numeric_limits<float>::infinity();
    CHECK_FALSE(bad.is_sane());
}
