#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>

#include <doctest.h>

#include "core/pattern.h"
#include "helpers/fake_memory.h"

namespace
{
std::vector<std::byte> bytes(std::initializer_list<int> values)
{
    std::vector<std::byte> out;
    for (const int value : values)
    {
        out.push_back(static_cast<std::byte>(value));
    }
    return out;
}
} // namespace

TEST_CASE("Pattern::parse: hex bytes and wildcards")
{
    const auto pattern = core::Pattern::parse("48 8B ? ?? 05");
    REQUIRE(pattern.has_value());
    CHECK(pattern->size() == 5);
    CHECK(core::Pattern::parse("48 8b 0d")->size() == 3); // lower case
    CHECK(core::Pattern::parse("  48\t8B  \n05 ")->size() == 3); // any whitespace
    CHECK(core::Pattern::parse("? 8B")->size() == 2);          // leading wildcard
}

TEST_CASE("Pattern::parse rejects bad patterns")
{
    CHECK_FALSE(core::Pattern::parse("").has_value());
    CHECK_FALSE(core::Pattern::parse("   ").has_value());
    CHECK_FALSE(core::Pattern::parse("? ?? ?").has_value()); // nothing but wildcards
    CHECK_FALSE(core::Pattern::parse("4").has_value());
    CHECK_FALSE(core::Pattern::parse("488B").has_value());
    CHECK_FALSE(core::Pattern::parse("G1").has_value());
    CHECK_FALSE(core::Pattern::parse("48 ??? 05").has_value());
    CHECK_FALSE(core::Pattern::parse("0x48").has_value());
}

TEST_CASE("Pattern: matches_at and find_all")
{
    const auto data = bytes({0x90, 0x48, 0x8B, 0x05, 0x11, 0x22, 0x48, 0x8B, 0x05, 0x33});
    const auto pattern = core::Pattern::parse("48 8B 05 ?");
    REQUIRE(pattern.has_value());

    CHECK(pattern->matches_at(data, 1));
    CHECK_FALSE(pattern->matches_at(data, 0));
    CHECK(pattern->matches_at(data, 6)); // ends exactly at the end of the data
    CHECK_FALSE(pattern->matches_at(data, 7));
    CHECK_FALSE(pattern->matches_at(data, 100));

    CHECK(pattern->find_all(data, 16) == std::vector<std::size_t>{1, 6});
    CHECK(pattern->find_all(data, 1) == std::vector<std::size_t>{1}); // max_hits
    CHECK(pattern->find_all(std::span<const std::byte>(data).first(3), 16).empty()); // shorter than the pattern
}

TEST_CASE("Pattern::find_all: overlapping matches, a match at 0, a leading wildcard")
{
    const auto data = bytes({0xAA, 0xAA, 0xAA, 0x01});
    CHECK(core::Pattern::parse("AA AA")->find_all(data, 16) == std::vector<std::size_t>{0, 1});
    CHECK(core::Pattern::parse("? AA")->find_all(data, 16) == std::vector<std::size_t>{0, 1});
    CHECK(core::Pattern::parse("? 01")->find_all(data, 16) == std::vector<std::size_t>{2});
    CHECK(core::Pattern::parse("01 ?")->find_all(data, 16).empty()); // would run past the end
}

TEST_CASE("rip_relative: RIP is the end of the instruction; the displacement is signed")
{
    static_assert(core::rip_relative(0x1000, 7, 0x10) == 0x1017);
    static_assert(core::rip_relative(0x1000, 7, -0x17) == 0x1000 - 0x10);
    CHECK(core::rip_relative(0x7FFD62930000, 7, 0) == 0x7FFD62930007);
}

TEST_CASE("copy_remote: chunked copy, unreadable pages zero-filled and counted")
{
    test::FakeMemory memory;
    memory.map(0x100000, 0x3000);
    memory.map(0x104000, 0x1000); // a one-page hole at 0x103000
    memory.put<std::uint32_t>(0x100000, 0x11111111);
    memory.put<std::uint32_t>(0x102FFC, 0x22222222);
    memory.put<std::uint32_t>(0x104000, 0x33333333);

    const core::RemoteCopy copy = core::copy_remote(memory, 0x100000, 0x5000, 0x2000);
    CHECK(copy.base == 0x100000);
    REQUIRE(copy.bytes.size() == 0x5000);
    CHECK(copy.unreadable_pages == 1);

    const auto u32_at = [&](std::uintptr_t address) {
        std::uint32_t value = 0;
        std::memcpy(&value, copy.view(address, 4).data(), 4);
        return value;
    };
    CHECK(u32_at(0x100000) == 0x11111111);
    CHECK(u32_at(0x102FFC) == 0x22222222);
    CHECK(u32_at(0x103000) == 0); // the hole
    CHECK(u32_at(0x104000) == 0x33333333);
}

TEST_CASE("RemoteCopy::view and rip_relative_target stay inside the copy")
{
    core::RemoteCopy copy;
    copy.base = 0x140000000;
    copy.bytes = bytes({0x48, 0x8B, 0x05, 0x10, 0x00, 0x00, 0x00, 0xC3});

    CHECK(copy.contains(0x140000000, 8));
    CHECK_FALSE(copy.contains(0x140000000, 9));
    CHECK_FALSE(copy.contains(0x13FFFFFFF, 1));
    CHECK(copy.view(0x140000008, 1).empty());
    CHECK(copy.view(0x140000008, 0).empty());

    CHECK(core::rip_relative_target(copy, 0x140000000, 3, 7) == 0x140000017);
    CHECK_FALSE(core::rip_relative_target(copy, 0x140000002, 3, 7).has_value()); // disp32 would run past the end
}
