<div align="center">

<img src="assets/logo.jpg" alt="ExternalCheat" width="120" />

# ExternalCheat

**An external Counter-Strike 2 cheat with a kernel-mode memory backend.**

Runs entirely in its own process. Never injects into the game. Reads and writes game memory through a custom driver that is manually mapped into the Windows kernel.

![Menu, in game](assets/screenshots/menu-in-game.jpg)

</div>

---

## Table of contents

- [What this is](#what-this-is)
- [Architecture](#architecture)
- [Memory access flow](#memory-access-flow)
- [The overlay](#the-overlay)
  - [Menu — Home](#menu--home)
  - [Menu — Aimbot](#menu--aimbot)
  - [Menu — Triggerbot](#menu--triggerbot)
  - [Menu — ESP](#menu--esp)
  - [Menu — Misc](#menu--misc)
  - [Menu — Keybinds](#menu--keybinds)
  - [Menu — Settings](#menu--settings)
- [In-game HUD](#in-game-hud)
  - [ESP](#esp)
  - [Radar](#radar)
  - [Watermark](#watermark)
- [Offset resolution](#offset-resolution)
- [Project layout](#project-layout)
- [Building](#building)
- [Running](#running)
- [Diagnostics](#diagnostics)
- [Troubleshooting](#troubleshooting)
- [Notes and limitations](#notes-and-limitations)
- [Improvements and alternatives considered](#improvements-and-alternatives-considered)

---

## What this is

ExternalCheat is a study project. It exists to explore four things that are interesting on their own:

- **Windows kernel programming.** A real driver with an IOCTL surface, PEB walking, and cross-process memory copies.
- **PE parsing.** Reading headers, sections, and imports out of a live process.
- **The Source 2 schema system.** Walking the game's own reflection data instead of hardcoding every field.
- **D3D11 / Dear ImGui overlays.** A topmost, layered, click-through window that draws on top of the game without ever taking focus.

It is *not* intended for live VAC-secured servers. It is built for `-insecure` bot matches so the memory-reading side can be tested and understood in isolation.

---

## Architecture

The system is split into two Visual Studio projects in one solution.

**`BigHDriver`** — the kernel-mode driver.

It is loaded with KDMapper, so it is not registered as a Windows service. It does not show up in `sc query` or `driverquery`. Once mapped, it stays resident until the machine reboots.

It exposes one symbolic link:

    \\.\x9f2a3b

and a small set of IOCTLs:

| Command           | Code  | Purpose                                                          |
| ----------------- | ----- | ---------------------------------------------------------------- |
| `init_code`       | 0x9A1 | Attach to a target PID.                                          |
| `read_code`       | 0x9A2 | Copy memory from the target process into the caller.             |
| `write_code`      | 0x9A3 | Copy memory from the caller into the target process.             |
| `get_pid_code`    | 0x9A4 | Resolve a PID from an image name (kernel process-list walk).     |
| `get_module_code` | 0x9A5 | Resolve a module base and size from a name (target PEB walk).    |

**`external`** — the user-mode application.

It opens a handle to `\\.\x9f2a3b`, resolves the game PID, attaches, resolves `client.dll` and `engine2.dll`, and then runs the feature logic each frame against a `core::Memory` abstraction. It never calls `ReadProcessMemory` or `WriteProcessMemory`; every read and write is a `DeviceIoControl` round trip.

---

## Memory access flow

1. User-mode opens `\\.\x9f2a3b`.
2. `get_pid_code("cs2.exe")` — the driver walks `ActiveProcessLinks` and returns the PID.
3. `init_code(pid)` — the driver looks up the `PEPROCESS` and stores it.
4. `get_module_code(pid, "client.dll")` and `get_module_code(pid, "engine2.dll")` — the driver attaches to the target's address space, walks the PEB `InLoadOrderModuleList`, and returns each module's base and size.
5. Every subsequent read and write is a `read_code` / `write_code` IOCTL that ends in `MmCopyVirtualMemory` on the kernel side.

Moving PID and module lookup into the kernel means the user-mode process never calls `CreateToolhelp32Snapshot` or `Module32First`, both of which are heavily instrumented by anti-cheat user-mode hooks.

---

## The overlay

The overlay is a topmost, layered, click-through window rendered with Dear ImGui on D3D11. It tracks the game window's client area each frame and draws ESP, HUD elements, and the menu.

Below are the menu pages as they actually look.

### Menu — Home

![Menu — Home](assets/screenshots/menu-home.png)

The Home page summarises what's currently active, the read/write timings, and the current profile. It's the first thing you see when the menu opens.

### Menu — Aimbot

![Menu — Aimbot](assets/screenshots/menu-aimbot.png)

FOV, smoothing, target bone selection, visibility checks, and per-feature toggles. Smoothing is defined per 60 Hz frame and scaled to the real frame time so it feels consistent at any overlay frame rate.

### Menu — Triggerbot

![Menu — Triggerbot](assets/screenshots/menu-triggerbot.png)

Reaction delay, shot delay, burst mode, and an optional head-only mode that requires the crosshair ray to pass within a small world-space radius of the head bone.

### Menu — ESP

![Menu — ESP](assets/screenshots/menu-esp.png)

Everything the ESP can draw: boxes, health bars, names, distance, snaplines, skeleton, and visibility-based colours. Each element has its own toggle, so the ESP can be as loud or as quiet as you want.

### Menu — Misc

![Menu — Misc](assets/screenshots/menu-misc.png)

Radar, bomb timer, spectator list. Also the less visual stuff — the menu key, the panic key, and the overlay visibility toggle.

### Menu — Keybinds

![Menu — Keybinds](assets/screenshots/menu-keybinds.png)

Every action in the tool is bindable. The Keybinds page also shows whether a bind is currently held, so you can verify before you go into a match.

### Menu — Settings

![Menu — Settings](assets/screenshots/menu-settings.png)

Profiles, theme, UI scale, and the diagnostic toggles.

Themes are pluggable and the whole UI is recoloured from a single palette — here's the violet theme:

![Menu — Settings, violet theme](assets/screenshots/menu-settings-violet.png)

---

## In-game HUD

### ESP

The ESP draws in the same pass as the menu, but only when the game window is focused.

Full ESP — boxes, health bars, names, distance, weapon:

![ESP, full](assets/screenshots/esp-full.jpg)

Corner-only — the same data, just the four corners of each box, which is what a lot of people use when they want a clean screen:

![ESP, corners only](assets/screenshots/esp-corners.jpg)

### Radar

A top-down radar drawn in the corner of the game window. Facing lines, edge fade for players outside the radar range, and an arrow for your own position:

![Radar](assets/screenshots/hud-radar.png)

### Watermark

A small watermark in the top-left corner showing the current profile and how long the overlay has been running. Useful for confirming at a glance that the tool is attached and reading frames:

![Watermark](assets/screenshots/hud-watermark.png)

---

## Offset resolution

Offsets change on every CS2 update, so the project uses a layered approach instead of trusting any single source.

- **Dumps.** `game/offsets.h` and `game/schema.h` hold the offsets and schema field layouts as captured by a dumper. These are the source of truth for the build they correspond to.
- **Signature scanning.** `game/signatures.h` holds byte patterns for the key globals. On startup, the tool copies `client.dll`'s `.text` section and resolves each global by pattern, so a slightly shifted offset still lands correctly.
- **Live schema.** For class fields (`C_CSPlayerPawn`, `C_BaseEntity`, `CCSPlayerController`, and so on), the tool reads the schema system out of the running game rather than trusting the dump. This catches field moves that a signature scan can't.
- **Validation.** All of the above is checked at runtime by `--diag` (see below).

If a build is wrong, `--diag` tells you exactly which checks failed, which is usually enough to know what a fresh dump needs to capture.

---

## Project layout

    src/external/
      app/          startup, mode selection (overlay / --diag / --live), frame loop
      core/         Memory interface, ProcessMemory, KernelInterface, PE, pattern scan, logging
      features/     aimbot, triggerbot, ESP, radar, bomb timer, spectators, targeting
      game/         offsets, schema, signatures, entity readers, view matrix, weapons, bones
      input/        key polling, keybinds, bind capture
      maths/        vectors, angles, projection, skeletons
      render/       panel and primitive drawing
      settings/     profile storage (JSON), presets, themes
      ui/           ImGui overlay: window, menu, pages, HUD, widgets
      vendor/       Dear ImGui, nlohmann/json, doctest

---

## Building

Requirements: Visual Studio 2022, Desktop C++ workload, WDK, Windows 10/11 SDK.

1. Open `cs2-external.sln`.
2. Set the configuration to **Release | x64**.
3. **Build → Build Solution**.
4. Outputs:
   - Driver: `BigHDriver/bin/x64/Release/BigHDriver.sys`
   - Cheat: `external/bin/x64/Release/cs2_external.exe`

The driver is deliberately unsigned. It is loaded with KDMapper, not the Service Control Manager.

---

## Running

> Test offline only. This is intended for `-insecure` bot matches, never a VAC-secured server.

1. In the VM, restore the clean snapshot with Tamper Protection, Memory Integrity (HVCI), and the Vulnerable Driver Blocklist all disabled.
2. From an **Administrator** command prompt:

       kdmapper.exe BigHDriver.sys

3. Launch CS2 with `-insecure` and join a Practice with Bots match.
4. Run the cheat as **Administrator**:

       cs2_external.exe

The console prints the driver connection, the game PID, the module bases, and then either draws the overlay or runs the chosen diagnostic mode.

---

## Diagnostics

Two non-overlay modes, useful after a CS2 update.

`cs2_external.exe --diag` runs the offset diagnostic and exits. It checks the game build, resolves each interface via the module's `InterfaceReg` list, scans every signature, and reads every schema field, printing OK/FAIL for each. Anything that fails tells you what a fresh dump needs to capture.

`cs2_external.exe --live` prints a console table of every player each tick: team, name, health, armour, weapon, position, distance, and observer state.

Both modes use the same memory backend as the overlay, so if they work, the overlay will.

---

## Troubleshooting

| Symptom                                    | Likely cause                                         | Fix                                                                 |
| ------------------------------------------ | ---------------------------------------------------- | ------------------------------------------------------------------- |
| KDMapper returns `0xC0000603`              | Vulnerable Driver Blocklist re-enabled               | Set `VulnerableDriverBlocklistEnable` to `0`, reboot                |
| BSOD on mapping                            | Memory Integrity (HVCI) re-enabled                   | Turn Memory Integrity OFF, reboot                                   |
| Driver maps but modules read as `size 0x0` | `SizeOfImage` field moved in `LDR_DATA_TABLE_ENTRY`  | Read PE headers from the target instead of trusting the driver      |
| `--diag` reports many signature failures   | CS2 updated since the last dump                      | Re-run the dumper, update `offsets.h` and `signatures.h`            |
| Overlay doesn't line up with the game      | Window moved or resolution changed                   | The overlay tracks the game window each frame; check `cover()`      |
| Console prints "not in a match"            | Not in a live game                                   | Join a Practice with Bots match                                     |

---

## Notes and limitations

- **The driver is not unloadable.** KDMapper does not register a `DriverUnload` routine, so once mapped it stays in memory until reboot. To update the driver, reboot the machine.
- **The mapper's footprint is the main risk.** KDMapper depends on a publicly known vulnerable driver. The cheat itself never injects into the game and never calls the usual user-mode memory APIs.
- **Offsets rot.** Any CS2 update invalidates the dumps. `--diag` tells you exactly what moved.
- **Behaviour is server-visible.** An external cheat can hide its memory reads, but it cannot hide what the player does with the information.

---

## Improvements and alternatives considered

The current overlay is a topmost layered window rendered on its own D3D11 swap chain. It works, it's simple, and it keeps the cheat fully external. But there are several other places an overlay can live in the Windows graphics stack, each with its own trade-offs. This section documents the ones that came up during design and why the current approach was chosen over them.

### Current: topmost layered window

**How it works.** `CreateWindowExW` with `WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT`. The window is created with a `DXGI_SWAP_EFFECT_DISCARD` swap chain, DWM extends the frame into the client area, and the back buffer is cleared to fully transparent each frame so only drawn content shows. `SetWindowPos` tracks the game window's client rect each frame.

**Pros.**
- No injection. The cheat process never touches `cs2.exe`.
- Simple to reason about: one window, one swap chain, one render loop.
- Works in borderless fullscreen and windowed mode.
- `WS_EX_TOOLWINDOW` keeps it out of Alt+Tab and the taskbar.
- Raw input with `RIDEV_INPUTSINK` lets the menu still see keys when the game has focus.

**Cons.**
- The window itself is a user-mode artifact. Window enumerators can find it.
- Doesn't work in exclusive fullscreen (DWM isn't in the display path there).
- Two D3D11 devices (the game's and the overlay's) means two GPU submissions.
- Colour can desync if the game uses HDR or a non-standard tonemap.

### Alternative: DWM overlay

**How it works.** Inject into `dwm.exe` and hook its `Present`. Draw into the compositor's own back buffer, so the overlay is blended into the desktop image before it reaches the display.

**Pros.**
- No window. The compositor itself is drawing your content.
- Works in borderless fullscreen.
- No user-mode artifact in the game's process list.

**Cons.**
- Injecting into a system process requires kernel-level access.
- A bug in the hook crashes `dwm.exe`, which takes the entire desktop down.
- Finding DWM's swap chain is undocumented and version-fragile.
- Exclusive fullscreen still bypasses DWM entirely, so it doesn't solve that case either.

### Alternative: NVIDIA overlay

**Two flavors.**

1. **The official one.** GeForce Experience / NVIDIA App inject a user-mode component into games and hook `Present` to draw the performance overlay, ShadowPlay indicators, and filters.
2. **The "hijack" approach.** Some projects try to detect NVIDIA's overlay window or process and attach to it, then draw custom content through NVIDIA's own pipeline.

**Why neither was chosen.**
- The official overlay has no public API for third-party content.
- The hijack approach requires modifying a known NVIDIA process, and anti-cheats explicitly check for the window-style changes that hijacking leaves behind. It's a well-known detection signature.

### Alternative: hardware overlay plane

**How it works.** The GPU's display controller has multiple planes: a primary plane for the desktop, and one or more overlay planes that can hold independent content (this is how video playback avoids touching the desktop's rendering). A buffer placed on an overlay plane is blended with the desktop in the display hardware during scanout, below the OS compositor entirely.

**Pros.**
- Invisible to any software screen capture, including DWM's own composition.
- Works over exclusive fullscreen, because the blend happens in the display engine, not in software.
- Zero GPU shader cost — the blending is free hardware work.

**Cons.**
- There is no public API for a normal application to claim an overlay plane. Doing it requires writing a kernel-mode display minidriver or talking directly to vendor display-manager internals.
- Not feasible without a signed kernel display component, and even then it's fragile across driver updates and GPU generations.

### Why the current approach was kept

The topmost window solves the problem this project actually has: draw an overlay on top of a borderless-fullscreen game, without injecting anything into it, without a system-process dependency, and without a signed kernel display driver. It's the least complex option that meets all three of those constraints. The other approaches each trade one of those constraints away — injection, system process stability, or driver signing — for a benefit (no window artifact, exclusive-fullscreen support, capture invisibility) that isn't needed for the intended use case.

---

<div align="center">

Built as a study project. Read it, build it, break it, fix it.

</div>
