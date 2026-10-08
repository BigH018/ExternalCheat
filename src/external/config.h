#pragma once

// Non-offset constants and tunables. Offsets live in game/offsets.h and game/schema.h, never here.
//
// PURE: no <Windows.h>.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace config
{
static_assert(sizeof(void*) == 8, "x64 only");

// --- Branding ------------------------------------------------------------------------------------------------------
inline constexpr char kAppName[] = "External Cheat";
inline constexpr char kAppAuthor[] = "by BigH";
inline constexpr char kAppEdition[] = "External";
inline constexpr wchar_t kOverlayClassName[] = L"ExternalCheatOverlay";
inline constexpr wchar_t kOverlayTitle[] = L"External Cheat - overlay";

// --- Pointer sanity ------------------------------------------------------------------------------------------------
// User-mode address range of an x64 Windows process. The first 64 KiB are never mapped.
inline constexpr std::uintptr_t kMinValidPointer = 0x10000;
inline constexpr std::uintptr_t kMaxValidPointer = 0x7FFFFFFFFFFF;
inline constexpr std::size_t kPageSize = 0x1000;

// --- The game and the modules we read from -------------------------------------------------------------------------
inline constexpr std::wstring_view kGameExe = L"cs2.exe";
inline constexpr std::wstring_view kClientModule = L"client.dll";
inline constexpr std::wstring_view kEngineModule = L"engine2.dll";
inline constexpr std::wstring_view kSchemaSystemModule = L"schemasystem.dll";
inline constexpr std::wstring_view kInputSystemModule = L"inputsystem.dll";
// The schema system's names for the client: its type scope, and the module name inside each class's info.
inline constexpr std::string_view kClientTypeScope = "client.dll";
inline constexpr std::string_view kClientSchemaModule = "client";

// --- Startup diagnostic (Phase 2) ----------------------------------------------------------------------------------
// `cs2_external.exe --diag` prints the offset diagnostic and exits (no overlay): the check to run after a CS2 update.
inline constexpr std::string_view kDiagnoseFlag = "--diag";
inline constexpr std::size_t kRemoteCopyChunk = 0x100000; // copying a module: 1 MiB per read, page by page on failure
inline constexpr std::size_t kMaxSignatureHits = 16;       // more matches than this = the pattern is useless
inline constexpr std::size_t kMaxInterfaces = 512;         // InterfaceReg list walk limit (guards against a loop)
inline constexpr std::size_t kMaxSchemaFields = 4096;      // per class (C_CSPlayerPawn has 104)
inline constexpr std::size_t kMaxNameLength = 256;         // interface, class and field names

// --- Game reads (Phase 3) ------------------------------------------------------------------------------------------
// `cs2_external.exe --live` prints a console table of every player (no overlay), refreshed every kLiveViewIntervalMs.
inline constexpr std::string_view kLiveViewFlag = "--live";
inline constexpr std::uint32_t kLiveViewIntervalMs = 250;
inline constexpr std::uint32_t kMaxPlayers = 64;          // player controllers live at entity indices 1..64
inline constexpr std::size_t kPlayerNameLength = 128;     // CBasePlayerController::m_iszPlayerName is char[128]
inline constexpr std::int32_t kMaxSaneHealth = 10000;     // more than this = a garbage read
inline constexpr float kMaxBoneDistance = 200.0f;         // a bone further from the feet (units) = a garbage read
inline constexpr float kMinHeadHeight = 30.0f;            // --diag: the head bone's height above the feet (units),
inline constexpr float kMaxHeadHeight = 80.0f;            //   crouching (~45) to standing (~60), with margin

// --- Keys ----------------------------------------------------------------------------------------------------------
// Every key is a keybind (input/actions, default binds there). A capture gives up after this long without a key.
inline constexpr std::uint64_t kBindCaptureTimeoutMs = 6000;

// --- Menu (ImGui) --------------------------------------------------------------------------------------------------
inline constexpr float kMenuWidth = 900.0f;  // initial size in pixels (shrunk to fit small game windows)
inline constexpr float kMenuHeight = 620.0f;
inline constexpr float kMenuMinWidth = 600.0f; // the menu can be resized, not below this (one column of panels)
inline constexpr float kMenuMinHeight = 420.0f;
inline constexpr float kMenuMaxScreenFraction = 0.92f;
inline constexpr float kHeaderHeight = 54.0f;   // logo, name, close button
inline constexpr float kTabBarHeight = 40.0f;   // the tabs under the header
inline constexpr float kFontSize = 17.0f;       // base UI font size in pixels
inline constexpr float kTitleFontSize = 19.0f;  // the app name in the header
inline constexpr float kHeaderLogoSize = 32.0f; // the logo in the menu header

// --- Overlay -------------------------------------------------------------------------------------------------------
inline constexpr float kWatermarkMargin = 10.0f;   // from the top-left corner of the game's client area
inline constexpr float kFrameOutlineThickness = 2.0f;

// --- Setting ranges ------------------------------------------------------------------------------------------------
// Each numeric setting's range, defined once: the menu's sliders use it, and loading a profile clamps to it.
template <class T>
struct Range
{
    T min;
    T max;

    [[nodiscard]] constexpr bool contains(T value) const noexcept { return value >= min && value <= max; }
    [[nodiscard]] constexpr T clamp(T value) const noexcept { return std::clamp(value, min, max); }
};

inline constexpr Range<float> kMaxDistance{0.0f, 300.0f};    // metres, every feature's max distance; 0 = no limit
inline constexpr Range<float> kEspThickness{1.0f, 4.0f};     // pixels
inline constexpr Range<float> kAimFov{0.5f, 30.0f};          // degrees from the crosshair
inline constexpr Range<float> kAimSmoothing{1.0f, 30.0f};    // 1 = snap
inline constexpr Range<int> kTriggerReaction{0, 500};        // ms
inline constexpr Range<int> kTriggerShotDelay{20, 1000};     // ms
inline constexpr Range<int> kTriggerBurst{2, 10};            // shots
inline constexpr Range<float> kRadarSize{150.0f, 600.0f};    // pixels
inline constexpr Range<float> kRadarRange{10.0f, 150.0f};    // metres from the centre to the edge
inline constexpr Range<float> kRadarDotSize{2.0f, 8.0f};     // pixels (radius)
inline constexpr Range<float> kBombTimerTop{0.0f, 1000.0f};  // pixels from the top of the game window
inline constexpr Range<float> kSpectatorTop{0.0f, 1000.0f};  // pixels from the top of the game window

// --- Profiles (Phase 8) --------------------------------------------------------------------------------------------
// <folder of cs2_external.exe>\profiles\<name>.json, plus .last_profile (the profile to load on the next start).
// Bump kProfileSchemaVersion (and add a migration in settings/profile_json.cpp) when a change would load wrongly from
// an older profile: a field renamed, moved or re-meant. A new field with a sensible default needs nothing.
inline constexpr int kProfileSchemaVersion = 1;
inline constexpr std::size_t kProfileNameMaxLength = 40;
inline constexpr char kDefaultProfile[] = "default"; // built in (the code defaults), read-only
inline constexpr char kProfilesFolder[] = "profiles";
inline constexpr char kProfileExtension[] = ".json";
inline constexpr char kLastProfileFile[] = ".last_profile";

// --- Aimbot and triggerbot (Phase 5) -------------------------------------------------------------------------------
// Smoothing is defined per 60 Hz frame (1 / smoothing of the remaining angle each 1/60 s), then scaled to the real
// frame time, so it feels the same at any overlay frame rate. Frame times above this are clamped (a stall).
inline constexpr float kSmoothingReferenceHz = 60.0f;
inline constexpr float kMaxFrameSeconds = 0.1f;
// How long the triggerbot holds attack for one shot. Proven 2026-10-06: a 10 ms press already fires one round.
inline constexpr std::uint32_t kTriggerTapMs = 30;
// Flashed = the flash overlay is more than this fraction of its maximum (m_flFlashOverlayAlpha / m_flFlashMaxAlpha).
inline constexpr float kFlashedFraction = 0.5f;
// Head only: the crosshair ray must pass within this many world units of the head bone.
inline constexpr float kTriggerHeadRadius = 5.0f;

// --- ESP drawing (Phase 4) -----------------------------------------------------------------------------------------
inline constexpr float kEspFontSize = 15.0f;         // labels over the bots, in pixels
inline constexpr float kEspBoxAspect = 0.5f;         // box width = height * this
inline constexpr float kEspBoxTopAboveEye = 8.0f;    // world units from the eyes to the top of the box
inline constexpr float kEspHeadRadius = 6.5f;        // world units around bone::kHeadCentre: crown to chin
inline constexpr float kEspLabelGap = 2.0f;          // pixels between the box and a label
inline constexpr float kEspHealthBarWidth = 3.0f;    // pixels
inline constexpr float kEspHealthBarGap = 3.0f;      // pixels between the box and the health bar
inline constexpr float kEspCornerFraction = 0.25f;   // corner box: each corner's arms = this much of the side
inline constexpr std::size_t kEspMaxNameLength = 20; // longer names are cut with "..."

// --- Radar (Phase 6) -----------------------------------------------------------------------------------------------
inline constexpr float kRadarMargin = 10.0f;        // pixels from the corner of the game window
inline constexpr float kRadarFacingLength = 2.5f;   // a facing line is this many dot radii long
inline constexpr float kRadarYouSize = 1.8f;        // your arrow, in dot radii
inline constexpr float kRadarEdgeFade = 0.55f;      // alpha factor for players clamped to the edge
inline constexpr float kRadarNorthUpYaw = 90.0f;    // rotation off: the radar looks along +y (the game's radar north)

// --- Bomb timer (Phase 6) ------------------------------------------------------------------------------------------
inline constexpr float kMaxBombTimer = 120.0f;               // a fuse longer than this (s) = a garbage read
inline constexpr float kDefuseSecondsNoKit = 10.0f;          // CS2's defuse times
inline constexpr float kDefuseSecondsKit = 5.0f;
inline constexpr float kBombPanelWidth = 250.0f;             // pixels

// --- Spectator list (Phase 6) --------------------------------------------------------------------------------------
inline constexpr float kSpectatorPanelWidth = 230.0f; // pixels
inline constexpr float kSpectatorMargin = 10.0f;      // pixels from the left or right edge of the game window

// --- Panels (render/panel: the bomb timer, the spectator list) -----------------------------------------------------
inline constexpr float kPanelPadding = 8.0f;
inline constexpr float kPanelRowGap = 4.0f;
inline constexpr float kPanelBarHeight = 6.0f;
inline constexpr float kPanelMarkOverhang = 3.0f; // a bar's marks stick out this far above and below it
inline constexpr float kPanelMarkThickness = 2.0f;

// --- Timing --------------------------------------------------------------------------------------------------------
inline constexpr std::uint32_t kStatusIntervalMs = 250;     // match status read on the Home page (~4 Hz)
inline constexpr std::uint32_t kHiddenPollIntervalMs = 100; // overlay hidden: how often to look at the game again
inline constexpr std::uint32_t kShutdownWaitMs = 3000;      // console closed: how long Windows waits for our cleanup
} // namespace config
