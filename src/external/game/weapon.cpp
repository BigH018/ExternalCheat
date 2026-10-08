#include "game/weapon.h"

#include <algorithm>
#include <array>

#include "game/handle.h"
#include "game/schema.h"

namespace game
{
namespace
{
struct WeaponEntry
{
    std::uint16_t id;
    WeaponInfo info;
};

using enum WeaponClass;

// CS2 item definition indices (items_game). Sorted by id. 2026-10-06, build 14189: AK-47 = 7, Glock-18 = 4,
// P2000 = 32 read live from bots' and the local player's weapons.
constexpr std::array kWeapons{
    WeaponEntry{1, {"Desert Eagle", pistol}},
    WeaponEntry{2, {"Dual Berettas", pistol}},
    WeaponEntry{3, {"Five-SeveN", pistol}},
    WeaponEntry{4, {"Glock-18", pistol}},
    WeaponEntry{7, {"AK-47", rifle}},
    WeaponEntry{8, {"AUG", rifle}},
    WeaponEntry{9, {"AWP", sniper}},
    WeaponEntry{10, {"FAMAS", rifle}},
    WeaponEntry{11, {"G3SG1", sniper}},
    WeaponEntry{13, {"Galil AR", rifle}},
    WeaponEntry{14, {"M249", heavy}},
    WeaponEntry{16, {"M4A4", rifle}},
    WeaponEntry{17, {"MAC-10", smg}},
    WeaponEntry{19, {"P90", smg}},
    WeaponEntry{23, {"MP5-SD", smg}},
    WeaponEntry{24, {"UMP-45", smg}},
    WeaponEntry{25, {"XM1014", shotgun}},
    WeaponEntry{26, {"PP-Bizon", smg}},
    WeaponEntry{27, {"MAG-7", shotgun}},
    WeaponEntry{28, {"Negev", heavy}},
    WeaponEntry{29, {"Sawed-Off", shotgun}},
    WeaponEntry{30, {"Tec-9", pistol}},
    WeaponEntry{31, {"Zeus x27", other}},
    WeaponEntry{32, {"P2000", pistol}},
    WeaponEntry{33, {"MP7", smg}},
    WeaponEntry{34, {"MP9", smg}},
    WeaponEntry{35, {"Nova", shotgun}},
    WeaponEntry{36, {"P250", pistol}},
    WeaponEntry{38, {"SCAR-20", sniper}},
    WeaponEntry{39, {"SG 553", rifle}},
    WeaponEntry{40, {"SSG 08", sniper}},
    WeaponEntry{41, {"Knife", knife}},
    WeaponEntry{42, {"Knife", knife}},
    WeaponEntry{43, {"Flashbang", grenade}},
    WeaponEntry{44, {"HE Grenade", grenade}},
    WeaponEntry{45, {"Smoke Grenade", grenade}},
    WeaponEntry{46, {"Molotov", grenade}},
    WeaponEntry{47, {"Decoy Grenade", grenade}},
    WeaponEntry{48, {"Incendiary Grenade", grenade}},
    WeaponEntry{49, {"C4", other}},
    WeaponEntry{57, {"Medi-Shot", other}},
    WeaponEntry{59, {"Knife", knife}},
    WeaponEntry{60, {"M4A1-S", rifle}},
    WeaponEntry{61, {"USP-S", pistol}},
    WeaponEntry{63, {"CZ75-Auto", pistol}},
    WeaponEntry{64, {"R8 Revolver", pistol}},
    WeaponEntry{500, {"Bayonet", knife}},
    WeaponEntry{503, {"Classic Knife", knife}},
    WeaponEntry{505, {"Flip Knife", knife}},
    WeaponEntry{506, {"Gut Knife", knife}},
    WeaponEntry{507, {"Karambit", knife}},
    WeaponEntry{508, {"M9 Bayonet", knife}},
    WeaponEntry{509, {"Huntsman Knife", knife}},
    WeaponEntry{512, {"Falchion Knife", knife}},
    WeaponEntry{514, {"Bowie Knife", knife}},
    WeaponEntry{515, {"Butterfly Knife", knife}},
    WeaponEntry{516, {"Shadow Daggers", knife}},
    WeaponEntry{517, {"Paracord Knife", knife}},
    WeaponEntry{518, {"Survival Knife", knife}},
    WeaponEntry{519, {"Ursus Knife", knife}},
    WeaponEntry{520, {"Navaja Knife", knife}},
    WeaponEntry{521, {"Nomad Knife", knife}},
    WeaponEntry{522, {"Stiletto Knife", knife}},
    WeaponEntry{523, {"Talon Knife", knife}},
    WeaponEntry{525, {"Skeleton Knife", knife}},
    WeaponEntry{526, {"Kukri Knife", knife}},
};

static_assert(std::ranges::is_sorted(kWeapons, {}, &WeaponEntry::id), "kWeapons must stay sorted by id");
} // namespace

WeaponInfo weapon_info(std::uint16_t id) noexcept
{
    const auto it = std::ranges::lower_bound(kWeapons, id, {}, &WeaponEntry::id);
    return it != kWeapons.end() && it->id == id ? it->info : WeaponInfo{};
}

std::string_view weapon_class_name(WeaponClass weapon_class) noexcept
{
    switch (weapon_class)
    {
    case pistol: return "pistol";
    case smg: return "SMG";
    case rifle: return "rifle";
    case sniper: return "sniper";
    case shotgun: return "shotgun";
    case heavy: return "heavy";
    case knife: return "knife";
    case grenade: return "grenade";
    case other: return "other";
    case unknown: break;
    }
    return "unknown";
}

std::optional<std::uint16_t> read_active_weapon_id(const core::Memory& memory, std::uintptr_t entity_system,
                                                   std::uintptr_t pawn) noexcept
{
    const auto services = memory.read<std::uintptr_t>(pawn + schema::C_BasePlayerPawn::m_pWeaponServices);
    if (!services || !core::is_plausible_pointer(*services, alignof(std::uintptr_t)))
    {
        return std::nullopt;
    }
    const auto handle = memory.read<std::uint32_t>(*services + schema::CPlayer_WeaponServices::m_hActiveWeapon);
    if (!handle)
    {
        return std::nullopt;
    }
    const std::uintptr_t weapon = resolve_handle(memory, entity_system, *handle);
    if (weapon == 0)
    {
        return std::nullopt;
    }
    return memory.read<std::uint16_t>(weapon + schema::C_EconEntity::m_AttributeManager +
                                      schema::C_AttributeContainer::m_Item +
                                      schema::C_EconItemView::m_iItemDefinitionIndex);
}
} // namespace game
