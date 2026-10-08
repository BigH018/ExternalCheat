#pragma once

// The view matrix: client.dll + dwViewMatrix holds it inline (16 floats, row-major; see maths/projection.h).
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <optional>

#include "core/memory.h"
#include "maths/projection.h"

namespace game
{
// The view matrix as it is right now. nullopt only if the read fails; check is_sane() before projecting (it is all
// zeros before the first frame of a match).
[[nodiscard]] std::optional<maths::ViewMatrix> read_view_matrix(const core::Memory& memory,
                                                                std::uintptr_t client_base) noexcept;
} // namespace game
