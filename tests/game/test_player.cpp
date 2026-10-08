#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

#include <doctest.h>

#include "game/offsets.h"
#include "game/player.h"
#include "game/schema.h"
#include "helpers/fake_entities.h"
#include "helpers/fake_memory.h"
#include "maths/angles.h"
#include "maths/projection.h"
#include "maths/vec.h"

namespace
{
namespace schema = game::schema;
namespace client = game::offsets::client;
using test::FakeEntityList;

constexpr std::uintptr_t kClientBase = 0x7FFD62930000;
constexpr std::uintptr_t kLocalPawnSlot = kClientBase + client::dwLocalPlayerPawn;
constexpr std::uintptr_t kGlobals = 0x4DF87480090;
constexpr std::uintptr_t kMapName = 0x4DF87490000;

// Every object of player n lives in its own 1 MiB block.
constexpr std::uintptr_t kPlayers = 0x4DF00000000;
constexpr std::uintptr_t kController = 0x0;
constexpr std::uintptr_t kPawn = 0x10000;
constexpr std::uintptr_t kSceneNode = 0x20000;
constexpr std::uintptr_t kWeaponServices = 0x30000;
constexpr std::uintptr_t kWeapon = 0x40000;

std::uintptr_t object(std::uint32_t player, std::uintptr_t which)
{
    return kPlayers + player * 0x100000 + which;
}

struct FakePlayer
{
    std::uint32_t index = 1;
    std::string_view name = "Kev";
    std::uint8_t team = 3;
    bool alive = true;
    std::int32_t health = 100;
    std::int32_t armor = 50;
    maths::Vec3 origin{278.5f, -874.9f, -163.2f};
    maths::Vec3 view_offset{0.0f, 0.0f, 64.0f};
    maths::Angles eye_angles{12.5f, -135.0f};
    std::uint32_t flags = game::kFlagOnGround;
    bool scoped = false;
    std::uint16_t weapon_id = 7;
    bool has_pawn = true;
};

// Maps a controller, its pawn, scene node, weapon services and weapon, and registers them in the entity list.
// Entity indices: controller = index, pawn = 100 + index, weapon = 200 + index.
void add_player(test::FakeMemory& memory, FakeEntityList& list, const FakePlayer& player)
{
    const std::uintptr_t controller = object(player.index, kController);
    memory.map(controller, 0x1000);
    list.add(player.index, controller, 0x100 + player.index, "cs_player_controller");
    memory.put_string(controller + schema::CBasePlayerController::m_iszPlayerName, player.name);
    memory.put<std::uint8_t>(controller + schema::C_BaseEntity::m_iTeamNum, player.team);
    memory.put<std::uint8_t>(controller + schema::CCSPlayerController::m_bPawnIsAlive, player.alive ? 1 : 0);
    if (!player.has_pawn)
    {
        memory.put<std::uint32_t>(controller + schema::CCSPlayerController::m_hPlayerPawn, 0xFFFFFFFF);
        return;
    }

    const std::uintptr_t pawn = object(player.index, kPawn);
    const std::uintptr_t node = object(player.index, kSceneNode);
    const std::uintptr_t services = object(player.index, kWeaponServices);
    const std::uintptr_t weapon = object(player.index, kWeapon);
    memory.map(pawn, 0x3800);
    memory.map(node, 0x200);
    memory.map(services, 0x100);
    memory.map(weapon, 0x1500);

    const std::uint32_t pawn_handle = list.add(100 + player.index, pawn, 0x200 + player.index);
    memory.put<std::uint32_t>(controller + schema::CCSPlayerController::m_hPlayerPawn, pawn_handle);
    memory.put<std::int32_t>(pawn + schema::C_BaseEntity::m_iHealth, player.health);
    memory.put<std::uint8_t>(pawn + schema::C_BaseEntity::m_lifeState, player.alive ? 0 : 2);
    memory.put<std::uintptr_t>(pawn + schema::C_BaseEntity::m_pGameSceneNode, node);
    memory.put<std::uint32_t>(pawn + schema::C_BaseEntity::m_fFlags, player.flags);
    memory.put<maths::Vec3>(pawn + schema::C_BaseModelEntity::m_vecViewOffset, player.view_offset);
    memory.put<std::int32_t>(pawn + schema::C_CSPlayerPawn::m_ArmorValue, player.armor);
    memory.put<maths::Angles>(pawn + schema::C_CSPlayerPawn::m_angEyeAngles, player.eye_angles);
    memory.put<std::uint8_t>(pawn + schema::C_CSPlayerPawn::m_bIsScoped, player.scoped ? 1 : 0);
    memory.put<maths::Vec3>(node + schema::CGameSceneNode::m_vecAbsOrigin, player.origin);
    memory.put<std::uintptr_t>(pawn + schema::C_BasePlayerPawn::m_pWeaponServices, services);
    memory.put<std::uint32_t>(services + schema::CPlayer_WeaponServices::m_hActiveWeapon,
                              list.add(200 + player.index, weapon, 0x300 + player.index));
    memory.put<std::uint16_t>(weapon + 0x149A, player.weapon_id); // the chain proven live in build 14189
}

game::EntityRef controller_ref(std::uint32_t index)
{
    return game::EntityRef{index, object(index, kController), FakeEntityList::make_handle(index, 0x100 + index)};
}

// client.dll's globals the snapshot reads: entity list, globals pointer, local controller, view matrix.
void map_client(test::FakeMemory& memory, std::uintptr_t local_controller, bool sane_view = true)
{
    namespace layout = game::offsets::layout;
    memory.map_value<std::uintptr_t>(kClientBase + client::dwEntityList, FakeEntityList::kSystem);
    memory.map_value<std::uintptr_t>(kClientBase + client::dwGlobalVars, kGlobals);
    memory.map_value<std::uintptr_t>(kClientBase + client::dwLocalPlayerController, local_controller);
    maths::ViewMatrix view;
    if (sane_view)
    {
        view.m = {-0.45f, -0.6f, 0.0f, -348.4f, 0.11f, -0.09f, 1.33f, 238.3f,
                  0.79f,  -0.6f, -0.11f, 1815.4f, 0.79f, -0.6f, -0.11f, 1821.5f};
    }
    memory.map_value(kClientBase + client::dwViewMatrix, view);
    memory.map(kGlobals, layout::kGlobalsReadSize);
    memory.put<std::int32_t>(kGlobals + layout::kGlobalsMaxClients, 64);
    memory.put<float>(kGlobals + layout::kGlobalsIntervalPerTick, 1.0f / 64.0f);
    memory.put<float>(kGlobals + layout::kGlobalsCurtime, 6242.4f);
    memory.put<std::uintptr_t>(kGlobals + layout::kGlobalsMapName, kMapName);
    memory.map(kMapName, 0x1000);
    memory.put_string(kMapName, "de_mirage");
}
} // namespace

