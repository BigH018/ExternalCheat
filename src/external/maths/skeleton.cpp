#include "maths/skeleton.h"

namespace maths
{
static_assert([] {
    for (const BoneLink link : kSkeletonLinks)
    {
        if (link.from >= kBoneCount || link.to >= kBoneCount)
        {
            return false;
        }
    }
    return true;
}(), "every skeleton link must use bones that are read");

std::vector<ScreenLine> project_skeleton(const ViewMatrix& view, const Bones& bones, Vec2 screen)
{
    std::array<std::optional<Vec2>, kBoneCount> projected{};
    for (std::size_t i = 0; i < kBoneCount; ++i)
    {
        projected[i] = world_to_screen(view, bones[i], screen);
    }
    std::vector<ScreenLine> lines;
    lines.reserve(kSkeletonLinks.size());
    for (const BoneLink link : kSkeletonLinks)
    {
        if (projected[link.from] && projected[link.to])
        {
            lines.push_back(ScreenLine{*projected[link.from], *projected[link.to]});
        }
    }
    return lines;
}
} // namespace maths
