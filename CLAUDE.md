# CLAUDE.md: External Cheat (by BigH), CS2 Offline Learning Trainer (External-First)

This file is the project's memory. Each session the user will say "read CLAUDE.md, then do task X", so a fresh session
must be able to work from this file alone. **Keep it accurate.** See the Maintenance rule (§16) at the bottom.

---

## 1. Scope and rules (read first, never violate)

I'm continuing a personal learning project on reverse engineering and low-level Windows programming, using
**Counter-Strike 2 (CS2)**, launched via Steam with `-insecure`.

- CS2 is closed-source, 64-bit, Source 2, DirectX 11. VAC is **disabled by `-insecure`**. Trusted Mode is off.
- I only play it **offline, single-player, against bots, on my own PC**. I never join a VAC-secured server with the
  tool running.
- **The project is external-first.** A user-mode console/exe that uses `ReadProcessMemory` and `WriteProcessMemory`
  against `cs2.exe`. **No DLL injection, no hooks, no MinHook, no in-process code** until (and unless) a later phase
  explicitly says so.
- **Learning goals:** process memory reading, pointer chains, entity system traversal (the chunked Source 2 layout),
  `CreateInterface` resolution from outside the process, schema system lookups, pattern/signature scanning for
  update-proof offsets, world-to-screen maths, and an overlay renderer (ImGui + DX11 or a transparent Win32 window).
- **Out of scope, permanently:** multiplayer or online use, VAC/Trusted Mode bypass or evasion, DMA, manual mapping,
  module/thread hiding, obfuscation, packing, string encryption, anti-debug, network code, telemetry, distribution,
  installers, licensing, "how to share this".
  **If a task ever seems to drift toward any of these, stop and tell the user.**

What that means in practice:
- The tool opens a handle to `cs2.exe` with `PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION`
  (read-only where possible; write only for the specific features that need it).
- No `LoadLibraryW`, no `CreateRemoteThread`, no `SetWindowsHookEx`, no `NtCreateThreadEx`, no APC injection.
- No driver, no IOMMU games, no PCIe tricks.
- No code that talks to a network. Nothing.
- No feature that only makes sense against humans or on a VAC-secured server.
- **Never** test against an online match. `-insecure` only, offline bots only.
- If the user ever says "I'll just try it on a casual for one round": **stop and tell them no.**

---

## 2. Session start protocol and how to work in this repo

1. Read this whole file.
2. Check **§14 Current status** to see where we are and what comes next, and **§2a** whether autonomous mode is on.
3. Confirm your understanding to the user in 2–3 lines (where we are, what the task is).
4. Plan before coding: list the files you'll touch and why. Ask if anything is unclear. Don't guess.
5. **One phase per session.** Don't start the next phase on your own.
6. **Ask before big architectural changes** (new module boundaries, new dependencies, threading changes, changing
   the render approach, switching from external to internal).
7. At the end of the task:
   1. Build Debug|x64 **and** Release|x64 with **zero warnings** (`/W4 /WX` makes warnings errors anyway).
      Run the unit tests (once the tests project exists).
      **If `cs2_external.exe` is running** (it locks `bin\<Config>\cs2_external.exe`, LNK1104), **force-close it
      and build**: `Get-Process cs2_external -ErrorAction SilentlyContinue | Stop-Process -Force`. Standing
      permission from the user (2026-10-06); don't ask, don't build into a scratch folder instead. Only our tool,
      never `cs2.exe`. Say in the summary that it was closed.
   2. Summarise what changed, then tell the user **exactly** what to do in-game to test it and what they should see.
   3. Update the phase checkboxes (§13), **Current status** (§14), the **Decision log** (§15), the file tree (§5),
      any gotchas you learned (§10), `docs/offsets.md`, and add a dated entry to `docs/DEVLOG.md`.
   4. **Stop and wait for the user's OK.** Only after approval, commit: small, focused commits with clear messages.
      **Commits and PRs must not include any Claude/AI co-author, "Generated with" or attribution lines.** The user
      is the sole author. Repo: private.

If you can't build here (e.g. no Visual Studio on the machine running the session), say so plainly. Never claim a
build or test passed when it wasn't run.

### 2a. Autonomous mode (the user switches it on and off)

**Current setting: OFF** (switched off 2026-10-07 by the user, mid-Phase 5; it had been on since 2026-10-06 night).

