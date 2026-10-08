#include <cstdint>
#include <cstring>
#include <string_view>

#include <doctest.h>

#include "config.h"
#include "core/pattern.h"
#include "game/offsets.h"
#include "game/schema_system.h"
#include "helpers/fake_memory.h"

namespace layout = game::offsets::layout;

namespace
{
// Writes `value` at `address` inside a RemoteCopy.
template <class T>
void put(core::RemoteCopy& copy, std::uintptr_t address, const T& value)
{
    std::memcpy(copy.bytes.data() + (address - copy.base), &value, sizeof(T));
}

void put_string(core::RemoteCopy& copy, std::uintptr_t address, std::string_view text)
{
    std::memcpy(copy.bytes.data() + (address - copy.base), text.data(), text.size());
    copy.bytes[address - copy.base + text.size()] = std::byte{0};
}

// A class info at `info` in the copy: self pointer, name and module pointers.
void put_class_info(core::RemoteCopy& copy, std::uintptr_t info, std::uintptr_t name, std::uintptr_t module)
{
    put<std::uintptr_t>(copy, info + layout::kClassInfoSelf, info);
    put<std::uintptr_t>(copy, info + layout::kClassInfoName, name);
    put<std::uintptr_t>(copy, info + layout::kClassInfoModule, module);
}
} // namespace

TEST_CASE("find_type_scope finds a module's scope by name")
{
    constexpr std::uintptr_t kSchemaSystem = 0x7FFDD9B76710;
    constexpr std::uintptr_t kScopes = 0x4DE00000000;
    constexpr std::uintptr_t kServerScope = 0x4DE00001000;
    constexpr std::uintptr_t kClientScope = 0x4DE00002000;

    test::FakeMemory memory;
    memory.map(kSchemaSystem, 0x200);
    memory.put<std::int32_t>(kSchemaSystem + layout::kSchemaSystemScopeCount, 2);
    memory.put<std::uintptr_t>(kSchemaSystem + layout::kSchemaSystemScopeData, kScopes);
    memory.map(kScopes, 16);
    memory.put<std::uintptr_t>(kScopes, kServerScope);
    memory.put<std::uintptr_t>(kScopes + 8, kClientScope);
    memory.map(kServerScope, 0x200);
    memory.put_string(kServerScope + layout::kTypeScopeName, "server.dll");
    memory.map(kClientScope, 0x200);
    memory.put_string(kClientScope + layout::kTypeScopeName, "client.dll");

    CHECK(game::find_type_scope(memory, kSchemaSystem, "client.dll") == kClientScope);
    CHECK(game::find_type_scope(memory, kSchemaSystem, "server.dll") == kServerScope);
    CHECK_FALSE(game::find_type_scope(memory, kSchemaSystem, "client").has_value());

    memory.put<std::int32_t>(kSchemaSystem + layout::kSchemaSystemScopeCount, 0);
    CHECK_FALSE(game::find_type_scope(memory, kSchemaSystem, "client.dll").has_value());
}

TEST_CASE("index_classes: self-pointing class infos of one module, by name")
{
    core::RemoteCopy copy;
    copy.base = 0x7FFD62930000;
    copy.bytes.resize(0x1000);
    const std::uintptr_t b = copy.base;
    put_string(copy, b + 0x800, "client");
    put_string(copy, b + 0x810, "server");
    put_string(copy, b + 0x820, "C_CSPlayerPawn");
    put_string(copy, b + 0x840, "C_BaseEntity");
    put_string(copy, b + 0x860, "Twice");

    put_class_info(copy, b + 0x100, b + 0x820, b + 0x800); // C_CSPlayerPawn, client
    put_class_info(copy, b + 0x200, b + 0x840, b + 0x810); // C_BaseEntity, but server: left out
    put_class_info(copy, b + 0x300, b + 0x860, b + 0x800); // "Twice", client
    put_class_info(copy, b + 0x400, b + 0x860, b + 0x800); // "Twice" again: ambiguous, left out
    put_class_info(copy, b + 0x500, 0x1234, b + 0x800);    // name outside the copy: left out
    put<std::uintptr_t>(copy, b + 0x608, b + 0x600);       // not a self-pointer at its own address: ignored

    const auto classes = game::index_classes(copy, "client");
    CHECK(classes.size() == 1);
    REQUIRE(classes.contains("C_CSPlayerPawn"));
    CHECK(classes.at("C_CSPlayerPawn") == b + 0x100);
}

TEST_CASE("read_class reads size, type scope and the declared fields")
{
    constexpr std::uintptr_t kInfo = 0x7FFD64B67420;
    constexpr std::uintptr_t kFields = 0x7FFD64B66640;
    constexpr std::uintptr_t kNames = 0x7FFD644D0000;
    constexpr std::uintptr_t kScope = 0x4DEDBA62000;

    test::FakeMemory memory;
    memory.map(kInfo, layout::kClassInfoReadSize);
    memory.put<std::uintptr_t>(kInfo + layout::kClassInfoSelf, kInfo);
    memory.put<std::int32_t>(kInfo + layout::kClassInfoSize, 0x3710);
    memory.put<std::int16_t>(kInfo + layout::kClassInfoFieldCount, 2);
    memory.put<std::uintptr_t>(kInfo + layout::kClassInfoFields, kFields);
    memory.put<std::uintptr_t>(kInfo + layout::kClassInfoTypeScope, kScope);
    memory.map(kFields, 2 * layout::kFieldStride);
    memory.put<std::uintptr_t>(kFields + layout::kFieldName, kNames);
    memory.put<std::int32_t>(kFields + layout::kFieldOffset, 0x1EA0);
    memory.put<std::uintptr_t>(kFields + layout::kFieldStride + layout::kFieldName, kNames + 0x20);
    memory.put<std::int32_t>(kFields + layout::kFieldStride + layout::kFieldOffset, 0x36CC);
    memory.map(kNames, config::kPageSize); // real memory is mapped a page at a time, and read_string relies on it
    memory.put_string(kNames, "m_bIsScoped");
    memory.put_string(kNames + 0x20, "m_iIDEntIndex");

    const auto live = game::read_class(memory, kInfo);
    REQUIRE(live.has_value());
    CHECK(live->size == 0x3710);
    CHECK(live->type_scope == kScope);
    REQUIRE(live->fields.size() == 2);
    CHECK(live->offset_of("m_bIsScoped") == 0x1EA0u);
    CHECK(live->offset_of("m_iIDEntIndex") == 0x36CCu);
    CHECK_FALSE(live->offset_of("m_iHealth").has_value()); // declared by C_BaseEntity, not here

    SUBCASE("not a class info (no self pointer)")
    {
        memory.put<std::uintptr_t>(kInfo + layout::kClassInfoSelf, 0);
        CHECK_FALSE(game::read_class(memory, kInfo).has_value());
    }
    SUBCASE("negative field count")
    {
        memory.put<std::int16_t>(kInfo + layout::kClassInfoFieldCount, -1);
        CHECK_FALSE(game::read_class(memory, kInfo).has_value());
    }
    SUBCASE("unreadable field name")
    {
        memory.put<std::uintptr_t>(kFields + layout::kFieldName, 0x1000000);
        CHECK_FALSE(game::read_class(memory, kInfo).has_value());
    }
}