TEST_CASE("read_local_pawn reads client.dll + dwLocalPlayerPawn")
{
    test::FakeMemory memory;
    memory.map_value<std::uintptr_t>(kLocalPawnSlot, 0x4DF564BF800);
    CHECK(game::read_local_pawn(memory, kClientBase) == 0x4DF564BF800);
}

TEST_CASE("read_local_pawn: 0 in the main menu, nullopt when the read fails")
{
    test::FakeMemory memory;
    memory.map_value<std::uintptr_t>(kLocalPawnSlot, 0);
    CHECK(game::read_local_pawn(memory, kClientBase) == std::uintptr_t{0});

    const test::FakeMemory empty;
    CHECK_FALSE(game::read_local_pawn(empty, kClientBase).has_value());
}

TEST_CASE("read_player: controller + pawn")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    add_player(memory, list, FakePlayer{.index = 2, .scoped = true});

    const auto player = game::read_player(memory, FakeEntityList::kSystem, controller_ref(2), 0);
    REQUIRE(player.has_value());
    CHECK(player->index == 2);
    CHECK(player->controller == object(2, kController));
    CHECK(player->pawn == object(2, kPawn));
    CHECK_FALSE(player->is_local);
    CHECK(player->name == "Kev");
    CHECK(player->team == game::Team::counter_terrorist);
    CHECK(player->alive);
    CHECK(player->health == 100);
    CHECK(player->armor == 50);
    CHECK(player->origin == maths::Vec3{278.5f, -874.9f, -163.2f});
    CHECK(player->eye_position() == maths::Vec3{278.5f, -874.9f, -163.2f + 64.0f});
    CHECK(player->eye_angles == maths::Angles{12.5f, -135.0f});
    CHECK(player->on_ground());
    CHECK(player->scoped);
    CHECK_FALSE(player->dormant);
    CHECK(player->weapon_id == std::uint16_t{7});
}

