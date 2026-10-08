#pragma once

// Schema field offsets: where a field sits inside a game class. One namespace per class, named exactly like the class.
//
// Source for every value: a2x/cs2-dumper output for CS2 build 14189, dumped 2026-10-06T16:04:45Z, copied (by script,
// never retyped) from docs/dumps/client_dll.json. The decimal value in each comment is the dump's own number.
//
// Proof: at startup app/diagnostics reads the same fields from the game's live schema system (game/schema_system)
// and compares them with these values. 2026-10-06, build 14189: all of them matched (and so did all 3013 fields of
// the 469 client classes the live schema system has).
//
// A field belongs to the class that declares it, not to every class that inherits it: m_iHealth is read from a pawn
// as pawn + C_BaseEntity::m_iHealth. Parents (from the dump): C_CSPlayerPawn -> C_CSPlayerPawnBase -> C_BasePlayerPawn
// -> ... -> C_BaseModelEntity -> C_BaseEntity; CCSPlayerController -> CBasePlayerController -> C_BaseEntity.
//
// Names match the dumper output exactly (not kPascalCase), as in offsets.h. Changing a value: CLAUDE.md §7.
//
// PURE: no <Windows.h>.

#include <array>
#include <cstdint>
#include <string_view>

namespace game::schema
{
// Every entity starts with CEntityInstance: m_pEntity points to its identity in the entity list (handle, designer
// name). Phase 6: the bomb timer's stale-pointer check. NOT in kFields: its class info isn't one of client.dll's own
// (another library registers it in the client scope), so the live schema index can't see it. Proven instead by the
// bomb's handle round-trip (2026-10-07: identity = entity-list slot 232, which holds the same bomb), which `--diag`'s
// "Bomb (dwPlantedC4)" check repeats whenever a bomb is planted.
namespace CEntityInstance
{
inline constexpr std::uintptr_t m_pEntity = 0x10; // 16, CEntityIdentity*
} // namespace CEntityInstance

namespace C_BaseEntity
{
inline constexpr std::uintptr_t m_pGameSceneNode = 0x330; // 816, CGameSceneNode*
inline constexpr std::uintptr_t m_iMaxHealth     = 0x348; // 840, int32
inline constexpr std::uintptr_t m_iHealth        = 0x34C; // 844, int32
inline constexpr std::uintptr_t m_lifeState      = 0x354; // 852, uint8 (0 = alive)
inline constexpr std::uintptr_t m_iTeamNum       = 0x3E7; // 999, uint8 (2 = T, 3 = CT)
inline constexpr std::uintptr_t m_fFlags         = 0x3F4; // 1012, uint32 (bit 0 = FL_ONGROUND)
inline constexpr std::uintptr_t m_vecVelocity    = 0x430; // 1072, CNetworkVelocityVector
} // namespace C_BaseEntity

namespace CGameSceneNode
{
inline constexpr std::uintptr_t m_vecAbsOrigin = 0xC8;  // 200, Vector (world position)
inline constexpr std::uintptr_t m_bDormant     = 0x103; // 259, bool
} // namespace CGameSceneNode

// A pawn's scene node is a CSkeletonInstance (child of CGameSceneNode). Its model state leads to the bone array
// (offsets::layout::kModelStateBones, hand-found).
namespace CSkeletonInstance
{
inline constexpr std::uintptr_t m_modelState = 0x140; // 320, CModelState (embedded)
} // namespace CSkeletonInstance

namespace C_BaseModelEntity
{
inline constexpr std::uintptr_t m_vecViewOffset = 0xF60; // 3936, CNetworkViewOffsetVector (eye height)
} // namespace C_BaseModelEntity

namespace C_BasePlayerPawn
{
inline constexpr std::uintptr_t m_pWeaponServices   = 0x12F0; // 4848, CPlayer_WeaponServices*
inline constexpr std::uintptr_t m_pObserverServices = 0x1308; // 4872, CPlayer_ObserverServices* (on the observer pawn)
inline constexpr std::uintptr_t m_hController       = 0x14BC; // 5308, CHandle<CBasePlayerController>
} // namespace C_BasePlayerPawn

namespace CPlayer_WeaponServices
{
inline constexpr std::uintptr_t m_hActiveWeapon = 0x60; // 96, CHandle<C_BasePlayerWeapon>
} // namespace CPlayer_WeaponServices

// Who a player spectates. Phase 6: the spectator list. Read from the controller's observer pawn (m_hObserverPawn): the
// player pawn's own pointer is null on the client (proven live 2026-10-07). Only meaningful while the player is dead:
// living players keep a stale mode and target.
namespace CPlayer_ObserverServices
{
inline constexpr std::uintptr_t m_iObserverMode   = 0x48; // 72, uint8 (ObserverMode_t: 2 = in-eye, 3 = chase)
inline constexpr std::uintptr_t m_hObserverTarget = 0x4C; // 76, CHandle<C_BaseEntity> (a player pawn)
} // namespace CPlayer_ObserverServices

namespace C_CSPlayerPawnBase
{
inline constexpr std::uintptr_t m_flFlashOverlayAlpha = 0x1504; // 5380, float (the white overlay right now)
inline constexpr std::uintptr_t m_flFlashMaxAlpha     = 0x150C; // 5388, float (its peak for this flash)
inline constexpr std::uintptr_t m_flFlashDuration     = 0x1510; // 5392, float
} // namespace C_CSPlayerPawnBase

namespace C_CSPlayerPawn
{
inline constexpr std::uintptr_t m_entitySpottedState = 0x1E88; // 7816, EntitySpottedState_t (embedded)
inline constexpr std::uintptr_t m_bIsScoped          = 0x1EA0; // 7840, bool
inline constexpr std::uintptr_t m_iShotsFired        = 0x1EB4; // 7860, int32
inline constexpr std::uintptr_t m_ArmorValue         = 0x1ECC; // 7884, int32
inline constexpr std::uintptr_t m_angEyeAngles       = 0x35F0; // 13808, QAngle
inline constexpr std::uintptr_t m_iIDEntIndex        = 0x36CC; // 14028, CEntityIndex (entity under the crosshair)
} // namespace C_CSPlayerPawn

namespace EntitySpottedState_t
{
inline constexpr std::uintptr_t m_bSpotted       = 0x8; // 8, bool
inline constexpr std::uintptr_t m_bSpottedByMask = 0xC; // 12, uint32[2] (a bit per player slot)
} // namespace EntitySpottedState_t

namespace CBasePlayerController
{
inline constexpr std::uintptr_t m_hPawn         = 0x6BC; // 1724, CHandle<C_BasePlayerPawn>
inline constexpr std::uintptr_t m_iszPlayerName = 0x6FC; // 1788, char[128]
} // namespace CBasePlayerController

namespace CCSPlayerController
{
inline constexpr std::uintptr_t m_hPlayerPawn   = 0x92C; // 2348, CHandle<C_CSPlayerPawn>
inline constexpr std::uintptr_t m_hObserverPawn = 0x930; // 2352, CHandle<C_CSObserverPawn>
inline constexpr std::uintptr_t m_bPawnIsAlive  = 0x934; // 2356, bool
} // namespace CCSPlayerController

// The active weapon's item definition index: weapon + m_AttributeManager + m_Item + m_iItemDefinitionIndex
// (0x1290 + 0x50 + 0x1BA = 0x149A in build 14189; read live 2026-10-06: 7 for an AK-47). Kept as a chain so each
// step can be re-verified.
namespace C_EconEntity
{
inline constexpr std::uintptr_t m_AttributeManager = 0x1290; // 4752, C_AttributeContainer (embedded)
} // namespace C_EconEntity

namespace C_AttributeContainer
{
inline constexpr std::uintptr_t m_Item = 0x50; // 80, C_EconItemView (embedded)
} // namespace C_AttributeContainer

namespace C_EconItemView
{
inline constexpr std::uintptr_t m_iItemDefinitionIndex = 0x1BA; // 442, uint16
} // namespace C_EconItemView

// The game rules object (client.dll + dwGameRules -> C_CSGameRules*). Phase 6: is a bomb planted (the diagnostic's
// cross-check of dwPlantedC4).
namespace C_CSGameRules
{
inline constexpr std::uintptr_t m_bBombPlanted = 0x8C7; // 2247, bool
} // namespace C_CSGameRules

// The planted bomb (client.dll + dwPlantedC4 -> C_PlantedC4*). Times are game times, like CGlobalVars::curtime.
// Phase 6: the bomb timer.
namespace C_PlantedC4
{
inline constexpr std::uintptr_t m_bBombTicking      = 0x1288; // 4744, bool
inline constexpr std::uintptr_t m_nBombSite         = 0x128C; // 4748, int32 (0 = A, 1 = B)
inline constexpr std::uintptr_t m_flC4Blow          = 0x12B8; // 4792, GameTime_t (when it explodes)
inline constexpr std::uintptr_t m_bHasExploded      = 0x12BD; // 4797, bool
inline constexpr std::uintptr_t m_flTimerLength     = 0x12C0; // 4800, float (seconds, 40 by default)
inline constexpr std::uintptr_t m_bBeingDefused     = 0x12C4; // 4804, bool
inline constexpr std::uintptr_t m_flDefuseLength    = 0x12D4; // 4820, float (10 s, 5 s with a kit)
inline constexpr std::uintptr_t m_flDefuseCountDown = 0x12D8; // 4824, GameTime_t (when the defuse completes)
inline constexpr std::uintptr_t m_bBombDefused      = 0x12DC; // 4828, bool
inline constexpr std::uintptr_t m_hBombDefuser      = 0x12E0; // 4832, CHandle<C_CSPlayerPawn>
} // namespace C_PlantedC4

// Every field above, for the startup diagnostic that compares them with the live schema system.
// Add a line here whenever you add a field above.
struct Field
{
    std::string_view class_name;
    std::string_view field_name;
    std::uintptr_t offset;
};

inline constexpr std::array kFields{
    Field{"C_BaseEntity", "m_pGameSceneNode", C_BaseEntity::m_pGameSceneNode},
    Field{"C_BaseEntity", "m_iMaxHealth", C_BaseEntity::m_iMaxHealth},
    Field{"C_BaseEntity", "m_iHealth", C_BaseEntity::m_iHealth},
    Field{"C_BaseEntity", "m_lifeState", C_BaseEntity::m_lifeState},
    Field{"C_BaseEntity", "m_iTeamNum", C_BaseEntity::m_iTeamNum},
    Field{"C_BaseEntity", "m_fFlags", C_BaseEntity::m_fFlags},
    Field{"C_BaseEntity", "m_vecVelocity", C_BaseEntity::m_vecVelocity},
    Field{"CGameSceneNode", "m_vecAbsOrigin", CGameSceneNode::m_vecAbsOrigin},
    Field{"CGameSceneNode", "m_bDormant", CGameSceneNode::m_bDormant},
    Field{"CSkeletonInstance", "m_modelState", CSkeletonInstance::m_modelState},
    Field{"C_BaseModelEntity", "m_vecViewOffset", C_BaseModelEntity::m_vecViewOffset},
    Field{"C_BasePlayerPawn", "m_pWeaponServices", C_BasePlayerPawn::m_pWeaponServices},
    Field{"C_BasePlayerPawn", "m_pObserverServices", C_BasePlayerPawn::m_pObserverServices},
    Field{"C_BasePlayerPawn", "m_hController", C_BasePlayerPawn::m_hController},
    Field{"CPlayer_WeaponServices", "m_hActiveWeapon", CPlayer_WeaponServices::m_hActiveWeapon},
    Field{"CPlayer_ObserverServices", "m_iObserverMode", CPlayer_ObserverServices::m_iObserverMode},
    Field{"CPlayer_ObserverServices", "m_hObserverTarget", CPlayer_ObserverServices::m_hObserverTarget},
    Field{"C_CSPlayerPawnBase", "m_flFlashOverlayAlpha", C_CSPlayerPawnBase::m_flFlashOverlayAlpha},
    Field{"C_CSPlayerPawnBase", "m_flFlashMaxAlpha", C_CSPlayerPawnBase::m_flFlashMaxAlpha},
    Field{"C_CSPlayerPawnBase", "m_flFlashDuration", C_CSPlayerPawnBase::m_flFlashDuration},
    Field{"C_CSPlayerPawn", "m_entitySpottedState", C_CSPlayerPawn::m_entitySpottedState},
    Field{"C_CSPlayerPawn", "m_bIsScoped", C_CSPlayerPawn::m_bIsScoped},
    Field{"C_CSPlayerPawn", "m_iShotsFired", C_CSPlayerPawn::m_iShotsFired},
    Field{"C_CSPlayerPawn", "m_ArmorValue", C_CSPlayerPawn::m_ArmorValue},
    Field{"C_CSPlayerPawn", "m_angEyeAngles", C_CSPlayerPawn::m_angEyeAngles},
    Field{"C_CSPlayerPawn", "m_iIDEntIndex", C_CSPlayerPawn::m_iIDEntIndex},
    Field{"EntitySpottedState_t", "m_bSpotted", EntitySpottedState_t::m_bSpotted},
    Field{"EntitySpottedState_t", "m_bSpottedByMask", EntitySpottedState_t::m_bSpottedByMask},
    Field{"CBasePlayerController", "m_hPawn", CBasePlayerController::m_hPawn},
    Field{"CBasePlayerController", "m_iszPlayerName", CBasePlayerController::m_iszPlayerName},
    Field{"CCSPlayerController", "m_hPlayerPawn", CCSPlayerController::m_hPlayerPawn},
    Field{"CCSPlayerController", "m_hObserverPawn", CCSPlayerController::m_hObserverPawn},
    Field{"CCSPlayerController", "m_bPawnIsAlive", CCSPlayerController::m_bPawnIsAlive},
    Field{"C_EconEntity", "m_AttributeManager", C_EconEntity::m_AttributeManager},
    Field{"C_AttributeContainer", "m_Item", C_AttributeContainer::m_Item},
    Field{"C_EconItemView", "m_iItemDefinitionIndex", C_EconItemView::m_iItemDefinitionIndex},
    Field{"C_CSGameRules", "m_bBombPlanted", C_CSGameRules::m_bBombPlanted},
    Field{"C_PlantedC4", "m_bBombTicking", C_PlantedC4::m_bBombTicking},
    Field{"C_PlantedC4", "m_nBombSite", C_PlantedC4::m_nBombSite},
    Field{"C_PlantedC4", "m_flC4Blow", C_PlantedC4::m_flC4Blow},
    Field{"C_PlantedC4", "m_bHasExploded", C_PlantedC4::m_bHasExploded},
    Field{"C_PlantedC4", "m_flTimerLength", C_PlantedC4::m_flTimerLength},
    Field{"C_PlantedC4", "m_bBeingDefused", C_PlantedC4::m_bBeingDefused},
    Field{"C_PlantedC4", "m_flDefuseLength", C_PlantedC4::m_flDefuseLength},
    Field{"C_PlantedC4", "m_flDefuseCountDown", C_PlantedC4::m_flDefuseCountDown},
    Field{"C_PlantedC4", "m_bBombDefused", C_PlantedC4::m_bBombDefused},
    Field{"C_PlantedC4", "m_hBombDefuser", C_PlantedC4::m_hBombDefuser},
};
} // namespace game::schema
