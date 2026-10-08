#pragma once

// Who a player spectates, read from outside: controller -> m_hObserverPawn (a C_CSObserverPawn) ->
// m_pObserverServices -> m_iObserverMode / m_hObserverTarget. Every controller has an observer pawn, alive or not;
// only a dead player's mode and target mean anything (proven live 2026-10-07, docs/offsets.md "Observer").
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <optional>

#include "core/memory.h"
#include "game/snapshot.h"

namespace game
{
struct ObserverState
{
    ObserverMode mode = ObserverMode::none;
    std::uint32_t target_handle = 0; // m_hObserverTarget as read (0xFFFFFFFF = none)
    std::uintptr_t target = 0;       // the entity it resolves to, 0 = none or stale
};

// The observer state of `controller`. nullopt if it has no observer pawn or the chain doesn't read (null services).
// A mode outside ObserverMode's range reads as none.
[[nodiscard]] std::optional<ObserverState> read_observer(const core::Memory& memory, std::uintptr_t entity_system,
                                                         std::uintptr_t controller) noexcept;
} // namespace game