TEST_CASE("read_player: the local player is the one whose controller matches")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    add_player(memory, list, FakePlayer{.index = 1, .name = "bigh18valorant", .team = 2});
    const auto player = game::read_player(memory, FakeEntityList::kSystem, controller_ref(1), object(1, kController));
    REQUIRE(player.has_value());
    CHECK(player->is_local);
    CHECK(player->team == game::Team::terrorist);
}

TEST_CASE("read_player: dead, in the air, no pawn")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    add_player(memory, list, FakePlayer{.index = 3, .alive = false, .health = 0, .flags = 0});
    add_player(memory, list, FakePlayer{.index = 4, .has_pawn = false});

    const auto dead = game::read_player(memory, FakeEntityList::kSystem, controller_ref(3), 0);
    REQUIRE(dead.has_value());
    CHECK(dead->pawn != 0);
    CHECK_FALSE(dead->alive);
    CHECK_FALSE(dead->on_ground());

    const auto no_pawn = game::read_player(memory, FakeEntityList::kSystem, controller_ref(4), 0);
    REQUIRE(no_pawn.has_value());
    CHECK(no_pawn->name == "Kev");
    CHECK(no_pawn->pawn == 0);
    CHECK_FALSE(no_pawn->alive);
    CHECK_FALSE(no_pawn->weapon_id.has_value());
}

TEST_CASE("read_player: a dead player's observer camera is read, a living player's left-over one isn't")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    add_player(memory, list, FakePlayer{.index = 3, .alive = false, .health = 0});
    add_player(memory, list, FakePlayer{.index = 4});
    const std::uint32_t watched = FakeEntityList::make_handle(104, 0x204); // player 4's pawn
    for (const std::uint32_t index : {3u, 4u})
    {
        // Every controller has an observer pawn with services, alive or not (as in the game).
        const std::uintptr_t observer_pawn = object(index, 0x50000);
        const std::uintptr_t services = object(index, 0x60000);
        memory.map(observer_pawn, 0x2000);
        memory.map(services, 0x100);
        memory.put<std::uint32_t>(object(index, kController) + schema::CCSPlayerController::m_hObserverPawn,
                                  list.add(150 + index, observer_pawn, 0x400 + index));
        memory.put<std::uintptr_t>(observer_pawn + schema::C_BasePlayerPawn::m_pObserverServices, services);
        memory.put<std::uint8_t>(services + schema::CPlayer_ObserverServices::m_iObserverMode, 2);
        memory.put<std::uint32_t>(services + schema::CPlayer_ObserverServices::m_hObserverTarget, watched);
    }

    const auto dead = game::read_player(memory, FakeEntityList::kSystem, controller_ref(3), 0);
    REQUIRE(dead.has_value());
    CHECK(dead->observer_mode == game::ObserverMode::in_eye);
    CHECK(dead->observer_target == object(4, kPawn));

    const auto alive = game::read_player(memory, FakeEntityList::kSystem, controller_ref(4), 0);
    REQUIRE(alive.has_value());
    CHECK(alive->observer_mode == game::ObserverMode::none);
    CHECK(alive->observer_target == 0);
}

TEST_CASE("read_player: health 100 but the controller says dead is not alive")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    add_player(memory, list, FakePlayer{.index = 5});
    memory.put<std::uint8_t>(object(5, kController) + schema::CCSPlayerController::m_bPawnIsAlive, 0);
    const auto player = game::read_player(memory, FakeEntityList::kSystem, controller_ref(5), 0);
    REQUIRE(player.has_value());
    CHECK_FALSE(player->alive);
}

