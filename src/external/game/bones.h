#pragma once

// The bone positions of a pawn: scene node (a CSkeletonInstance) + m_modelState + 0x80 -> an array of 32-byte bones
// whose first 12 bytes are the world position. Layout in game/offsets.h (offsets::layout::kModelStateBones), joint
// indices in maths/skeleton.h. Hand-found, not in any dump: proof in docs/offsets.md "Bones".
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <optional>

#include "core/memory.h"
#include "maths/skeleton.h"

namespace game
{
// The bones of the pawn whose scene node is `scene_node`, in one read. nullopt if the array can't be read, or if any
// bone isn't finite or lies further than config::kMaxBoneDistance from `origin` (a garbage or stale array).
[[nodiscard]] std::optional<maths::Bones> read_bones(const core::Memory& memory, std::uintptr_t scene_node,
                                                     maths::Vec3 origin);
} // namespace game
