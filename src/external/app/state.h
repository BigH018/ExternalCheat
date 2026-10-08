#pragma once

// Everything the app keeps between frames that the menu also shows or edits. Main thread only, so no locks.
// Plain data: the menu edits `settings`; app/frame fills in the rest.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "config.h"
#include "core/process.h"
#include "features/feature_summary.h"
#include "features/triggerbot.h"
#include "game/snapshot.h"
#include "input/bind_capture.h"
#include "settings/presets.h"
#include "settings/settings.h"

namespace app
{
// What the bootstrap found (main.cpp), for the Home page.
struct GameInfo
{
    DWORD pid = 0;
    core::ModuleInfo client;
    core::ModuleInfo engine;
};

// The startup offset diagnostic's result (app/diagnostics), for the Home page. The details are in the console.
struct OffsetReport
{
    bool ran = false;
    std::uint32_t game_build = 0; // engine2.dll + dwBuildNumber; 0 = couldn't be read
    int checks = 0;
    int failures = 0;

    [[nodiscard]] bool ok() const noexcept { return ran && failures == 0; }
};

// A profile operation the Settings page asks for; app/frame does it at the start of the next frame.
enum class ProfileOp : std::uint8_t
{
    load,    // name = the profile to load
    save,    // the current settings over the current profile
    save_as, // name = the new profile (becomes the current one)
    rename,  // the current profile -> name
    remove,  // name = the profile to delete
    reset,   // the current settings back to the code defaults (not saved)
};

struct ProfileRequest
{
    ProfileOp op = ProfileOp::load;
    std::string name;
};

// Which profile is loaded and what it looked like when it was last loaded or saved (for the unsaved-changes marker).
struct ProfileState
{
    std::string current = config::kDefaultProfile;
    settings::Settings saved;          // the current profile as on disk
    std::vector<std::string> names;    // "default" first, then the saved profiles
    std::vector<std::string> warnings; // problems found in the last loaded profile (also in the console)
    std::string folder;                // where the files are (UTF-8, shown on the Settings page)
    std::string message;               // the last operation's result ("Profile \"rage\" saved")
    bool message_failed = false;
};

// What the menu asks app/frame to do on the next frame (the menu never acts itself).
struct Requests
{
    std::optional<ProfileRequest> profile;
    std::optional<settings::Preset> preset;
    bool exit = false;       // the Exit button (Settings page): shut down like the exit key
    bool close_menu = false; // the menu's close button: like pressing the menu key
};

struct AppState
{
    GameInfo game;
    OffsetReport offsets;
    settings::Settings settings; // edited by the menu, applied live by app/frame
    ProfileState profiles;       // Phase 8: the profile `settings` came from
    Requests requests;

    [[nodiscard]] bool unsaved_changes() const { return settings != profiles.saved; }

    // Which features are on, for the watermark. Filled from the settings by app/frame every frame.
    features::ActiveFeatures active;

    // What the aimbot and triggerbot are doing this frame, for their pages.
    struct AimStatus
    {
        bool active = false;     // enabled, aim key held / toggled on, menu closed, game focused
        bool has_target = false; // a target inside the FOV this frame
    } aim_status;
    struct TriggerStatus
    {
        bool active = false; // enabled, activation allows it, menu closed, game focused
        features::TriggerBlock block = features::TriggerBlock::not_in_match;
    } trigger_status;

    bool menu_open = false;
    bool focus_warning = false; // the menu opened but couldn't take focus from the game

    // Keybinds: the menu starts a capture (Keybinds page, bind buttons); app/frame feeds it keys and stores the result.
    input::BindCapture capture;
    std::uint64_t now_ms = 0;     // GetTickCount64 at the start of this frame (for the capture's timeout)
    bool menu_key_failed = false; // the menu key couldn't be registered as a hotkey (another program holds it)

    // Match status, refreshed every config::kStatusIntervalMs while the overlay is visible.
    bool pawn_read_ok = false;
    std::uintptr_t local_pawn = 0; // 0 = no local pawn (main menu, loading screen)
    game::GameSnapshot snapshot;   // every player, globals, view matrix: every frame while a feature needs it

    int overlay_width = 0; // the game's client area, which the overlay covers
    int overlay_height = 0;
    float fps = 0.0f;      // overlay frames per second (ImGui's running average)

    [[nodiscard]] bool in_match() const noexcept { return pawn_read_ok && local_pawn != 0; }
};
} // namespace app
