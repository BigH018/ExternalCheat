#include <cstdint>

#include <doctest.h>

#include "game/handle.h"
#include "game/offsets.h"
#include "helpers/fake_entities.h"
#include "helpers/fake_memory.h"

namespace
{
using test::FakeEntityList;

constexpr std::uintptr_t kClientBase = 0x7FFD62930000;
constexpr std::uintptr_t kEntity = 0x4DF5A1B4200;
constexpr std::uintptr_t kWeapon = 0x4DF60000000;
} // namespace

TEST_CASE("CHandle: index bits and validity")
{
    CHECK(game::handle_index(0x1B182E6) == 742); // the local player's AK-47 in build 14189
    CHECK(game::handle_index(0x8E0001) == 1);
    CHECK(game::is_valid_handle(0x8E0001));
    CHECK_FALSE(game::is_valid_handle(0xFFFFFFFF));
    CHECK_FALSE(game::is_valid_handle(0x00007FFF));
}

TEST_CASE("read_entity_system reads client.dll + dwEntityList and rejects implausible pointers")
{
    test::FakeMemory memory;
    const std::uintptr_t slot = kClientBase + game::offsets::client::dwEntityList;
    memory.map_value<std::uintptr_t>(slot, FakeEntityList::kSystem);
    CHECK(game::read_entity_system(memory, kClientBase) == FakeEntityList::kSystem);

    memory.put<std::uintptr_t>(slot, 0);
    CHECK_FALSE(game::read_entity_system(memory, kClientBase).has_value());
    memory.put<std::uintptr_t>(slot, FakeEntityList::kSystem + 1); // misaligned
    CHECK_FALSE(game::read_entity_system(memory, kClientBase).has_value());
}

TEST_CASE("entity_at walks the chunked list")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    list.add(1, kEntity, 0x11C);
    list.add(742, kWeapon, 0x363); // second chunk (742 >> 9 == 1), slot 230

    CHECK(game::entity_at(memory, FakeEntityList::kSystem, 1) == kEntity);
    CHECK(game::entity_at(memory, FakeEntityList::kSystem, 742) == kWeapon);
    CHECK(game::identity_address(memory, FakeEntityList::kSystem, 742) ==
          FakeEntityList::kChunks + FakeEntityList::kChunkSpacing + 230 * 0x70);

    CHECK(game::entity_at(memory, FakeEntityList::kSystem, 2) == 0);    // empty slot in a mapped chunk
    CHECK(game::entity_at(memory, FakeEntityList::kSystem, 5000) == 0); // chunk 9 isn't allocated
    CHECK_FALSE(game::identity_address(memory, FakeEntityList::kSystem, 0x8000).has_value()); // past the last chunk
}

TEST_CASE("resolve_handle checks the serial")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    const std::uint32_t handle = list.add(742, kWeapon, 0x363);
    CHECK(handle == 0x1B182E6);

    CHECK(game::resolve_handle(memory, FakeEntityList::kSystem, handle) == kWeapon);
    CHECK(game::resolve_handle(memory, FakeEntityList::kSystem, FakeEntityList::make_handle(742, 0x364)) == 0);
    CHECK(game::resolve_handle(memory, FakeEntityList::kSystem, 0xFFFFFFFF) == 0);
    CHECK(game::resolve_handle(memory, FakeEntityList::kSystem, FakeEntityList::make_handle(743, 0x363)) == 0);

    // The slot reused by another entity: the old handle no longer resolves, the new one does.
    const std::uint32_t reused = list.add(742, kEntity, 0x400);
    CHECK(game::resolve_handle(memory, FakeEntityList::kSystem, handle) == 0);
    CHECK(game::resolve_handle(memory, FakeEntityList::kSystem, reused) == kEntity);
}

TEST_CASE("resolve_handle rejects an implausible entity pointer")
{
    test::FakeMemory memory;
    FakeEntityList list(memory);
    const std::uint32_t handle = list.add(3, 0x1234, 1); // below the first valid address
    CHECK(game::resolve_handle(memory, FakeEntityList::kSystem, handle) == 0);
}
