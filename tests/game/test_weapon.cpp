#include <cstdint>

#include <doctest.h>

#include "game/schema.h"
#include "game/weapon.h"
#include "helpers/fake_entities.h"
#include "helpers/fake_memory.h"

namespace
{
namespace schema = game::schema;
using test::FakeEntityList;

constexpr std::uintptr_t kPawn = 0x4DF564BF800;
constexpr std::uintptr_t kServices = 0x4DF57000000;
constexpr std::uintptr_t kWeapon = 0x4DF58000000;
constexpr std::uintptr_t kItemDefinitionIndex = schema::C_EconEntity::m_AttributeManager +
                                                schema::C_AttributeContainer::m_Item +
                                                schema::C_EconItemView::m_iItemDefinitionIndex;

void map_pawn_with_weapon(test::FakeMemory& memory, FakeEntityList& list, std::uint16_t id)
{
    memory.map(kPawn, 0x3800);
    memory.map(kServices, 0x100);
    memory.map(kWeapon, 0x1500);
    memory.put<std::uintptr_t>(kPawn + schema::C_BasePlayerPawn::m_pWeaponServices, kServices);
    memory.put<std::uint32_t>(kServices + schema::CPlayer_WeaponServices::m_hActiveWeapon,
                              list.add(742, kWeapon, 0x363, "weapon_ak47"));
    memory.put<std::uint16_t>(kWeapon + kItemDefinitionIndex, id);
}
} // namespace

TEST_CASE("weapon_info: names and classes by item definition index")
{
    CHECK(game::weapon_info(7).name == "AK-47"); // read live in build 14189
    CHECK(game::weapon_info(7).weapon_class == game::WeaponClass::rifle);
    CHECK(game::weapon_info(4).name == "Glock-18");
    CHECK(game::weapon_info(32).name == "P2000");
    CHECK(game::weapon_info(61).name == "USP-S");
    CHECK(game::weapon_info(9).weapon_class == game::WeaponClass::sniper);
    CHECK(game::weapon_info(40).weapon_class == game::WeaponClass::sniper);
    CHECK(game::weapon_info(14).weapon_class == game::WeaponClass::heavy);
    CHECK(game::weapon_info(35).weapon_class == game::WeaponClass::shotgun);
    CHECK(game::weapon_info(34).weapon_class == game::WeaponClass::smg);
    CHECK(game::weapon_info(43).weapon_class == game::WeaponClass::grenade);
    CHECK(game::weapon_info(49).weapon_class == game::WeaponClass::other);
    CHECK(game::weapon_info(507).weapon_class == game::WeaponClass::knife);
    CHECK(game::weapon_info(526).name == "Kukri Knife");

    CHECK(game::weapon_info(0).name.empty());
    CHECK(game::weapon_info(0).weapon_class == game::WeaponClass::unknown);
    CHECK(game::weapon_info(5).name.empty());
    CHECK(game::weapon_info(65535).name.empty());
}

TEST_CASE("weapon_class_name")
{
    CHECK(game::weapon_class_name(game::WeaponClass::rifle) == "rifle");
    CHECK(game::weapon_class_name(game::WeaponClass::smg) == "SMG");
    CHECK(game::weapon_class_name(game::WeaponClass::unknown) == "unknown");
}

TEST_CASE("read_active_weapon_id follows pawn -> weapon services -> handle -> item definition index")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    map_pawn_with_weapon(memory, list, 7);
    CHECK(kItemDefinitionIndex == 0x149A);
    CHECK(game::read_active_weapon_id(memory, FakeEntityList::kSystem, kPawn) == std::uint16_t{7});

    SUBCASE("no active weapon")
    {
        memory.put<std::uint32_t>(kServices + schema::CPlayer_WeaponServices::m_hActiveWeapon, 0xFFFFFFFF);
        CHECK_FALSE(game::read_active_weapon_id(memory, FakeEntityList::kSystem, kPawn).has_value());
    }
    SUBCASE("no weapon services")
    {
        memory.put<std::uintptr_t>(kPawn + schema::C_BasePlayerPawn::m_pWeaponServices, 0);
        CHECK_FALSE(game::read_active_weapon_id(memory, FakeEntityList::kSystem, kPawn).has_value());
    }
    SUBCASE("the weapon was dropped and its slot reused")
    {
        list.add(742, kWeapon, 0x364);
        CHECK_FALSE(game::read_active_weapon_id(memory, FakeEntityList::kSystem, kPawn).has_value());
    }
}
