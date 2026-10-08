#include "game/writes.h"

#include <array>
#include <cmath>

#include "game/offsets.h"

namespace game
{
namespace layout = offsets::layout;

bool set_button(core::Memory& memory, std::uintptr_t client_base, std::uintptr_t button_rva, bool down) noexcept
{
    return memory.safe_write<std::uint32_t>(client_base + button_rva,
                                            down ? layout::kButtonPressed : layout::kButtonReleased);
}

std::optional<maths::Angles> read_view_angles(const core::Memory& memory, std::uintptr_t client_base) noexcept
{
    const auto raw = memory.read<std::array<float, 2>>(client_base + offsets::client::dwViewAngles);
    if (!raw || !std::isfinite((*raw)[0]) || !std::isfinite((*raw)[1]))
    {
        return std::nullopt;
    }
    return maths::Angles{(*raw)[0], (*raw)[1]};
}

bool write_view_angles(core::Memory& memory, std::uintptr_t client_base, maths::Angles angles) noexcept
{
    const maths::Angles safe = maths::normalize(angles);
    const std::array<float, 2> raw{safe.pitch, safe.yaw};
    static_assert(sizeof(raw) == layout::kViewAnglesWriteSize);
    return memory.safe_write(client_base + offsets::client::dwViewAngles, raw);
}
} // namespace game
