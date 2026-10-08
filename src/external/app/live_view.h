#pragma once

// The Phase 3 debug view: `cs2_external.exe --live` redraws a console table of the match (globals, view matrix, every
// player) about 4 times a second, in place. No overlay. Ctrl+C or the game closing ends it.

#include <string>
#include <vector>

#include <Windows.h>

#include "core/memory.h"
#include "core/process.h"
#include "game/snapshot.h"

namespace app
{
// The table for one snapshot (which took `read_ms` to read), one string per console line.
[[nodiscard]] std::vector<std::string> format_live_view(const game::GameSnapshot& snapshot, double read_ms);

// Runs the view until Ctrl+C (core::shutdown_requested) or the game exits. Returns the process exit code.
int run_live_view(const core::Memory& memory, HANDLE process, const core::ModuleInfo& client);
} // namespace app
