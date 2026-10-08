#include <cstdint>

#include <doctest.h>

#include "game/schema.h"
#include "game/visibility.h"
#include "helpers/fake_memory.h"

TEST_CASE("player_slot: controller index - 1")
{
    CHECK(game::player_slot(1) == 0);
    CHECK(game::player_slot(20) == 19);
    CHECK(game::player_slot(64) == 63);
}

TEST_CASE("is_spotted_by: one bit per slot across both uint32 halves")
{
    CHECK(game::is_spotted_by(0b1, 0));
    CHECK_FALSE(game::is_spotted_by(0b1, 1));
    CHECK(game::is_spotted_by(0b100, 2));
    CHECK(game::is_spotted_by(std::uint64_t{1} << 31, 31));
    CHECK(game::is_spotted_by(std::uint64_t{1} << 32, 32)); // slot 32 is bit 0 of the second uint32
    CHECK(game::is_spotted_by(std::uint64_t{1} << 63, 63));
    CHECK_FALSE(game::is_spotted_by(~std::uint64_t{0}, 64)); // out of range
    CHECK_FALSE(game::is_spotted_by(0, 5));
}

TEST_CASE("read_spotted_by_mask reads both uint32s of m_bSpottedByMask as one little-endian uint64")
{
    constexpr std::uintptr_t kPawn = 0x4DF8618D800;
    const std::uintptr_t mask = kPawn + game::schema::C_CSPlayerPawn::m_entitySpottedState +
                                game::schema::EntitySpottedState_t::m_bSpottedByMask;
    test::FakeMemory memory;
    memory.map(kPawn, 0x3800);
    memory.put<std::uint32_t>(mask, 0x00000005);     // slots 0 and 2
    memory.put<std::uint32_t>(mask + 4, 0x00000001); // slot 32
    const auto read = game::read_spotted_by_mask(memory, kPawn);
    REQUIRE(read.has_value());
    CHECK(game::is_spotted_by(*read, 0));
    CHECK_FALSE(game::is_spotted_by(*read, 1));
    CHECK(game::is_spotted_by(*read, 2));
    CHECK(game::is_spotted_by(*read, 32));

    const test::FakeMemory nothing;
    CHECK_FALSE(game::read_spotted_by_mask(nothing, kPawn).has_value());
}
