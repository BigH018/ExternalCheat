#include <cstdint>

#include <doctest.h>

#include "core/memory.h"
#include "helpers/fake_memory.h"

// The pure parts of core/memory: pointer checks and the Memory base class's typed wrappers, driven through FakeMemory.

TEST_CASE("is_plausible_pointer: user-space range")
{
    CHECK_FALSE(core::is_plausible_pointer(0));
    CHECK_FALSE(core::is_plausible_pointer(0xFFFF));
    CHECK(core::is_plausible_pointer(0x10000));
    CHECK(core::is_plausible_pointer(0x7FFFFFFFFFFE));
    CHECK_FALSE(core::is_plausible_pointer(0x7FFFFFFFFFFF));
    CHECK_FALSE(core::is_plausible_pointer(0xFFFF800000000000));
}

TEST_CASE("is_plausible_pointer: alignment")
{
    CHECK(core::is_plausible_pointer(0x10008, 8));
    CHECK_FALSE(core::is_plausible_pointer(0x10004, 8));
    CHECK(core::is_plausible_pointer(0x10004, 4));
    CHECK(core::is_plausible_pointer(0x10001, 1));
    CHECK_FALSE(core::is_plausible_pointer(0x10000, 0));
}

TEST_CASE("is_plausible_range")
{
    CHECK(core::is_plausible_range(0x10000, 8));
    CHECK_FALSE(core::is_plausible_range(0x10000, 0));
    CHECK_FALSE(core::is_plausible_range(0, 8));
    CHECK(core::is_plausible_range(0x7FFFFFFFFFF0, 0xF));
    CHECK_FALSE(core::is_plausible_range(0x7FFFFFFFFFF0, 0x10));
    CHECK_FALSE(core::is_plausible_range(0x7FFFFFFFFFF0, SIZE_MAX));
}

TEST_CASE("pointer checks are usable at compile time")
{
    static_assert(core::is_plausible_pointer(0x10000));
    static_assert(!core::is_plausible_pointer(0));
    static_assert(core::is_plausible_range(0x10000, 4));
}

TEST_CASE("Memory: safe_read / read / safe_write")
{
    test::FakeMemory memory;
    memory.map(0x200000, 0x100);
    memory.put<std::int32_t>(0x200010, 100);

    std::int32_t health = 0;
    CHECK(memory.safe_read(0x200010, health));
    CHECK(health == 100);

    CHECK(memory.read<std::int32_t>(0x200010) == 100);
    CHECK_FALSE(memory.read<std::int32_t>(0x300000).has_value());

    CHECK(memory.safe_write<std::int32_t>(0x200010, 42));
    CHECK(memory.read<std::int32_t>(0x200010) == 42);
    CHECK_FALSE(memory.safe_write<std::int32_t>(0x300000, 42));
}

TEST_CASE("Memory: a failed read leaves the output untouched")
{
    test::FakeMemory memory;
    memory.map(0x200000, 0x10);

    std::uint64_t out = 77;
    SUBCASE("unmapped")
    {
        CHECK_FALSE(memory.safe_read(0x300000, out));
    }
    SUBCASE("runs off the end of a region")
    {
        CHECK_FALSE(memory.safe_read(0x20000C, out));
    }
    CHECK(out == 77);
}

TEST_CASE("Memory: implausible requests never reach the implementation")
{
    test::FakeMemory memory;
    std::int32_t out = 0;
    CHECK_FALSE(memory.safe_read(0, out));
    CHECK_FALSE(memory.safe_read(0xFFFF800000000000, out));
    CHECK_FALSE(memory.read_bytes(0x200000, &out, 0));
    CHECK_FALSE(memory.read_bytes(0x200000, nullptr, sizeof(out)));
    CHECK_FALSE(memory.safe_write<std::int32_t>(0, 1));
    CHECK(memory.read_count() == 0);
    CHECK(memory.write_count() == 0);
}

TEST_CASE("read_string: up to the terminator, within max_length")
{
    test::FakeMemory memory;
    memory.map(0x200000, 0x100);
    memory.put_string(0x200000, "SchemaSystem_001");
    memory.put_string(0x200040, "");

    CHECK(core::read_string(memory, 0x200000, 64) == "SchemaSystem_001");
    CHECK(core::read_string(memory, 0x200000, 17) == "SchemaSystem_001"); // 16 characters + terminator
    CHECK_FALSE(core::read_string(memory, 0x200000, 16).has_value());      // no terminator within 16
    CHECK(core::read_string(memory, 0x200040, 64) == "");
    CHECK_FALSE(core::read_string(memory, 0x300000, 64).has_value());      // unmapped
}

TEST_CASE("read_string: a string at the very end of the last mapped page still reads")
{
    // Mapped up to a page boundary and nothing after it, like a string at the end of a module's .rdata. One 256-byte
    // read would fail; read_string reads up to the page end first.
    test::FakeMemory memory;
    memory.map(0x200000, 0x1000);
    memory.put_string(0x200FF8, "client"); // 6 characters + NUL end at 0x200FFF
    CHECK(core::read_string(memory, 0x200FF8, 256) == "client");

    // A string that runs over the boundary into memory that isn't there: no terminator, nullopt.
    memory.put_bytes(0x200FFC, "abcd", 4);
    CHECK_FALSE(core::read_string(memory, 0x200FFC, 256).has_value());
}
