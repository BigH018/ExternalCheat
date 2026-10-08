#pragma once

// The offset diagnostic (Phase 2), run once at startup: proves what game/offsets.h and game/schema.h say against the
// running game and prints every result to the console.
//
//   1. Build: engine2.dll + dwBuildNumber vs the build the dumps came from.
//   2. Interfaces: SchemaSystem_001, Source2Client002, Source2EngineToClient001, InputSystemVersion001 found by
//      walking each module's InterfaceReg list, vs interfaces.json; and the entity system global.
//   3. Signatures: every pattern in offsets::signatures, scanned in a copy of client.dll's .text, vs the dump value.
//   4. Schema: every field in schema::kFields, read from the live schema system, vs schema.h.
//   5. Buttons and globals: jump/attack and the main dw* pointers, as they read right now (informational).
//
// Nothing here changes what the tool uses: the dump values stay in charge, and a failed check is a warning that the
// dumps are stale (re-run the dumper; CLAUDE.md §7). `cs2_external.exe --diag` runs only this and exits.

#include <Windows.h>

#include "app/state.h"
#include "core/kernel_interface.h"
#include "core/memory.h"
#include "core/process.h"

namespace app
{
    [[nodiscard]] OffsetReport run_diagnostics(KernelInterface& kernel, const core::Memory& memory, DWORD pid,
        const core::ModuleInfo& client, const core::ModuleInfo& engine);
} // namespace app