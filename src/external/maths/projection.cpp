#include "maths/projection.h"

namespace maths
{
std::optional<Vec2> world_to_screen(const ViewMatrix& view, Vec3 world, Vec2 screen) noexcept
{
    const auto row = [&](int r) {
        return view.at(r, 0) * world.x + view.at(r, 1) * world.y + view.at(r, 2) * world.z + view.at(r, 3);
    };
    const float w = row(3);
    if (!(w >= kMinClipW)) // also false for NaN
    {
        return std::nullopt;
    }
    const float ndc_x = row(0) / w;
    const float ndc_y = row(1) / w;
    return Vec2{screen.x * 0.5f * (1.0f + ndc_x), screen.y * 0.5f * (1.0f - ndc_y)};
}
} // namespace maths
