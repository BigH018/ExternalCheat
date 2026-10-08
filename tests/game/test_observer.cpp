#include <cstdint>
#include <optional>

#include <doctest.h>

#include "game/observer.h"
#include "game/schema.h"
#include "helpers/fake_entities.h"
#include "helpers/fake_memory.h"

namespace
{
using test::FakeEntityList;
namespace schema = game::schema;

constexpr std::uintptr_t kController = 0x4DF5A001000;
constexpr std::uintptr_t kObserverPawn = 0x4DF61000000;
constexpr std::uintptr_t kServices = 0x4DF62000000;
constexpr std::uintptr_t kTarget = 0x4DF63000000;

// A controller (index 13) whose observer pawn (index 189) has observer services, as in the game: mode and target set
// by the test. The watched pawn is index 271.
struct ObserverWorld
{
    test::FakeMemory memory;
    FakeEntityList list{memory};
    std::uint32_t target_handle = 0;

    ObserverWorld()
    {
        memory.map(kController, 0x1000);
        memory.map(kObserverPawn, 0x2000);
        memory.map(kServices, 0x100);
        memory.map(kTarget, 0x100);
        list.add(13, kController, 0x113, "cs_player_controller");
        const std::uint32_t observer_handle = list.add(189, kObserverPawn, 0x2B6, "c_cs_observer_for_precache");
        memory.put<std::uint32_t>(kController + schema::CCSPlayerController::m_hObserverPawn, observer_handle);
        memory.put<std::uintptr_t>(kObserverPawn + schema::C_BasePlayerPawn::m_pObserverServices, kServices);
        target_handle = list.add(271, kTarget, 0x2B6, "c_cs_player_for_precache");
    }

    void watch(game::ObserverMode mode, std::uint32_t handle)
    {
        memory.put<std::uint8_t>(kServices + schema::CPlayer_ObserverServices::m_iObserverMode,
                                 static_cast<std::uint8_t>(mode));
        memory.put<std::uint32_t>(kServices + schema::CPlayer_ObserverServices::m_hObserverTarget, handle);
    }

    [[nodiscard]] std::optional<game::ObserverState> read() const
    {
        return game::read_observer(memory, FakeEntityList::kSystem, kController);
    }
};
} // namespace

TEST_CASE("read_observer: first person on a pawn, through the observer pawn's services")
{
    ObserverWorld world;
    world.watch(game::ObserverMode::in_eye, world.target_handle);
    const auto state = world.read();
    REQUIRE(state.has_value());
    CHECK(state->mode == game::ObserverMode::in_eye);
    CHECK(state->target_handle == world.target_handle);
    CHECK(state->target == kTarget);
}

TEST_CASE("read_observer: the death cam (roaming, no target) and a stale target")
{
    ObserverWorld world;
    world.watch(game::ObserverMode::roaming, 0xFFFFFFFF);
    auto state = world.read();
    REQUIRE(state.has_value());
    CHECK(state->mode == game::ObserverMode::roaming);
    CHECK(state->target == 0);

    // The watched slot was reused since (different serial): no target.
    world.watch(game::ObserverMode::chase, FakeEntityList::make_handle(271, 0x2B5));
    state = world.read();
    REQUIRE(state.has_value());
    CHECK(state->mode == game::ObserverMode::chase);
    CHECK(state->target == 0);
}

TEST_CASE("read_observer: an out-of-range mode reads as none")
{
    ObserverWorld world;
    world.watch(static_cast<game::ObserverMode>(9), world.target_handle);
    const auto state = world.read();
    REQUIRE(state.has_value());
    CHECK(state->mode == game::ObserverMode::none);
}

TEST_CASE("read_observer: no observer pawn, or no services")
{
    ObserverWorld world;
    world.watch(game::ObserverMode::in_eye, world.target_handle);
    world.memory.put<std::uintptr_t>(kObserverPawn + schema::C_BasePlayerPawn::m_pObserverServices, 0);
    CHECK_FALSE(world.read().has_value());

    world.memory.put<std::uint32_t>(kController + schema::CCSPlayerController::m_hObserverPawn, 0xFFFFFFFF);
    CHECK_FALSE(world.read().has_value());
}

TEST_CASE("watches_target: only first and third person")
{
    CHECK(game::watches_target(game::ObserverMode::in_eye));
    CHECK(game::watches_target(game::ObserverMode::chase));
    CHECK_FALSE(game::watches_target(game::ObserverMode::none));
    CHECK_FALSE(game::watches_target(game::ObserverMode::fixed));
    CHECK_FALSE(game::watches_target(game::ObserverMode::roaming));
}
