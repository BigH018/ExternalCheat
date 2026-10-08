#include "game/bones.h"

#include <array>
#include <cstddef>
#include <cstring>

#include "config.h"
#include "game/offsets.h"
#include "game/schema.h"

namespace game
{
namespace layout = offsets::layout;

std::optional<maths::Bones> read_bones(const core::Memory& memory, std::uintptr_t scene_node, maths::Vec3 origin)
{
    const auto array = memory.read<std::uintptr_t>(scene_node + schema::CSkeletonInstance::m_modelState +
                                                   layout::kModelStateBones);
    if (!array || !core::is_plausible_pointer(*array, alignof(float)))
    {
        return std::nullopt;
    }
    std::array<std::byte, maths::kBoneCount * layout::kBoneStride> raw{};
    if (!memory.read_bytes(*array, raw.data(), raw.size()))
    {
        return std::nullopt;
    }
    maths::Bones bones{};
    for (std::size_t i = 0; i < maths::kBoneCount; ++i)
    {
        std::memcpy(&bones[i], raw.data() + i * layout::kBoneStride, sizeof(maths::Vec3));
        if (!bones[i].is_finite() || bones[i].distance_to(origin) > config::kMaxBoneDistance)
        {
            return std::nullopt;
        }
    }
    return bones;
}
} // namespace game
