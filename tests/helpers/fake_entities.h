#pragma once

// A fake CS2 entity system inside a FakeMemory, laid out like the real one (game/offsets.h, offsets::layout):
// chunk pointers at system + 0x10, 512 identities of 0x70 bytes per chunk, each identity holding the entity pointer
// (+0x0), its handle (+0x10) and a pointer to its designer name (+0x20).
//
// Chunks are mapped on first use. The entities themselves are up to the test: map them where you like.

#include <cstdint>
#include <set>
#include <string_view>

#include "game/offsets.h"
#include "helpers/fake_memory.h"

namespace test
{
class FakeEntityList
{
public:
    static constexpr std::uintptr_t kSystem = 0x4DEDB552800;
    static constexpr std::uintptr_t kChunks = 0x4DE10000000; // chunk n at kChunks + n * kChunkSpacing
    static constexpr std::uintptr_t kChunkSpacing = 0x100000;
    static constexpr std::uintptr_t kNames = 0x4DE20000000;

    explicit FakeEntityList(FakeMemory& memory) : memory_(memory)
    {
        namespace layout = game::offsets::layout;
        memory_.map(kSystem, layout::kEntityChunks + layout::kEntityChunkCount * sizeof(std::uintptr_t));
        memory_.map(kNames, 0x10000);
    }

    [[nodiscard]] static constexpr std::uint32_t make_handle(std::uint32_t index, std::uint32_t serial) noexcept
    {
        return (serial << 15) | index;
    }

    // The address entity `index`'s identity has (maps its chunk if needed).
    std::uintptr_t identity(std::uint32_t index)
    {
        namespace layout = game::offsets::layout;
        const std::uint32_t chunk = index >> layout::kEntityChunkShift;
        const std::uintptr_t chunk_base = kChunks + chunk * kChunkSpacing;
        if (mapped_chunks_.insert(chunk).second)
        {
            memory_.map(chunk_base, (layout::kEntityChunkMask + 1) * layout::kIdentitySize);
            memory_.put<std::uintptr_t>(kSystem + layout::kEntityChunks + chunk * sizeof(std::uintptr_t), chunk_base);
        }
        return chunk_base + (index & layout::kEntityChunkMask) * layout::kIdentitySize;
    }

    // Puts `entity` at `index` with the given serial and designer name. Returns its handle.
    std::uint32_t add(std::uint32_t index, std::uintptr_t entity, std::uint32_t serial,
                      std::string_view designer_name = "")
    {
        namespace layout = game::offsets::layout;
        const std::uintptr_t at = identity(index);
        const std::uint32_t handle = make_handle(index, serial);
        memory_.put<std::uintptr_t>(at + layout::kIdentityEntity, entity);
        memory_.put<std::uint32_t>(at + layout::kIdentityHandle, handle);
        if (!designer_name.empty())
        {
            memory_.put_string(next_name_, designer_name);
            memory_.put<std::uintptr_t>(at + layout::kIdentityDesignerName, next_name_);
            next_name_ += designer_name.size() + 1;
        }
        return handle;
    }

private:
    FakeMemory& memory_;
    std::set<std::uint32_t> mapped_chunks_;
    std::uintptr_t next_name_ = kNames;
};
} // namespace test
