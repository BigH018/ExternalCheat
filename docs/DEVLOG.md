# Dev log

## 2026-10-06: Phase 0, scaffold and process handle

**Built**
- Private GitHub repo `BigH018/cs2-external`; initial commit holds `CLAUDE.md`, `.gitignore`, `.gitattributes`.
- `cs2-external.sln` (Debug|x64, Release|x64) with two projects: `external` (`cs2_external.exe`) and `tests`
  (`tests.exe`). Shared settings in `props/common.props`.
- `core/process`: `find_process` (Toolhelp32 process snapshot), `open_handle` (OpenProcess with an exact access
  mask), `module_base` (Toolhelp32 module snapshot, retries on `ERROR_BAD_LENGTH`), and the `UniqueHandle` RAII
  wrapper.
- `core/memory`: `read_bytes` / `write_bytes` (`__try/__except`-guarded RPM/WPM, exact byte count required),
  `safe_read<T>`, `read<T>` (optional), `safe_write<T>`, and `is_plausible_pointer` (constexpr).
- `core/log`: `logger::info/warn/error` using `std::format`.
- `game/offsets.h` with `dwLocalPlayerPawn` only; `game/player` with `read_local_pawn`.
- `main.cpp`: finds `cs2.exe`, opens a **read-only** handle, prints the PID, `client.dll` and `engine2.dll` bases, and
  the local pawn pointer, then waits for Enter.
- Vendored doctest v2.5.3; `tests/core/test_memory.cpp` (9 cases, 39 assertions) runs the RPM/WPM wrappers against
  our own process, including a reserved-but-uncommitted page and a read that runs off the end of a committed page.

**Problems and fixes**
- Property sheets imported before `Microsoft.Cpp.props` get overridden by the toolset defaults. `common.props` is
  now imported after it. `WindowsTargetPlatformVersion` and `CharacterSet` have to be set *before* it, so those two
  live in each `.vcxproj`.

**Verified here:** Debug and Release build with zero warnings; tests pass in both; with CS2 closed the tool prints
`cs2.exe not found`.

### Later the same day: in-game check + AC-inspired refactor

- **In-game (build 14189, offline deathmatch, frozen bots):** PID 18592, `client.dll` `0x7FFD62930000`,
  `engine2.dll` `0x7FFDD9CE0000`, local pawn `0x4DF564BF800`. Phase 0 acceptance met. Main menu (`none`) and the
  access-denied path weren't exercised.
- **Dumps:** copied `info.json`, `client_dll.json`, `interfaces.json`, `buttons.json` (and the identical
  `offsets.json`) from the user's dumper output. `info.json` gives the build (14189). The real `client_dll.json` has
  542 classes / 14 enums, not the 3301 / 569 CLAUDE.md used to claim.
- **Refactor after reading the AC project:** `core::Memory` became a pure interface (`read_bytes` → private virtual
  `do_read`) with `core::ProcessMemory` (RPM/WPM, the only Win32 part) and `tests/helpers/fake_memory.h`. `game/`
  code now takes `const core::Memory&`, so it's testable without the game (`tests/game/test_player.cpp`). Added
  `config.h` (pointer bounds, process/module names). Logger switched to AC's noexcept `[+] [!] [x]` version.
  Reformatted to AC's style (unindented namespaces).
- **Tests:** 13 cases / 57 assertions, Debug and Release. Re-ran the tool in-game after the refactor: same output.

## 2026-10-06: Phase 1, overlay window + ImGui shell

**Built**
- Vendored Dear ImGui **v1.92.9b** (core + Win32 + DX11 backends; 15 files, every blob hash checked against the tag)
  and compiled it in a new static lib, `external/vendor.vcxproj`. ImGui config defines live in `props/imgui.props`.
- `ui/overlay_window`: our own `WS_POPUP` window (`TOPMOST | LAYERED | TOOLWINDOW`, plus `TRANSPARENT | NOACTIVATE`
  while the menu is closed) with its own D3D11 device and swap chain. Per-pixel transparency comes from
  `DwmExtendFrameIntoClientArea(-1)` plus a blt-model (`DXGI_SWAP_EFFECT_DISCARD`) swap chain cleared to (0,0,0,0).
  It covers the game's client rect, resizing the swap chain when the game window changes size.
- INSERT is a `RegisterHotKey` on the overlay, registered only while CS2 or the overlay has focus. Opening the menu
  drops `WS_EX_TRANSPARENT | WS_EX_NOACTIVATE` and calls `SetForegroundWindow(overlay)`: the game loses focus, lets go
  of the mouse, and stops getting clicks. Closing restores click-through and hands focus back to the game. Focus going
  anywhere else (Alt+Tab, the game's title bar) closes the menu; the overlay hides whenever neither CS2 nor the overlay
  has focus, or the game is minimised.
- `ui/imgui_layer` (context, Segoe UI fonts, backends), `ui/theme` (the AC navy palette), `ui/widgets` (page header,
  cards, info rows, pills, "planned" cards), `ui/menu` (header + grouped sidebar), `ui/pages/*` (Home with live status;
  ESP/Aimbot/Triggerbot/Player/Misc placeholders; Settings with live watermark / frame-outline switches), `ui/hud`
  (watermark + frame outline on the background draw list).
- `app/frame` + `app/state`: the orchestrator loop. The Home page's match status re-reads the local pawn at ~4 Hz.
- `core/process`: `find_main_window(pid)` (largest visible unowned top-level window) and `is_running(handle)`;
  `core/runtime.h`: `shutdown_requested` / `shutdown_complete`, set by the console control handler (Ctrl+C, console
  closed). `main.cpp` makes the process per-monitor DPI aware v2 before any window exists.

**Problems and fixes**
- ImGui 1.92.8+ swapped `ImDrawList::AddRect`'s last two parameters (`thickness` before `flags`). The old order is
  `= delete`d under `IMGUI_DISABLE_OBSOLETE_FUNCTIONS`, so it was a compile error (C2280), not a silent bug.
- `handle_message` first used `hwnd_`, which is still null during the messages `CreateWindowExW` sends
  (`WM_NCCREATE`...). It now uses the `hwnd` it receives.

**Verified here:** Debug and Release build with zero warnings; `tests.exe` 13/13 in both. Smoke test with CS2 running
(not focused): device (feature level 11.0), ImGui and the game window were all found, and the overlay stayed hidden.
In-game check by the user still to do.

### Later the same day: logo, active-feature watermark, Player dropped (user feedback)

- **Logo** (moved up from Phase 10), the AC way: the user's `miraikitsu-chibi-gojo.{jpg,ico}` became
  `assets/logo.{jpg,ico}`. `tools/make_logo_header.py` (Pillow) turns the jpg into `src/external/ui/logo_pixels.h`
  (128x128 RGBA, committed); `ImGuiLayer` registers it as an ImGui user texture, which the DX11 backend uploads and
  releases itself. It shows in the menu header, on Home (96 px) and in the watermark. `external.rc` makes the ico the
  exe's icon. README shows the jpg.
- **Watermark:** `[logo] Internal Cheat by BigH | <active features> | INSERT: menu`. The feature list comes from the
  new pure `features/feature_summary` ("ESP · Aimbot", fixed order; "No features on" when empty), 4 new test cases.
  Nothing can be switched on yet, so it says "No features on" until Phase 4 fills `AppState::active` from the settings.
  Home's status card shows the same line.
- **Player page and Phase 6 dropped** at the user's request (no health/armour/ammo writes). Sidebar: COMBAT (Aimbot,
  Triggerbot), VISUALS (ESP, Misc), SETUP (Settings). Roadmap and docs updated; Phase 6 stays as "DROPPED".
