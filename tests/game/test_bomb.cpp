#include <cstdint>
#include <limits>

#include <doctest.h>

#include "config.h"
#include "game/bomb.h"
#include "game/offsets.h"
#include "game/schema.h"
#include "helpers/fake_entities.h"
#include "helpers/fake_memory.h"

namespace
{
using test::FakeEntityList;
namespace schema = game::schema;
namespace c4 = schema::C_PlantedC4;

constexpr std::uintptr_t kClient = 0x7FFD60000000;
constexpr std::uintptr_t kRules = 0x4DFA98D8000;
constexpr std::uintptr_t kBomb = 0x4DF70000000;
constexpr std::uintptr_t kBombNode = 0x4DF70100000;
constexpr std::uintptr_t kDefuser = 0x4DF71000000;
constexpr std::uint32_t kBombIndex = 1500;
constexpr std::uint32_t kDefuserIndex = 101;

// A bot match on a defuse map: the client globals, the game rules, and (optionally) a ticking bomb on B with 31 s of
// 40 left at curtime 1000, being defused by the pawn at index 101. dwPlantedC4 points at the bomb while it's planted.
struct BombWorld
{
    test::FakeMemory memory;
    FakeEntityList list{memory};

    BombWorld()
    {
        memory.map_value(kClient + game::offsets::client::dwEntityList, FakeEntityList::kSystem);
        memory.map_value(kClient + game::offsets::client::dwGameRules, kRules);
        memory.map_value(kClient + game::offsets::client::dwPlantedC4, std::uintptr_t{0});
        memory.map(kRules, 0x1000);
    }

    void set_planted(bool planted)
    {
        memory.put<std::uint8_t>(kRules + schema::C_CSGameRules::m_bBombPlanted, planted ? 1 : 0);
    }

    void plant(std::uint32_t serial = 0x56)
    {
        set_planted(true);
        memory.map(kBomb, 0x2000);
        memory.map(kBombNode, 0x200);
        list.add(kBombIndex, kBomb, serial); // no designer name, as in the game
        memory.put<std::uintptr_t>(kBomb + schema::CEntityInstance::m_pEntity, list.identity(kBombIndex));
        memory.put<std::uintptr_t>(kClient + game::offsets::client::dwPlantedC4, kBomb);
        memory.put<std::uintptr_t>(kBomb + schema::C_BaseEntity::m_pGameSceneNode, kBombNode);
        memory.put<maths::Vec3>(kBombNode + schema::CGameSceneNode::m_vecAbsOrigin, {-1880.5f, 260.0f, -160.0f});
        memory.put<std::uint8_t>(kBomb + c4::m_bBombTicking, 1);
        memory.put<std::int32_t>(kBomb + c4::m_nBombSite, 1);
        memory.put<float>(kBomb + c4::m_flC4Blow, 1031.0f);
        memory.put<float>(kBomb + c4::m_flTimerLength, 40.0f);
        memory.put<std::uint8_t>(kBomb + c4::m_bBeingDefused, 1);
        memory.put<float>(kBomb + c4::m_flDefuseCountDown, 1004.5f);
        memory.put<float>(kBomb + c4::m_flDefuseLength, 5.0f);
        const std::uint32_t defuser = list.add(kDefuserIndex, kDefuser, 0x22, "c_cs_player_for_precache");
        memory.put<std::uint32_t>(kBomb + c4::m_hBombDefuser, defuser);
    }
};
} // namespace

TEST_CASE("read_bomb_planted: the game rules' flag; nullopt without rules")
{
    BombWorld world;
    world.set_planted(false);
    CHECK(game::read_bomb_planted(world.memory, kClient) == false);
    world.set_planted(true);
    CHECK(game::read_bomb_planted(world.memory, kClient) == true);

    test::FakeMemory empty;
    empty.map_value(kClient + game::offsets::client::dwGameRules, std::uintptr_t{0});
    CHECK_FALSE(game::read_bomb_planted(empty, kClient).has_value());
}

