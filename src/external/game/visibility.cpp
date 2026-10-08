#include "game/visibility.h"

#include "game/schema.h"

namespace game
{
std::optional<std::uint64_t> read_spotted_by_mask(const core::Memory& memory, std::uintptr_t pawn) noexcept
{
    return memory.read<std::uint64_t>(pawn + schema::C_CSPlayerPawn::m_entitySpottedState +
                                      schema::EntitySpottedState_t::m_bSpottedByMask);
}
} // namespace game
