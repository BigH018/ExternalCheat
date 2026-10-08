#pragma once

// The spectator list: dead players watching you in first or third person. While you're dead and watch someone
// yourself, the list is about them instead (who else watches the same player). Pure: the snapshot in, a panel out.
//
// PURE: no <Windows.h>, no ImGui.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "game/snapshot.h"
#include "maths/vec.h"
#include "render/primitives.h"
#include "settings/settings.h"

namespace features
{
struct Spectator
{
    std::string name;
    game::ObserverMode mode = game::ObserverMode::in_eye;
    bool enemy = false; // under the team mode, relative to you
};

struct SpectatorInfo
{
    std::string watched; // empty = you; otherwise the name of the player you spectate
    std::vector<Spectator> spectators; // in controller index order
};

// The pawn whose spectators the list shows: yours while you're alive, the one you watch in first or third person
// while you're dead. 0 when there is none (not in a match, your death cam, a free camera).
[[nodiscard]] std::uintptr_t watched_pawn(const game::GameSnapshot& game) noexcept;

// Who watches watched_pawn(). nullopt when there is nobody to be watched.
[[nodiscard]] std::optional<SpectatorInfo> spectator_info(const game::GameSnapshot& game, settings::TeamMode team_mode);

// "1st person", "3rd person" (the modes a spectator is listed in), "" for the rest.
[[nodiscard]] const char* observer_mode_name(game::ObserverMode mode) noexcept;

// The panel on the chosen side of the game window. Nothing with the list off, nobody to be watched, or (with
// hide_when_empty) nobody watching. `line_height` is the font's height in pixels.
[[nodiscard]] std::vector<render::Primitive> build_spectator_list(const game::GameSnapshot& game,
                                                                  const settings::SpectatorSettings& settings,
                                                                  settings::TeamMode team_mode, maths::Vec2 screen,
                                                                  float line_height);
} // namespace features
