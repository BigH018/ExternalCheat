#pragma once

// The active weapon: which one a pawn holds, its display name and its class.
//
// The id is the weapon's item definition index (C_EconItemView::m_iItemDefinitionIndex), not its designer name:
// USP-S and P2000, or M4A1-S and M4A4, can share a designer name but never an id.
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <optional>
#include <string_view>

#include "core/memory.h"

namespace game
{
// What the triggerbot's weapon filter (Phase 5) sorts by.
enum class WeaponClass : std::uint8_t
{
    unknown,
    pistol,
    smg,
    rifle,
    sniper,
    shotgun,
    heavy, // machine guns
    knife,
    grenade,
    other, // C4, Zeus, Medi-Shot
};

struct WeaponInfo
{
    std::string_view name; // "AK-47"
    WeaponClass weapon_class = WeaponClass::unknown;
};

// The weapon with item definition index `id`. Unknown ids give {"", unknown}.
[[nodiscard]] WeaponInfo weapon_info(std::uint16_t id) noexcept;

// "pistol", "rifle", ...
[[nodiscard]] std::string_view weapon_class_name(WeaponClass weapon_class) noexcept;

// The item definition index of the weapon `pawn` holds: pawn -> m_pWeaponServices -> m_hActiveWeapon (a handle) ->
// weapon + m_AttributeManager + m_Item + m_iItemDefinitionIndex. nullopt if the pawn holds nothing or a read fails.
[[nodiscard]] std::optional<std::uint16_t> read_active_weapon_id(const core::Memory& memory,
                                                                 std::uintptr_t entity_system,
                                                                 std::uintptr_t pawn) noexcept;
} // namespace game
