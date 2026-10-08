# Vendored third-party code

Never edit files in this folder. Upgrade by replacing them with a newer tagged release and updating this table.

| Library | Version | Licence | Source | Files | Blob hash (`git hash-object`) |
|---|---|---|---|---|---|
| doctest | 2.5.3 | MIT (`doctest/LICENSE.txt`) | https://github.com/doctest/doctest, tag `v2.5.3` | `doctest/doctest.h` | `0b36ccafaed7f4802cdf1b1c322977489b1dd658` |
| Dear ImGui | 1.92.9b | MIT (`imgui/LICENSE.txt`) | https://github.com/ocornut/imgui, tag `v1.92.9b` (commit `f1cc2ae`) | core + `backends/imgui_impl_win32.*` + `backends/imgui_impl_dx11.*` (15 files, listed below) | every file matches the tag's tree (below) |
| nlohmann/json | 3.12.0 | MIT (`nlohmann/LICENSE.MIT`) | https://github.com/nlohmann/json, release `v3.12.0` (single-header asset `json.hpp`) | `nlohmann/nlohmann/json.hpp`, `nlohmann/LICENSE.MIT` | `json.hpp` `82d69f7c5d044c9887c96b90c97f5639083ecd14`, `LICENSE.MIT` `a1dacc8dbbd907c4b622ff1f08e279c27465dcbc` |

## nlohmann/json v3.12.0 (Phase 8)

Approved by the user 2026-10-07. Copied from the AC project's vendored copy, then checked against upstream: the
SHA-256 of `json.hpp` (`aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63`) equals the official
`v3.12.0` release asset (`gh release download v3.12.0 -R nlohmann/json -p json.hpp`), and `LICENSE.MIT`'s blob hash
equals the one in the `v3.12.0` tag (`gh api repos/nlohmann/json/contents/LICENSE.MIT?ref=v3.12.0`).

- Header-only, used only by `src/external/settings/profile_json.cpp`. `external.vcxproj` and `tests.vcxproj` put
  `external/nlohmann` on their *external* include path (warnings off) and include `<nlohmann/json.hpp>`. Not part of
  `vendor.vcxproj` (nothing to compile).

## Dear ImGui v1.92.9b (Phase 1)

Copied 2026-10-06: the core files and the Win32 backend from the AC project's vendored copy (same tag), the DX11
backend straight from the tag (`gh api repos/ocornut/imgui/contents/backends/<file>?ref=v1.92.9b`). Every file's git
blob hash was compared with the tag's tree (`gh api repos/ocornut/imgui/git/trees/v1.92.9b?recursive=1`) and matches:

| File | Blob hash |
|---|---|
| `LICENSE.txt` | `d5ba3155f0f438ba765551a5e2419bf3eaa8be64` |
| `imconfig.h` | `b40db389829a6bea9a2ed4104228c31e6117dd5a` |
| `imgui.h` | `611c5f28dd3398fb30d25cb6b1b08fecb36c512c` |
| `imgui.cpp` | `7fa2d49e8dfa3d676399b4863126ba9ab1609ca6` |
| `imgui_internal.h` | `d2a8595a1c828837b4572fd58038296b125ac61a` |
| `imgui_draw.cpp` | `e582f5cd80eec8e68f0f2506e5914a7251dce56c` |
| `imgui_tables.cpp` | `f8fab3778b1d8a6e46bf7837dcc9973dfbac3804` |
| `imgui_widgets.cpp` | `5ed7d6febf130c0529410a077fbc081b02cbd90c` |
| `imstb_rectpack.h` | `ad089213096115f070b52e0a3e554f3cbcd30f2d` |
| `imstb_textedit.h` | `583508f0b603d310eed047572df5dfac61351787` |
| `imstb_truetype.h` | `ec4d42c7dc55ce71aac39fe20d61dcf724855dfd` |
| `backends/imgui_impl_win32.h` | `d2a96005960262e664ba30afdc69fdfdbfa60a0e` |
| `backends/imgui_impl_win32.cpp` | `787329bf619b837e444121356d1349fc23ee8e4d` |
| `backends/imgui_impl_dx11.h` | `56166b92a4cf4bb185baec48bea88a289d2abac6` |
| `backends/imgui_impl_dx11.cpp` | `73f722a0834cc811ad20c4cbe9deac1f0e2c29c3` |

- No demo, no other backends, no fonts/misc folders.
- Compiled by `external/vendor.vcxproj` (static lib `vendor.lib`, x64) with warnings off and `/sdl` off, so our own
  code keeps `/W4 /WX`. `src/external/external.vcxproj` references it (builds it first and links it).
- **Configuration is not done in `imconfig.h`.** The defines live in `props/imgui.props`, imported by both
  `vendor.vcxproj` and `external.vcxproj`, because ImGui's config must be identical in every translation unit:
  `IMGUI_DISABLE_OBSOLETE_FUNCTIONS`, `IMGUI_IMPL_WIN32_DISABLE_GAMEPAD`, `IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS`. The
  props file also puts `imgui/` and `imgui/backends/` on the *external* include path (warnings off).
- The DX11 backend links `d3dcompiler.lib` (it compiles its two small shaders at startup with `D3DCompile`;
  `d3dcompiler_47.dll` ships with Windows 10/11).
