#pragma once

// The ESP: which players to draw and what to draw for each, as render primitives. Pure: a snapshot and the settings
// in, primitives out; render/painter puts them on screen.
//
// PURE: no <Windows.h>, no ImGui.

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "color.h"
#include "game/snapshot.h"
#include "maths/projection.h"
#include "maths/vec.h"
#include "render/primitives.h"
#include "settings/settings.h"

namespace features
{
// A player's box on screen.
struct ScreenBox
{
    maths::Vec2 min; // top-left
    maths::Vec2 max; // bottom-right

    [[nodiscard]] float width() const noexcept { return max.x - min.x; }
    [[nodiscard]] float height() const noexcept { return max.y - min.y; }
    [[nodiscard]] float centre_x() const noexcept { return (min.x + max.x) * 0.5f; }
};

// The box around `player`: from the feet to a little above the eyes, config::kEspBoxAspect as wide as it is tall.
// nullopt if the feet or the top are behind the camera, or the box is too small to see.
[[nodiscard]] std::optional<ScreenBox> player_box(const maths::ViewMatrix& view, const game::PlayerSnapshot& player,
                                                  maths::Vec2 screen);

// Green at full health, yellow at half, red near zero.
[[nodiscard]] Color health_colour(int health) noexcept;

// The name as drawn: cut to config::kEspMaxNameLength characters with "...", "?" if empty.
[[nodiscard]] std::string display_name(std::string_view name);

// "24 m".
[[nodiscard]] std::string distance_text(float metres);

// Everything the ESP draws this frame. Nothing outside a match, without a sane view matrix, or with the ESP off.
// `team_mode` decides who is an enemy; `line_height` is the label font's height in pixels (labels are stacked by it).
[[nodiscard]] std::vector<render::Primitive> build_esp(const game::GameSnapshot& game,
                                                       const settings::EspSettings& settings,
                                                       settings::TeamMode team_mode, maths::Vec2 screen,
                                                       float line_height);
} // namespace features
