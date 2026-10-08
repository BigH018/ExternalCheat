#pragma once

// Module-base RVAs (stable globals), button globals, interface instances, signatures and the engine-internal layouts
// that no dumper outputs. Every value records where it came from.
//
// Source for this file unless noted otherwise:
//   a2x/cs2-dumper output for CS2 build 14189, dumped 2026-10-06T16:04:45Z (docs/dumps/info.json), copied (by script,
//   never retyped) from docs/dumps/offsets.json, buttons.json and interfaces.json. The decimal value in each comment
//   is the dump's own number.
//
// Names match the dumper output exactly (not kPascalCase) so they can be grepped across dumps.
// Changing a value: CLAUDE.md §7 "Offset change rule" (prove it, keep the old value as a comment, log it).
//
// PURE: no <Windows.h>.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace game::offsets
{
// The CS2 build the values below were dumped from. app/diagnostics compares it with engine2.dll + dwBuildNumber.
inline constexpr std::uint32_t kDumpBuildNumber = 14189;

// --- client.dll globals (offsets.json) ----------------------------------------------------------------------------
namespace client
{
inline constexpr std::uintptr_t dwCSGOInput                           = 0x2578160; // 39289184
inline constexpr std::uintptr_t dwEntityList                          = 0x2717828; // 40990760
inline constexpr std::uintptr_t dwGameEntitySystem                    = 0x2717828; // 40990760 (same global)
inline constexpr std::uintptr_t dwGameEntitySystem_highestEntityIndex = 0x2120;    // 8480 (inside the entity system)
inline constexpr std::uintptr_t dwGameRules                           = 0x255EE50; // 39186000
inline constexpr std::uintptr_t dwGlobalVars                          = 0x222DE98; // 35839640
inline constexpr std::uintptr_t dwGlowManager                         = 0x255EE60; // 39186016
inline constexpr std::uintptr_t dwLocalPlayerController               = 0x253A068; // 39034984
// client.dll + dwLocalPlayerPawn -> C_CSPlayerPawn* (0 when not in a match). Verified in-game 2026-10-06 (build
// 14189): 0x4DF564BF800 in an offline deathmatch. It is a field of the prediction object (dwPrediction + 0xF8).
inline constexpr std::uintptr_t dwLocalPlayerPawn                     = 0x2562808; // 39200776
inline constexpr std::uintptr_t dwPlantedC4                           = 0x24CA930; // 38578480
inline constexpr std::uintptr_t dwPrediction                          = 0x2562710; // 39200528
inline constexpr std::uintptr_t dwViewAngles                          = 0x25787E8; // 39290856
inline constexpr std::uintptr_t dwViewMatrix                          = 0x2567FA0; // 39223200
inline constexpr std::uintptr_t dwViewRender                          = 0x2568968; // 39225704
inline constexpr std::uintptr_t dwWeaponC4                            = 0x24C6AF0; // 38562544
} // namespace client

// --- engine2.dll globals (offsets.json) ---------------------------------------------------------------------------
namespace engine2
{
inline constexpr std::uintptr_t dwBuildNumber                       = 0x61CFE8; // 6410216 (uint32: the CS2 build)
inline constexpr std::uintptr_t dwNetworkGameClient                 = 0x91AFC0; // 9547712
inline constexpr std::uintptr_t dwNetworkGameClient_clientTickCount = 0x398;    // 920 (inside the network client)
inline constexpr std::uintptr_t dwNetworkGameClient_deltaTick       = 0x24C;    // 588
// As dumped. Not a plausible field offset (the other _ values are < 0x400); not used until someone proves it.
inline constexpr std::uintptr_t dwNetworkGameClient_isBackgroundMap = 0x2C143F; // 2888767
inline constexpr std::uintptr_t dwNetworkGameClient_localPlayer     = 0xF8;     // 248
inline constexpr std::uintptr_t dwNetworkGameClient_maxClients      = 0x240;    // 576
inline constexpr std::uintptr_t dwNetworkGameClient_serverTickCount = 0x24C;    // 588
inline constexpr std::uintptr_t dwNetworkGameClient_signOnState     = 0x230;    // 560 (6 = fully connected)
inline constexpr std::uintptr_t dwWindowHeight                      = 0x91F334; // 9564980
inline constexpr std::uintptr_t dwWindowWidth                       = 0x91F330; // 9564976
} // namespace engine2

// --- Other modules' globals (offsets.json) ------------------------------------------------------------------------
namespace inputsystem
{
inline constexpr std::uintptr_t dwInputSystem = 0x46BC0; // 289728 (= the InputSystemVersion001 instance)
} // namespace inputsystem

namespace matchmaking
{
inline constexpr std::uintptr_t dwGameTypes = 0x1B0FD0; // 1773520
} // namespace matchmaking

namespace soundsystem
{
inline constexpr std::uintptr_t dwSoundSystem = 0x535350; // 5460816
} // namespace soundsystem

// --- Button globals, all in client.dll (buttons.json) --------------------------------------------------------------
// Reading is harmless. Writing one is a game write (Phase 5/6): its value format gets an in-game check first.
// Read on 2026-10-06 (build 14189, not pressed): the first 4 bytes of jump and attack were 0x01000100.
namespace buttons
{
inline constexpr std::uintptr_t attack       = 0x2231FD0; // 35856336
inline constexpr std::uintptr_t attack2      = 0x2232060; // 35856480
inline constexpr std::uintptr_t back         = 0x22322A0; // 35857056
inline constexpr std::uintptr_t duck         = 0x2232570; // 35857776
inline constexpr std::uintptr_t forward      = 0x2232210; // 35856912
inline constexpr std::uintptr_t jump         = 0x22324E0; // 35857632
inline constexpr std::uintptr_t left         = 0x2232330; // 35857200
inline constexpr std::uintptr_t lookatweapon = 0x2578080; // 39288960
inline constexpr std::uintptr_t reload       = 0x2231F40; // 35856192
inline constexpr std::uintptr_t right        = 0x22323C0; // 35857344
inline constexpr std::uintptr_t showscores   = 0x2577F60; // 39288672
inline constexpr std::uintptr_t sprint       = 0x2231EB0; // 35856048
inline constexpr std::uintptr_t turnleft     = 0x22320F0; // 35856624
inline constexpr std::uintptr_t turnright    = 0x2232180; // 35856768
inline constexpr std::uintptr_t use          = 0x2232450; // 35857488
inline constexpr std::uintptr_t zoom         = 0x2577FF0; // 39288816
} // namespace buttons

// --- Interface instances we look up (interfaces.json): RVA of what CreateInterface(name) returns -------------------
// game/interfaces finds them at runtime by walking each module's InterfaceReg list; these are for the comparison.
// "GameEntitySystem" is not a registered interface in CS2: the entity system is the global client::dwGameEntitySystem.
namespace interfaces
{
inline constexpr std::uintptr_t Source2Client002         = 0x255C3A0; // 39175072, client.dll
inline constexpr std::uintptr_t Source2EngineToClient001 = 0x61FFF0;  // 6422512, engine2.dll ("EngineClient")
inline constexpr std::uintptr_t SchemaSystem_001         = 0x76710;   // 485136, schemasystem.dll
inline constexpr std::uintptr_t InputSystemVersion001    = 0x46BC0;   // 289728, inputsystem.dll
} // namespace interfaces

// --- Signatures (client.dll .text) ---------------------------------------------------------------------------------
// A signature finds a global from the code that uses it, so it survives most updates that move the global. Each one
// matches a RIP-relative instruction (mov/lea [rip + disp32]): target = match + instruction_size + disp32, plus `add`.
//
// Found by hand 2026-10-06 against the live client.dll of build 14189 (every RIP-relative reference to each dumped
// global, then the shortest pattern without call targets or struct offsets in it). Each one was checked to match
// only places that resolve to exactly the dumped value. The dump values in client:: stay what the code uses; the
// diagnostic reports when a signature and the dump disagree (= the dump is stale, or the signature broke).
struct Signature
{
    std::string_view name;          // the dumper's name for the global it finds
    std::string_view pattern;       // IDA style: hex bytes and ? wildcards
    std::uint32_t disp_offset;      // where the disp32 sits inside the match
    std::uint32_t instruction_size; // length of that instruction (RIP = match + instruction_size)
    std::uintptr_t add;             // added to the target (a field inside the global it finds), usually 0
    std::uintptr_t dumped;          // the dump's value, for the comparison
};

namespace signatures
{
// dwLocalPlayerPawn has no code reference of its own: it is a field of the prediction object. The signature finds
// the function that returns &prediction (lea rax, [rip+x]; ret) and adds the field offset, which is
// dwLocalPlayerPawn - dwPrediction in the dump (0x2562808 - 0x2562710 = 0xF8).
inline constexpr std::uintptr_t kPredictionLocalPawn = 0xF8;

inline constexpr std::array kClient{
    // mov [rip+x], rcx; jmp ...; int3   (the entity system being stored)
    Signature{"dwEntityList", "48 89 0D ? ? ? ? E9 ? ? ? ? CC", 3, 7, 0, client::dwEntityList},
    // mov rax, [rip+x]; mov [r14+...], edi
    Signature{"dwLocalPlayerController", "48 8B 05 ? ? ? ? 41 89 BE", 3, 7, 0, client::dwLocalPlayerController},
    // lea rax, [rip+prediction]; ret; int3 x8; the next function's prologue
    Signature{"dwLocalPlayerPawn", "48 8D 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 40 53 56 41 54 41 56 41 57", 3, 7,
              kPredictionLocalPawn, client::dwLocalPlayerPawn},
    // lea rcx, [rip+x]; shl rax, 6   (indexing an array of 64-byte matrices)
    Signature{"dwViewMatrix", "48 8D 0D ? ? ? ? 48 C1 E0 06", 3, 7, 0, client::dwViewMatrix},
    // mov [rip+x], rdx; mov [rdx+...], rax
    Signature{"dwGlobalVars", "48 89 15 ? ? ? ? 48 89 42", 3, 7, 0, client::dwGlobalVars},
    // mov [rip+x], rbx; call [rip+...]; test al, al   (2 matches in build 14189, both the same global)
    Signature{"dwGameRules", "48 89 1D ? ? ? ? FF 15 ? ? ? ? 84 C0", 3, 7, 0, client::dwGameRules},
    // mov [rip+x], r14; jmp short; mov rax, [rip+...]
    Signature{"dwPlantedC4", "4C 89 35 ? ? ? ? EB ? 48 8B 05", 3, 7, 0, client::dwPlantedC4},
    // mov [rip+x], rax; xorps xmm0, xmm0; movups [rip+...], xmm0
    Signature{"dwCSGOInput", "48 89 05 ? ? ? ? 0F 57 C0 0F 11 05", 3, 7, 0, client::dwCSGOInput},
};
} // namespace signatures

// --- Engine-internal layouts (no dumper outputs these) -------------------------------------------------------------
// Found by hand 2026-10-06 against the live process (build 14189) with a read-only script; proof in docs/offsets.md.
// If one of these moves, the diagnostic says which step failed, and the dump values above keep working.
namespace layout
{
// The exported CreateInterface starts with `mov r9, [rip + s_pInterfaceRegs]` (4C 8B 0D disp32).
inline constexpr std::array<std::uint8_t, 3> kCreateInterfaceOpcode{0x4C, 0x8B, 0x0D};
inline constexpr std::uint32_t kCreateInterfaceInstructionSize = 7;

// InterfaceReg { void* (*create)(); const char* name; InterfaceReg* next; }
inline constexpr std::uintptr_t kInterfaceRegCreate = 0x0;
inline constexpr std::uintptr_t kInterfaceRegName = 0x8;
inline constexpr std::uintptr_t kInterfaceRegNext = 0x10;

// Every create function of the interfaces we use is `lea rax, [rip + instance]; ret` (48 8D 05 disp32 C3).
inline constexpr std::array<std::uint8_t, 3> kCreateFnOpcode{0x48, 0x8D, 0x05};
inline constexpr std::uint8_t kCreateFnReturn = 0xC3;
inline constexpr std::uint32_t kCreateFnInstructionSize = 7;

// CSchemaSystem: CUtlVector<CSchemaSystemTypeScope*> of type scopes { int32 count; pad; T* data } at +0x190.
inline constexpr std::uintptr_t kSchemaSystemScopeCount = 0x190;
inline constexpr std::uintptr_t kSchemaSystemScopeData = 0x198;
// CSchemaSystemTypeScope: the module name as char[256] at +0x8 ("client.dll").
inline constexpr std::uintptr_t kTypeScopeName = 0x8;
inline constexpr std::size_t kTypeScopeNameSize = 256;

// SchemaClassInfoData: static data inside the module that declares the class. +0x0 points to itself.
inline constexpr std::uintptr_t kClassInfoSelf = 0x0;
inline constexpr std::uintptr_t kClassInfoName = 0x8;       // const char*
inline constexpr std::uintptr_t kClassInfoModule = 0x10;    // const char*, "client"
inline constexpr std::uintptr_t kClassInfoSize = 0x20;      // int32, sizeof the class
inline constexpr std::uintptr_t kClassInfoFieldCount = 0x24; // int16
inline constexpr std::uintptr_t kClassInfoFields = 0x30;    // SchemaClassFieldData*
inline constexpr std::uintptr_t kClassInfoTypeScope = 0x50; // CSchemaSystemTypeScope*
inline constexpr std::size_t kClassInfoReadSize = 0x58;

// SchemaClassFieldData, an array of field_count entries: { const char* name; type*; int32 offset; ... }.
inline constexpr std::uintptr_t kFieldName = 0x0;
inline constexpr std::uintptr_t kFieldOffset = 0x10;
inline constexpr std::size_t kFieldStride = 0x20;

// --- The entity system (client.dll + dwEntityList) ---
// Proven 2026-10-06 (build 14189): the local player's weapon handle resolved by hand (Phase 2), then all 20 players of
// a bot match walked by index (Phase 3). An entity's index picks a chunk of 512 identities; chunk pointers start at
// +0x10. An identity (CEntityIdentity) is 0x70 bytes: entity pointer at +0x0, the entity's whole handle at +0x10
// (index | serial << 15), designer name ("cs_player_controller", "weapon_ak47") at +0x20.
inline constexpr std::uintptr_t kEntityChunks = 0x10;
inline constexpr std::uint32_t kEntityChunkShift = 9;      // index >> 9 = chunk
inline constexpr std::uint32_t kEntityChunkMask = 0x1FF;   // index & 0x1FF = slot inside the chunk
inline constexpr std::size_t kEntityChunkCount = 64;       // 64 * 512 = every index a 15-bit handle can hold
inline constexpr std::size_t kIdentitySize = 0x70;
inline constexpr std::uintptr_t kIdentityEntity = 0x0;
inline constexpr std::uintptr_t kIdentityHandle = 0x10;
inline constexpr std::uintptr_t kIdentityDesignerName = 0x20;

// CHandle (uint32): the entity index in the low 15 bits, a serial above them. 0xFFFFFFFF = no entity.
inline constexpr std::uint32_t kHandleIndexMask = 0x7FFF;
inline constexpr std::uint32_t kInvalidHandle = 0xFFFFFFFF;

// Player controllers sit at entity indices 1..maxClients (slot = index - 1). Their designer name is the check.
// Proven 2026-10-06: 20 players at indices 1..20, all "cs_player_controller".
inline constexpr std::string_view kPlayerControllerDesignerName = "cs_player_controller";

// --- CGlobalVars (client.dll + dwGlobalVars -> CGlobalVars*) ---
// Proven 2026-10-06 (build 14189) by sampling the struct twice, 2 s apart, in a bot match: realtime and curtime both
// advanced by 2.0, tickcount by 128 (64 tick), curtime = tickcount * interval_per_tick, maxClients read 64, the map
// name read "de_mirage". (The "typical" layout CLAUDE.md used to give, curtime at +0x10, is wrong for this build.)
inline constexpr std::uintptr_t kGlobalsRealtime = 0x0;        // float, seconds since the game started
inline constexpr std::uintptr_t kGlobalsFrameCount = 0x4;      // int32
inline constexpr std::uintptr_t kGlobalsMaxClients = 0x10;     // int32
inline constexpr std::uintptr_t kGlobalsIntervalPerTick = 0x1C; // float (1/64)
inline constexpr std::uintptr_t kGlobalsCurtime = 0x30;        // float, game time
inline constexpr std::uintptr_t kGlobalsTickCount = 0x44;      // int32
inline constexpr std::uintptr_t kGlobalsMapName = 0x188;       // const char*, "de_mirage"
inline constexpr std::size_t kGlobalsReadSize = 0x190;         // one read covers every field above

// --- Bones (pawn -> m_pGameSceneNode (a CSkeletonInstance) + m_modelState + 0x80 -> bone array) ---
// Not in any dump. Found 2026-10-06 (build 14189) in a bot match: for all 20 pawns the pointer at model state + 0x80
// leads to an array of 32-byte bones { Vector position; float scale (1.0); Quaternion rotation }, whose positions sit
// on the pawn (within ~22 units sideways, -1.5 to +64 units up). The indices (game/bones.h) were mapped from those
// positions in each bot's own frame, on CT and T models alike.
inline constexpr std::uintptr_t kModelStateBones = 0x80;
inline constexpr std::size_t kBoneStride = 0x20;

// --- Button state values (written as uint32 at client.dll + buttons::*) ---
// Proven 2026-10-06 (build 14189) with live writes in an offline bot match: 65537 on `jump` made the local player
// jump (vertical velocity 286, +52 units), 256 released it; one 30 ms press of `attack` fired one round (clip 30 ->
// 29), three taps fired three, even a 10 ms press fired. At rest the four bytes read 00 01 00 01; after our release
// they read 00 01 00 00, which the game accepts (the next press works the same).
inline constexpr std::uint32_t kButtonPressed = 65537;
inline constexpr std::uint32_t kButtonReleased = 256;

// --- View angles (client.dll + dwViewAngles: float pitch, yaw, roll) ---
// Proven 2026-10-06 with live writes: yaw + 10 and pitch - 5 written there showed up in the local pawn's
// m_angEyeAngles within 200 ms; writing the old values back restored the view. Only pitch and yaw are written.
inline constexpr std::size_t kViewAnglesWriteSize = 2 * sizeof(float);
} // namespace layout
} // namespace game::offsets
