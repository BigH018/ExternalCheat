#include "game/observer.h"

#include "game/handle.h"
#include "game/schema.h"

namespace game
{
std::optional<ObserverState> read_observer(const core::Memory& memory, std::uintptr_t entity_system,
                                           std::uintptr_t controller) noexcept
{
    const auto pawn_handle = memory.read<std::uint32_t>(controller + schema::CCSPlayerController::m_hObserverPawn);
    const std::uintptr_t observer_pawn = pawn_handle ? resolve_handle(memory, entity_system, *pawn_handle) : 0;
    if (observer_pawn == 0)
    {
        return std::nullopt;
    }
    const auto services = memory.read<std::uintptr_t>(observer_pawn + schema::C_BasePlayerPawn::m_pObserverServices);
    if (!services || !core::is_plausible_pointer(*services, alignof(std::uintptr_t)))
    {
        return std::nullopt;
    }
    const auto mode = memory.read<std::uint8_t>(*services + schema::CPlayer_ObserverServices::m_iObserverMode);
    const auto target = memory.read<std::uint32_t>(*services + schema::CPlayer_ObserverServices::m_hObserverTarget);
    if (!mode || !target)
    {
        return std::nullopt;
    }

    ObserverState state;
    state.mode = *mode <= static_cast<std::uint8_t>(ObserverMode::roaming) ? static_cast<ObserverMode>(*mode)
                                                                           : ObserverMode::none;
    state.target_handle = *target;
    state.target = resolve_handle(memory, entity_system, *target);
    return state;
}
} // namespace game