TEST_CASE("read_planted_bomb: every field, the defuser's handle resolved to its pawn")
{
    BombWorld world;
    world.plant();
    const auto bomb = game::read_planted_bomb(world.memory, FakeEntityList::kSystem, kBomb);
    REQUIRE(bomb.has_value());
    CHECK(bomb->entity == kBomb);
    CHECK(bomb->site == 1);
    CHECK(bomb->position == maths::Vec3{-1880.5f, 260.0f, -160.0f});
    CHECK(bomb->ticking);
    CHECK_FALSE(bomb->exploded);
    CHECK_FALSE(bomb->defused);
    CHECK(bomb->being_defused);
    CHECK(bomb->blow_time == 1031.0f);
    CHECK(bomb->timer_length == 40.0f);
    CHECK(bomb->defuse_end == 1004.5f);
    CHECK(bomb->defuse_length == 5.0f);
    CHECK(bomb->defuser_pawn == kDefuser);
}

TEST_CASE("read_planted_bomb: garbage is rejected")
{
    BombWorld world;
    world.plant();
    world.memory.put<float>(kBomb + c4::m_flTimerLength, config::kMaxBombTimer + 1.0f);
    CHECK_FALSE(game::read_planted_bomb(world.memory, FakeEntityList::kSystem, kBomb).has_value());

    world.memory.put<float>(kBomb + c4::m_flTimerLength, 40.0f);
    world.memory.put<float>(kBomb + c4::m_flC4Blow, std::numeric_limits<float>::quiet_NaN());
    CHECK_FALSE(game::read_planted_bomb(world.memory, FakeEntityList::kSystem, kBomb).has_value());

    world.memory.put<float>(kBomb + c4::m_flC4Blow, 1031.0f);
    world.memory.put<std::uintptr_t>(kBomb + schema::C_BaseEntity::m_pGameSceneNode, 0);
    CHECK_FALSE(game::read_planted_bomb(world.memory, FakeEntityList::kSystem, kBomb).has_value());
}

TEST_CASE("read_planted_bomb: nobody defusing leaves the defuser at 0")
{
    BombWorld world;
    world.plant();
    world.memory.put<std::uint8_t>(kBomb + c4::m_bBeingDefused, 0);
    world.memory.put<std::uint32_t>(kBomb + c4::m_hBombDefuser, game::offsets::layout::kInvalidHandle);
    const auto bomb = game::read_planted_bomb(world.memory, FakeEntityList::kSystem, kBomb);
    REQUIRE(bomb.has_value());
    CHECK_FALSE(bomb->being_defused);
    CHECK(bomb->defuser_pawn == 0);
}

TEST_CASE("read_bomb: none while dwPlantedC4 is 0")
{
    BombWorld world;
    world.set_planted(false);
    CHECK_FALSE(game::read_bomb(world.memory, kClient, FakeEntityList::kSystem).has_value());
}

TEST_CASE("read_bomb: dwPlantedC4 points at the bomb, whose handle leads back to it")
{
    BombWorld world;
    world.plant();
    const auto bomb = game::read_bomb(world.memory, kClient, FakeEntityList::kSystem);
    REQUIRE(bomb.has_value());
    CHECK(bomb->entity == kBomb);
    CHECK(bomb->site == 1);
    CHECK(bomb->blow_time == 1031.0f);
}

TEST_CASE("read_bomb: a stale pointer (its slot now holds another entity, or no identity) is no bomb")
{
    BombWorld world;
    world.plant(0x56);
    world.list.add(kBombIndex, 0x4DF7F000000, 0x57); // the slot was reused
    CHECK_FALSE(game::read_bomb(world.memory, kClient, FakeEntityList::kSystem).has_value());

    world.plant(0x58);
    world.memory.put<std::uintptr_t>(kBomb + schema::CEntityInstance::m_pEntity, 0);
    CHECK_FALSE(game::read_bomb(world.memory, kClient, FakeEntityList::kSystem).has_value());
}