- `.gitattributes`: `*.jpg` / `*.jpeg` marked binary.

**Verified here:** Debug and Release build with zero warnings, tests 17/17 in both. In-game check of the new watermark,
logo and menu still to do.

### Later the same day: name, watermark layout, header (user feedback with a screenshot)

- Renamed to **External Cheat - by BigH** (`config::kAppName`; overlay class `ExternalCheatOverlay`).
- Watermark: logo + "External Cheat by BigH", then the active features **one per line** with an accent dot ("No
  features on" when none). The "INSERT: menu" part is gone. New `features::active_feature_names()` returns the list;
  tests updated (17 cases / 66 assertions).
- Menu header: the "In match" and "INSERT closes" pills are gone; only the "External" pill stays.
- Home: the subtitle under the name is gone; the name sits beside the logo, centred on it.

**Verified here:** Debug and Release build with zero warnings, tests 17/17 in both.

### Later the same day: feature plan update, Phase 1 approved

- The user asked for a bunny hop, an ESP scoped indicator, a visibility check for aimbot/ESP and a more configurable
  triggerbot. All external; checked against the build-14189 dump: `m_fFlags` + the `jump` button (bunny hop),
  `m_bIsScoped` (scoped), `m_entitySpottedState.m_bSpottedByMask` (visibility heuristic: exact line of sight would
  need the game's trace, i.e. internal), `m_iIDEntIndex` + the `attack` button + `m_flFlashDuration` (triggerbot).
  CLAUDE.md §3 has a feasibility table, §7 the field list, the roadmap the new items (Phase 6 is now "Misc", replacing
  the dropped Player phase). `docs/offsets.md` lists the values as planned (not in code yet).
- Caught while writing the docs: two button offsets typed from memory as hex were wrong; converted from the dump's
  decimal values instead (`jump` `0x22324E0`, `attack` `0x2231FD0`). Lesson already in CLAUDE.md: copy, never retype.
- UI placeholders (ESP, Aimbot, Triggerbot, Misc) list the new options; `features::ActiveFeatures` gained
  `bunny_hop` (tests updated).
- Phase 1 approved by the user after the in-game check (menu over an offline bot match, 1920x1080, 270 FPS).

## 2026-10-06: Phase 2, offsets, signatures, interfaces, schema

CS2 was running (build 14189, `-insecure`, an offline bot match), so every layout was proven against the live process
with a read-only Python/ctypes script (scratch, not in the repo) **before** the C++ was written, instead of guessed.

**Built**
- `game/offsets.h`: all 29 dumped globals (client, engine2, inputsystem, matchmaking, soundsystem), all 16 buttons,
  the RVAs of the 4 interfaces we use, `kDumpBuildNumber = 14189`, 8 signatures, and the hand-found engine layouts
  (`offsets::layout`). Values generated by a script from `docs/dumps/*.json`, not retyped.
- `game/schema.h`: 29 fields across 13 classes (everything the Phase 2 acceptance list and Phases 3-6 need), plus
  `schema::kFields`, the table the diagnostic walks.
- `core/pattern`: IDA-style `Pattern` (parse, `matches_at`, `find_all` with a memchr anchor), `copy_remote` (1 MiB
  reads, page-by-page fallback, unreadable pages zero-filled and counted), `rip_relative` / `rip_relative_target`.
- `core/pe`: a loaded module's headers, sections and exports read through `core::Memory` (forwarders rejected).
- `core::read_string` (in `memory.h`): page-bounded string reads.
- `game/interfaces`: `CreateInterface` from outside. Its first instruction (`mov r9, [rip+x]`) gives the head of the
  module's `InterfaceReg` list; each create function (`lea rax, [rip+instance]; ret`) is decoded, not called.
- `game/schema_system`: the type scope from `SchemaSystem_001` (vector at +0x190), and every class info found in one
  pass over a copy of client.dll (class infos are static data whose first qword points to itself).
- `game/signatures`: resolve a signature over the copy; several matches are fine if they all agree.
- `app/diagnostics` + `main.cpp`: the startup diagnostic (build, interfaces, signatures, schema, buttons/globals).
  `cs2_external.exe --diag` prints it and exits (exit code 0 = all OK, 2 = something failed). The Home page shows
  "Offsets: build 14189: all 45 checks OK".
- Tests: `test_pattern`, `test_pe`, `test_interfaces`, `test_schema_system`, `test_signatures`, `test_offsets`, and
  `read_string` cases; `helpers/fake_pe.h` builds a tiny PE32+ in a FakeMemory; FakeMemory got `put_bytes` /
  `put_string`. 41 cases / 652 assertions.

**Found along the way**
- The `InterfaceReg` walk matched `interfaces.json` for all 59 interfaces in client, engine2, schemasystem,
  inputsystem and tier0.
- The live schema system matched the dump for **3013 fields in 469 classes**, 0 mismatches. The dump's 542 also has
  classes from other libraries (entity2, pulse_runtime_lib, compositematerialslib) registered in the client scope.
- `dwLocalPlayerPawn` has no code reference of its own: it's a field of the prediction object (`dwPrediction + 0xF8`).
  Its signature finds the prediction getter and adds 0xF8.
- Of the a2x-style signatures tried first, five still hit; `dwEntityList`, `dwPlantedC4` and the prediction one
  didn't, or hit the wrong place, and were rebuilt from the stores to each global.
- `GameEntitySystem` isn't a registered interface in CS2; it's the global `dwGameEntitySystem` (= `dwEntityList`).
- **CLAUDE.md §7 had three wrong facts**, all proven live and corrected: the weapon item-definition chain is `0x149A`
  (not `0x14FA`: an AK-47 reads 7 there, `0x14FA` reads 0); an entity identity is `0x70` bytes (not `0x78`); and
  `identity + 0x10` holds the whole handle (`0x1B182E6`), not just the serial.
- `dwNetworkGameClient_isBackgroundMap` in the dump (`0x2C143F`) doesn't look like a field offset; copied but flagged.

**Problems and fixes**
- A unit test mapped a 0x40-byte FakeMemory region for strings; `read_string` reads up to the page end, which real
  memory always allows (mapping is per page), but FakeMemory doesn't. The test now maps a whole page.

**Verified here:** Debug and Release build with zero warnings; `tests.exe` 41/41 in both. `cs2_external.exe --diag`
(Debug and Release) against the running game: **all 45 checks OK**; copying client.dll (41 MiB) takes ~8 ms.
In-game check by the user: normal start in an offline bot match printed all 45 checks OK and the overlay started.
Approved and committed.

## 2026-10-06 (night, autonomous mode): Phase 3, entity list + snapshots

Auto-approved: user was asleep and did not personally verify this phase. If in-game testing fails, this commit is the
first suspect for rollback.

CS2 was running (build 14189, de_mirage, a bot match with frozen bots), so the layouts were proven with a read-only
Python script first, then the C++ was checked with the new `--live` view.

**Built**
- `game/handle`: entity system pointer, identity address (chunk + slot × 0x70), `entity_at`, `resolve_handle` (the
  identity's handle must equal the handle, so a reused slot doesn't resolve).
- `game/entities`: `designer_name`, `find_player_controllers` (indices 1..maxClients named `cs_player_controller`).
- `game/player`: `read_player` (controller: name, team, pawn handle, alive; pawn: health, life state, position, eye
  height, armour, flags, scoped, dormant, weapon) with validity checks; `read_game` (globals, view matrix if sane,
  every player, which one is local).
- `game/globals` (CGlobalVars in one read), `game/view` (view matrix), `game/weapon` (pawn → weapon id → name and
  class), `game/snapshot.h`, `maths/vec.h`, `maths/projection.h` (`ViewMatrix` + `is_sane`).
- `app/live_view` + `--live`: a console table at ~4 Hz, in place (VT escapes) or as plain frames when redirected.
- Home page: Map, Players, You rows from a ~4 Hz snapshot.
- Tests: `helpers/fake_entities.h` (a fake chunked entity system) and 29 new cases (handle, entities, player, globals,
  view, weapon, vec). 70 cases / 1264 assertions.

**Found along the way**
- `CGlobalVars` doesn't have the "typical" layout CLAUDE.md gave: in build 14189, `+0x10` is maxClients (64), curtime
  is at `+0x30`, tickcount at `+0x44`, the map name at `+0x188`. Proven by sampling twice 2 s apart. CLAUDE.md fixed.
- A pawn's designer name is `c_cs_player_for_precache`, not `cs_player_pawn`.
- A full snapshot of 20 players costs ~0.33 ms (≈600 RPM calls): fine to do every frame for the ESP.

**Problems and fixes**
- Scripted edits through a Bash heredoc lost a level of backslashes: `game\view.cpp` turned into a vertical tab
  (MSBuild: "hexadecimal value 0x0B is an invalid character"), `app\frame` / `game\test_*` replacements silently
  didn't match (LNK2019 for `run_live_view`, missing tests), and `'\n'` became a raw newline in C++. Fixed with the
  Edit tool; backslash text is no longer edited through heredocs.
- `--live` redirected to a file failed ("no virtual terminal support"); it now falls back to plain frames.

**Verified here:** Debug and Release build with zero warnings; `tests.exe` 70/70 in both. `--live` (Release) against
the running game: 20 players, names/teams/weapons/positions right, globals ticking at 64 Hz, view matrix sane.
`--diag`: all 45 checks OK. The normal start still brings up the overlay and logs "In a match".
**Not exercised in-game:** death and respawn, a match restart, a map change, the main menu (covered by unit tests
only). Bots were frozen, so moving positions weren't seen either.

## 2026-10-06 (night, autonomous mode): Phase 4, world-to-screen + ESP

Auto-approved: user was asleep and did not personally verify this phase. If in-game testing fails, this commit is the
first suspect for rollback.

**Proven live first (read-only script + screenshots, build 14189, de_mirage):**
- Row-major projection: the point 1000 units along the view angles lands exactly on (960, 540); up = up the screen,
  right = right, behind = negative w. Gives the Phase 5 angle convention too (pitch positive = down).
- **Bones:** scene node + `m_modelState` (0x140) + 0x80 → 32-byte bones. The joint indices differ from the commonly
  published ones (bone 27 is a look-at point 1000 units ahead; the legs are 17-22), so they were mapped from live
  positions in each bot's own frame, on CT and T models.
- Spotted-by masks all read 0: no bot was in line of sight (matches the screen and the radar). The bit numbering
  stays unverified until a bot is in plain view.

**Built**
- `maths/projection` (`world_to_screen`), `maths/skeleton` (indices, links, `project_skeleton`), `color.h`,
  `render/primitives.h`, `render/painter`, `settings/settings.h` (overlay + ESP; `config::Range`),
  `game/bones`, `game/visibility`, `features/esp` (`build_esp` and helpers), the ESP page (every option, colour
  pickers with opacity), `app/frame` (snapshot every frame while the ESP is on, ESP under the HUD, `active.esp`).
- Snapshot: `spotted_by_mask`, `bones`, `head_position()`, `slot()`.
- `--diag` section 6 "Match reads": CGlobalVars sanity and the bones on every alive player (48 checks in a match).
- Tests: projection (synthetic camera + live matrix), skeleton, bones, visibility, colour, settings, ESP (box
  geometry, team modes, skips, visibility colours, labels, styles). 97 cases / 1728 assertions.

**Problems and fixes**
- The "transposed matrix is wrong" test first compared the crosshair point, which a transposed matrix also puts near
  the centre (everything collapses there). It now uses a bot's feet, which don't project at all transposed.
- `Set-Content -Encoding utf8` (PowerShell 5.1) added a BOM to `main.cpp`; stripped.

**Verified here:** Debug and Release build with zero warnings; tests 97/97 in both; `--diag` all 48 checks OK
(bones: 20 of 20 players, head 52-60 units up). With every ESP option switched on in a scratch build (reverted, not
committed), screenshots over the live game showed boxes, skeletons, head circles, health, names, weapons, distances
and snaplines on all 19 bots, the skeletons inside their boxes; the ESP page rendered correctly. Read time with
bones: ~0.37 ms per snapshot.
**Not exercised in-game:** the visible colour (no bot in line of sight), SCOPED (bots had pistols), moving or dead
bots, other resolutions, a map change.

## 2026-10-07: Phase 5, aimbot + triggerbot

Started in autonomous mode the night before; the session ran out of context mid-phase without a `HANDOFF.md`. This
session picked up from the uncommitted tree (angles, aimbot, targeting, writes, settings and a key list existed; the
triggerbot, the frame wiring, the pages, the tests and the project entries didn't, and `esp_page.cpp` still used the
old `esp.team_mode`, so the tree didn't build). The user switched autonomous mode **off** partway through.

**User feedback on Phases 3-4:** everything works except the ESP head circle, which sat on the neck / shoulders.

**Head fix (proven live, build 14189):** bone 6 is the head joint at the base of the skull (~4 units below the eyes);
projected onto a screenshot of a bot in plain view it lands on the jaw. Bone 7 sits at eye height, 4.6 units forward,
inside the head. The head circle (now radius 6.5), head aim and the triggerbot's head check use bone 7
(`maths::bone::kHeadCentre`); the skeleton keeps bone 6. No offset changed.

**Proven live along the way**
- Spotted-by bit: with one bot in view, only its mask had bit 0 (our slot) set. Phase 4's open item is closed.
- `m_iIDEntIndex` = the target pawn's entity index (211 on the head, -1 fifteen degrees off). View restored.
- View-angle writes stick (40/40 at 100 ms and 10 ms intervals).

**Built**
- `maths/angles` (normalize, calc_aim_angles, angular distance, frame-rate-independent smoothing, step_towards,
  forward, distance_to_ray), `features/targeting` (shared enemy / live / visible / distance checks, now also used by
  the ESP), `features/aimbot` (aim point head / body / nearest, candidates, priority, compute_aim, FOV circle),
  `features/triggerbot` (target + block reasons, the firing state machine: reaction, single / burst / hold, shot
  delay), `features/activation` (hold / toggle), `game/writes` (button state, view angles), `input/keys` (the keys a
  feature can use until Phase 7).
- Settings: `GeneralSettings::team_mode` (shared by every feature), `AimbotSettings`, `TriggerbotSettings`.
- Pages: Aimbot and Triggerbot with every option and a live status line; `ui/pages/controls` shared by the feature
  pages (team mode on each).
- `app/frame`: polls the aim / trigger keys, aims and fires only while the game is in front and the menu is closed,
  writes attack only on a change, lets go of attack when the overlay hides or the tool exits; FOV circle; watermark
  lists Aimbot / Triggerbot. The overlay opens a read-write handle; `--diag` and `--live` stay read-only.
- Snapshot: `pawn_index`, `LocalState` (crosshair entity, flash alpha, view angles). Two schema fields from the dump
  (`m_flFlashOverlayAlpha`, `m_flFlashMaxAlpha`), checked by `--diag` (50 checks).
- Tests: angles, aimbot, triggerbot (state machine timings for each fire mode), writes, settings, head circle on bone
  7; `helpers/fake_game.h`. 125 cases / 2143 assertions.

**Problems and fixes**
- A Python heredoc edit of `external.vcxproj` turned `features\aimbot` into `features<BEL>imbot` (the known backslash
  collapse); the file was restored with git and edited with the Edit tool.

**Verified here:** Debug and Release build with zero warnings; tests 125/125 in both; `--diag` all 50 checks OK.
**Not verified (needs the user in-game):** the aimbot and triggerbot running inside the overlay (aiming, smoothing
feel, FOV circle size, every triggerbot option), the pages' layout.
**User check (2026-10-07):** aimbot, triggerbot and the fixed head circle all good in-game; approved for commit.

## 2026-10-07: spotted-by delay measured (no code change)

The user asked whether visibility could be faster and wanted the delay split into server compute vs replication +
interpolation, with option 2 (read the server's mask from server.dll) only if replication was the bulk.

- Found the server entity system (the client's entity-list signature hits once in server.dll) and the client →
  server mapping (same indices; server handles have 7 extra serial bits). Details in `docs/offsets.md`.
- The spotted bit depends on where you look (on at 45° off, off at 90°+), so the measurement could be driven by
  view-angle writes alone (the user ran up to a bot once; then hands off).
- 60 transitions: server compute median ~250 ms (0-490: a ~0.5 s re-check), replication ~1-2 ms, input ~12 ms.
- **Result: replication is ~1 % of the delay, so option 2 was not built** (as the user decided in advance). Option 3
  (own ray cast) is deferred: the user wants to come back to it later (CLAUDE.md roadmap, "Later").
- Also seen: 2 of 60 single view-angle writes were dropped by the game.

## 2026-10-07: bunny hop dropped

User decision: bunny hop is out (advised it's an internal-style feature that feels bad externally). Removed from the
plan (CLAUDE.md §3, Phase 6), the Misc page placeholder, `features::ActiveFeatures` and the watermark names (test
updated). The proven `jump` button format stays in `docs/offsets.md`. Also checked: server.dll has no console variable
for the spotting re-check interval (`CCSEntitySpotting` is a built-in game system), so the ~0.5 s can't be tuned.

## 2026-10-07: Phase 6 started, radar (1 of 4)

Phase 6 is done one feature at a time, each checked in-game by the user. First: the **radar**, our own, drawn by the
overlay (the game's radar isn't touched, nothing is written to the game).

- `features/radar` (pure): `radar_panel` (corner), `radar_offset` (world → radar, rotated so where you look is up, or
  north-up = world +y like the game's radar), `radar_direction` (a player's yaw on the radar), `clamp_to_square`,
  `build_radar` → primitives: background, cross, half-range ring, border, range label, a dot per player (enemy
  visible / hidden from the spotted-by mask, teammates in teams mode), facing lines, names, out-of-range players faded
  on the edge, your arrow.
- `settings::RadarSettings` (off by default; top-right, 260 px, 40 m, dots 4 px, rotate on) + ranges in `config.h`.
- Misc page: Radar and Radar colours cards; the other three Phase 6 features listed as coming next.
- `render`: `FilledCircle`, `FilledTriangle`, `TextAnchor::bottom_right`.
- `PlayerSnapshot::eye_angles` from `m_angEyeAngles` (already in `schema.h`), proven live on all 19 bots (`docs/offsets.md`
  "Players").
- The radar counts as a per-frame feature (the snapshot is read every frame while it's on) and shows in the watermark.

**Problems and fixes**
- First screenshot attempt showed no overlay: `Start-Process -WindowStyle Minimized` gave the tool's console the
  foreground, so the overlay hid itself (correct behaviour). Running it with `-NoNewWindow` keeps CS2 in front.
- `python -I` hides the user site-packages, so Pillow wasn't found for the screenshot; inline one-liners run without
  `-I`.

**Verified here:** Debug and Release build with zero warnings; tests 138/138 in both (+13: radar maths, layout,
filters, colours, edge clamping, facing, settings defaults, eye angles read); `--diag` all 50 checks OK. A screenshot
over the live game (de_mirage, 1920x1080, radar temporarily on by default, then reverted) showed the radar in the
top-right with every player, your arrow up, facing lines, faded edge dots and "40 m"; the 3 "visible" enemies matched
the 3 red dots on the game's own radar, and the bot in the doorway ahead sat just above your arrow facing you.
**Not verified (needs the user in-game):** the Misc page controls, rotation off, other corners and sizes, moving
around (dots tracking smoothly), teams vs free-for-all.
**User check (2026-10-07):** all good in-game; approved for commit.

## 2026-10-07: Phase 6, bomb timer (2 of 4)

Radar approved by the user and committed. Next: the **bomb timer** (read-only).

**First version: wrong.** It searched the entity list for the designer name `planted_c4`, a guess, gated by
`C_CSGameRules::m_bBombPlanted`. The user planted a bomb and saw nothing. Read live: the bomb's identity has no
designer name at all, and the dumper output names nothing like it. The user asked not to guess and to check the full
dumper output (`C:/Users/Harry/Desktop/output`): it has `dwPlantedC4` = `0x24CA930` (same as ours) and the
`C_PlantedC4` fields (all matching), but no layout for the global, so that was proven live:
- `client.dll + dwPlantedC4` → the `C_PlantedC4` itself (one dereference), 0 with no bomb, set from the plant to the
  next round start (also after a defuse); seen on two plants.
- Its identity's handle resolves back to it through the entity list (slot 232): used as the stale-pointer guard.
- `m_flC4Blow - curtime` counted 39.78 → 36.78 over 3 s right after a plant: the same clock.
- `--diag` while it ticked: "Bomb (dwPlantedC4) site 0, 18.0 s of 40 s left" OK.

**What's in the code now**
- `game/bomb`: `read_bomb` (dwPlantedC4 → stale guard → `read_planted_bomb`), `read_planted_bomb` (C_PlantedC4
  fields, position, defuser pawn), `read_bomb_planted` (game rules, for `--diag`).
- `features/bomb_timer` (pure): phase (ticking / defusing / defused / exploded), seconds and fraction left, the
  defuse verdict (over 10 s: no kit needed, over 5 s: kit needed, else too late), a defuse in progress (time left, in
  time or not, the defuser's name), distance; `build_bomb_timer` → a panel at the top-centre.
- 11 schema fields from the dump (`C_CSGameRules::m_bBombPlanted`, 10 on `C_PlantedC4`), all matching the live schema;
  `CEntityInstance::m_pEntity` (dump value, not live-schema checkable: `--diag` first reported it "class not found",
  so it was taken out of `kFields`; the bomb check covers it).
- `--diag`: game rules (a check) and, while a bomb is planted, the bomb read the overlay's way (a check): 62 checks
  in a match, 63 with a bomb down.
- Settings: `BombTimerSettings` (off by default; height, defuse hint, distance); Misc page card.

**Verified here:** Debug and Release zero warnings; tests 153/153; `--diag` all 62 OK (and the bomb check OK during a
planted round).
**User check (2026-10-07):** all works. The user asked for two lines on the countdown bar: the latest a defuse can
start without a kit (10 s left, yellow mark) and with one (5 s, red mark); added (`BarMark` in `features/bomb_timer`,
positions tested), everything else unchanged. Tests 154/154. Approved for commit.

## 2026-10-07: Phase 6, spectator list (3 of 4)

Bomb timer approved and committed. Next: the **spectator list** (read-only).

**Proven before coding** (the full dumper output, then live reads; CS2 build 14189, a round-based de_mirage bot
match the user was playing): `CCSPlayerController::m_hObserverPawn` → observer pawn → `m_pObserverServices` →
`m_iObserverMode` / `m_hObserverTarget`. A 25-minute read-only monitor logged every change: dead bots sit in roaming
(death cam) for ~5 s, then watch a player pawn in first person; several bots of both teams watched **our pawn**
(target handle = our `m_hPlayerPawn`). Living players keep stale values, so only dead players are read. Our own
deaths showed us watching other pawns. Details: `docs/offsets.md` "Observer".

**What's in the code now**
- 4 schema fields from the dump (`m_hObserverPawn`, `m_pObserverServices`, `m_iObserverMode`, `m_hObserverTarget`),
  all matching the live schema.
- `game/observer`: `read_observer` (controller → observer pawn → services → mode + resolved target).
  `game::ObserverMode` + `watches_target` in `snapshot.h`; `PlayerSnapshot::observer_mode` / `observer_target`, filled
  by `read_player` for dead players only.
- `features/spectators` (pure): `watched_pawn` (yours while alive, the one you watch while dead), `spectator_info`
  (dead players in first / third person on it, enemy or teammate), `build_spectator_list` → a panel on the left or
  right ("Spectators 2", one row per name with "1st person" / "3rd person", or "Nobody"; "Watching Kev" while dead).
- `render/panel`: the bomb timer's panel writer (rows, bars with marks, background + border) moved out of
  `features/bomb_timer` so both panels share it; the bomb timer draws the same as before (its tests unchanged).
  `config::kBombPanelPadding` etc. became `kPanel*`.
- Settings: `SpectatorSettings` (off by default; side, height, show the camera, hide when nobody watches); Misc page
  card; the watermark lists "Spectators".
- `--diag`: "Observer services" check (every controller's chain reads; every watched target is a known pawn): 67
  checks in a match. `--live`: dead players show "watching X (1st person)" or "free camera".

**Problems and fixes**
- A Python heredoc collapsed `\b` into a backspace when editing the `.vcxproj` files (the known gotcha); redone with
  the Edit tool.

**Verified here:** Debug and Release zero warnings; tests 167/167 in both (+13: observer reads, the dead/alive gate
in `read_player`, spectator filters, the dead-local path, panel rows / sides / empty / hidden, settings defaults);
`--diag` all 67 OK; `--live` named whom every dead bot watched. The user's running `cs2_external.exe` was closed for
the build.
**Not verified here:** the panel itself on screen, the Misc page controls, chase mode.
**User (2026-10-07):** approved for commit.

## 2026-10-07: Hitsound dropped, Phase 6 done

Spectator list approved by the user and committed. The user then **dropped the hitsound** (Phase 6's last planned
feature) before any work on it. Removed: `features::ActiveFeatures::hitsound` and its watermark name, the Misc page's
"Coming next" card, and the plan entries in CLAUDE.md (§3, §13). `test_feature_summary` now uses the spectator list
in place of the hitsound.

**Verified here:** Debug and Release zero warnings; tests 167/167 in both.

Phase 6 ends with the radar, the bomb timer and the spectator list. Next: Phase 7, keybind engine.

## 2026-10-07: Phase 7, keybind engine

**Built** (the AC project's engine, ported and adapted to the external overlay)
- `input/keys`: every bindable key (letters, digits, numpad, F1-F24, navigation, punctuation, L/R Shift/Ctrl/Alt,
  Mouse 1-5) with names, `KeySet` (bitset<256>), `vk_from_name` (case-insensitive, for Phase 8's JSON),
  `is_mouse_button`, `is_modifier`, `can_be_hotkey`. Generic Shift/Ctrl/Alt and Escape aren't bindable.
- `input/actions`: the `ActionId` registry (constexpr, in id order, checked by a static_assert): menu, panic (END),
  exit (DELETE), aim key (Mouse 1, hold), aimbot on/off, trigger key (Mouse 4, hold), triggerbot on/off, ESP / radar /
  bomb timer / spectator list on/off (unbound). `BindMode` (hold / toggle / press) moved here from `settings`.
- `input/keybinds`: `KeybindEngine` (rising edges, HOLD / TOGGLE / PRESS, priming frame, suspension: menu open = only
  panic and exit, capture = nothing), `find_conflicts`.
- `input/bind_capture`: waits for release, takes the next key (lowest VK), Escape clears, timeout 6 s
  (`config::kBindCaptureTimeoutMs`). The menu key listens only to keys `RegisterHotKey` takes and Escape cancels it.
- `input/key_poll`: `GetAsyncKeyState` over the table + Escape, `is_key_down`.
- `settings::KeybindSettings` (the binds); `AimbotSettings::key/mode` and `TriggerbotSettings::key` removed;
  `TriggerActivation` is now always / key (the key's own mode says hold or toggle). `features/activation.h` removed.
- `app/frame`: keys are polled while the game or the overlay is in front, a running capture is fed (Mouse 1 left out
  while the cursor is over the menu), then the engine, then the actions (exit, panic: every feature off + toggles off
  + menu closed, the on/off keys). The aimbot and triggerbot read their key states from the engine.
- `ui/overlay_window`: the menu hotkey is registered with the bound key (re-registered when it changes, released
  while a capture runs); a failure is logged once per key and shown on the Keybinds page.
- UI: `ui/keybind_widgets` (key button with capture / conflict colours, Hold/Toggle combo, bind row), a **Keybinds**
  page (SETUP group: every action by category, conflict notice, reset), bind rows on the Aimbot, Triggerbot, ESP and
  Misc pages. Home and the console name the bound keys.

**Problems and fixes**
- A capture ending on a key press would fire that key's action in the same frame (bind panic to F → instant panic).
  The frame a capture ends still counts as `Suspension::capture`, so the engine records the key as already down.
- Binding Mouse 1 by clicking would be ambiguous with clicking the menu (the key button's own click restarted the
  capture). Mouse 1 is left out of the capture while ImGui wants the mouse: it's bound by clicking outside the menu,
  and clicking the key button again cancels.
- The menu key can't be a mouse button or a modifier (RegisterHotKey) and can't be unbound (the menu would never open
  again): enforced in `key_allowed` and the capture.
- Python heredocs collapsed backslashes again in the `.vcxproj` edit (`\a` → bell); redone with the Write/Edit tools.

**Verified here:** Debug and Release zero warnings; tests 186/186 in both (+20: key table and names, registry,
engine modes / priming / suspension / conflicts, capture rules, keybind defaults; the old `KeyActivation` test
removed). Started against the running game (de_mirage bot match): `--diag` part all 67 OK, the console names INSERT /
END / DELETE, the overlay found the window and the match. **Not verified here:** any key press, the capture, the
Keybinds page on screen (no input was sent to the game).

### Later the same day: user check, presses from raw input

**User check:** every keybind works, but the on/off keys "had to be held" and spamming them lost presses.
**Cause:** keys were polled once per overlay frame, and a press was a difference between two polls. While the game
takes the GPU the overlay's frames get slow (with the menu open the game sleeps, so everything felt fine there), so a
tap could start and end between two polls. Not measured directly (CS2 wasn't in front without taking focus from the
user), but it explains both symptoms; the frozen test bots hid any ESP lag.
**Fix (the user chose raw input over a polling thread or GetAsyncKeyState's low bit):**
- `ui/overlay_window` registers keyboard + mouse raw input with `RIDEV_INPUTSINK` and feeds every `WM_INPUT` to a new
  pure `input/key_tracker` (counts key-downs, filters auto-repeat, resolves L/R Shift/Ctrl/Alt, maps mouse button
  flags). The hidden wait no longer wakes on raw input.
- `input/keybinds`: the engine takes a `KeyFrame` (held keys from `GetAsyncKeyState` + press counts from raw input)
  and flips toggles once per press; `ActionStates` carries press counts. `presses_from_edges` is the fallback if raw
  input can't be registered. The capture sees keys held or pressed during the frame.
- `app/frame`: takes the presses at the top of every frame (also hidden: presses in other programs are dropped), and
  an on/off key flips its feature once per press.

**Verified here:** Debug and Release zero warnings; tests 191/191 (+5: raw keyboard mapping, auto-repeat, taps per
frame, mouse flags, saturation; engine tests rewritten for press counts). Against the running game: no raw input
warning; CPU over 6 s the same as the previous build (1.5-2.2 s each, measured side by side; the cost was there
before, likely the driver in `Present`). **Not verified here:** the key presses themselves.

**User (2026-10-07):** all good after the fix (on/off keys flip on each tap); approved for commit. Phase 7 done.

## 2026-10-07: Phase 8, settings and profiles (JSON) + presets

**Built**
- Vendored **nlohmann/json v3.12.0** (approved by the user): copied from the AC project, SHA-256 of `json.hpp` checked
  against the official release asset, `LICENSE.MIT` against the tag (`external/README.md`).
- `settings/profile_json` (pure, the AC design): one field list per section (`visit_*` templates) drives both writing
  and reading, with nested sections (`esp.colours`, `triggerbot.weapons`, `radar.colours`). Forgiving load: unknown
  keys ignored, missing keys keep their defaults, wrong types / bad enums / bad colours / keys an action can't take
  fall back to the default, numbers clamped to their `config::Range` (whole numbers rounded); every problem becomes a
  warning like `esp.thickness: 99 adjusted to 4 (range/rounding)`. `schema_version` 1 + a migration hook. Colours
  `#RRGGBBAA`, keys by name (`"Mouse 4"`), null = unbound.
- `settings/profile_store` (ported from AC): `<exe folder>\profiles\<name>.json`, atomic save (`.tmp` + replace),
  built-in read-only `default`, `.last_profile`, safe names (no paths, no device names), startup falls back to
  `default` with a warning if the last profile is gone or broken.
- `settings/presets` (pure): Off / Chill / Medium / Rage with the values the user picked. Features and strengths only;
  keybinds, colours, team mode, team checks, max distances, the overlay and positions are never touched.
- `input/actions`: a Presets category with four press actions (unbound by default).
- `app/frame`: loads the last profile before the overlay starts (and logs each warning); the menu queues profile
  operations and presets in `AppState::requests`, done at the start of the next frame; loading or resetting cancels a
  capture and turns toggle keys off. `AppState::unsaved_changes()` compares the settings with the profile as saved.
- Settings page: Profiles card (list with the loaded one marked, double-click loads; Load, Delete with confirmation,
  Save, Reset to defaults, a name box with Save as / Rename current, the result line, the loaded profile's warnings,
  the folder) and a Presets card; Home shows the profile and "unsaved changes".
- Every settings struct got a defaulted `operator==`. `Color::rgba(0xRRGGBBAA)` / `to_rgba()`; the three defaults
  with a fractional alpha (skeleton, FOV circle, radar background) are now byte-exact so they survive a save and load
  unchanged (0.9 → 0xE6, 0.35 → 0x59, 0.72 → 0xB8: not visible).
- `profiles/default.json` committed; a test checks it equals the code defaults (writes `default.json.expected` on a
  mismatch).

**Problems and fixes**
- Backslashes in a Python heredoc again: `external\nlohmann` in the `.vcxproj` edit became a newline and `\t` a tab.
  Redone with the Edit tool.
- The default.json test couldn't write its `.expected` file until `profiles/` existed.

**Verified here:** Debug and Release zero warnings; tests 216/216 in both (+25: JSON round trips, format, forgiving
load, keybinds, schema version, default.json; store names / save / load / read-only default / rename / delete / last
profile / startup fallback; presets). Against the running game: with no profiles the tool logs `Profile "default"
loaded (0 warnings)`; with a hand-broken `smoke_test.json` as the last profile it logs its four warnings, clamps the
thickness, keeps INSERT as the menu key (Mouse 1 refused) and names F8 as panic. Test files deleted afterwards.
**Not verified here:** the Settings page on screen and any button on it (no input was sent to the game).

**User (2026-10-07):** everything works; approved for commit. Phase 8 done.

## 2026-10-07: Phase 9, panic, clean shutdown, polish

Most of Phase 9 had already been done in Phase 7: panic (END), the exit key (DELETE) and the final key layout. This
session added what was missing and reviewed the robustness cases in code.

**Built**
- Settings page: an **Exit** card. "Exit the tool" exits straight away; with unsaved changes it first asks "Really
  exit? Unsaved changes are lost" (with Cancel). It queues `AppState::requests.exit`; `app/frame` handles it at the
  start of the next frame, the same way the exit key does.
- A deliberate exit (exit key, Exit button, Alt+F4 on the menu) sets `core::shutdown_requested`, so the console closes
  instead of waiting at "Press Enter to exit...". "cs2.exe has closed" and errors still wait, so a double-clicked
  console stays readable.
- `app/frame`: the loop moved into `loop()`, with `run()` catching any `std::exception`. The shutdown (release attack,
  close the menu and give focus back, ImGui, overlay) now runs after an error too. Before this, an error unwound
  straight to `main()`, and the triggerbot could leave attack pressed in the game.
- Panic also drops a queued preset and a queued profile load. A preset clicked one frame before panic would
  otherwise have turned the features back on in the same frame.
- The console banner still said "CS2 External - Phase 5". It now reads "External Cheat by BigH (CS2, offline
  only...)". The Home page's exit hint and the startup log mention the Exit button.

**Robustness review** (code paths, not all exercised in-game)

| Case | What happens |
|---|---|
| Death | The snapshot keeps the dead local player. The aimbot (`compute_aim`) and the triggerbot (`not_in_match`) skip while you're dead; the ESP and radar keep drawing; the spectator list switches to "Watching ...". |
| Map change / loading screen | The local pawn reads 0 and the snapshot is empty or garbage-filtered (view matrix sanity check, pawn validity, handle serials), so nothing is drawn or aimed. Home shows "Not in a match". The game window stays the same. |
| Alt+Tab | The overlay hides, the triggerbot lets go of attack, the menu closes without taking focus back, and the menu hotkey is unregistered. Toggle keys keep their state; hold keys read released. |
| Minimise | Same as Alt+Tab (`IsIconic`). |
| Game window recreated | `find_game_window` notices the dead HWND, closes the menu, and waits for the new window. |
| Game closes | `is_running` fails, so the tool exits normally (overlay removed, handle closed) and the console waits for Enter. |
| Game restart | The tool exits with the old game (above). Start it again after the new game is in the menu. Waiting for a new cs2.exe was not built (Decision log). |
| Ctrl+C / console closed | Already in place: the handler sets the flag, the main thread shuts down, and closing the console waits up to 3 s. |
| Error in a frame | New: caught in `run()`, logged, and the full shutdown still runs. |

**Verified here:** Debug and Release zero warnings; tests 216/216 in both; `--diag` against the running game (build
14189, in a match): all 67 checks OK, banner correct. The overlay started in the background (profile "goat" loaded,
D3D11 + ImGui ready, game window found), then was force-closed (no input is ever sent to the user's game).
**Not verified here:** the Exit button, the exit key without "Press Enter", panic, 10 start/stop cycles, a map change,
the game closing.

**User (2026-10-07):** all good; approved for commit. Phase 9 done.

## 2026-10-07: Phase 10, UI redesign

The user's brief: more readable and user-friendly, customisable, a clean layout, and clearly different from the AC
project's look, keeping the logo and the name. Their choices: **top tabs + two columns of panels**; a **dark black /
grey / off-white** look that is easy on the eyes at night; customisation = **themes + accent colour**.

**Built**
- `settings/themes` (pure): five dark themes as one table. Midnight (default: graphite, off-white text, soft white
  accent), Black (OLED), Graphite (lighter grey, steel-blue accent), Violet (the logo's turtleneck + gold sparkles), Ice
  (the logo's teal). Also `contrast_ratio` (WCAG) and `text_on` (the readable text colour on an accent fill).
  `OverlaySettings` gained `theme` and `accent`, saved in profiles (`overlay.theme`, `overlay.accent`).
- `ui/theme`: a runtime `Palette` (the theme plus the accent, with derived hover / soft / on-accent colours).
  `use_theme` rebuilds it and ImGui's colours only when they change (called every frame after NewFrame). Fonts gained
  Segoe UI Semibold for titles and tabs.
- `ui/widgets` rewritten as a small UI kit:
  - `Columns`: two columns of panels, one under 720 px.
  - Panels with the feature's main switch in the title.
  - Rows: label and "?" on the left, the control in an aligned column on the right (switch, slider, choice, colour,
    info). A choice is segmented buttons, or a dropdown when they don't fit. A label too long for its column puts the
    control on the next line.
  - An animated switch, toggle chips, buttons (normal / accent / danger), notices with a coloured bar, and a drawn
    "?" help marker.
- `ui/icons`: seven line icons drawn with the draw list (no icon font).
- `ui/menu`: a header (logo, name, "External" pill, close button) and a tab bar with icons. A green dot marks the tabs
  whose feature is on. When the menu is narrow, only the selected tab keeps its label.
- Every page rebuilt in two columns. Home is a dashboard: status line; Features (every switch) and Presets; Match, Tool
  and Keys. Settings: Profiles; Appearance (theme preview tiles, accent picker, "Use the theme's accent"); Overlay;
  Exit. Hold | Toggle and every 2-3 option choice are segmented buttons.
- The watermark follows the theme (chrome background, hairline border, accent dots).
- `tests/settings/test_themes.cpp` (+4 cases): the WCAG reference values; every theme is dark, with readable text,
  dimmed text, accent, status colours and text on the accent; the defaults; an out-of-range theme falls back.
  Theme and accent were added to the profile round-trip and preset tests. `profiles/default.json` gained the two keys.

**Problems and fixes**
- `ImDrawList::AddRect` / `AddPolyline` with the old (flags, thickness) argument order: `= delete` in ImGui 1.92.8+
  (C2280), as CLAUDE.md warns. Swapped.
- The first offscreen renders showed:
  - The "?" markers and the status dot sat high: they're now centred on the frame height.
  - Colour swatches weren't right-aligned.
  - At ~700 px the Hold | Toggle segments clipped "Toggle". Now the layout drops to one column under 720 px, and the
    mode selector is sized for its longest name.
  - The Home banner repeated the header's logo and name, so it was replaced by a status line.
- Backslashes in bash again when generating the harness build script; generated it from PowerShell instead.

**Verified here:** Debug and Release zero warnings; tests 220/220 in both. Every page was rendered offscreen with mock
data by a scratch harness (outside the repo; CLAUDE.md §10 "Menu"), in Midnight, Violet, Graphite and Ice, and in a
narrow window (760 x 640), then checked on the screenshots. The overlay starts against the running game, and the
user's existing profile loads with 0 warnings (no theme keys: defaults).
**Not verified here:** the menu in the real game (opening it needs the menu key), clicking anything, switching themes
live, the close button.

**User (2026-10-07):** "so much better, so clean"; approved for commit. Phase 10 done.

## 2026-10-07: Phase 11, educational README

The user asked for the README to be drafted like the AC project's (`BigH018/internal-assault-cube`) and to include
pictures of the tool and in-game screenshots; they left CS2 open (de_mirage Deathmatch, frozen bots) near players for it.

**Built**
- `README.md`, in the AC README's structure: centred header with the logo, a scope note, screenshots, contents,
  features (plus a table of what an external tool can't do), requirements, build, run (`--diag`, `--live`), the menu
  page by page, default hotkeys, presets with their values, profiles, the console (real `--diag` and `--live` output),
  troubleshooting, tests, project layout, then **How it works** in 20 sections (external vs internal, the handle,
  `core::Memory`, where offsets come from, signature scanning, `CreateInterface` decoded, the schema system's
  self-pointing class infos, the chunked entity system and handles, players / weapons / bones, world-to-screen, the
  overlay window, the spotted-by mask and its measured delay, aimbot smoothing, the triggerbot's button writes, the
  read-only misc features and the bomb timer's "never guess" lesson, keybinds and raw input, safety, the frame loop,
  what to do after a CS2 update, lessons per phase), and credits.
- `docs/screenshots/` (13 files): `esp-full.jpg`, `esp-corners.jpg`, `menu-in-game.jpg` (1920x1080 JPEG, ~400 KiB),
  `menu-{home,aimbot,triggerbot,esp,misc,keybinds,settings,settings-violet}.png` (the menu window cropped, 901x621,
  ~70 KiB), `hud-watermark.png`, `hud-radar.png`.

**How the pictures were taken** (scratch scripts, not in the repo)
- A copy of the Release exe in `bin\Showcase` (git-ignored, deleted afterwards) with two profiles, "showcase" (full
  ESP with teammates, aimbot FOV circle, triggerbot on its key, radar with names, bomb timer, spectators) and
  "corners" (corner boxes, snaplines, enemies only). The user's `bin\Release\profiles` were never touched.
- CS2 brought to the front from a script (Alt tap + `SetForegroundWindow`), the tool started with `-NoNewWindow`, the
  screen grabbed with Pillow. The menu opened with a synthetic INSERT (the real `RegisterHotKey` path) and the tabs
  were clicked by script, only while the menu was open. The Violet tile was clicked live for one shot (not saved).
- The second ESP shot needed another angle: one `dwViewAngles` write (pitch 4.69 / yaw 53.44 → 4 / 75), written back
  afterwards and read again to confirm (4.6886 / 53.4423).
- Every run exited with the exit key (DELETE): "Exit key: exiting", ImGui shut down, overlay removed.

**Problems and fixes**
- The first menu shots came from a scratch folder in Temp: the Settings page showed that long path and "readme_*"
  profile names. Re-shot from `bin\Showcase` with cleaner names.
- The radar crop cut off names drawn past the radar's left edge; widened.
- A `|` inside a code span broke a README table row; rewritten.

**Verified here:** Debug and Release zero warnings; tests 220/220 in both (3908 assertions); `--diag` all 67 checks OK
during the screenshot runs; every relative link and image in the README exists. **No code or offset changed.**
**Not pictured:** the bomb timer (no bomb in Deathmatch), a spectator (bots respawn at once), an enemy in the visible
colour (every enemy was behind cover; teammates show visible / hidden instead).

**User (2026-10-07):** approved ("push it"). Phase 11 done.