TEST_CASE("read_player: a garbage pawn is dropped, the controller part stays")
{
    SUBCASE("health out of range")
    {
        test::FakeMemory memory;
        FakeEntityList list(memory);
        add_player(memory, list, FakePlayer{.index = 6, .health = 123456789});
        const auto player = game::read_player(memory, FakeEntityList::kSystem, controller_ref(6), 0);
        REQUIRE(player.has_value());
        CHECK(player->pawn == 0);
        CHECK(player->health == 0);
        CHECK_FALSE(player->alive);
        CHECK(player->name == "Kev");
    }
    SUBCASE("non-finite position")
    {
        test::FakeMemory memory;
        FakeEntityList list(memory);
        const float nan = std::numeric_limits<float>::quiet_NaN();
        add_player(memory, list, FakePlayer{.index = 7, .origin = {nan, 0.0f, 0.0f}});
        const auto player = game::read_player(memory, FakeEntityList::kSystem, controller_ref(7), 0);
        REQUIRE(player.has_value());
        CHECK(player->pawn == 0);
        CHECK_FALSE(player->alive);
    }
    SUBCASE("stale pawn handle")
    {
        test::FakeMemory memory;
        FakeEntityList list(memory);
        add_player(memory, list, FakePlayer{.index = 8});
        list.add(108, object(8, kPawn), 0x999); // the pawn's slot now holds a newer serial
        const auto player = game::read_player(memory, FakeEntityList::kSystem, controller_ref(8), 0);
        REQUIRE(player.has_value());
        CHECK(player->pawn == 0);
    }
}

TEST_CASE("read_player: an unreadable controller gives nullopt")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    CHECK_FALSE(game::read_player(memory, FakeEntityList::kSystem, controller_ref(9), 0).has_value());
}

TEST_CASE("read_game: a bot match")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    map_client(memory, object(1, kController));
    add_player(memory, list, FakePlayer{.index = 1, .name = "bigh18valorant", .team = 2});
    add_player(memory, list, FakePlayer{.index = 2, .name = "Kev"});
    add_player(memory, list, FakePlayer{.index = 3, .name = "Blackwolf", .team = 2, .alive = false, .health = 0});

    const game::GameSnapshot snapshot = game::read_game(memory, kClientBase);
    CHECK(snapshot.in_match);
    CHECK(snapshot.globals.max_clients == 64);
    CHECK(snapshot.globals.map_name == "de_mirage");
    REQUIRE(snapshot.view.has_value());
    CHECK(snapshot.view->at(3, 3) == doctest::Approx(1821.5f));
    REQUIRE(snapshot.players.size() == 3);
    CHECK(snapshot.players[1].name == "Kev");
    CHECK_FALSE(snapshot.players[2].alive);
    REQUIRE(snapshot.local() != nullptr);
    CHECK(snapshot.local()->name == "bigh18valorant");
}

TEST_CASE("read_game: a zeroed view matrix isn't passed on")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    map_client(memory, object(1, kController), false);
    add_player(memory, list, FakePlayer{.index = 1});
    const game::GameSnapshot snapshot = game::read_game(memory, kClientBase);
    CHECK(snapshot.in_match);
    CHECK_FALSE(snapshot.view.has_value());
}

TEST_CASE("read_game: main menu and no game")
{
    SUBCASE("no local controller")
    {
        test::FakeMemory memory;
        FakeEntityList list(memory);
        map_client(memory, 0);
        const game::GameSnapshot snapshot = game::read_game(memory, kClientBase);
        CHECK_FALSE(snapshot.in_match);
        CHECK(snapshot.players.empty());
        CHECK(snapshot.local() == nullptr);
    }
    SUBCASE("nothing readable")
    {
        const test::FakeMemory memory;
        const game::GameSnapshot snapshot = game::read_game(memory, kClientBase);
        CHECK_FALSE(snapshot.in_match);
        CHECK(snapshot.players.empty());
    }
}
