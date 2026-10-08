#include <cstdint>

#include <doctest.h>

#include "game/entities.h"
#include "helpers/fake_entities.h"
#include "helpers/fake_memory.h"

namespace
{
using test::FakeEntityList;

constexpr std::uintptr_t kController = 0x4DF5A000000; // controller n at kController + n * 0x1000
constexpr char kControllerName[] = "cs_player_controller";

std::uintptr_t controller(std::uint32_t index)
{
    return kController + index * 0x1000;
}
} // namespace

TEST_CASE("designer_name reads the identity's name")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    list.add(0, 0x4DF5CD0A800, 1, "worldent");
    list.add(742, 0x4DF60000000, 0x363, "weapon_ak47");
    list.add(5, 0x4DF61000000, 1); // no name

    CHECK(game::designer_name(memory, FakeEntityList::kSystem, 0) == "worldent");
    CHECK(game::designer_name(memory, FakeEntityList::kSystem, 742) == "weapon_ak47");
    CHECK_FALSE(game::designer_name(memory, FakeEntityList::kSystem, 5).has_value());
    CHECK_FALSE(game::designer_name(memory, FakeEntityList::kSystem, 9000).has_value());
}

TEST_CASE("find_player_controllers: indices 1..max_clients named cs_player_controller")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    list.add(0, 0x4DF5CD0A800, 1, "worldent");
    list.add(1, controller(1), 0x11C, kControllerName);
    list.add(2, controller(2), 0x265, kControllerName);
    // 3: empty slot
    list.add(4, controller(4), 0x10, "cs_player_pawn"); // something else at a player index
    list.add(5, controller(5), 0x22, kControllerName);
    list.add(65, controller(65), 0x1, kControllerName); // past max_clients

    const auto found = game::find_player_controllers(memory, FakeEntityList::kSystem, 64);
    REQUIRE(found.size() == 3);
    CHECK(found[0].index == 1);
    CHECK(found[0].entity == controller(1));
    CHECK(found[0].handle == FakeEntityList::make_handle(1, 0x11C));
    CHECK(found[1].index == 2);
    CHECK(found[2].index == 5);

    const auto first_two = game::find_player_controllers(memory, FakeEntityList::kSystem, 2);
    CHECK(first_two.size() == 2);

    // A garbage max_clients is clamped to 64 rather than walking thousands of indices.
    CHECK(game::find_player_controllers(memory, FakeEntityList::kSystem, 100000).size() == 3);
}

TEST_CASE("find_player_controllers: an empty or missing list")
{
    test::FakeMemory memory;
    FakeEntityList list(memory); // no chunks mapped at all
    CHECK(game::find_player_controllers(memory, FakeEntityList::kSystem, 64).empty());

    const test::FakeMemory nothing;
    CHECK(game::find_player_controllers(nothing, FakeEntityList::kSystem, 64).empty());
}