The user switches it with **"autonomous mode on"** / **"autonomous mode off"**. Whoever is told that updates the
"Current setting" line above (and, when switching on, the standing decisions below if the user gives new ones).
When OFF, the normal protocol above applies (stop after each phase and wait for the user's OK). When ON:

1. **"Verified in-game by the user" counts as auto-approved.** Don't wait. Do whatever in-game checking can be done
   from here (the read-only diagnostic and live reads against the running game; live writes only if a standing
   decision below allows them), then tick the box with "auto-approved" in the note.
2. **Commit, push, continue.** Normal end-of-phase workflow (Debug + Release zero warnings, tests, docs per §16), then
   commit, push, and start the next phase straight away. Still one phase per commit series.
3. **Annotate every auto-approved commit** (message and its `docs/DEVLOG.md` entry) with this line:
   `Auto-approved: user was asleep and did not personally verify this phase. If in-game testing fails, this commit is
   the first suspect for rollback.`
   No AI attribution lines, as always.
4. **Stop at ~90% of the usable context** (or when getting close): stop mid-phase rather than rush, and write
   `HANDOFF.md` at the repo root (below). Never start a new phase with less than ~10% left: a half-written phase is
   worse than a clean stopping point.
5. **Stop at a real blocker** and write `HANDOFF.md` plus a `docs/DEVLOG.md` note immediately: the build fails, a
   test fails and a couple of attempts don't fix it, an offset doesn't match the dumps, the game updated, or anything
   touches §1. Don't guess past a blocker.
6. **Decisions that would normally wait for the user** (the Phase 4 visibility choice, new dependencies, big
   architectural changes) are taken from the standing decisions below; anything not covered there is a blocker.

**`HANDOFF.md` contents:** the phase and exactly where it stopped (done / half-done / not started); build state (Debug?
Release? tests?) and the last known green commit hash; every file touched in the unfinished phase and what's left on
each; every offset, signature or schema field changed but not verified in-game; the next concrete step; anything
surprising, any shaky assumption, anywhere a fresh session should be suspicious.

**Standing decisions for the current ON period (given 2026-10-06):**
- Live **write** tests against the running CS2 (offline bot match, `-insecure`) are allowed: button value formats,
  aimbot angle writes, triggerbot and bunny hop. Never against anything but that offline match (§1 unchanged).
- Vendoring **nlohmann/json v3.12.0** (Phase 8) is approved.
- Phase 4 visibility: option **(2), the spotted-by mask**.
- **Push** every commit to `origin main`.

---

## 3. Project overview and end goal

**External Cheat - by BigH (CS2 Offline)** (the name, set by the user 2026-10-06; it replaced the AC project's
"Internal Cheat"): an external trainer for CS2 with an in-game
overlay menu, running **only** under `-insecure` against bots.

The goal is a learning project, not parity with any public cheat and not a release. Feature targets, in order of
implementation:

- **Overlay menu** toggled with **INSERT**: a Home dashboard and top tabs with icons (**Aimbot, Triggerbot, ESP, Misc,
  Keybinds, Settings**; a sidebar until Phase 10). Help in (?) tooltips, five themes + an accent colour, resizable by
  dragging its corner, settings applied live.
- **ESP** drawn in the overlay: 2D box, corner box, head circle, approximate skeleton (Phase 4+, needs bones),
  name, health bar/number, distance, weapon name, **scoped indicator** (the bot is zoomed in with an AWP, SSG 08,
  SCAR-20, G3SG1, AUG or SG 553), **visible / hidden colours** (spotted-by heuristic), snaplines (origin choice),
  team mode, enemies-only, colour picker with opacity.
- **Aimbot:** hold/toggle key, head/body target, priority (crosshair / distance / lowest health), FOV radius +
  circle, smoothing, team check, max distance, **visible only** (spotted-by heuristic).
- **Triggerbot** (fully configurable): fires automatically with a living enemy under the crosshair. Activation
  always on / hold key / toggle key; reaction delay before the first shot; fire mode single tap / burst (N shots) /
  hold while on target; delay between shots; team check; visible only; max distance; weapon-class filter (pistol,
  SMG, rifle, sniper, shotgun, heavy); snipers only when scoped; don't fire while flashed or in the air; head only
  (after bones are found).
- ~~Bunny hop~~: **dropped** by the user (2026-10-07): judged not worth it externally (timing from outside the game
  feels bad). No bunny hop page, no jump writes.
- **Watermark** on the game: the logo, "External Cheat by BigH", and under it the features that are on, one per
  line. No menu-key hint.
- ~~Player values (health / armour / ammo writes)~~: **dropped** by the user (2026-10-06). No Player page, no
  player-value writes.
- **Radar / misc** (Phase 6): enlarged radar, enemy dots on radar, bomb timer, spectator list.
- ~~Hitsound~~: **dropped** by the user (2026-10-07). No hitsound, no overlay audio.
- **Keybinds** for every action, **profiles** saved as JSON, **panic key**, clean shutdown.
- **Presets** (Off / Chill / Medium / Rage) that switch features and strengths.

**What external cannot do, and we accept that:**
- No silent aim (that requires a hook inside `CreateMove`).
- No weapon code patches (no spread, no recoil via patching).
- No calling game functions (TraceLine, etc.). An exact "is this bot visible right now" check is a game trace,
  which only internal code can call. External uses the **spotted-by heuristic** instead (see the feasibility table
  below); an own ray cast against the map's collision mesh, parsed from the game files, would be external too but is
  a big separate project, not planned.
- No sub-tick angle writes.
- If the user ever wants those, **that's a separate Phase 12+ decision** with a full re-scope, not a silent slide
  into internal.

**Feasibility check of the features the user asked for on 2026-10-06** (all external, nothing internal needed):

| Feature | External? | How (fields from build 14189's dump, re-verified in Phase 2) |
|---|---|---|
| ~~Bunny hop~~ (dropped 2026-10-07) | Yes | Each frame while space is held: read the local pawn's `m_fFlags` (`C_BaseEntity` `0x3F4`, bit 0 = `FL_ONGROUND`). On landing, press jump by writing the `jump` button state (`buttons.json`, client.dll `+0x22324E0`); fallback `SendInput`. Needs a write handle. Timing depends on our loop rate vs the 64-tick server: a vsync-limited loop can miss hops, so Phase 6 may need a faster polling path |
| ESP: scoped indicator | Yes | One byte per bot: `C_CSPlayerPawn::m_bIsScoped` (`0x1EA0`); the weapon name (Phase 3) says which gun |
| Visibility check (aimbot, ESP, triggerbot) | **Heuristic only** | Exact line of sight = the game's trace = internal. External: `C_CSPlayerPawn::m_entitySpottedState` (`0x1E88`) → `EntitySpottedState_t::m_bSpottedByMask` (`+0xC`), a bit per player slot that the game itself sets when that player has line of sight (it feeds the radar). In an offline match our own client runs the server, so it's kept up to date, but it lags a little and is a "spotted" flag, not a per-frame ray |
| Triggerbot (configurable) | Yes | Crosshair target: local `C_CSPlayerPawn::m_iIDEntIndex` (`0x36CC`, the entity under the crosshair). Fire by writing the `attack` button state (`buttons.json` `+0x2231FD0`) or `SendInput`. Flash: `C_CSPlayerPawnBase::m_flFlashDuration` (`0x1510`). Air: `m_fFlags`. Scoped: local `m_bIsScoped`. Head only needs the bone array (§7) |

**Reference projects (read-only, for understanding, not for copying whole):**
- **Osiris** (public CS2 internal): study the entity system walk, schema lookups, interface names, bone matrix
  reading. **The technique transfers even if the injection doesn't.** Write my own code.
- **`a2x/cs2-dumper`** (offset dumper): the authoritative source for `dwEntityList`, `dwLocalPlayerController`,
  `dwLocalPlayerPawn`, `dwViewMatrix`, `dwGameRules`, `dwGlobalVars`. Subscribe to it.
- **`sezzyaep/CS2-OFFSETS`** (offset mirror): second source to cross-check a2x.
- **`xsip/cs2-sdk-offsets-interfaces-dump`**: schema field offsets and interface names/vtables. The one to use
  for inside-entity offsets.
- **`scros22/cs2-universal-offsets`**: a newer external SDK generator with a live web browser at `cs2-sdk.com`.
- **`Leksa667/cs2-lks-dumper-cpp`**: a C++ port of the dumper if you want to modify the dumper itself.
- **The AC 1.3.0.2 internal project** (this same user's earlier repo): the architecture, module split, test
  strategy, keybind engine, and JSON profile shape carry over. Reuse the *patterns*, not the code.

**Everything stays offline. `-insecure` only. No VAC servers. Ever.**

---

## 4. Tech stack and build configuration

| What | Choice |
|---|---|
| IDE / compiler | MSVC toolset **v143** (VS 2022's toolset), built with **Visual Studio Community 2026** |
| Language | **C++20** (`/std:c++20`) |
| Platform | **x64 ONLY.** CS2 is 64-bit. There is no x86 configuration, on purpose |
| Projects | `external` (console exe → `cs2_external.exe`), `tests` (→ `tests.exe`), `vendor` (static lib → `vendor.lib`) |
| CRT | **Static** (`/MTd` Debug, `/MT` Release): no VC++ runtime DLL needed |
| UI | **Dear ImGui v1.92.x** with the **Win32 + DX11** backends (for the overlay's own window), vendored under `external/imgui/` |
| Overlay window | A transparent, click-through, topmost Win32 window with its own DX11 swap chain. **No DLL injection, no hooking of the game's Present.** We render our own window on top of the game |
| Memory access | `ReadProcessMemory` / `WriteProcessMemory` from a user-mode handle |
| Tests | `tests` console exe compiling the pure modules; **doctest v2.5.3** (header-only, `external/doctest/`) |
| JSON | **nlohmann/json v3.12.0** (single header, `external/nlohmann/`) |

**Don't add dependencies without asking the user first.** Vendored code lives in `external/`. Every vendored library
gets a line in `external/README.md` (version, licence, source tag, blob hash).

### Build configuration (`props/common.props`, imported by every project)
- `/std:c++20`, `/W4`, `/WX` (warnings are errors), linker `/WX`, `/sdl`, `/permissive-`, `/utf-8`, `/Zc:__cplusplus`,
  `/EHsc`, `/MP`.
- Defines: `WIN32`, `_WIN64`, `WIN32_LEAN_AND_MEAN`, `NOMINMAX`, plus `_DEBUG` / `NDEBUG`. Character set: Unicode.
- Output: `bin\<Configuration>\`. Intermediates: `obj\<Project>\<Configuration>\`. Both git-ignored.
- `WindowsTargetPlatformVersion` = `10.0` (latest installed Windows SDK; no SDK version pinned).
- **Import order matters:** `common.props` is imported in the `PropertySheets` ImportGroup **after**
  `Microsoft.Cpp.props` (otherwise the toolset defaults override it). `WindowsTargetPlatformVersion` and
  `CharacterSet` must be set **before** `Microsoft.Cpp.props`, so those two (and only those two) live in each
  `.vcxproj`. Copy that layout for every new project.
- Solution platform is `x64`.
- No `.vcxproj.filters` files: use **Solution Explorer → Show All Files**.
- Change shared settings in `props/common.props`, not in individual projects.
- **ImGui config defines** live in `props/imgui.props` (imported by `vendor` and `external`), never in `imconfig.h`.
- Third-party code is compiled in its own static library project (`external/vendor.vcxproj`).

### Build and run
```powershell
# repo root. Full MSBuild path:
$msb = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
& $msb cs2-external.sln /m /nologo /v:minimal /p:Configuration=Debug   /p:Platform=x64
& $msb cs2-external.sln /m /nologo /v:minimal /p:Configuration=Release /p:Platform=x64

# unit tests (built by the solution, no game needed):
bin\Debug\tests.exe

# start CS2 first (Steam → CS2 → Properties → Launch Options):
#   -insecure -console -novid -nojoy
# Join a "Practice with Bots" match, then:
bin\Release\cs2_external.exe

# offset diagnostic only (no overlay), e.g. after a CS2 update; exit code 0 = all checks OK, 2 = a check failed:
bin\Release\cs2_external.exe --diag
```
- Run the external tool as **administrator** if `OpenProcess` fails (the game runs as admin on some setups).
- The overlay is a separate window. It sits topmost over the game and is click-through until you press INSERT.
- No DLL, no injection, no unload sequence beyond closing the exe.

### Machine notes (fill in on first session)
- **Visual Studio Community 2026** with the **Desktop development with C++** workload.
- Toolsets installed side by side: v143 and v145. The projects use **v143**.
- Windows SDK: latest installed (10.0.26100.0 or newer) via `WindowsTargetPlatformVersion 10.0`.
- **`msbuild` is not on PATH** in the user's terminal. Use the full path above (find with `vswhere`).
- **Steam** installed, CS2 installed, `-insecure` verified working (no VAC prompt, bots load).
- **a2x/cs2-dumper** run successfully. Full output (every module, in .cs/.hpp/.json/.rs/.zig) is at
  `C:\Users\Harry\Desktop\output`; only the JSON files we use are copied into `docs/dumps/`. Copy, never retype.
- **Python 3 + Pillow** (optional): only to regenerate `src/external/ui/logo_pixels.h` after changing
  `assets/logo.jpg` (`python tools/make_logo_header.py`). The generated header is committed; builds don't need Python.
- **CS2 test setup:** an offline Deathmatch against bots with the bots frozen (they can't shoot), round time 60 min.

---

## 5. File tree

Status markers: ✅ exists, 🔲 planned (phase number in brackets).

```
cs2-external/
  CLAUDE.md                       ✅ this file: project memory and rules
  README.md                       ✅ the educational README (Phase 11): screenshots, features, build/run/use,
                                     troubleshooting, tests, layout, "How it works" in 20 sections, credits
  .gitignore                      ✅ VS/C++ build output, .vs, local profiles/logs
  .gitattributes                  ✅ CRLF for VS files, text normalisation
  cs2-external.sln                ✅ solution: Debug|x64, Release|x64 (external, tests, vendor)
  assets/
    logo.jpg                      ✅ the logo (README, and the source of ui/logo_pixels.h)
    logo.ico                      ✅ the exe's icon (32x32)
  tools/
    make_logo_header.py           ✅ assets/logo.jpg → src/external/ui/logo_pixels.h (128x128 RGBA, needs Pillow)
  props/
    common.props                  ✅ shared build settings (C++20, /W4 /WX, /MT, output dirs); import AFTER Cpp.props
    imgui.props                   ✅ ImGui config defines + external include paths (vendor + external import it)
  docs/
    DEVLOG.md                     ✅ dated log
    offsets.md                    ✅ running notes on every offset, signature, schema field, with proof
    screenshots/                  ✅ the README's pictures (Phase 11): 2 in-game ESP shots, the menu over the game,
                                     7 menu pages + the Violet theme (cropped PNGs), watermark + radar crops
    dumps/
      info.json                   ✅ a2x/cs2-dumper run info: CS2 build 14189, 2026-10-06T16:04:45Z
      offsets.json                ✅ a2x/cs2-dumper output (module globals, 29 entries)
      client_dll.json             ✅ a2x/cs2-dumper output (542 schema classes, 14 enums)
      interfaces.json             ✅ a2x/cs2-dumper output (111 interfaces across 31 modules)
      buttons.json                ✅ a2x/cs2-dumper output (16 button globals)
  external/
    README.md                     ✅ what's vendored, versions, licences, blob hashes
    vendor.vcxproj                ✅ static lib (vendor.lib) compiling Dear ImGui, warnings off
    imgui/                        ✅ Dear ImGui v1.92.9b core + Win32 + DX11 backends (15 files, hashes in README)
    doctest/                      ✅ doctest v2.5.3 (doctest.h + LICENSE.txt)
    nlohmann/                     ✅ nlohmann/json v3.12.0 (json.hpp + LICENSE.MIT, hashes in README)
  profiles/
    default.json                  ✅ mirror of the built-in "default" (= code defaults, tested); user profiles live
                                     next to the exe (bin\<Config>\profiles\), git-ignored
  src/
    external/
      external.vcxproj            ✅ console exe → bin\<Config>\cs2_external.exe, x64, static CRT
      external.rc                 ✅ the exe icon (assets/logo.ico)
      main.cpp                    ✅ bootstrap: DPI awareness, console Ctrl handler, find cs2.exe → read-only handle
                                     → print PID, module bases, local pawn → offset diagnostic → app::run
                                     (`--diag`: exit after the diagnostic; exit code 0 = all OK, 2 = a check failed;
                                     `--live`: the console live view instead of the diagnostic and overlay)
      core/
        process.h/.cpp            ✅ UniqueHandle, find_process, open_handle, module_base (Toolhelp32),
                                     find_main_window (EnumWindows), is_running
        memory.h                  ✅ PURE: is_plausible_pointer/range + core::Memory interface (read_bytes,
                                     safe_read<T>, read<T>, write_bytes, safe_write<T>) + read_string (page-bounded)
        process_memory.h/.cpp     ✅ ProcessMemory : Memory, the only RPM/WPM calls (__try/__except-guarded)
        pattern.h/.cpp            ✅ PURE: Pattern (IDA style: parse, matches_at, find_all), RemoteCopy + copy_remote
                                     (chunked, unreadable pages zero-filled), rip_relative / rip_relative_target
        pe.h/.cpp                 ✅ PURE: a loaded module's PE headers, sections and exports via core::Memory
        log.h                     ✅ logger::info/warn/error (std::format, stdout)
        runtime.h                 ✅ shutdown_requested / shutdown_complete (set by the console Ctrl handler)
      game/                       (THE ONLY place that dereferences game memory)
        offsets.h                 ✅ PURE: all 29 dumped globals, 16 buttons, 4 interface RVAs, kDumpBuildNumber,
                                     8 signatures (Signature + signatures::kClient), hand-found layouts (layout::)
        schema.h                  ✅ PURE: 44 field offsets in 17 classes (from dumps/client_dll.json) + kFields (43:
                                     CEntityInstance::m_pEntity isn't live-schema checkable)
        interfaces.h/.cpp         ✅ PURE: CreateInterface from outside (export → InterfaceReg list walk, lea/ret
                                     create functions decoded, not called)
        schema_system.h/.cpp      ✅ PURE: find_type_scope (SchemaSystem_001), index_classes (self-pointing class
                                     infos in a client.dll copy), read_class (live fields)
        signatures.h/.cpp         ✅ PURE: resolve_signature over a module copy (several hits must agree)
        snapshot.h                ✅ PURE data: Team, ObserverMode + watches_target, PlayerSnapshot (controller + pawn
                                     copy, pawn_index, eye_angles, observer mode/target while dead, eye_position,
                                     head_position, on_ground), LocalState (crosshair entity, flash, view angles),
                                     PlantedBomb, GameSnapshot (globals, view matrix if sane, players, bomb, local())
        player.h/.cpp             ✅ read_local_pawn; read_player (controller → pawn, validity checks: garbage pawn
                                     dropped); read_game (entity system, globals, view, every controller)
        entities.h/.cpp           ✅ designer_name, find_player_controllers (indices 1..maxClients,
                                     "cs_player_controller")
        bomb.h/.cpp               ✅ read_bomb (dwPlantedC4 → C_PlantedC4, handle round-trip guard), read_planted_bomb,
                                     read_bomb_planted (game rules, for --diag)
        handle.h/.cpp             ✅ read_entity_system, identity_address, entity_at, resolve_handle (serial check)
        view.h/.cpp               ✅ read_view_matrix (client.dll + dwViewMatrix, inline 4x4)
        bones.h/.cpp              ✅ read_bones: scene node + m_modelState + 0x80 → 23 bone positions in one read,
                                     garbage rejected (non-finite, > 200 units from the feet)
        visibility.h/.cpp         ✅ player_slot, is_spotted_by (bit per slot), read_spotted_by_mask
        observer.h/.cpp           ✅ read_observer: controller → m_hObserverPawn → observer services → mode + target
        writes.h/.cpp             ✅ the only game writes: set_button (65537 / 256), read/write_view_angles
        weapon.h/.cpp             ✅ read_active_weapon_id (pawn → services → handle → 0x149A), weapon_info (id →
                                     name + WeaponClass), weapon_class_name
        globals.h/.cpp            ✅ read_globals (one read: realtime, framecount, maxClients, interval, curtime,
                                     tickcount, map name), is_sane
      maths/                      (PURE)
        vec.h                     ✅ Vec2, Vec3 (ops, dot, length, distance, is_finite), units_to_metres
        angles.h/.cpp             ✅ Angles, normalize, calc_aim_angles, angular_distance, smoothing_fraction,
                                     step_towards, forward, distance_to_ray
        projection.h/.cpp         ✅ ViewMatrix (row-major 4x4, at, is_sane), world_to_screen (w < 0.01 rejected)
        skeleton.h/.cpp           ✅ bone indices (proven live; kHeadCentre = 7 is "the head"), kSkeletonLinks (17),
                                     project_skeleton
      features/                   (PURE: data in, decisions out)
        esp.h/.cpp                ✅ player_box, health_colour, is_enemy, display_name, distance_text, build_esp →
                                     primitives (boxes/corners, outline, head circle, skeleton, health bar/number,
                                     name, weapon, distance, SCOPED, snaplines, visible/hidden colours)
        targeting.h/.cpp          ✅ is_enemy, is_live_target, is_visible_to, distance_metres, within_distance (shared)
        aimbot.h/.cpp             ✅ aim_point, find_candidates, select_target, compute_aim, fov_circle
        triggerbot.h/.cpp         ✅ trigger_target, trigger_block (reasons), Triggerbot state machine
        feature_summary.h/.cpp    ✅ ActiveFeatures + feature_summary() → "ESP · Aimbot" for the watermark
        radar.h/.cpp              ✅ radar_panel (corner), radar_offset / radar_direction (world → radar, rotated or
                                     north-up), clamp_to_square, build_radar → primitives (dots, facing, names, edge)
        bomb_timer.h/.cpp         ✅ bomb_timer_info (phase, time left, defuse verdict, defuser, distance),
                                     build_bomb_timer → the top-centre panel
        spectators.h/.cpp         ✅ watched_pawn (you, or whom you watch while dead), spectator_info (dead players in
                                     1st/3rd person on it), build_spectator_list → a side panel
      render/
        primitives.h              ✅ Line, Rect, FilledRect, Circle, FilledCircle, FilledTriangle, Text (+ TextAnchor),
                                     Primitive variant (PURE)
        painter.h/.cpp            ✅ paint(draw list, primitives, font, size): ImGui, text with a shadow
        panel.h/.cpp              ✅ PanelWriter (PURE): rows, bars with marks, background + border (bomb timer,
                                     spectator list)
      input/
        keys.h/.cpp               ✅ PURE: every bindable VK ↔ name (Mouse 1-5, letters, digits, F1-F24, numpad, L/R
                                     modifiers...), KeySet, vk_from_name, is_mouse_button, is_modifier, can_be_hotkey
        actions.h/.cpp            ✅ PURE: BindMode, Bind, ActionId registry (constexpr, id order) + defaults,
                                     categories (+ Presets), allowed modes, key_allowed (the menu key: keyboard, never
                                     unbound); ActionDef::key is the profile id
        keybinds.h/.cpp           ✅ PURE: KeyFrame (held + press counts), KeybindEngine (HOLD/TOGGLE/PRESS by press
                                     count, priming, suspension), presses_from_edges (fallback), find_conflicts
        key_poll.h/.cpp           ✅ GetAsyncKeyState polling (poll_keys, is_key_down): the held keys
        key_tracker.h/.cpp        ✅ PURE: KeyTracker counts presses from raw input events (auto-repeat filtered),
                                     raw_keyboard_vk (L/R Shift/Ctrl/Alt)
        bind_capture.h/.cpp       ✅ PURE: BindCapture (release wait, next key, Esc clears / cancels, timeout)
      settings/
        settings.h                ✅ Settings { overlay (menu theme + accent, watermark, outline), general (team mode),
                                     esp, aimbot, triggerbot, radar, bomb_timer, spectators, keybinds } + enums; every
                                     struct has a defaulted ==
        themes.h/.cpp             ✅ PURE: MenuTheme (Midnight, Black, Graphite, Violet, Ice) + ThemeColours table,
                                     theme_name, contrast_ratio (WCAG), text_on (text colour on an accent fill)
        profile_json.h/.cpp       ✅ PURE: Settings <-> JSON text: one field list per section (nested: colours,
                                     weapons) for write + read; forgiving load (warnings, clamping), schema_version
        profile_store.h/.cpp      ✅ ProfileStore(folder): clean_name, list ("default" first), load / save (atomic:
                                     .tmp + replace) / rename / remove, .last_profile, load_startup (std::filesystem)
        presets.h/.cpp            ✅ PURE: Preset Off/Chill/Medium/Rage + apply_preset (features and strengths only)
      ui/
        imgui_layer.h/.cpp        ✅ ImGuiLayer: context, fonts, logo texture, Win32 + DX11 backends;
                                     imgui_message_hook
        logo_pixels.h             ✅ GENERATED (tools/make_logo_header.py): the logo as 128x128 RGBA pixels
        overlay_window.h/.cpp     ✅ transparent, click-through, topmost window + own D3D11 device/swap chain; covers the
                                     game's client rect; the menu hotkey (the bound key); raw input (keyboard +
                                     mouse, background) → take_presses; set_interactive
        theme.h/.cpp              ✅ Palette (theme colours + accent, derived hover/soft/on-accent), use_theme (rebuilds
                                     palette + ImGui colours when the theme or accent changes), apply_style, Fonts
                                     (regular, semibold, bold), scaled()
        icons.h/.cpp              ✅ tab icons drawn with the draw list (home, crosshair, bolt, ESP box, radar,
                                     keyboard, gear)
        widgets.h/.cpp            ✅ Columns (two columns of panels, one under 720 px), page_intro, panel_begin/end
                                     (title + main switch), subheading; rows (label + "?" left, control column right):
                                     switch / slider / choice (segmented or dropdown) / colour / info; toggle_switch
                                     (animated), segmented, chips, button (normal/accent/danger), help_marker, hint,
                                     notice, pill, dot, image_rounded
        menu.h/.cpp               ✅ header (logo, name, "External" pill, close button) + icon tabs (green dot = that
                                     feature is on; icons only for unselected tabs when narrow) + current page;
                                     MenuState (page, Profiles selection / name box / delete + exit confirmations)
        hud.h/.cpp                ✅ watermark (logo + active features) + frame outline, background draw list
        keybind_widgets.h/.cpp    ✅ key_button (capture, conflict colours), mode_selector (Hold | Toggle segmented),
                                     bind_row (a row: key + mode in the control column)
        pages/                    ✅ pages.h + one file per page, each two columns of panels: home (match status line;
                                     Features switches, Presets; Match, Tool, Keys), aimbot / triggerbot (main switch in
                                     the panel title, every option, live status), esp, misc (radar + colours, bomb
                                     timer, spectator list), keybinds (every action by category, conflicts, reset),
                                     settings (Profiles; Appearance: theme tiles + accent; Overlay; Exit);
                                     controls.h/.cpp: choice<Enum>, team_mode_row, max_distance_row
      color.h                     ✅ Color (RGBA floats): rgb(0xRRGGBB), rgba(0xRRGGBBAA) / to_rgba, faded, lerp
      config.h                    ✅ PURE: branding, pointer bounds, page size, process/module names, --diag flag and
                                     scan limits, bind capture timeout, menu sizes, timings; [5+] Range<T> + every
                                     setting range
      app/                        (the orchestrator, from AC: the only place that wires everything together)
        diagnostics.h/.cpp        ✅ the startup offset diagnostic: build, interfaces, signatures, schema, buttons/
                                     globals → console + OffsetReport for the Home page
        frame.h/.cpp              ✅ app::run: loads the last profile, then the loop (errors caught: shutdown still runs) (pump → game window/focus →
                                     menu key → keybinds (capture, engine, panic/exit/on-off/presets) → requests
                                     (profile operations, presets) → snapshot → aimbot write → triggerbot attack write
                                     → draw → present) and the overlay's teardown
        state.h                   ✅ AppState (game info, offset report, settings, ProfileState + unsaved_changes,
                                     Requests (profile op, preset), active features, aim/trigger status, menu open,
                                     bind capture, match status, game snapshot, overlay size/FPS)
        live_view.h/.cpp          ✅ `--live`: console table (~4 Hz) of globals, view matrix, every player, read time;
                                     redraws in place in a console, plain frames when redirected
  tests/
    tests.vcxproj                 ✅ console exe (doctest) → bin\<Config>\tests.exe
    main.cpp                      ✅ DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
    helpers/fake_memory.h         ✅ FakeMemory : core::Memory (mapped regions at fake addresses, read/write counts,
                                     put_bytes/put_string)
    helpers/fake_pe.h             ✅ map_fake_pe: a tiny PE32+ (headers, .text/.rdata, exports) in a FakeMemory
    core/test_memory.cpp          ✅ pointer checks + Memory's typed wrappers + read_string (via FakeMemory)
    core/test_process_memory.cpp  ✅ ProcessMemory RPM/WPM against our own process (uncommitted page, partial read)
    helpers/fake_entities.h       ✅ FakeEntityList: a chunked entity system (identities, handles, designer names)
    game/test_player.cpp          ✅ read_local_pawn, read_player (alive/dead/no pawn/garbage/stale), read_game
    game/test_entities.cpp        ✅ designer_name, find_player_controllers (gaps, other entities, max_clients clamp)
    game/test_handle.cpp          ✅ handle bits, entity_at across chunks, resolve_handle (serial, reuse, invalid)
    game/test_globals.cpp         ✅ read_globals (live values), is_sane
    game/test_view.cpp            ✅ read_view_matrix (live values), is_sane
    game/test_weapon.cpp          ✅ weapon_info table, read_active_weapon_id chain
    maths/test_vec.cpp            ✅ Vec2/Vec3 ops, is_finite, units_to_metres
    core/test_pattern.cpp         ✅ pattern parse, matches/find_all, copy_remote with a hole, rip_relative
    core/test_pe.cpp              ✅ headers, sections, exports (missing, prefix, forwarded, x86 rejected)
    game/test_interfaces.cpp      ✅ InterfaceReg walk, lea/ret decode, bad prologue, loop limit, broken link
    game/test_schema_system.cpp   ✅ type scope lookup, class index (module filter, duplicates), read_class
    game/test_signatures.cpp      ✅ one hit, agreeing/disagreeing hits, add, section bounds, bad pattern
    game/test_offsets.cpp         ✅ table sanity: signatures parse + wildcard their disp32, no duplicates, derived
                                     values (pawn = prediction + 0xF8, weapon chain = 0x149A)
    maths/test_angles.cpp         ✅ wrap/clamp, aim angles vs forward, short-way deltas, smoothing at any frame rate
    maths/test_projection.cpp     ✅ synthetic camera (centre, right, up, behind) + the live matrix (crosshair, a bot's
                                     feet, transpose is wrong)
    maths/test_skeleton.cpp       ✅ links valid, projection with ends behind the camera
    game/test_bones.cpp           ✅ read_bones layout, garbage rejected
    game/test_visibility.cpp      ✅ slot bits across both halves, read_spotted_by_mask
    features/test_esp.cpp         ✅ box geometry, health colour, team modes, skips, visibility colours, labels, styles
    features/test_aimbot.cpp      ✅ targeting, aim points (bone 7), candidates/filters, priorities, smoothing, FOV circle
    features/test_triggerbot.cpp  ✅ target filters, block reasons, state machine timings (single/burst/hold)
    game/test_writes.cpp          ✅ button values, view angle read/normalized write (FakeMemory)
    helpers/fake_game.h           ✅ hand-built snapshots (you at the origin looking along +x)
    features/test_feature_summary.cpp ✅ the watermark's feature line (empty, one, order, all)
    features/test_radar.cpp       ✅ corners, world → radar (rotated / north-up), directions, edge clamping, filters,
                                     colours, facing lines, names
    features/test_bomb_timer.cpp  ✅ defuse verdict, phases, time left, defuse in time / late, defuser, panel rows
    game/test_bomb.cpp            ✅ game rules flag, C_PlantedC4 read + garbage, read_bomb (none, live, stale pointer)
    game/test_observer.cpp        ✅ read_observer (in-eye, death cam, stale target, bad mode, broken chain), watches_target
    features/test_spectators.cpp  ✅ who counts as a spectator, the dead-local path, panel rows / sides / empty / hidden
    settings/test_settings.cpp    ✅ defaults inside their ranges (ESP, aimbot, triggerbot, radar, bomb timer,
                                     spectators), keybind defaults, config::Range
    settings/test_profile_json.cpp ✅ round trips (defaults, everything customised), format, invalid JSON, missing /
                                     unknown / wrong-typed / out-of-range values, colours, keybinds, schema version,
                                     profiles/default.json == code defaults (writes default.json.expected on mismatch)
    settings/test_profile_store.cpp ✅ names, save/list/load, overwrite, read-only default, missing/broken/warnings,
                                     rename/delete (+ last marker), last profile + startup fallback (temp folder)
    settings/test_themes.cpp      ✅ WCAG contrast reference values, every theme dark + readable (text, dim, faint,
                                     accent, status, text on the accent), defaults, out-of-range theme, text_on
    settings/test_presets.cpp     ✅ untouched parts (keybinds, colours, team options, distances, positions), ranges,
                                     Off / Chill / Medium / Rage values
    input/test_keys.cpp           ✅ key table (sorted, unique, no Escape / generic modifiers), names, round trip,
                                     mouse / modifier / hotkey rules
    input/test_keybinds.cpp       ✅ registry (order, ids, defaults), modes/keys allowed, engine (priming, press, hold,
                                     toggle by press count, menu open, capture, unbound), conflicts, key frames
    input/test_bind_capture.cpp   ✅ release wait, lowest key, Escape clears, menu key rules, cancel/move/timeout
    input/test_key_tracker.cpp    ✅ raw keyboard → VK (L/R modifiers, fake keys), auto-repeat, several taps per frame,
                                     mouse button flags, saturation
    test_color.cpp                ✅ rgb, lerp, faded
```

---

## 6. Architecture

### 6.1 Module responsibilities and dependency rules
- **Pure modules** (no `<Windows.h>`, no ImGui, no raw game pointers): `maths/`, `features/`,
  `render/primitives.h`, `game/snapshot.h`, `input/keys|actions|keybinds`, `settings/`, `config.h`,
  `core/memory.h` (the interface, not the RPM implementation), `core/pattern`, `core/pe`, `game/offsets.h`,
  `game/schema.h`, `game/interfaces`, `game/schema_system`, `game/signatures`, `game/handle`, `game/entities`,
  `game/player`, `game/globals`, `game/view`, `game/weapon`. They take plain data (or a
  `core::Memory&`) and return plain data, so the `tests` project can compile and test them without the game.
- **`core/`** owns the process handle, memory read/write, pattern scanning, and logging. **No DLL lifetime code**
  (there is no DLL).
- **`game/`** is the only code that reads or writes game memory. It turns remote pointers into `PlayerSnapshot`
  copies and applies writes that features asked for.
- **`game/interfaces` + `game/schema`** are the only code that resolves interfaces and schema field offsets
  remotely.
- **ImGui** is included only in `ui/` and `render/painter`. The UI **never touches game memory**: it edits
  `Settings` and queues requests, and the main loop applies them.
- **`main.cpp`** is only the bootstrap: find the process, open the handle, build `core::ProcessMemory`, hand over to
  `app/`, and handle shutdown. (Phase 0 does its single read straight from `main.cpp`.)
- **`app/frame`** (from Phase 1, same role as in the AC project) is the orchestrator: the only place that wires
  input, settings, game reads, features, writes and drawing together, one frame at a time.

### 6.2 Threading model
| Thread | Who | Allowed to |
|---|---|---|
| Main thread | ours (`main.cpp`) | everything: process handle, game reads, feature logic, overlay rendering, keybinds |

- **External has one thread.** No DLL lifetime problem, no hook in-flight problem, no cross-thread state to
  synchronise. This is the single biggest engineering simplification over internal.
- The game runs on its own threads; we only read its memory from our process. We never call into the game.
- The overlay window has its own message pump on the main thread (standard Win32). ImGui is driven from there.

### 6.3 Data flow (one frame, Phase 1+)
```
main loop (60+ Hz, or vsync-limited):
  1. pump Win32 messages for the overlay window
  2. keybinds: held keys (GetAsyncKeyState) + press counts (raw input, pumped in 1) → engine → ActionStates
  3. read:    game/ → GameState {local PlayerSnapshot, players, view matrix, globals} (copies)
              (all reads go through core::Memory; the real one is ProcessMemory, __try/__except-guarded)
  4. features (pure): aimbot → angles (write via WriteProcessMemory) · esp → primitives
  5. apply:   game/ writes
  6. draw:    ImGui NewFrame → render/painter(primitives) on background draw list → ui::menu (if open)
              → ImGui Render → ImGui_ImplDX11_RenderDrawData on the overlay's swap chain
  7. Present the overlay swap chain
```

### 6.4 Shutdown (no unload sequence)
1. User presses the exit key (DELETE), clicks Exit (Settings page), presses Alt+F4 on the menu or Ctrl+C in the console,
   or closes the console; or CS2 closes. An error thrown inside the loop is caught and still runs steps 2-5.
2. Restore everything we changed in the game: the triggerbot lets go of `attack` (also when the overlay hides); the
   aimbot's view angle writes simply stop. Nothing else is changed because there are no patches or hooks.
3. Shut down ImGui and the DX11 overlay.
4. Close the process handle.
5. Exit. A shutdown the user asked for skips main's "Press Enter to exit" (`core::shutdown_requested`); errors and "cs2.exe
   has closed" keep it, so a double-clicked console stays readable.

**There is no trampoline, no in-flight counter, no `FreeLibraryAndExitThread`. External is dramatically simpler
to shut down safely.**

### 6.5 Settings and keybinds (same design as AC)
- One `Settings` root (pure structs): `overlay` (menu theme + accent too), `general`, `esp`, `aimbot`, `triggerbot`,
  `radar`, `bomb_timer`, `spectators`, `keybinds`.
- Numeric ranges defined once (`config.h`) and used by the UI sliders and the JSON clamping.
- Colours stored as RGBA floats in memory, `"#RRGGBBAA"` in JSON.
- Profiles live in `profiles\` next to the exe; `default` is built in.
- Keybinds: `BindMode` HOLD / TOGGLE / PRESS, keyboard + mouse buttons.
- Presets change feature switches and strengths only, never keybinds, colours or general settings.

---

## 7. Offsets, signatures and struct reconstruction

All offsets and signatures live in `src/external/game/offsets.h` and `src/external/game/schema.h`. Game:
**Counter-Strike 2**, module `client.dll` (the entity system, local player, view matrix, game rules, and global
vars are there). Some things are in `engine2.dll`.

**CS2 offsets change on every game update.** This is the single biggest difference from AssaultCube. Do not hardcode
absolute addresses for anything that changes. Use **schema fields** for inside-entity offsets and **signatures** for
module-base RVAs.

### Where the offsets come from
Do not find the stable offsets by hand. They are dumped automatically after every CS2 patch by the community:

- **`a2x/cs2-dumper`** — the reference. Subscribe to it. Its `output/offsets.json` and `output/client_dll.json`
  are the source of truth. The user has already run it and the output is in `docs/dumps/`.
- **`sezzyaep/CS2-OFFSETS`** — a mirror explicitly advertised as "always updated." Cross-check a2x against this.
- **`xsip/cs2-sdk-offsets-interfaces-dump`** — schema field offsets and interface names + vtables. The one to use
  for inside-entity fields.
- **`scros22/cs2-universal-offsets`** — a newer external SDK generator with a live web browser at `cs2-sdk.com`.

Copy the values you need into `offsets.h` and `schema.h`, and **log the source (repo + commit + date) next to each
one** so the provenance is clear.

### Three kinds of offsets

1. **Stable module RVAs:** `dwEntityList`, `dwLocalPlayerController`, `dwLocalPlayerPawn`, `dwViewMatrix`,
   `dwGlobalVars`, `dwGameRules`, `dwPlantedC4`, `dwCSGOInput`. These come straight from the dumper. They change
   on updates but slowly.
2. **Schema field offsets:** the offsets of fields *inside* `C_CSPlayerPawn` (health, team, position, bone matrix
   pointer, etc.). These move the most, and they are resolved from `client_dll.json` (or read at runtime from the
   game's own schema system, if the remote walk is viable).
3. **Signature-scanned RVAs:** store an IDA-style byte pattern in `offsets.h`, scan `client.dll` for it, get the
   address. Survives almost every update unless the pattern itself changes. Use this for anything the dumpers
   don't cover (most notably the bone array pointer — see below).

### Offset change rule
You may change an offset **only if** you:
1. **Prove it first** with a diagnostic (in-game snapshot diff, Cheat Engine, x64dbg, or a new dumper run) showing
   the old value is wrong and the new one is right. If the proof needs the user in-game, write the diagnostic and
   ask them to run it.
2. **Keep the old value as a comment** next to the new one, plus the date and the dumper commit it came from.
3. **Record** old value, new value, evidence and reason in the Decision log (§15) and `docs/offsets.md`.
4. **Flag it** at the very top of the phase summary as **"OFFSET CHANGED"**.

**Never change an offset based on a guess alone.**

### What the dumper gives you (already in `docs/dumps/`)
- `offsets.json`: the module globals. 29 offsets across `client.dll`, `engine2.dll`, `inputsystem.dll`,
  `matchmaking.dll`, `soundsystem.dll`. Every `dw*` symbol you'll reference.
- `client_dll.json`: 542 classes and 14 enums (build 14189), with every field offset. This is where health, team, position,
  eye angles, weapon handle, player name, and everything else lives.
- `interfaces.json`: 111 interfaces across 31 modules, with their module and vtable RVA.
- `buttons.json`: 16 button flags (attack, jump, duck, etc.).

### Fields and buttons the planned features need (from build 14189's dump, copied into code in Phase 2)
| Name | Where | Dump value | Used by |
|---|---|---|---|
| `m_iHealth` | `C_BaseEntity` | `0x34C` | ESP, aimbot, triggerbot |
| `m_fFlags` | `C_BaseEntity` | `0x3F4` | triggerbot (in air) |
| `m_vecVelocity` | `C_BaseEntity` | `0x430` | (unused since bunny hop was dropped) |
| `m_bIsScoped` | `C_CSPlayerPawn` | `0x1EA0` | ESP scoped indicator, triggerbot "only when scoped" |
| `m_entitySpottedState` | `C_CSPlayerPawn` | `0x1E88` | visibility heuristic |
| `m_bSpottedByMask` | `EntitySpottedState_t` | `+0xC` | visibility heuristic (bit per player slot) |
| `m_iIDEntIndex` | `C_CSPlayerPawn` | `0x36CC` | triggerbot (entity under the crosshair) |
| `m_iShotsFired` | `C_CSPlayerPawn` | `0x1EB4` | triggerbot burst counting |
| `m_flFlashDuration` | `C_CSPlayerPawnBase` | `0x1510` | triggerbot "don't fire while flashed" |
| `jump` | `buttons.json`, client.dll | `0x22324E0` | (unused since bunny hop was dropped; format proven) |
| `attack` | `buttons.json`, client.dll | `0x2231FD0` | triggerbot |
| `dwPlantedC4` | client.dll → `C_PlantedC4*` | `0x24CA930` | bomb timer (0 = no bomb) |
| `m_pEntity` | `CEntityInstance` | `0x10` | bomb timer stale-pointer guard (the identity) |
| `m_bBombPlanted` | `C_CSGameRules` (via `dwGameRules`) | `0x8C7` | `--diag` cross-check |
| `m_hObserverPawn` | `CCSPlayerController` | `0x930` | spectator list (controller → observer pawn) |
| `m_pObserverServices` | `C_BasePlayerPawn` (read on the observer pawn) | `0x1308` | spectator list |
| `m_iObserverMode`, `m_hObserverTarget` | `CPlayer_ObserverServices` | `0x48`, `0x4C` | spectator list (2 = in-eye, 3 = chase; target = a player pawn) |
| `m_flC4Blow`, `m_nBombSite`, `m_bBeingDefused`, `m_flDefuseCountDown`, ... | `C_PlantedC4` | `0x12B8`, `0x128C`, `0x12C4`, `0x12D8` | bomb timer (all 10 in `schema.h`) |

Writing a button global is a game write: it needs `PROCESS_VM_WRITE | PROCESS_VM_OPERATION`, added in Phase 5 (with
the aimbot's angle writes). The exact value format of a button write is verified in-game before it's used.

### What the dumper does NOT give you (must be found by hand)
1. **The bone array pointer.** Not in the schema. **Found in Phase 4** (build 14189, `offsets::layout::kModelStateBones`,
   `game/bones`, proof in `docs/offsets.md` "Bones"): pawn → `m_pGameSceneNode` (a `CSkeletonInstance`) +
   `m_modelState` (`0x140`, schema) + **`0x80`** → array of 32-byte bones (position, scale, rotation). Joint indices
   (`maths/skeleton.h`) are **not** the commonly published ones: head joint 6 (the jaw; **"the head" is bone 7**, the
   middle of the head, proven 2026-10-07), neck 5, spine 4/3/2, pelvis 1, arms 9-11 and
   13-15, legs 17-19 and 20-22. `--diag` checks the head bone height on every alive player. How it was originally
   meant to be found (kept for the next time it moves):
   - **Cheat Engine:** get a pawn, follow `pawn + m_pGameSceneNode`, walk forward into the model state, look for a
     pointer to a block of `matrix3x4a_t` values whose first entry tracks the pawn's world position as the pawn
     moves. Once found, record the byte offset in `docs/offsets.md` with the build number.
   - **Pattern scan:** find the function in `client.dll` that computes bone matrices or reads the array, and read
     the offset from the instruction.
   For Phase 4, skip bones entirely. Add them later. A box ESP with name, health, team, distance is a working ESP.
2. **`CGlobalVars` internal layout.** Proven live in Phase 3 (build 14189, `offsets::layout::kGlobals*`): `0x00`
   realtime, `0x04` framecount, `0x10` maxClients (int, 64), `0x1C` interval_per_tick (1/64), `0x30` curtime, `0x44`
   tickcount, `0x188` map name (`const char*`). The "typical" layout this file used to give (curtime `0x10`,
   maxClients `0x18`) is wrong for this build.
3. **Entity list iteration logic.** Chunks are 512 entries each; `entity_list + 0x10 + (index >> 9) * 0x8` is the
   chunk pointer; `chunk + (index & 0x1FF) * 0x70` is the entity identity; `identity + 0x0` is the entity pointer;
   `identity + 0x10` holds the entity's **whole handle** (index + serial); `identity + 0x20` points to the designer
   name (`weapon_ak47`). Proven live 2026-10-06 (build 14189, `docs/offsets.md` "Entity list"): the identity is
   **0x70** bytes (this file used to say 0x78, which reads 0) and `+0x10` is the handle, not the bare serial.
   Player controllers sit at indices `1..maxClients` with the designer name `cs_player_controller` (Phase 3:
   `game/entities`, `find_player_controllers`); a pawn's `m_hController` equals its controller's own handle.

### Interfaces from outside the process (`game/interfaces`, done in Phase 2)
- Every module that registers interfaces exports its own `CreateInterface`. `core/pe` finds the export by parsing the
  remote PE's export directory; its first instruction, `mov r9, [rip + s_pInterfaceRegs]` (`4C 8B 0D disp32`), gives
  the head of that module's `InterfaceReg { create, name, next }` list, which we walk with RPM. Each create function
  is `lea rax, [rip + instance]; ret`, so the instance is decoded from the instruction instead of calling it.
- `SchemaSystem_001` is in **`schemasystem.dll`**; `Source2Client002` (client.dll), `Source2EngineToClient001`
  (engine2.dll, "EngineClient"), `InputSystemVersion001` (inputsystem.dll = `dwInputSystem`).
- **There is no `GameEntitySystem` interface in CS2.** The entity system is the global `client.dll +
  dwGameEntitySystem` (same RVA as `dwEntityList`).
- The startup diagnostic logs every interface pointer; when features use one (none yet), a null one disables them.

### Schema system from outside the process (`game/schema_system`, done in Phase 2)
- `SchemaSystem_001` + `0x190` is a `CUtlVector` of type scopes (count, then data at `+0x198`); a scope's name is a
  `char[256]` at `+0x8` (`"client.dll"`).
- Class infos (`SchemaClassInfoData`) are **static data in client.dll** and start with a pointer to themselves, so one
  pass over a copy of client.dll finds all of them (469 "client" classes in build 14189) without walking the scope's
  hash table. Layout and field array: `offsets::layout`, proof in `docs/offsets.md`.
- The schema values the code uses stay **compile-time constants** in `schema.h` (copied from the dump); the live
  schema is the startup proof, not a runtime override (see the Decision log, Phase 2).

### Handle resolution (`game/handle.h`, done in Phase 3)
Every `m_h*` field (weapons, pawns, observers, defusers) is a `CHandle` (uint32): low 15 bits are the index, high
bits are the serial. Resolution:
```cpp
index    = handle & 0x7FFF;
chunk    = read(entity_list + 0x10 + (index >> 9) * 0x8);
identity = chunk + (index & 0x1FF) * 0x70;            // an address, not a read (0x70 proven in build 14189)
entity   = read(identity + 0x0);
if ((read<uint32>(identity + 0x10) >> 15) != (handle >> 15)) return nullptr; // stale (+0x10 holds the full handle)
```

### Struct reconstruction approach
Model only what we use. Unknown bytes are explicit padding, and **every** field is either resolved from
`client_dll.json` or pinned with a `static_assert`:

```cpp
// game/structs.h
struct Vec3 { float x, y, z; };

// Accessor helpers rather than direct struct member access, because the field offsets
// are runtime values from the dump, not compile-time constants.
namespace game::fields {
    std::int32_t health(std::uintptr_t pawn) {
        return memory.read<std::int32_t>(pawn + schema::base_entity::m_iHealth); // memory: const core::Memory&
    }
    // ...
}
static_assert(sizeof(void*) == 8, "x64 only");
```
Rules:
- Never construct, copy-assign into, or `delete` a game object. Read through a validated pointer, copy into a
  `PlayerSnapshot`, write single fields back.
- Verify each schema offset once at startup; log it.
- The combined offset for a weapon's `m_iItemDefinitionIndex` from the weapon pointer is **0x149A** in build 14189
  (`m_AttributeManager` 0x1290 → `m_Item` 0x50 → `m_iItemDefinitionIndex` 0x1BA; proven live: an AK-47 reads 7; this
  file used to say 0x14FA, which reads 0). Keep the chain explicit in code so it's re-verifiable.

### Validity checks
- **Pointer sanity:** non-null, in user space (`0x10000 ≤ p < 0x7FFFFFFFFFFF`), 8-byte aligned where required.
- **Local player:** valid = valid controller pointer, valid pawn handle, finite position, sane life state.
- **Players:** valid controller + pawn, alive check (`m_lifeState == 0`), team check.
- **Entity list:** clamp iteration count; skip nulls, non-players, invalid handles, dormant entities
  (`m_bDormant`).
- All reads go through `core::Memory` (`safe_read<T>` / `read<T>`); the real implementation, `ProcessMemory`, is
  `__try/__except`-guarded. A bad read returns false, never
  crashes our process.
- **The view matrix can be all zeros** before the first frame of a match: check for sanity before projecting.

### Source 2 `Vec3` and angles
- `Vec3 { float x, y, z; }` — 12 bytes.
- The angle convention is **not AC's**. Verify yaw 0 direction, pitch sign, and yaw range in-game with a
  diagnostic before writing any aim math.
- The view matrix is **4x4 floats, row-major** in Source 2 (unlike OpenGL's column-major). **Confirmed in Phase 4:**
  `clip.x = m[0]x + m[1]y + m[2]z + m[3]`, `w = m[12]x + ... + m[15]` = distance along the view direction; a point
  along the view angles lands exactly on the screen centre (`maths/projection`, live-data unit test).
- **Angles (proven in Phase 4, for Phase 5):** `m_angEyeAngles` = `dwViewAngles` = (pitch, yaw, roll) in degrees;
  pitch **positive = looking down**; yaw from +x towards +y (counter-clockwise seen from above); forward =
  (cos p · cos y, cos p · sin y, −sin p). World: x/y horizontal, z up, 1 unit = 1 inch.

---

## 8. Current behaviour (what exists now)

*(Filled in as phases complete.)*

- **Phase 0 (done, verified in-game 2026-10-06):** `cs2_external.exe` finds `cs2.exe` (Toolhelp32), opens it with a
  **read-only** handle (`PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION`), prints the PID, the `client.dll` and
  `engine2.dll` bases and sizes, and the local pawn pointer (`client.dll + dwLocalPlayerPawn`), then waits for Enter.
  Game closed → `cs2.exe not found` (exit 1). Access denied → tells the user to run as administrator. In the main
  menu the pawn prints as `none`; a non-plausible value prints a "dwLocalPlayerPawn may be stale" warning.
- **Phase 1 (done, verified in-game by the user 2026-10-06):** after the Phase 0 printout the tool keeps running and puts
  its own transparent, topmost, click-through window over CS2's client area (it follows the game window). A watermark
  sits in the top-left corner: logo + "External Cheat by BigH", and under it the active features one per line
  (`No features on` until Phase 4). INSERT opens the menu (header: logo, name, an "External" pill; sidebar: Home / Aimbot / Triggerbot / ESP / Misc /
  Settings): the overlay takes focus so the game lets go of the mouse. INSERT again closes it and gives focus
  back. The overlay hides when neither the game nor the overlay has focus, or the game is minimised. Home shows the
  logo + name, PID, module bases, match status, active features (local pawn re-read at ~4 Hz), overlay size and FPS. Settings has live Watermark and
  Frame outline switches. The exe carries the logo as its icon. Exit: Ctrl+C / closing the console, Alt+F4 with the menu open, or CS2 closing.
- **Phase 2 (done, verified in-game 2026-10-06):** after the
  Phase 0 printout the tool runs the **offset diagnostic** and prints, as `OK` / `FAIL` lines: the game's build number
  vs the dumps' (14189); the four interfaces found by walking `InterfaceReg` lists (vs `interfaces.json`), the
  `dwInputSystem` cross-check and the entity system global; the 8 signatures scanned in a copy of client.dll's
  `.text` (vs the dump); the client type scope and all 29 `schema.h` fields read from the live schema system; then,
  informational, the `jump`/`attack` button globals and the main `dw*` pointers as they read right now. Last line:
  `all 45 checks OK` (or how many failed). Home shows an "Offsets" row with the same result. `cs2_external.exe --diag`
  runs only the diagnostic and exits (0 = all OK, 2 = a check failed).
- **Phase 3 (built 2026-10-06, auto-approved):** the tool reads the whole match: the entity list (chunked, handles
  checked by serial), every player controller (indices 1..maxClients, designer name `cs_player_controller`), each
  controller's pawn (health, armour, life state, position, eye height, flags, scoped, dormant, active weapon id → name
  and class), `CGlobalVars` (map, tick, curtime, maxClients) and the view matrix (sanity-checked). One read of 20
  players takes ~0.33 ms. `cs2_external.exe --live` shows it as a console table refreshed ~4 times a second (in place
  in a console; plain frames when redirected to a file). In the overlay, Home shows Map, Players ("20 in the match,
  20 alive") and You (name, team, HP, weapon).
- **Phase 4 (built 2026-10-06, auto-approved):** the **ESP** (menu → ESP → Enabled; off by default). Over every bot
  that is alive, not dormant, in front of the camera and within the max distance: a box (full or corners, optional
  dark outline), head circle, skeleton (from the bones), health bar and number, name, "SCOPED" when zoomed in,
  weapon, distance in metres, snaplines (from the bottom, centre or top). Team mode: Teams (enemies only, or
  teammates too in team colours) or Free for all. Colours: enemy/team × visible/hidden, from the spotted-by mask
  (visible = your slot's bit is set), each with opacity; thickness slider. Everything applies live. While the ESP is
  on, the game snapshot is read every frame (~0.4 ms). The watermark lists "ESP". `--diag` also checks CGlobalVars
  and the bones on live players (48 checks in a match). User check (2026-10-07): everything works; the head circle
  sat on the neck, fixed in Phase 5 (bone 7, radius 6.5).
- **Phase 5 (done, verified in-game by the user 2026-10-07):** **Aimbot** (menu → Aimbot → Enabled; off by default): while
  the aim key is held (Mouse 1 by default; hold or toggle; key from a short list until Phase 7) it turns the view
  towards the best bot inside the FOV (priority crosshair / distance / lowest health; aim at head = bone 7, body =
  chest, or the nearest of five bones), with framerate-independent smoothing (1 = snap), team check, max distance,
  visible only (spotted-by), and an FOV circle. It writes `dwViewAngles` every frame it has a target. **Triggerbot**
  (menu → Triggerbot → Enabled): fires while a valid enemy is under the crosshair (`m_iIDEntIndex`): activation always /
  hold / toggle, reaction delay, single tap / burst N / hold, delay between shots, team check, visible only, head only,
  max distance, weapon classes, snipers only scoped, not while flashed, not in the air; a live status line says why
  it isn't firing. It writes the `attack` button (65537 / 256) only on a change, never releases while you hold Mouse 1
  yourself, and lets go when the menu opens, the game loses focus or the tool exits. Neither acts while the menu is
  open or the game isn't in front. Team mode is shared by every feature. The overlay's handle is now read-write;
  `--diag` / `--live` stay read-only. `--diag`: 50 checks.
- **Phase 6, radar (done, verified in-game by the user 2026-10-07):** menu → Misc → Radar → Enabled (off by
  default). Our own radar, drawn by the overlay (the game's radar isn't touched, nothing is written): a see-through
  square in a corner (top-right by default; size 150-600 px, range 10-150 m), you as an arrow in the middle, every
  live player as a dot (enemies visible / hidden from the spotted-by mask, teammates in teams mode if wanted), a
  facing line per dot (`m_angEyeAngles`), optional names, out-of-range players faded on the edge (or hidden), a cross,
  a half-range ring and the range in metres. Rotate with view (where you look is up) or north-up like the game's
  radar. Its own colours, each with opacity. The watermark lists "Radar".
- **Phase 6, bomb timer (done, verified in-game by the user 2026-10-07):** menu →
  Misc → Bomb timer → Enabled (off by default). While a bomb is planted, a panel at the top-centre (height
  adjustable): "BOMB A" and the seconds left, a bar coloured by whether a defuse started now makes it (green: without a
  kit, yellow: only with a kit, red: too late) with two marks on it (yellow at 10 s left = the latest defuse without a
  kit, red at 5 s = the latest with one) and the hint as a line, and while someone defuses: their name, the
  defuse time left with a bar, "will make it" / "too late"; "DEFUSED" / "EXPLODED" at the end; your distance to the
  bomb. Read-only: `client.dll + dwPlantedC4` points at the bomb from the plant until the next round (0 otherwise),
  checked against the entity list every frame. `--diag`: 62 checks in a match, 63 while a bomb is down.
- **Phase 6, spectator list (done, approved by the user 2026-10-07):** menu → Misc → Spectator list → Enabled (off
  by default). A panel on the right (or left; height adjustable, default under the radar): "Spectators" and a count,
  then every dead player watching you in first or third person, enemies red and teammates blue, with "1st person" /
  "3rd person" (optional); "Nobody" when nobody watches (or no panel, optional). While you're dead and watch someone,
  it says "Watching Kev" and lists who else watches them. Read-only: controller → observer pawn → observer services
  (mode, target pawn), only for dead players. Bots watch their killer after a ~5 s death cam, so it fills in
  round-based modes (Casual / Competitive), hardly in Deathmatch. `--live` shows whom each dead player watches.
  `--diag`: 67 checks in a match.
- **Phase 7 (done, verified in-game by the user 2026-10-07):** every key is a **keybind**. Menu → Keybinds lists every
  action by category (General: menu INSERT, panic END, exit DELETE; Aimbot: aim key Mouse 1 hold, on/off; Triggerbot:
  trigger key Mouse 4 hold, on/off; ESP on/off; Misc: radar / bomb timer / spectator list on/off; the on/off keys are
  unbound by default). Click a key button, press any key or mouse button (Mouse 1: click outside the menu), Esc clears
  it, clicking again cancels, 6 s timeout. Hold / Toggle for the aim and trigger keys. A key on several actions turns
  red with a tooltip, and the page lists the conflicts. Each feature page has its own bind rows. The menu key can be
  any keyboard key that isn't a modifier (it stays a `RegisterHotKey`) and can't be cleared. Panic turns every feature
  off and closes the menu; exit closes the tool cleanly. While the menu is open only the menu key, panic and exit work.
  The triggerbot's activation is Always / Trigger key. Binds aren't saved yet (Phase 8). After the user's first
  check (on/off keys needed holding, spamming lost presses): presses now come from **raw input**, so every press
  counts however slow the overlay's frames are; each press flips an on/off key or a toggle once.
- **Phase 8 (done, verified in-game by the user 2026-10-07):** every setting and keybind is saved in **profiles**:
  `profiles\<name>.json` next to `cs2_external.exe` (so `bin\Debug` and `bin\Release` each have their own). On start
  the last loaded or saved profile loads (console: `Profile "x" loaded (N warnings) from ...`, each warning on its own
  line); `default` is the built-in defaults and read-only. Menu → Settings → Profiles: the list (loaded one marked,
  double-click loads), Load, Delete (asks once more), Save, Reset to defaults, a name box with Save as / Rename current,
  a result line, the loaded profile's problems, the folder. "unsaved changes" shows there and on Home. A hand-edited
  profile with bad values loads anyway: unknown keys ignored, bad values back to the default, numbers clamped, each a
  warning. **Presets** (Settings → Presets, or keys in the Keybinds page's Presets category, unbound by default): Off /
  Chill / Medium / Rage switch the features and their strength, never keybinds, colours, team mode, team checks, max
  distances or positions; the result is an unsaved change.
- **Phase 9 (done, verified in-game by the user 2026-10-07):** panic (END), the exit key (DELETE) and the final key layout were
  already done in Phase 7. New: an **Exit** card on the Settings page ("Exit the tool"; with unsaved changes it asks
  "Really exit? Unsaved changes are lost" first). Exiting on purpose (exit key, Exit button, Alt+F4) now closes the
  console too instead of waiting for Enter. An error inside the overlay loop still lets go of attack and removes the
  overlay. Panic also drops a preset or profile load clicked just before it. The console banner says "External Cheat
  by BigH" (it still said "Phase 5"). Robustness review: see `docs/DEVLOG.md` (2026-10-07, Phase 9).
- **Phase 10 (done, verified in-game by the user 2026-10-07):** the menu is redesigned. Header: logo, "External Cheat by
  BigH", the "External" pill and a close button (same as the menu key). Under it a row of **tabs with drawn icons**
  (Home, Aimbot, Triggerbot, ESP, Misc, Keybinds, Settings); a green dot on a tab means that feature is on; in a narrow
  menu only the selected tab keeps its label. Every page is **two columns of panels** (one column under 720 px): a
  feature's main switch sits in its panel's title, and every option is a row with the label (and a "?" with help) on
  the left and its control in a column that lines up on the right: switches, sliders, segmented buttons (a dropdown
  when they don't fit), colour swatches. **Home** is a dashboard: the match status, every feature's switch, the
  **presets** (moved here from Settings), Match, Tool and Keys panels. **Settings**: Profiles, **Appearance** (five
  dark themes as preview tiles: Midnight (default, black/grey/off-white), Black, Graphite, Violet, Ice; an accent
  colour picker; "Use the theme's accent"), Overlay (watermark, frame outline), Exit. The theme and accent are saved
  in profiles (`overlay.theme`, `overlay.accent`); old profiles load with the defaults. The watermark follows the theme.
- **Phase 11 (done, approved by the user 2026-10-07):** no code change. `README.md` is the full educational
  README, in the AC project's style: header, screenshots (in-game ESP, the menu over the live game, every page),
  features + what external can't do, requirements / build / run / menu / hotkeys / presets / profiles / console,
  troubleshooting, tests, layout, "How it works" (20 sections: handle, `core::Memory`, offsets, signatures,
  `CreateInterface`, schema, entity system, players and bones, projection, overlay, spotted-by, aimbot, triggerbot,
  misc, keybinds, safety, the frame, after an update, lessons per phase), credits. Pictures in `docs/screenshots/`.

---

## 9. Coding conventions

- C++20, standard library first. RAII for every handle and resource.
- Naming: namespaces `lower_case`; types `PascalCase`; functions and variables `snake_case`; constants `kPascalCase`;
  private members `trailing_underscore_`. Files `snake_case.h/.cpp`. `#pragma once`.
- Includes are relative to `src/external`: `#include "core/log.h"`. Standard headers before project headers.
- Fixed-width integers (`std::int32_t`, `std::uintptr_t`) for anything that mirrors game memory.
- **x64 pointers are 8 bytes.** Use `std::uintptr_t`, never `std::uint32_t`, for addresses.
  `static_assert(sizeof(void*) == 8)`.
- Allman braces, 4-space indent, ~120 columns.
- No magic numbers: offsets and signatures in `game/offsets.h` and `game/schema.h`, tunables/ranges in `config.h`.
- Logging with `logger::info/warn/error` only. Hot-path logging must be rate-limited.
- Simple, readable code over clever code. No macros where a `constexpr`/template/function works.
- Tests mirror the source layout.

---

## 10. Known gotchas

### External specifics
- **`ReadProcessMemory` fails silently on bad addresses.** Always check the return value and the bytes-read count.
  Wrap every read in `core::safe_read<T>` and handle the failure path.
- **Read in chunks.** Reading a large entity struct with one `RPM` call is faster than reading it field by field.
- **`RPM` is slow relative to in-process reads.** A few hundred reads per frame is fine; tens of thousands is not.
  Cache aggressively. Re-read only what changed.
- **Pointer chains are the classic external pattern:** read a base pointer, add an offset, read the next pointer,
  add another offset. Each level is a separate `RPM`. Keep the chains short.
- **The handle must have `PROCESS_VM_READ`.** For write features, add `PROCESS_VM_WRITE` and `PROCESS_VM_OPERATION`.
  Open with the minimum you need.
- **Run as administrator** if `OpenProcess` fails on `cs2.exe` (it will on some setups, because the game runs
  elevated).
- **The game may protect itself against external memory reads** in some configurations. With `-insecure`, it does
  not.
- **Don't call `ReadProcessMemory` on a page that might be freed.** A crash in the target is not your fault, but
  a crash in your process is. `safe_read` handles this.
- **RPM/WPM report a bad remote address through the return value, not an exception.** The `__try/__except` in
  `core::ProcessMemory::do_read` only protects our side of the copy. Functions containing `__try` must not hold C++
  objects with destructors (C2712), which is why the guard lives in the non-template `do_read`/`do_write`.
- **A partial read is a failed read.** `ProcessMemory` requires `bytes_read == size`; `safe_read` reads into a
  temporary, so on failure the caller's variable is untouched.
- **Implausible requests never reach RPM.** `core::Memory::read_bytes` rejects null buffers, empty sizes and ranges
  outside user space before calling the implementation (tested with `FakeMemory::read_count()`).
- **`CreateToolhelp32Snapshot(TH32CS_SNAPMODULE)` can fail with `ERROR_BAD_LENGTH`** while the game is still loading
  modules. `core::module_base` retries.
- **Scan a copy, not the remote.** `core::copy_remote` reads a whole module (client.dll: 41 MiB) in 1 MiB reads in
  ~8 ms; every pattern and the schema class index then run on the local copy. Patterns are matched only inside
  `.text` (`core::pe::find_section`), never the data sections.
- **Strings: `core::read_string`, never a fixed-size read.** A name near the end of the last mapped page would make
  a 256-byte read fail; `read_string` reads up to the page end first. In tests, map at least a page (`config::kPageSize`)
  for strings: FakeMemory regions aren't page-granular, real memory is.

### Signatures
- **A signature must never match its own displacement.** The disp32 changes every build; `test_offsets` checks that
  every signature wildcards it. Also wildcard call/jump targets (`E8`/`E9 ? ? ? ?`) and struct offsets inside
  following instructions (`41 89 BE ? ? ? ?`): both move between builds.
- **Several matches are fine if they agree.** `dwGameRules`' pattern matches twice (two copies of the same code);
  `resolve_signature` accepts that and reports `ambiguous` only when matches point at different addresses.
- **Remembered/public patterns rot.** Three of the a2x-style patterns no longer worked on build 14189. Build new ones
  from the live module: list every RIP-relative reference to the dumped global, prefer the instruction that **stores**
  the global (`mov [rip+x], reg`), and check that the pattern matches nowhere else.

### CS2 / Source 2 specifics
- **Offsets change on every update.** Subscribe to `a2x/cs2-dumper`. The moment a patch lands, the old offsets are
  wrong.
- **Schema system is your friend.** Field offsets inside entities should be resolved from the dump at runtime, not
  hardcoded ad-hoc. The dump files in `docs/dumps/` are the source of truth until the next patch.
- **`-insecure` disables VAC but not all integrity checks.** Do not join any online server with the tool running.
- **`client.dll` and `engine2.dll`** are both relevant. Load both module bases at startup.
- **Entity list is chunked**, not a flat array. Do not try to index it as `entities[i]` from a base pointer.
- **Players are two entities (controller + pawn).** Read the controller for name/team/money; read the pawn for
  health/position/bones/weapon.
- **`CHandle` is 32-bit:** low bits are the index, high bits are the serial. Validate before dereferencing.
- **The view matrix is row-major** in Source 2. Verify on this build.
- **Bone matrix layout** may be `matrix3x4a_t` or `CTransform`. Verify per update.
- **The game is not single-threaded.** You are reading its memory while it runs. A bot could be spawning or dying
  mid-read. `safe_read` and validity checks handle this; do not assume a snapshot is internally consistent across
  two separate reads (re-read the whole struct if you need consistency).
- **Never call a game function.** External has no way to do this safely. Features that need a game-thread call
  (silent aim, TraceLine) are out of scope for this project. That includes `CreateInterface`: we decode what it would
  do (`game/interfaces`) instead.
- **`dwLocalPlayerPawn` has no code reference of its own.** It's a field of the prediction object
  (`dwPrediction + 0xF8` in build 14189), so its signature finds the prediction and adds the field offset. If the
  pawn signature breaks after an update, check that offset first.
- **The dump's class count isn't the live "client" class count.** `client_dll.json` lists 542 classes because other
  libraries (entity2, pulse_runtime_lib, compositematerialslib) register theirs in the client scope; the live
  `index_classes(copy, "client")` finds 469. Classes we need come from client itself.
- **The bone indices aren't the published ones.** Public CS2 code uses legs 22-27; in build 14189 bone 27 is a look-at
  point 1000 units in front of the face, and the legs are 17-22. Indices were mapped from live positions in each
  bot's own frame (`maths/skeleton.h`). If skeletons look scrambled after an update, re-map them the same way.
- **Bone 6 is the head joint, not the head.** It sits at the base of the skull, level with the jaw (~4 units below
  the eyes). The middle of the head is bone 7 (eye height, 4.6 units forward). Aim, head circle and head-only use
  `kHeadCentre` (7); checked on a screenshot.
- **The spotted-by bit lags because the server re-checks it only every ~0.5 s** (measured 2026-10-07: server compute
  median ~250 ms, replication to the client ~1-2 ms). It is also FOV-dependent (off when the target is 90°+ off your
  view). Reading the server's copy in server.dll would not help; only an own ray cast (option 3, deferred: see the roadmap) would.
- **The triggerbot must only write attack on a change.** Writing "released" every frame would swallow the user's own
  clicks; `app/frame` also skips a release while Mouse 1 is physically down.
- **Projection: reject w < 0.01, not w < 0.** A point near the camera plane divides by almost nothing. A transposed
  (column-major) read of the matrix doesn't fail loudly: points collapse towards the centre (unit test guards it).
- **Screenshots work for checking the overlay.** `PIL.ImageGrab.grab()` captures the game and the layered overlay
  together (borderless 1920x1080), which is how Phase 4 was checked without the user. Start the tool with
  `Start-Process ... -NoNewWindow -RedirectStandardOutput <log>`: a new (even minimised) console window takes the
  foreground and the overlay correctly hides. Only screenshot while CS2 is already in front (never steal the user's
  focus). `python -I` hides the user site-packages where Pillow lives.
- **A pawn's designer name isn't `cs_player_pawn`.** In build 14189 a bot's pawn identity reads
  `c_cs_player_for_precache`. Find pawns through the controller's `m_hPlayerPawn`, never by designer name.
- **Read bools as bytes.** `read<bool>` of a byte that isn't 0/1 is undefined behaviour in C++; `game/player` reads
  `std::uint8_t` and compares with 0.
- **A garbage pawn is dropped, not half-trusted.** `read_player` fills the pawn part only after health (0..10000), the
  scene node and a finite position all read sanely; otherwise the snapshot keeps the controller part with pawn 0.
- **`dwPlantedC4` points straight at the `C_PlantedC4`** (one dereference) from the plant to the next round start,
  0 otherwise; the second dereference of public code is garbage, and `dwPlantedC4 - 0x8` is not a "planted" flag
  (`0x800001BF` with and without a bomb). The bomb's identity has **no designer name**: don't look for it by name.
- **Not every class in the dump is checkable in the live schema.** `CEntityInstance` (and other classes other
  libraries register in the client scope) isn't one of client.dll's own class infos, so `--diag` reports "class not
  found". Keep such fields out of `schema::kFields` and prove them another way.
- **Observer services live on the observer pawn, and only dead players' values mean anything.** The player pawn's
  `m_pObserverServices` is null on the client; read `m_hObserverPawn`'s. Every controller has an observer pawn even
  while alive, with a stale mode/target (a respawned bot still "watches" whoever it last did). The target is a player
  **pawn** handle; a fresh death shows roaming without a target for ~5 s (death cam) first.
- **`dwGameRules` → `C_CSGameRules*`** (proven: round time 3600 in a 60-minute match). Its `m_bBombPlanted` is the
  cheap "is there a bomb" gate.
- **`dwNetworkGameClient_isBackgroundMap` (`0x2C143F`) looks wrong in the dump** (the other `dwNetworkGameClient_*`
  values are < 0x400). Don't use it without proof.

### Overlay window
- **The overlay is a separate topmost window.** It does not hook the game's Present, does not touch the game's
  swap chain, and does not need to know the game's DX11 device.
- **Click-through:** use `WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST` and toggle `WS_EX_TRANSPARENT` when
  the menu is open vs closed.
- **The overlay must track the game window's position and size.** Get the game's HWND (find `cs2.exe`'s main
  window), and on every frame, read its client rect and move the overlay to match.
- **DPI awareness:** make the overlay DPI-aware (`SetProcessDpiAwarenessContext` or the manifest) so the overlay
  coordinates match the game's client coordinates.
- **The overlay can be captured by screenshots and OBS.** If that matters, don't use the overlay for anything you
  don't want visible. (Offline only, so it shouldn't.)
- **Alt-tab behaviour:** the overlay should stay topmost over the game but not over other windows. Done by polling
  `GetForegroundWindow()` every frame (more robust than `WM_ACTIVATE`, which the click-through overlay never gets):
  visible only while the game or the overlay is in front.
- **Per-pixel alpha needs the blt model.** `DwmExtendFrameIntoClientArea(-1)` + a `DXGI_SWAP_EFFECT_DISCARD` swap chain
  cleared to (0,0,0,0). A flip-model swap chain on an HWND ignores alpha (that would need DirectComposition).
- **`WS_EX_TRANSPARENT` only passes clicks through on a `WS_EX_LAYERED` window.** The layered window uses
  `LWA_ALPHA 255`, so with click-through off it takes every click over the game, not only those on the menu.
- **Opening the menu must take focus**, otherwise the game keeps the mouse (hidden, relative) and still shoots/turns.
  `SetForegroundWindow` from a background process is normally refused. It works here because the menu key is a
  `RegisterHotKey`: receiving `WM_HOTKEY` counts as user input to our process. If it ever fails, the Home page and
  the console say to click the menu once. That's why the menu key stays a hotkey after Phase 7 (it's not an engine
  action): the other keybinds are polled with `GetAsyncKeyState`, which isn't input to our process.
- **The menu key can only be a keyboard key that isn't a modifier, and never unbound.** `RegisterHotKey` takes no
  mouse buttons and Shift/Ctrl/Alt alone; an unbound menu key would leave no way to open the menu.
  `input::key_allowed` and the capture enforce it. A key another program (or Windows: F12) holds fails to register:
  logged once, shown on the Keybinds page. While a capture runs the hotkey is released, so the menu key can be
  captured like any key.
- **The hotkey swallows the key system-wide while registered**, so it is registered only while CS2 or the overlay
  has focus.
- **`LONG_PTR` style masks:** `~` on a `DWORD` style mask zero-extends when widened and clears the high bits.
  Keep masks as `LONG_PTR` (`kClickThroughStyles`).
- **CS2 must be in Windowed or Fullscreen Windowed** for the overlay to show on top. Exclusive fullscreen can minimise
  the game or hide topmost windows.
- **While the menu is open, the game runs unfocused**, so CS2's `engine_no_focus_sleep` drops the game's FPS. Normal.
- **ImGui 1.92.8+: `ImDrawList::AddRect(min, max, col, rounding, thickness, flags)`.** The old `flags, thickness`
  order is `= delete`d under `IMGUI_DISABLE_OBSOLETE_FUNCTIONS` (C2280). Same for `AddPolyline` and `PathStroke`.

### Keybinds
- **Never derive presses from once-per-frame polling.** While the game takes the GPU, the overlay's frames get slow
  (with the menu open the game sleeps, so it looked fine there); a tap between two polls was lost, and on/off keys
  "had to be held". Presses come from raw input (`WM_INPUT`, `RIDEV_INPUTSINK`, counted by `input::KeyTracker`); the
  held state still comes from `GetAsyncKeyState`. Never merge both press sources: a press seen by polling in one frame
  and by raw input in the next would count twice and undo a toggle. Polling edges are only the fallback when raw
  input can't be registered.
- **Take the raw presses every frame, also while hidden**, so presses typed in other programs are dropped instead of
  firing when the game comes back. The hidden wait excludes `QS_RAWINPUT` (mouse moves would wake it constantly).
- **Raw input counts a "down" for a key that's already down as auto-repeat**, not a press. Shift/Ctrl/Alt arrive as
  the generic VK: right Shift by scan code `0x36`, right Ctrl/Alt by `RI_KEY_E0`.
- **The frame a capture ends counts as `Suspension::capture`.** Otherwise the captured key's own press would fire its
  new action on the same frame (bind panic to F → instant panic).
- **Mouse 1 over the menu isn't captured** (`io.WantCaptureMouse`): a click there is a click. Otherwise the key
  button's own release click restarts the capture after Mouse 1 is bound. Mouse 1 is bound by clicking outside the
  menu (the overlay takes those clicks while the menu is open, so the game never gets them).
- **Toggle keys survive Alt+Tab** (not polled = no edge); hold keys read released. Panic turns every toggle off.
- **Generic Shift/Ctrl/Alt aren't bindable**: they read "down" together with the L/R keys, so a capture would pick
  the generic one. Use LSHIFT etc.

### Profiles
- **A default colour must be byte-exact** (`Color::rgba(0xRRGGBBAA)`, not `rgb(hex, 0.35f)`): profiles store colours as
  `#RRGGBBAA`, so 0.35 would load back as 89/255 and the defaults wouldn't round-trip (`test_profile_json` checks).
- **Changing a default means updating `profiles/default.json`**: the test fails and writes `default.json.expected`
  next to it; review and rename it.
- **Never rename a setting's JSON key or an `ActionDef::key`**: old profiles would lose that value (with a warning).
  Rename only with a `schema_version` bump and a migration in `profile_json.cpp`.
- **Each build config has its own profiles** (`bin\Debug\profiles`, `bin\Release\profiles`).
- **Loading a profile resets toggle keys and cancels a bind capture** (`settings_replaced` in `app/frame`).

### Menu (Phase 10 design)
- **Colours only through `ui::palette()`**, which `ui::use_theme` rebuilds when the theme or accent changes (called
  every frame in `app/frame` after NewFrame). No inline hex values in pages; a new theme is a row in
  `settings/themes.h`, and `test_themes` checks it stays dark and readable.
- **A theme's accent becomes a saved setting**, so it must be opaque and byte-exact (`Color::rgb(0xRRGGBB)`).
- **`help_marker` and `dot` are sized to the frame height**: use them only on a line of framed controls (a row, a panel
  title, after a button). On a plain text line they'd sit low.
- **`widgets::Columns` isn't nestable** (it's an ImGui table); close it (scope) before full-width content under it.
- **Segmented buttons are equally wide**: size them for the longest name (`bind_row` does), or text gets clipped.
- **Screenshots of the real menu over the game** (Phase 11, with the user's OK to use the game): bring CS2 to the front
  from a script (an Alt tap with `keybd_event`, then `SetForegroundWindow`; a background process is refused without
  it), start the tool with `-NoNewWindow`, then a synthetic INSERT (`keybd_event`) fires the `RegisterHotKey` and the
  menu opens and takes focus; tab clicks with `SetCursorPos` + `mouse_event` only while the menu is open (a click with
  the menu closed would shoot in the game). Run a **copy** of the exe from its own folder (`bin\Showcase`, git-ignored,
  deleted afterwards) so the screenshot profiles never touch the user's `bin\Release\profiles`; the Settings page shows
  that folder. A view turn for a different angle is a `dwViewAngles` write: note the old angles and write them back.
- **Checking the menu without the game's focus:** the menu only opens with the menu key, so screenshots of the real
  menu come from a scratch harness (outside the repo) that compiles `src/external` (minus `app/` and `main.cpp`) with
  `cl`, renders `ui::draw_menu` with a mock `AppState` into an offscreen D3D11 texture and saves a BMP per page.
  Link against `bin\Debug\vendor.lib` with `/MTd` and the v143 toolset (`vcvars64.bat -vcvars_ver=14.44`); add
  `external\nlohmann` to the includes.

### Tests (doctest)
- **Define `DOCTEST_CONFIG_USE_STD_HEADERS`** (set in `tests.vcxproj`).
- doctest is on the **external** include path with external warnings off.

---

## 11. Dos and don'ts

**Do**
- Respect §1. If a task drifts toward online use, evasion, stealth, networking or distribution, stop and tell the
  user.
- Open the process handle with the minimum access rights needed.
- Read and write game memory only through `core::Memory` (`safe_read<T>` / `read<T>` / `safe_write<T>`). Code that
  only reads takes `const core::Memory&`; code that writes takes `core::Memory&`. Never a raw `RPM`/`WPM` call
  outside `core/process_memory`. Test `game/` code with `tests/helpers/fake_memory.h`.
- Keep maths and feature logic pure (no Win32, no ImGui, no game pointers) and unit-test it.
- Keep commits small and focused; commit only after the user approves.
- Build Debug and Release with **zero warnings** before calling a task done.
- Restore anything you change in the game on panic and/or shutdown.
- Ask the user before new dependencies or big architectural changes.
- **Record every offset, signature, and schema field in `docs/offsets.md`, with its source (repo + commit + date)
  and proof.**

**Don't**
- Don't inject, hook, or call into the game. External means external.
- Don't touch game state from any thread other than the main thread.
- Don't let exceptions escape (there is no game code to escape into, but be disciplined about it).
- Don't change a verified offset without a diagnostic proving it.
- Don't hardcode offsets or signatures outside `game/offsets.h` and `game/schema.h`.
- Don't write to the game's code. No patches. That's an internal-only technique.
- Don't add DMA, manual mapping, module/thread hiding, obfuscation, anti-debug, network code or packaging.
- Don't edit vendored code in `external/`.
- Don't add Claude/AI co-author or attribution lines to commits or PRs.
- **Don't ever join a VAC-secured server with the tool running.** `-insecure` only. Offline bots only.

---

## 12. Testing

- **Unit tests** (from Phase 1): `tests` console exe (x64, doctest v2.5.3) that compiles the **pure** sources from
  `src/external` (plus `core/process_memory.cpp`, which is Win32 but harmless to test) and `tests/**/*.cpp`.
  Code that reads remote memory is tested against `FakeMemory`; remote modules against `helpers/fake_pe.h`. Run: `bin\Debug\tests.exe` (and `bin\Release\tests.exe`). When adding a pure `.cpp` under
  test, add it to `tests.vcxproj` too.
- **In-game tests:** every phase lists acceptance criteria in §13. The user runs them; record the result in §14 and
  DEVLOG.
- **Offset re-verification test** (after every CS2 update): `bin\Release\cs2_external.exe --diag` with CS2 in a bot
  match. It checks the build number, interfaces, signatures and every schema field and ends with `all N checks OK` or
  `N of M checks FAILED` (exit code 0 / 2). Any FAIL is a flag. Cross-check against a fresh `a2x/cs2-dumper` run.
- **Live game reads:** `bin\Release\cs2_external.exe --live` in a bot match: every player's name, team, health,
  weapon and position, ~4 Hz. `--live > file.txt` writes plain frames (handy for checking from a script).

---

## 13. Roadmap

Each phase ends with: zero-warning Debug + Release builds, tests passing (once they exist), the user's in-game
check, docs updated (§2 step 7), then a commit after approval.

### Phase 0: Scaffold + process handle
- [x] Solution, `external` (console exe) and `tests` projects, shared `props/common.props` (C++20, x64, `/MT`,
      `/W4 /WX`)
- [x] `.gitignore`, `.gitattributes`, README stub, `docs/DEVLOG.md`, `docs/offsets.md`, `docs/dumps/`,
      `external/README.md`, this file
- [x] `core/process`: find `cs2.exe` (Toolhelp32), open a handle, look up `client.dll` and `engine2.dll` module
      bases
- [x] `core/memory`: `safe_read<T>` / `safe_write<T>` wrappers around `ReadProcessMemory` / `WriteProcessMemory`,
      with `__try/__except` guards and an `is_plausible_pointer` helper (as the `core::Memory` interface +
      `core::ProcessMemory`, with `FakeMemory` for tests)
- [x] `core/log`: `logger::info/warn/error`
- [x] `main.cpp`: open the handle, print the module bases and the local pawn pointer, wait for Enter, exit
- [x] Builds with zero warnings (Debug + Release); `tests.exe` 13/13 cases pass in both
- [x] Verified in-game by the user (launch with `-insecure`, offline bots): 2026-10-06, build 14189, offline
      deathmatch: PID, both module bases and local pawn `0x4DF564BF800` printed; game closed → "cs2.exe not found".
      Not exercised: main menu (`none`) and the access-denied path (the game isn't elevated on this PC).

**Acceptance:** the tool finds `cs2.exe`, prints the PID, the `client.dll` base, the `engine2.dll` base, and the
local pawn pointer (plausible hex). It exits cleanly. Running it with the game closed says "cs2.exe not found."
Running it without admin, if the game is elevated, says "OpenProcess failed" and tells the user to run as admin.

### Phase 1: Overlay window + ImGui shell
- [x] Vendor Dear ImGui (pinned tag) + Win32 + DX11 backends (v1.92.9b, `external/vendor.vcxproj`)
- [x] `ui/overlay_window`: transparent, click-through, topmost Win32 window with its own DX11 swap chain; tracks
      the game window's client rect; toggles `WS_EX_TRANSPARENT` on INSERT
- [x] `ui/imgui_layer`: ImGui context + backends for the overlay window; shutdown on exit
- [x] `ui/theme` (dark), `ui/menu` with sidebar pages (ESP / Aimbot / Triggerbot / Misc / Settings,
      placeholders), branding "External Cheat - by BigH"
- [x] Logo (moved up from Phase 10 at the user's request): exe icon, menu header, Home, watermark, README
- [x] Watermark lists the active features one per line (`features/feature_summary`, tested); Player page removed;
      no menu-key hints in the watermark or the header; header keeps only the "External" pill
- [x] INSERT toggles the menu (temporary hard-coded until Phase 7)
- [x] Builds with zero warnings (Debug + Release); `tests.exe` 17/17 in both
- [x] Verified in-game by the user (2026-10-06): menu open over an offline bot match (screenshot: 1920x1080,
      270 FPS, "in a match"), logo/watermark/header iterated on twice, then approved for commit

**Acceptance:** the overlay appears on top of CS2, sits over the game window exactly, and is click-through when
the menu is closed. INSERT opens the menu; the cursor is visible and clicks work while the menu is open; clicking
through the menu doesn't shoot the game. Alt-tab hides the overlay when the game loses focus. Closing the tool
removes the overlay and leaves the game running.

### Phase 2: Offsets, signatures, interfaces, schema
- [x] Copy `dw*` offsets from `docs/dumps/offsets.json` into `offsets.h`, with provenance (all 29 + 16 buttons +
      4 interface RVAs, by script)
- [x] Copy the schema fields we need from `docs/dumps/client_dll.json` into `schema.h`, with provenance (29 fields,
      13 classes)
- [x] `core/pattern`: remote signature scanner (read the module range in chunks, scan locally), tested; 8 signatures
      in `offsets.h`, each proven to resolve to exactly the dump value
- [x] `game/interfaces`: resolve `CreateInterface` from outside the process (export directory via `core/pe`, then the
      `InterfaceReg` list); `SchemaSystem_001`, `Source2Client002`, `Source2EngineToClient001`,
      `InputSystemVersion001`. `GameEntitySystem` turned out not to be an interface (it's the global
      `dwGameEntitySystem`, checked instead)
- [x] A diagnostic that prints every signature hit, every interface pointer, and every schema field offset, each
      checked against the live game (`app/diagnostics`, `--diag`); the schema values are read from the game's own
      schema system (`game/schema_system`)
- [x] Builds with zero warnings (Debug + Release); `tests.exe` 41/41 in both. `--diag` against the live game (build
      14189, offline bot match): all 45 checks OK
- [x] Verified in-game by the user (2026-10-06, build 14189, offline bot match): normal start printed all 45
      checks OK, then the overlay started; approved for commit

**Acceptance:** the console shows non-null interface pointers, every schema field offset (health, team, life
state, origin, view offset, eye angles, weapon services, active weapon, shots fired, dormant, flags, scoped,
spotted state, crosshair entity index, flash duration) and the `jump` / `attack` button globals, and every signature
address. Every value matches what a fresh `a2x/cs2-dumper` run of the current CS2 build shows, or the diagnostic
explains the mismatch.

### Phase 3: Entity list + local player + snapshots
- [x] `game/entities`: walk the chunked entity list remotely; yield entities; filter to `C_CSPlayerController`
      (indices 1..maxClients, designer name `cs_player_controller`)
- [x] `game/handle`: `CHandle` → entity pointer resolution, with serial validation
- [x] `game/player`: build `PlayerSnapshot` from a controller (resolve pawn via `m_hPlayerPawn`); read health,
      team, position, life state, name, weapon id; validity checks (+ armour, eye height, flags, scoped, dormant)
- [x] `game/view`: read the view matrix; sanity check
- [x] `game/globals`: read `CGlobalVars` (curtime, maxClients, interval_per_tick; + tickcount, map name). Layout
      found live (the "typical" one was wrong)
- [x] `game/weapon`: read the active weapon id; map to a name (+ weapon class, for the Phase 5 filter)
- [x] `app/live_view`: debug console view (~4 Hz) with local player + every bot (`--live`), and Home page rows
- [x] `maths/vec`, `game/snapshot.h` (pure), tested
- [x] Builds with zero warnings (Debug + Release); tests 70/70 in both
- [x] Verified in-game: **auto-approved** (user asleep). Checked here against the running game (build 14189, de_mirage
      bot match, bots frozen): `--live` showed all 20 players with the right names, teams, weapons (AK-47 / Glock-18
      / P2000 ids 7 / 4 / 32), positions and distances; globals tick at 64 Hz; view matrix sane; `--diag` all 45 OK;
      the overlay starts and detects the match. **Not exercised in-game:** death/respawn, a match restart, a map
      change, the main menu (unit-tested only)

**Acceptance:** live view shows the correct name, team, health, life state, weapon, and position for you and
every bot in a bot match; dying and respawning updates correctly; a bot match restart doesn't crash; the view
matrix is sane before and during a frame.

### Phase 4: World-to-screen + ESP
- [x] `maths/projection`: `ViewMatrix`, `world_to_screen` (row-major, matching this build's matrix); tests (incl.
      the live matrix: the view direction lands on the crosshair)
- [x] `maths/skeleton`: joint specs, bone index → screen positions (bones found: `game/bones`, model state + 0x80)
- [x] `settings/settings.h` (ESP section; the overlay options move in) + `app/frame` fills
      `AppState::active` from it, so the watermark shows "ESP"
- [x] `features/esp`: pure decision; `render/primitives` + `render/painter` on the overlay's ImGui background
      draw list
- [x] ESP page: 2D box, corner box, head circle, skeleton (optional), name, health bar/number, distance, weapon
      name, snaplines (origin choice), team mode, enemies only, colours with opacity, thickness (+ outline, max
      distance)
- [x] Scoped indicator: a "SCOPED" tag on bots with `m_bIsScoped` set (option on the ESP page)
- [x] Visibility choice: taken from the §2a standing decision (user asleep): **option (2), the spotted-by mask**.
      The original item, kept for reference:
      **Before building any visibility check, stop and remind the user of the three options and let them choose**
      (they asked for this on 2026-10-06): (1) the game's own trace = internal, out of scope; (2) the spotted-by
      mask = external, no map geometry, cheap, but slightly delayed and "spotted" rather than per-pixel visible;
      (3) our own ray cast against the map's collision mesh parsed from the game files = external, exact and
      per-frame, but a big project (Valve file formats, a BVH, no moving doors/props). Default plan is (2).
- [x] Visible / hidden colours from the spotted-by heuristic (`game/visibility`: `is_spotted_by(mask, local slot)`,
      tested against FakeMemory)
- [x] Builds with zero warnings (Debug + Release); tests 97/97 in both; `--diag` all 48 checks OK
- [x] Verified in-game: **auto-approved** (user asleep). Checked here with screenshots of the overlay over the live
      game (build 14189, de_mirage, 1920x1080, bots frozen): boxes, skeletons, head circles, health bars/numbers,
      names, weapons, distances and snaplines on all 19 bots; skeletons inside their boxes, feet on the box bottom;
      enemy/team hidden colours right; the watermark lists "ESP"; the ESP page renders. **Not exercised in-game:**
      a bot in line of sight (no bot was visible from the player's spot, every spotted mask read 0, so the
      "visible" colour and the slot bit are unverified), a scoped bot (bots had pistols), moving bots, dead bots,
      other resolutions, a map change. **User check 2026-10-07:** all fine except the head circle (on the neck);
      fixed in Phase 5 (bone 7). The visible colour was proven live 2026-10-07 (spotted bit = our slot)

**Acceptance:** boxes line up with bots at near/far distance and different resolutions; nothing drawn for bots
behind you; skeleton faces the right way (if bones are done); colours/opacity change live; dead bots not drawn;
map change doesn't crash; a bot scoped in with an AWP shows "SCOPED" and loses it when it unscopes; visible /
hidden colours follow line of sight (with the heuristic's small lag); tests pass (including the row-major matrix
centre test).

### Phase 5: Aimbot + triggerbot
- [x] `maths/angles`: `Angles`, `normalize`, `clamp`, `calc_aim_angles`, `angular_distance`, `is_within_fov`,
      `smoothing_fraction`, `step_towards` (+ `forward`, `distance_to_ray`); tests
- [x] `features/aimbot`: candidates (FOV, dead, team, max distance), priority (crosshair / distance / lowest
      health), head/body/nearest-bone, compute aim; tests
- [x] `features/triggerbot`: state machine (idle → reaction delay → tap → between shots → cooldown; hold), fire
      modes single / burst / hold, filters (team, visible, distance, weapon class, scoped, flashed, in air, head);
      tests for every option
- [x] Write handle (`core::kReadWriteAccess`, overlay mode only); angle write via `game/writes` on the main thread;
      triggerbot fires via the `attack` button (65537 / 256, proven live; no `SendInput`); nothing fires or aims
      while the menu is open or the game isn't focused
- [x] Visible-only option for the aimbot (spotted-by heuristic, bit proven live 2026-10-07)
- [x] Aimbot page + Triggerbot page: enable, activation key (hold/toggle), aim point, priority, FOV radius +
      circle, smoothing, team check, max distance, visible only; triggerbot: every option in §3 (+ live status)
- [x] ESP head circle fixed (user report): centred on bone 7, radius 6.5
- [x] Builds with zero warnings (Debug + Release); tests 125/125 in both; `--diag` all 50 checks OK
- [x] Verified in-game by the user (2026-10-07): aimbot, triggerbot and the fixed head circle all good; approved

**Acceptance:** holding the aim key locks on the right target for each priority; head/body/nearest-bone works;
smoothing feels consistent at different framerates; FOV circle matches which bots are eligible; team check and
max distance filter correctly; visible-only skips bots behind walls; triggerbot fires only with an enemy under the
crosshair, and each option (activation, delay, fire mode, filters) behaves as labelled; neither feature fires while
the menu is open or the game isn't focused; tests pass.

**Honest note:** external aimbot cannot write angles on the game's logic thread, so the view will feel slightly
laggier than an internal one. That's the cost of external. It's still usable for offline learning.

### Phase 6: Misc: radar, bomb timer, spectators
(Replaces the dropped "Player values" phase: no Player page, no health / armour / ammo writes, no Set / Freeze.
Bunny hop and the hitsound were dropped from this phase by the user on 2026-10-07.)
One feature at a time, each with its own in-game check by the user (and its own commit after approval):
- [x] **Radar** (`features/radar`, Misc page cards, `PlayerSnapshot::eye_angles`): built 2026-10-07, tests 138/138,
      screenshot over the live game OK. Verified in-game by the user (2026-10-07): all good, approved
- [x] **Bomb timer** (`game/bomb`, `features/bomb_timer`, `dwPlantedC4` → `C_PlantedC4`): built 2026-10-07; the first
      version (designer-name search) showed nothing in the user's test and was replaced; the read path is proven live
      (two plants, countdown vs curtime, `--diag` bomb check OK). Verified in-game by the user (2026-10-07); then, at
      their request, two latest-defuse marks on the bar (10 s / 5 s). Tests 154/154
- [x] **Spectator list** (`game/observer`, `features/spectators`, `render/panel`): built 2026-10-07, tests 167/167,
      `--diag` 67/67; the observer chain proven live (bots watching our pawn). Approved by the user (2026-10-07)
- ~~**Hitsound**~~: dropped by the user (2026-10-07), never built
- [x] Misc page: every feature above with its options (radar, bomb timer, spectator list)
- [x] Builds with zero warnings (Debug + Release); tests 167/167
- [x] Verified in-game by the user: radar and bomb timer checked in-game; spectator list approved for commit
      (2026-10-07)

**Acceptance:** each misc feature does what its label says; nothing changes in the game while the menu is open or
the game isn't focused; tests pass.

### Phase 7: Keybind engine
- [x] `input/keys`, `input/actions` (registry + defaults), `input/keybinds` (HOLD/TOGGLE/PRESS), tested
- [x] `input/bind_capture` (pure, tested)
- [x] `input/key_poll` on the main thread; act only while the game (or the overlay) is focused; suspended during
      capture
- [x] Keybinds page: every action by category, click to capture, mode selector, conflict highlight; bind buttons
      on each page
- [x] Actions: menu toggle (INSERT, still the overlay's hotkey), panic (END), exit (DELETE), aimbot (HOLD/TOGGLE),
      aimbot enable, ESP toggle, triggerbot (HOLD/TOGGLE) + enable, radar / bomb timer / spectator list toggles.
      Presets move to Phase 8 (they don't exist yet). Panic and exit have their basic behaviour now; Phase 9 adds the
      exit button and the robustness pass
- [x] Builds with zero warnings (Debug + Release); tests 191/191
- [x] Verified in-game by the user (2026-10-07): every keybind works; after the raw input fix the on/off keys flip on
      each tap, spamming included; approved

**Acceptance:** each mode works with keyboard keys and mouse buttons (incl. MOUSE4/5); capturing a bind doesn't
fire it; conflicts are shown; rebinding the menu key works; tests pass.

### Phase 8: Settings and profiles (JSON) + presets
- [x] Vendor nlohmann/json (approved 2026-10-07; v3.12.0, copied from AC, hash checked against the release)
- [x] `settings/profile_json`: one field list per section for write + read; forgiving load, `schema_version` +
      migration hook (pure, tested)
- [x] `settings/profile_store`: `ProfileStore(folder)`, atomic save, read-only `default`, last profile, load on
      startup (std::filesystem only, tested on a temp folder)
- [x] Settings page: profile list, save / save as / load / rename / delete / reset, unsaved-changes marker (+ Home)
- [x] `settings/presets`: Off / Chill / Medium / Rage, feature bundles + strengths, on the Settings page and
      bindable hotkeys (Presets category, unbound by default)
- [x] `profiles/default.json` committed and equal to code defaults (test)
- [x] Builds with zero warnings (Debug + Release); tests 216/216
- [x] Verified in-game by the user (2026-10-07): everything works; approved

**Acceptance:** settings survive restart via the last profile; a hand-edited profile with bad values loads with
warnings and clamped values; `default` can't be overwritten; each preset switches the right features and leaves
keybinds and colours alone; tests pass.

### Phase 9: Panic, clean shutdown, polish
- [x] Panic: disables aimbot/ESP/triggerbot (so the angle writes stop), closes the menu (done in Phase 7; now also
      drops a preset / profile load queued just before)
- [x] Exit action + Settings-page button: restore everything, shut down the overlay, close the handle, exit (key in
      Phase 7; Exit card with an unsaved-changes confirmation; a deliberate exit no longer waits for Enter)
- [x] Final key layout: **DELETE = exit, END = panic** (INSERT menu) (the defaults since Phase 7)
- [x] Robustness review: map change, death, alt-tab, minimise, game close, game restart (code review, DEVLOG table;
      fixed: errors in the loop now still release attack and remove the overlay)
- [x] Builds with zero warnings (Debug + Release); tests 216/216; `--diag` 67/67
- [x] Verified in-game by the user (2026-10-07): all good; approved

**Acceptance:** panic returns the game to normal instantly; 10 start/stop cycles with no crash, leak, input or
cursor problem; after exit the game behaves exactly like an untouched game.

### Phase 10: UI redesign
- [x] Rename to the final name (decided with the user): **External Cheat - by BigH**, done in Phase 1
- [x] Logo embedded (menu header + Home via an ImGui user texture): done early, in Phase 1
- [x] Home dashboard + **top tabs** with icons (the user picked tabs over a sidebar; icons drawn with the draw list,
      no icon font)
- [x] Colours: **dark black / grey / off-white** (the user's choice, easy on the eyes at night) as the default theme
      "Midnight", plus four more (Black, Graphite, and Violet / Ice from the logo) and an accent colour picker, saved in
      profiles; (?) help tooltips on every option; two-column pages
- ~~Menu size setting~~: not chosen by the user (2026-10-07); the menu is still resizable by dragging its corner
- [x] Builds with zero warnings (Debug + Release); tests 220/220
- [x] Verified in-game by the user (2026-10-07): "so much better, so clean"; approved

### Phase 11: Educational README
- [x] A comprehensive, educational `README.md`: what the project is (and isn't: §1), features, how it works
      (process handle, RPM/WPM, entity system walk, handle resolution, schema lookups, signature scanning,
      world-to-screen, overlay window), how offsets and signatures were found (the dumper, signatures built from the
      live module, read-only scripts against the live game; no Cheat Engine or IDA/Ghidra was used, so the README
      says so honestly), safety (guarded reads, shutdown), build and use (keys, presets, profiles), project layout,
      tests, and what was learned per phase. Drafted in the AC README's style (the user's request)
- [x] Screenshots (the user's request): in-game ESP ×2, the menu opened over the live game, every page, the Violet
      theme, watermark + radar crops (`docs/screenshots/`)
- [x] Final pass on this file
- [x] Builds with zero warnings (Debug + Release); tests 220/220
- [x] Reviewed by the user (2026-10-07): approved ("push it")

**Acceptance:** someone who has never seen the project can understand what it does, how each technique works and
why, build it and use it, from the README alone.

### Later (the user wants to come back to it): exact visibility, own ray cast (option 3)
Deferred 2026-10-07. The game's spotted bit lags 0-0.5 s (the server re-checks every ~0.5 s; measured, see
`docs/offsets.md` "Spotted-by mask"), and nothing faster reaches the client. Fully external fix: trace our own rays
against the map's collision geometry, read from the game files (not from game memory).
- [ ] Research: where the map's collision / physics data lives in the VPK and its format (Source 2 resource files);
      decide parse-at-startup vs cache
- [ ] Load the current map's collision triangles; build a BVH (pure, tested)
- [ ] `is_visible(eye, point)`: ray vs BVH; test eye → head / chest / pelvis per bot every frame within a budget
- [ ] Replace or combine with the spotted bit for ESP colours, aimbot and triggerbot visible-only; `--diag` check
- [ ] Known limits: doors, props and smokes that move or appear aren't in the static map
- Cheaper partial idea noted on the way (not chosen): server-side bot vision (`CCSBot::m_isEnemyVisible` in
  server.dll): an enemy bot that sees you is visible to you; faster only when it faces you. Measure its update rate first.

### Phase 12+ (only if the user decides to): Internal
- [ ] This is a **separate, explicitly-scoped decision**, not a silent slide. If the user wants silent aim, no
      spread, no recoil, or sub-tick angle writes, that requires a DLL, injection, and hooks. Re-read §1, re-scope
      the project, and start a new `CLAUDE.md` section for the internal side. Do not bolt it onto the external
      codebase.

### Recurring: after every CS2 update
- [ ] Re-run `a2x/cs2-dumper` (or pull the latest from a mirror)
- [ ] Diff the new `output/offsets.json` against `docs/dumps/offsets.json`; copy the new values into `offsets.h`
      and log the change in `docs/offsets.md`
- [ ] Diff the new `output/client_dll.json` against `docs/dumps/client_dll.json`; copy the new field offsets into
      `schema.h` and log any that changed
- [ ] Every signature that no longer hits: find the new address, update `offsets.h`, log it
- [ ] Rebuild, re-run tests, re-run the tool, spot-check ESP/aimbot/triggerbot/values

---

## 14. Current status

- **Phase 0: done, verified in-game (2026-10-06, CS2 build 14189), approved, committed and pushed** to
  `github.com/BigH018/cs2-external` (private, `main`) as three commits: build setup, core code + tests, docs + dumps.
- **Dumps:** `docs/dumps/` holds `info.json`, `offsets.json`, `client_dll.json`, `interfaces.json` and
  `buttons.json`, copied byte-for-byte from the user's a2x/cs2-dumper run (build 14189, 2026-10-06T16:04:45Z). The
  dumper's git commit isn't in `info.json` and isn't recorded; build number + timestamp are the provenance.

- **Phase 1: done, verified in-game and approved by the user (2026-10-06), committed and pushed.** Debug + Release
  build with zero warnings, tests 17/17. After the user's first look: logo added everywhere (exe icon, menu header,
  Home, watermark, README), the watermark lists the active features, the Player page/phase was dropped. A smoke test with CS2 running but not focused found the device, ImGui and
  the game window, and the overlay correctly stayed hidden. INSERT, focus, click-through and alignment are untested
  until the user tries them in-game.

- **Feature plan updated (2026-10-06):** bunny hop (Phase 6, which replaced the dropped Player phase), ESP scoped
  indicator + visible colours (Phase 4), visible-only aimbot and a fully configurable triggerbot (Phase 5). All
  external; visibility is the spotted-by heuristic (§3 feasibility table).

- **Phase 2: done, verified in-game by the user (2026-10-06: all 45 checks OK), approved, committed and pushed.** Debug + Release
  build with zero warnings, tests 41/41. CS2 was running during the session (build 14189, `-insecure`, bot match),
  so every layout was proven live with a read-only scratch script before the C++ was written, and `--diag` was run
  here against the game: **all 45 checks OK**. Found and fixed in CLAUDE.md: the weapon id chain is `0x149A`, an entity
  identity is `0x70` bytes, `identity + 0x10` is the full handle (all three were wrong before, proven live).
  **No offset in code changed**: everything new was copied from the dumps, and the dump values all matched.

- **Phase 3: done, auto-approved (autonomous mode, 2026-10-06 night), committed and pushed.** Debug + Release zero
  warnings, tests 70/70. Checked against the running game with `--live` (all 20 players correct). New hand-found
  layouts (entity list in code, `CGlobalVars`) in `offsets::layout`, proven live. **No dumped offset changed.**
  To check when awake: `cs2_external.exe --live` while dying/respawning and restarting the match.

- **Phase 4: done, auto-approved (autonomous mode, 2026-10-06 night), committed and pushed.** Debug + Release zero
  warnings, tests 97/97, `--diag` 48/48. ESP checked with screenshots over the live game. **Bones found** (model
  state + 0x80, hand-found, proven live). **No dumped offset changed**; one schema field added
  (`CSkeletonInstance::m_modelState`, from the dump). **To check when awake:** the visible colour with a bot in plain
  view (unverified: no bot was in line of sight), "SCOPED" on a sniper bot, a map change.

- **User back (2026-10-07):** Phases 3-4 checked in-game by the user: all fine except the head circle (fixed in
  Phase 5). **Autonomous mode switched OFF.**

- **Phase 5: done, verified in-game by the user (2026-10-07), approved, committed and pushed.** The previous session
  hit its context limit mid-phase (no HANDOFF.md); this one finished it from the uncommitted tree. Debug + Release
  zero warnings, tests 125/125, `--diag` 50/50. Proven live here: the spotted-by bit, `m_iIDEntIndex` = pawn index,
  view-angle writes stick, bone 7 = middle of the head. **No offset changed** (two schema fields added from the dump).
  The user checked aimbot, triggerbot and the head circle in-game: all good.

- **Spotted-by delay measured (2026-10-07):** ~0.5 s server re-check is the whole delay; replication ~1-2 ms. Option 2
  (server.dll mask) therefore not built; option 3 deferred by the user (roadmap: "Later"). server.dll entity list + mapping documented in
  `docs/offsets.md` (unused).

- **Bunny hop dropped (2026-10-07, user decision):** removed from the plan, the Misc page and `ActiveFeatures`.

- **Option 3 (own ray cast visibility) deferred** to a "Later" roadmap section, to come back to after the main phases.

- **Phase 6, radar: done, verified in-game by the user (2026-10-07), committed and pushed.** Debug + Release zero
  warnings, tests 138/138, `--diag` 50/50. **No offset changed** (`m_angEyeAngles` was already in `schema.h`; now
  read for every pawn, proven live).

- **Phase 6, bomb timer: done, verified in-game by the user (2026-10-07), committed and pushed** (with the two
  latest-defuse marks the user asked for afterwards). First version (name search) didn't work in the user's test; now
  `dwPlantedC4`, proven live. Debug + Release zero warnings, tests 154/154, `--diag` 62/62 (+ the bomb check OK while
  a bomb ticked). **No offset changed**: 12 schema fields added from the dump.

- **Phase 6, spectator list: done, approved by the user (2026-10-07), committed and pushed.** Debug +
  Release zero warnings, tests 167/167, `--diag` 67/67. **No offset changed**: 4 schema fields added from the dump,
  the chain proven live first (`docs/offsets.md` "Observer"). The bomb timer's panel code moved to `render/panel`
  (shared; the bomb timer draws the same).

- **Hitsound dropped (2026-10-07, user decision):** removed from the plan, the Misc page's "Coming next" card and
  `features::ActiveFeatures`. **Phase 6 is done.**

- **Phase 7: done, verified in-game by the user (2026-10-07), approved, committed and pushed.** The user's first check: every
  keybind works, but on/off keys needed holding and spamming lost presses (taps between two once-per-frame polls).
  Fixed with raw input press counting (the user chose it over an input thread). Debug + Release zero warnings, tests
  191/191; raw input registers against the running game, CPU unchanged (~25-35% of a core while the overlay shows,
  the same before the change). **No offset changed.**

- **Phase 8: done, verified in-game by the user (2026-10-07), approved, committed and pushed.** nlohmann/json v3.12.0 vendored (approved).
  Debug + Release zero warnings, tests 216/216. Startup profile loading checked against the running game (default,
  and a hand-broken profile: warnings logged, values clamped, a refused menu key kept INSERT). **No offset changed.**
  Not checked here: the Settings page and its buttons on screen.

- **Phase 9: done, verified in-game by the user (2026-10-07), approved, committed and pushed.** Most of it existed since Phase 7 (panic, exit
  key, key layout). Added the Exit button, the no-"Press Enter" deliberate exit, the error guard around the loop, and
  the panic/request fix; robustness reviewed in code. Debug + Release zero warnings, tests 216/216, `--diag` 67/67,
  overlay start checked against the running game. **No offset changed.** Not checked here: anything that needs keys
  or clicks in the game (exit key, Exit button, panic, 10 start/stop cycles, map change, game close).

- **Phase 10: done, verified in-game by the user (2026-10-07), approved, committed and pushed.** The whole menu redesigned (top tabs with
  icons, two columns of panels, aligned label/control rows, switches, segmented buttons, chips), five dark themes +
  accent colour (saved in profiles, `overlay.theme` / `overlay.accent`), restyled watermark, a close button. Debug +
  Release zero warnings, tests 220/220; every page rendered offscreen with mock data (scratch harness) and checked in
  four themes and a narrow window; the overlay starts against the running game. **No offset changed.**

- **Phase 11: done, approved by the user (2026-10-07), committed and pushed.** `README.md` written in the AC
  README's style; 13 pictures in `docs/screenshots/`, all from the live game (build 14189, de_mirage Deathmatch, bots
  frozen): the tool ran from a scratch copy with two screenshot profiles, the menu was opened with a synthetic INSERT
  and the tabs clicked by script; one view-angle write for the second ESP shot, restored afterwards. Debug + Release
  zero warnings, tests 220/220, `--diag` 67/67. **No offset changed, no code changed.** Not pictured: the bomb timer
  (no bomb in Deathmatch), a spectator (bots respawn at once in Deathmatch), an enemy in the visible colour (every
  enemy was behind cover).

**Next:** the main roadmap is done; open items are the "Later"
section (own ray cast visibility) and the recurring after-update checklist.

---

## 15. Decision log

- **2026-10-06:** Project started as an **external-first** offline-only CS2 trainer for learning. `-insecure` only.
  No VAC servers. Private repo. The user is the sole author (no AI attribution in commits or PRs).
- **2026-10-06:** **External-first, not internal.** The reason: external teaches the fundamentals (pointer chains,
  entity system, schema lookups, world-to-screen) without the crash risk, dev-loop friction, and injection
  complexity of a DLL. Internal features (silent aim, no spread, no recoil, sub-tick angle writes) are explicitly
  deferred to a separate Phase 12+ decision, not bundled in.
- **2026-10-06:** x64 only. No x86 configuration. CS2 is 64-bit.
- **2026-10-06:** Use the community offset dumpers (`a2x/cs2-dumper`, `sezzyaep/CS2-OFFSETS`,
  `xsip/cs2-sdk-offsets-interfaces-dump`, `scros22/cs2-universal-offsets`) as the source of truth for stable RVAs
  and schema fields. Record provenance (repo + commit + date) next to every offset in `offsets.h` and `schema.h`.
- **2026-10-06:** Use **schema system** for field offsets inside entities rather than hardcoding them where the
  remote walk is viable; fall back to a dumped `client_dll.json` with provenance if the walk is too fragile.
- **2026-10-06:** The overlay is a **separate topmost window**, not a hook of the game's Present. This is the
  defining architectural difference from an internal overlay.
- **2026-10-06:** `docs/offsets.md` is a first-class artifact, not an afterthought. Every offset, every signature,
  every schema field, with its source and its proof.
- **2026-10-06:** The bone array pointer is **not** in the schema dump. It must be found by hand (Cheat Engine or
  pattern scan) and recorded in `docs/offsets.md` with the build number. Skip bones in Phase 4; add them later.
- **2026-10-06 (Phase 0):** Repo created: `github.com/BigH018/cs2-external`, **private**, default branch `main`, the
  user (BigH018) is the only collaborator. Commits use the user's git identity with no AI attribution lines.
- **2026-10-06 (Phase 0):** The Phase 0 handle is **read-only** (`PROCESS_VM_READ |
  PROCESS_QUERY_LIMITED_INFORMATION`, `core::kReadOnlyAccess`). Write access is added only in the phase whose
  feature needs it (Phase 5 angle writes; Phase 6 player values was dropped later the same day).
- **2026-10-06 (Phase 0):** Offset and schema constants keep the **dumper's exact names** (`dwLocalPlayerPawn`,
  later `m_iHealth`) instead of `kPascalCase`, so they can be grepped across dumps. A deliberate exception to §9.
- **2026-10-06 (Phase 0):** The local-pawn read lives in `game/player.cpp` (`read_local_pawn`) rather than in
  `main.cpp`, to keep the "only `game/` dereferences game memory" rule from day one. `game/offsets.h` holds only
  `dwLocalPlayerPawn` until Phase 2 copies in the rest.
- **2026-10-06 (Phase 0):** doctest v2.5.3 vendored now (already in the §4 stack) so the `tests` project exists from
  Phase 0, with `core/memory` tests against our own process.
- **2026-10-06 (Phase 0):** `common.props` is imported after `Microsoft.Cpp.props`; `WindowsTargetPlatformVersion`
  and `CharacterSet` live in each `.vcxproj` (see §4). Same layout as the AC project.
- **2026-10-06 (Phase 0, user said re-architecting is fine):** reviewed the AC project (`BigH018/internal-assault-cube`)
  and adopted its patterns:
  - **`core::Memory` is an interface** (pure header) with `ProcessMemory` (RPM/WPM) as the real implementation and
    `tests/helpers/fake_memory.h` for tests. This is the external version of AC's pure `safe_memory.h`: AC could
    test `game/` code because game memory was local; externally every read is RPM, so `game/` code takes a
    `core::Memory&` and the Phase 3 entity-list tests run against a fake chunked entity list. Read-only code takes
    `const Memory&`, so "this code can't write" is checked by the compiler. Range checks happen once in the base
    (NVI: public `read_bytes` → private virtual `do_read`). A virtual call per RPM is negligible next to the syscall.
  - **`config.h` exists from Phase 0** and holds the pointer bounds (`kMinValidPointer`/`kMaxValidPointer`) and the
    process/module names, like AC's `config.h`.
  - **Logger = AC's:** `noexcept`, drops the line if formatting throws, `[+]` / `[!]` / `[x]` prefixes.
  - **`app/` (frame, state, live_view)** is added to the plan as the orchestrator from Phase 1; `main.cpp` stays a
    bootstrap.
  - **Code style = AC's:** namespace contents not indented, closing `} // namespace x`.
- **2026-10-06 (Phase 0):** CLAUDE.md's earlier "3301 classes / 569 enums" for `client_dll.json` was wrong. The real
  build-14189 dump has 542 classes and 14 enums (the copy the user pasted was complete).
- **2026-10-06 (Phase 1):** Dear ImGui pinned to **v1.92.9b** (commit `f1cc2ae`), the same tag as the AC project, with
  the **DX11** backend instead of AC's OpenGL2. Compiled in `vendor.lib` (warnings off); config defines in
  `props/imgui.props`, as in AC.
- **2026-10-06 (Phase 1):** Overlay transparency = layered popup + `DwmExtendFrameIntoClientArea(-1)` + blt-model
  swap chain, not DirectComposition (less code, no `dcomp` dependency; works on Windows 10/11).
- **2026-10-06 (Phase 1):** Opening the menu **takes focus** (`SetForegroundWindow` on the overlay, allowed because
  INSERT is a `RegisterHotKey`) and closing hands it back. This is how "clicks in the menu don't shoot" works
  externally: the game stops getting input because it isn't focused. The planned `input/cursor` (CursorControl) was
  **dropped**: our window owns its own cursor via ImGui's `WM_SETCURSOR` handling, and the game releases its own when
  unfocused. Revisit only if the in-game test shows a cursor problem.
- **2026-10-06 (Phase 1):** Visibility follows `GetForegroundWindow()` polled every frame (game or overlay in front →
  shown), not `WM_ACTIVATE`.
- **2026-10-06 (Phase 1):** Added `ui/hud` (watermark + optional frame outline, drawn every frame) so the overlay is
  visibly running and its alignment can be checked in-game. Overlay options live in `app::OverlayOptions` until
  the `Settings` struct (now Phase 4).
- **2026-10-06 (Phase 1):** Exit paths for now: Ctrl+C / closing the console (console control handler →
  `core::shutdown_requested`; for a console close, the handler waits up to 3 s for `shutdown_complete`), Alt+F4 on
  the focused menu, or CS2 exiting. DELETE/END come in Phase 9.
- **2026-10-06 (Phase 1, user request):** **Player values dropped**: no Player page, no health / armour / ammo writes,
  no Set / Freeze (Phase 6 kept as "DROPPED" so phase numbers don't shift). The only planned game write left is the
  aimbot's view angles (Phase 5). `settings/settings.h` moves to Phase 4, where the first setting (ESP) appears.
- **2026-10-06 (Phase 1, user request):** **Logo moved up from Phase 10**, done the AC way: `assets/logo.jpg` (README +
  source of `ui/logo_pixels.h`, generated by `tools/make_logo_header.py`, committed so builds need no Python),
  `assets/logo.ico` (exe icon via `external.rc`). In the overlay it's an ImGui user texture (no image decoder
  dependency). The user's files `miraikitsu-chibi-gojo.{jpg,ico}` were renamed to `assets/logo.{jpg,ico}`.
- **2026-10-06 (Phase 1, user request):** The **watermark shows the logo and the active features** ("ESP · Aimbot";
  "No features on" when none). The list comes from the pure `features/feature_summary` (`ActiveFeatures` in
  `AppState::active`, filled from the settings by `app/frame` once features exist). Sidebar groups are now COMBAT
  (Aimbot, Triggerbot), VISUALS (ESP, Misc), SETUP (Settings).
- **2026-10-06 (Phase 1, user request):** Renamed to **External Cheat - by BigH** (`config::kAppName`, overlay window
  class/title). The watermark lists the active features **vertically** (one per line, accent dot) under the title and
  no longer shows the menu key. The menu header lost its "In match" and "INSERT closes" pills (only "External"
  stays; match status is still on Home). Home's header is just the logo and the name (subtitle removed).
  `features::active_feature_names()` added for the vertical list; `feature_summary()` (joined) is kept for Home.
- **2026-10-06 (user request):** New features, all **external** (feasibility table in §3): **bunny hop** (Misc,
  Phase 6, via `m_fFlags` + a `jump` button write), **ESP scoped indicator** (`m_bIsScoped`, Phase 4), **visibility**
  for ESP/aimbot/triggerbot as the **spotted-by heuristic** (`m_entitySpottedState.m_bSpottedByMask`), because exact
  line of sight needs the game's trace (internal). **Triggerbot made fully configurable** (activation, delay, fire
  modes, filters; Phase 5). Phase 6 became "Misc" (bunny hop + radar, bomb timer, spectators, hitsound, previously
  unscheduled). Button writes (jump, attack) join the aimbot's angle write as the only game writes; the write handle
  arrives in Phase 5. The UI placeholders and `features::ActiveFeatures` (now with `bunny_hop`) match this plan.
- **2026-10-06 (Phase 2):** **The dump values stay in charge; the live game is the proof, not a runtime override.**
  `offsets.h` / `schema.h` are compile-time constants copied from the dumps. At startup, `app/diagnostics` proves them
  (signatures, the live schema system, the build number) and only *reports* a mismatch. Reason: §7's offset change
  rule (prove, keep the old value, log it) needs a human in the loop, constants keep `game/` code testable without a
  live schema, and a silently self-updating offset would hide exactly the breakage we want to see. The live values
  are one step away if that ever changes (the diagnostic already has them).
- **2026-10-06 (Phase 2):** **Prove before coding.** CS2 was running, so every engine layout (InterfaceReg, the schema
  system, class infos, signatures) was first checked against the live process with a read-only Python/ctypes script
  in the session's scratch folder (not in the repo), then written in C++ and checked again by `--diag`.
- **2026-10-06 (Phase 2):** Schema classes are found by **scanning a copy of client.dll for self-pointing class
  infos** (one pass, 469 classes), not by walking the type scope's `CUtlTSHash`. The hash table layout is complex
  and changes; the self pointer + name + module layout is simple, and each info's type-scope pointer still ties it
  to `SchemaSystem_001`. Revisit if a class we need ever goes missing from the index.
- **2026-10-06 (Phase 2):** One `core::copy_remote` of client.dll (41 MiB, ~8 ms) serves both the signature scan and
  the class index, then is freed. Signatures only scan `.text`.
- **2026-10-06 (Phase 2):** `--diag` command-line flag (diagnostic only, then exit; exit code 0/2), so the
  after-every-update check (§13) doesn't need the overlay. New files beyond the plan: `core/pe`, `game/schema_system`,
  `game/signatures`, `app/diagnostics`, `tests/helpers/fake_pe.h`.
- **2026-10-06 (Phase 2):** CLAUDE.md corrected after live proof: weapon id chain `0x149A` (was `0x14FA`), entity
  identity `0x70` (was `0x78`), `identity + 0x10` = full handle (was "serial"), no `GameEntitySystem` interface,
  `SchemaSystem_001` lives in `schemasystem.dll`.
- **2026-10-06 (user request):** **Autonomous mode** (§2a), switched on/off by the user. ON: in-game checks are
  auto-approved, each phase is committed (marked "Auto-approved: ...") and pushed, and the next phase starts; stop
  with a `HANDOFF.md` at ~90% context or at a real blocker. Switched ON the same night with standing decisions: live
  write tests allowed (offline match only), nlohmann/json approved, visibility = spotted-by mask, push every commit.
- **2026-10-06 (Phase 3, auto-approved):** Player controllers are found by **index 1..maxClients + designer name**
  (`cs_player_controller`), not by walking all ~300+ entities or reading class info: 20 RPM-cheap checks, proven live.
- **2026-10-06 (Phase 3):** The live view is a **separate `--live` mode** (like `--diag`) instead of printing to the
  normal console: 20 rows at 4 Hz would bury the log. The overlay's Home page shows a summary instead.
- **2026-10-06 (Phase 3):** `game/snapshot.h` replaces the planned `game/structs.h`; `ViewMatrix` lives in
  `maths/projection.h` now (world_to_screen joins it in Phase 4). Snapshots read field by field (≈30 reads per player,
  0.33 ms for 20 players) rather than in big blocks: simple, and fast enough to do every frame.
- **2026-10-06 (Phase 4, auto-approved):** Visibility = **option (2), the spotted-by mask**, from the §2a standing
  decision (the "stop and ask" item was answered in advance). Slot = controller index − 1.
- **2026-10-06 (Phase 4):** **Bones done in Phase 4** after all (the plan said "skip bones"): CS2 was running, so
  model state + 0x80 could be proven on 20 live pawns in minutes, and `--diag` now re-checks it every launch. The
  skeleton, the head circle and (Phase 5) head aim use it; without bones the head falls back to the eye position.
- **2026-10-06 (Phase 4):** `settings::Settings` replaces `app::OverlayOptions` (overlay + esp sections, `config::Range`
  for numbers). The ESP is **off by default**. The box is computed from the feet and the eyes (+8 units), not from
  the bones, so it works without bones; width = height / 2.
- **2026-10-06 (Phase 4):** "Team mode" means **Teams vs Free for all** (CS2 deathmatch is free-for-all), plus "Show
  teammates" in Teams mode. Primitives are plain data (`render/primitives.h`), so the whole ESP decision is
  unit-tested; only `render/painter` touches ImGui.
- **2026-10-07 (Phase 5):** **"The head" is bone 7**, not bone 6 (the jaw), after the user's report and a screenshot
  check. Head circle radius 5 → 6.5. Not an offset change (§7's rule doesn't apply): only which proven bone is used.
- **2026-10-07 (Phase 5):** **Team mode moved to `settings::GeneralSettings`**, shared by ESP, aimbot and triggerbot
  (shown on each page). Shared target checks live in `features/targeting`.
- **2026-10-07 (Phase 5):** The triggerbot fires through the **`attack` button write**, not `SendInput` (proven live).
  Taps are 30 ms; writes only on a change; release skipped while the user holds Mouse 1. Flashed =
  `m_flFlashOverlayAlpha` > half of `m_flFlashMaxAlpha` (instead of `m_flFlashDuration`, which doesn't say how much of
  the flash is left).
- **2026-10-07 (Phase 5):** Keys are polled with `GetAsyncKeyState` in `app/frame` and picked from
  `input::kBindableKeys` (16 keys) until Phase 7's keybind engine. Aimbot default key Mouse 1 (hold), triggerbot
  Mouse 4 (hold).
- **2026-10-07 (Phase 5):** Only the overlay opens a **read-write** handle; `--diag` and `--live` stay read-only.
- **2026-10-07:** **Autonomous mode switched OFF** by the user (back at the PC).
- **2026-10-07:** **No server-side spotted read (option 2).** The user's condition was "only if replication +
  interpolation is the bulk"; measured, it is ~1-2 ms of a ~250 ms median (the server's ~0.5 s re-check is the rest).
  Option 3 (own ray cast) **deferred** by the user: "do it later and come back to it" (roadmap: "Later").
- **2026-10-07 (user decision):** **Bunny hop dropped.** The user was advised it's a poor fit for external (jump timing
  from outside the game feels bad). Removed from §3, the Phase 6 plan, the Misc page placeholder and
  `features::ActiveFeatures`. The `jump` button format stays documented (proven) in case it's ever wanted.
- **2026-10-07 (Phase 6):** The **radar is our own overlay panel**, not the game's radar. "Enlarged radar" and
  "enemy dots on radar" both come from it (size and range sliders; every player drawn). Forcing the game's radar
  (writing `m_bSpotted` on every bot) was not done: it's a write every frame for something a read-only drawing does
  as well. No map image underneath (that would mean parsing the map's overview from the game files); dots on a dark
  square, rotated with your view by default.
- **2026-10-07 (Phase 6):** The bomb timer reads **`client.dll + dwPlantedC4` → `C_PlantedC4*`**, proven live on two
  plants, with the bomb's handle round-trip through the entity list as the stale-pointer guard. A first version
  searched the entity list for a guessed designer name (`planted_c4`) and showed nothing in the user's test: the
  bomb's identity has no name. **User rule from this: never guess a name or offset; check the full dumper output
  (`C:\Users\Harry\Desktop\output`) and prove it live first.**
- **2026-10-07 (Phase 6):** Phase 6 runs **one feature per check**: build one, the user checks it in-game, commit,
  then the next (as the roadmap says), even with autonomous mode off and several features left.
- **2026-10-07 (Phase 6):** The spectator list reads **every dead player's observer target** (controller →
  `m_hObserverPawn` → observer services), proven live (bots watched our pawn). It lists spectators of **you**, or,
  while you're dead and watch someone in first/third person, of **that player** ("Watching Kev"). Free camera and the
  death cam count as watching nobody. The panel code the bomb timer had was moved to a shared `render/panel`
  (PanelWriter) instead of being copied. No colour pickers for the list: enemy red, teammate blue.
- **2026-10-07 (user decision):** **Hitsound dropped.** Removed from §3, the Phase 6 plan, the Misc page's
  "Coming next" card (the card is gone: nothing else was planned there) and `features::ActiveFeatures`. Phase 6 ends
  with the radar, the bomb timer and the spectator list.
- **2026-10-07 (Phase 7):** **The menu key stays the overlay's `RegisterHotKey`**, registered with whatever key is
  bound to `menu_toggle`, not an engine action: the hotkey is what lets the menu take focus. So it must be a keyboard
  key that isn't a modifier, and it can't be unbound.
- **2026-10-07 (Phase 7):** **Panic and exit are wired now** (the Phase 7 action list names them): panic = every
  feature's Enabled off, toggle keys off, attack released, menu closed; exit = the existing clean shutdown. Phase 9
  keeps the exit button and the robustness review. **Preset actions wait for Phase 8**, when presets exist.
- **2026-10-07 (Phase 7):** `BindMode` moved from `settings` to `input/actions` (+ `press`). The aim key, trigger key
  and modes live in `settings::KeybindSettings`; `TriggerActivation` became always / key. `features::KeyActivation`
  is gone (the engine does it). Toggle keys now **keep their state over Alt+Tab** (Phase 5 reset them on focus loss).
- **2026-10-07 (Phase 7):** On/off keys for the radar, bomb timer and spectator list were added too (unbound by
  default): the registry makes each one a single line. Key names: AC's (`INSERT`, `LSHIFT`, `F5`) but `Mouse 1`-`5`
  for the mouse, as the UI showed before.
- **2026-10-07 (Phase 7, user choice):** **Presses come from raw input** (`RegisterRawInputDevices`, keyboard + mouse,
  `RIDEV_INPUTSINK` on the overlay window), not from once-per-frame polling, which lost taps while the overlay's
  frames were slow. Chosen by the user over a polling thread (would break the one-thread rule) and GetAsyncKeyState's
  "pressed since last call" bit (documented as unreliable). Raw input is not a hook (§1): nothing runs in or changes
  the game, Windows only reports input it delivers anyway. Two presses in one frame flip a toggle twice.
- **2026-10-07 (Phase 8, user choices):** **nlohmann/json v3.12.0 vendored** (copied from the AC project's copy, its
  SHA-256 checked against the official release asset). **Presets** with the proposed values: Off = every feature off;
  Chill = corner-box ESP, body aimbot FOV 3 / smoothing 12 / visible only, no triggerbot; Medium = full ESP with
  skeleton and head circle, head aimbot FOV 6 / smoothing 6 / visible only, triggerbot on the key, 80 ms, single;
  Rage = everything with snaplines, head aimbot FOV 30 / smoothing 1, not visible-only, triggerbot always, 0 ms, hold,
  no flash/air/scope limits. Chill, Medium and Rage also turn the radar, bomb timer and spectator list on.
- **2026-10-07 (Phase 8):** Profiles follow the AC design (one field list per section, forgiving load, atomic save,
  read-only built-in `default`, `.last_profile`) and live **next to the exe**. Profiles save **everything**,
  keybinds included; presets never change keybinds, colours, team mode, team checks, max distances, the overlay or
  positions. Profile operations run in `app/frame` at the start of the next frame (the menu only queues them).
  The profile shows on Home (not in the header: the user wanted the header to keep only the "External" pill).
- **2026-10-07 (Phase 8):** The three fractional-alpha colour defaults became byte-exact (`Color::rgba`), so the
  defaults survive `#RRGGBBAA` unchanged; the visible difference is under 0.5 %.
- **2026-10-07 (Phase 9):** Phase 9 was mostly done by Phase 7, so it became a **review, not a rewrite**: no new
  modules. A shutdown the user asks for (exit key, Exit button, Alt+F4) sets `core::shutdown_requested`, so main()
  skips "Press Enter"; "cs2.exe has closed" and errors keep the wait (the console should stay readable). The Exit
  button asks for confirmation only when there are unsaved changes. When the game closes the tool **exits** rather
  than waiting for a new cs2.exe (the handle, module bases and offsets belong to that process); restart the tool with
  the game.
- **2026-10-07 (Phase 10, user choices):** **Top tabs + two columns of panels** (over an icon rail or a feature list),
  so it no longer looks like the AC project's sidebar. **Look: dark black / grey / off-white**, easy on the eyes at
  night (the user's words), as the default theme "Midnight"; customisation = **themes + an accent colour** only (the
  user didn't pick menu size / density, menu look options or watermark options). Picking a theme also sets its accent.
  The header keeps only the "External" pill (the user's Phase 1 wish) plus a close button; the match status lives on
  Home. Presets moved from Settings to Home (dashboard). The Home page lost its big logo: the header already shows it.
- **2026-10-07 (Phase 10):** Themes are **pure data in `settings/themes`** (so the default accent in `settings.h` and the
  palette come from one table, and contrast is unit-tested); `ui/theme` turns them into ImGui colours. Theme and accent
  live in the existing `overlay` profile section (two new keys, no schema bump: missing keys keep their defaults).
- **2026-10-07 (Phase 11, user request):** The README follows the **AC project's README** structure (centred header,
  screenshots first, features, use, troubleshooting, tests, layout, a numbered "How it works", lessons per phase,
  credits) and **its pictures come from the live game**, not the offscreen harness: the user left CS2 open near bots
  for it. The README says plainly what external can't do and that no Cheat Engine / IDA was used (the roadmap item
  had listed them as possible sources). Images: in-game shots as JPEG (~400 KiB), menu pages as cropped PNG
  (~70 KiB).
- **2026-10-06 (Phase 3):** Weapons are named from the **item definition index** (table in `game/weapon.cpp`), not the
  designer name, because some weapons share a designer name (USP-S / P2000). Each also gets a `WeaponClass` for the
  Phase 5 triggerbot filter.

---

## 16. Maintenance rule

**After EVERY task**, before finishing:
1. Update the **file tree** (§5): status markers, new/removed files, one-line descriptions.
2. Tick the **roadmap** checkboxes (§13) and update **Current status** (§14) and its **Next** line.
3. Update `docs/offsets.md` with every offset, signature, and schema field found this session, plus its source
   (repo + commit + date) and how it was proven.
4. Update any rules, gotchas or patterns that changed (§4–§12) and add to the **Decision log** (§15).
5. Add a dated entry to `docs/DEVLOG.md` (what was built, bugs found and how they were fixed).
