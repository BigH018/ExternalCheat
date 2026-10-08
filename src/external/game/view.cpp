#include "game/view.h"

#include "game/offsets.h"

namespace game
{
std::optional<maths::ViewMatrix> read_view_matrix(const core::Memory& memory, std::uintptr_t client_base) noexcept
{
    return memory.read<maths::ViewMatrix>(client_base + offsets::client::dwViewMatrix);
}
} // namespace game
