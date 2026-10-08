#pragma once

// The only game writes the tool makes: a button's state (attack, later jump) and the view angles. Each write is a
// single field; nothing in the game's code is changed. Formats proven live (offsets::layout, docs/offsets.md
// "Writes").
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <optional>

#include "core/memory.h"
#include "maths/angles.h"

namespace game
{
// Presses (down) or releases a button: client.dll + `button_rva` (offsets::buttons::*).
[[nodiscard]] bool set_button(core::Memory& memory, std::uintptr_t client_base, std::uintptr_t button_rva,
                              bool down) noexcept;

// The camera's view angles (client.dll + dwViewAngles). nullopt if the read fails or the angles aren't finite.
[[nodiscard]] std::optional<maths::Angles> read_view_angles(const core::Memory& memory,
                                                            std::uintptr_t client_base) noexcept;

// Points the camera: writes pitch and yaw (normalized first; roll is left alone).
[[nodiscard]] bool write_view_angles(core::Memory& memory, std::uintptr_t client_base, maths::Angles angles) noexcept;
} // namespace game
