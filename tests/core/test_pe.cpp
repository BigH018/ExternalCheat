#include <cstdint>

#include <doctest.h>

#include "core/pe.h"
#include "helpers/fake_pe.h"

namespace
{
constexpr std::uintptr_t kBase = 0x7FFD60000000;
} // namespace

TEST_CASE("pe::read_headers and find_section")
{
    test::FakeMemory memory;
    test::map_fake_pe(memory, kBase, {});

    const auto headers = core::pe::read_headers(memory, kBase);
    REQUIRE(headers.has_value());
    CHECK(headers->size_of_image == test::kFakePeSize);
    CHECK(headers->export_rva == test::kFakePeExports);
    CHECK(headers->export_size == test::kFakePeExportsSize);
    REQUIRE(headers->sections.size() == 2);

    const auto text = core::pe::find_section(*headers, ".text");
    REQUIRE(text.has_value());
    CHECK(text->rva == test::kFakePeText);
    CHECK(text->size == test::kFakePeTextSize);
    CHECK(core::pe::find_section(*headers, ".rdata").has_value());
    CHECK_FALSE(core::pe::find_section(*headers, ".data").has_value());
}

TEST_CASE("pe::read_headers rejects what isn't a mapped PE32+ image")
{
    test::FakeMemory memory;
    CHECK_FALSE(core::pe::read_headers(memory, kBase).has_value()); // nothing mapped

    test::map_fake_pe(memory, kBase, {});
    memory.put<std::uint16_t>(kBase, 0x0000); // no "MZ"
    CHECK_FALSE(core::pe::read_headers(memory, kBase).has_value());

    memory.put<std::uint16_t>(kBase, 0x5A4D);
    memory.put<std::uint16_t>(kBase + 0x80 + 4, 0x014C); // an x86 module
    CHECK_FALSE(core::pe::read_headers(memory, kBase).has_value());
}

TEST_CASE("pe::find_export: by exact name, forwarders and missing names give nullopt")
{
    test::FakeMemory memory;
    test::map_fake_pe(memory, kBase,
                      {{"CreateInterface", 0x1000},
                       {"CreateInterfaceEx", 0x1100},
                       {"Forwarded", test::kFakePeExports + 0x300}}); // RVA inside the export directory

    CHECK(core::pe::find_export(memory, kBase, "CreateInterface") == kBase + 0x1000);
    CHECK(core::pe::find_export(memory, kBase, "CreateInterfaceEx") == kBase + 0x1100);
    CHECK_FALSE(core::pe::find_export(memory, kBase, "Create").has_value()); // a prefix isn't a match
    CHECK_FALSE(core::pe::find_export(memory, kBase, "Missing").has_value());
    CHECK_FALSE(core::pe::find_export(memory, kBase, "Forwarded").has_value());
}
