#pragma once

// The orchestrator (same role as in the AC project): the only place that wires the overlay window, ImGui, game reads
// and the menu together, one frame at a time. Main thread only.
//
// Before the first frame: the last used profile (settings/profile_store) becomes the settings.
// One frame: pump messages (raw input counts key presses) -> follow the game window and focus -> menu key (the
// overlay's hotkey) -> keybinds (poll, bind capture, engine, actions: panic, exit, on / off keys, presets) -> the
// menu's requests from the last frame (profile operations, presets) -> game snapshot
// (every frame while a feature is on, ~4 Hz otherwise) -> aimbot (view angle write) -> triggerbot (attack button
// write) -> ESP, radar, panels -> HUD + menu -> present.

#include <Windows.h>

#include "app/state.h"
#include "core/memory.h"
#include "core/process.h"

namespace app
{
// What the bootstrap hands over. `process` and `memory` stay valid until run() returns.
struct Context
{
    DWORD pid = 0;
    HANDLE process = nullptr; // for is_running (PROCESS_QUERY_LIMITED_INFORMATION)
    core::Memory& memory; // read-write: the aimbot and triggerbot write single fields (game/writes)
    core::ModuleInfo client;
    core::ModuleInfo engine;
    OffsetReport offsets; // the startup diagnostic's result, for the Home page
};

// Runs the overlay until the user exits (the exit key, core::shutdown_requested or Alt+F4 on the menu), the game
// closes, or the overlay fails. Tears the overlay down before returning. Returns the process exit code.
int run(const Context& ctx);
} // namespace app
