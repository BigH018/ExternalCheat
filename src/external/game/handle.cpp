#include "game/handle.h"

namespace game
{
namespace layout = offsets::layout;

std::optional<std::uintptr_t> read_entity_system(const core::Memory& memory, std::uintptr_t client_base) noexcept
{
    const auto system = memory.read<std::uintptr_t>(client_base + offsets::client::dwEntityList);
    if (!system || !core::is_plausible_pointer(*system, alignof(std::uintptr_t)))
    {
        return std::nullopt;
    }
    return system;
}

std::optional<std::uintptr_t> identity_address(const core::Memory& memory, std::uintptr_t entity_system,
                                               std::uint32_t index) noexcept
{
    const std::uint32_t chunk_number = index >> layout::kEntityChunkShift;
    if (chunk_number >= layout::kEntityChunkCount)
    {
        return std::nullopt;
    }
    const auto chunk =
        memory.read<std::uintptr_t>(entity_system + layout::kEntityChunks + chunk_number * sizeof(std::uintptr_t));
    if (!chunk || !core::is_plausible_pointer(*chunk))
    {
        return std::nullopt;
    }
    return *chunk + (index & layout::kEntityChunkMask) * layout::kIdentitySize;
}

std::uintptr_t entity_at(const core::Memory& memory, std::uintptr_t entity_system, std::uint32_t index) noexcept
{
    const auto identity = identity_address(memory, entity_system, index);
    if (!identity)
    {
        return 0;
    }
    const auto entity = memory.read<std::uintptr_t>(*identity + layout::kIdentityEntity);
    return entity && core::is_plausible_pointer(*entity, alignof(std::uintptr_t)) ? *entity : 0;
}

std::uintptr_t resolve_handle(const core::Memory& memory, std::uintptr_t entity_system, std::uint32_t handle) noexcept
{
    if (!is_valid_handle(handle))
    {
        return 0;
    }
    const auto identity = identity_address(memory, entity_system, handle_index(handle));
    if (!identity)
    {
        return 0;
    }
    // The slot may have been reused since the handle was stored: then the serial differs.
    const auto current = memory.read<std::uint32_t>(*identity + layout::kIdentityHandle);
    if (!current || *current != handle)
    {
        return 0;
    }
    const auto entity = memory.read<std::uintptr_t>(*identity + layout::kIdentityEntity);
    return entity && core::is_plausible_pointer(*entity, alignof(std::uintptr_t)) ? *entity : 0;
}
} // namespace game
