#include <array>
#include <cstdint>

#include <doctest.h>

#include "config.h"
#include "core/pattern.h"
#include "game/interfaces.h"
#include "helpers/fake_pe.h"

// A fake module that registers two interfaces, laid out like CS2's modules:
//   CreateInterface: mov r9, [rip + head]      head -> node1 -> node2 -> null
//   node1: create1 = lea rax, [rip + instance]; ret      node2: create2 = something else

namespace
{
constexpr std::uintptr_t kBase = 0x7FFDD9B00000;
constexpr std::uintptr_t kCreateInterface = kBase + 0x1000;
constexpr std::uintptr_t kCreate1 = kBase + 0x1100;
constexpr std::uintptr_t kCreate2 = kBase + 0x1200;
constexpr std::uintptr_t kHead = kBase + test::kFakePeData;
constexpr std::uintptr_t kInstance1 = kBase + test::kFakePeData + 0x100;
constexpr std::uintptr_t kNames = kBase + test::kFakePeData + 0x200;
constexpr std::uintptr_t kNodes = 0x4DE00000000; // InterfaceRegs live in the module's data in reality; anywhere works
constexpr std::uintptr_t kNode1 = kNodes;
constexpr std::uintptr_t kNode2 = kNodes + 0x18;

// `opcode disp32` at `at` so that it points at `target`.
void put_rip_instruction(test::FakeMemory& memory, std::uintptr_t at, std::array<std::uint8_t, 3> opcode,
                         std::uintptr_t target)
{
    memory.put_bytes(at, opcode.data(), opcode.size());
    memory.put<std::int32_t>(at + 3, static_cast<std::int32_t>(target - (at + 7)));
}

void put_node(test::FakeMemory& memory, std::uintptr_t node, std::uintptr_t create, std::uintptr_t name,
              std::uintptr_t next)
{
    memory.put<std::uintptr_t>(node, create);
    memory.put<std::uintptr_t>(node + 8, name);
    memory.put<std::uintptr_t>(node + 16, next);
}

test::FakeMemory make_module()
{
    test::FakeMemory memory;
    test::map_fake_pe(memory, kBase, {{"CreateInterface", 0x1000}});
    put_rip_instruction(memory, kCreateInterface, {0x4C, 0x8B, 0x0D}, kHead);
    put_rip_instruction(memory, kCreate1, {0x48, 0x8D, 0x05}, kInstance1);
    memory.put<std::uint8_t>(kCreate1 + 7, 0xC3);
    memory.put<std::uint8_t>(kCreate2, 0xE9); // jmp: not the lea/ret shape
    memory.put_string(kNames, "SchemaSystem_001");
    memory.put_string(kNames + 0x40, "Other001");

    memory.map(kNodes, 0x100);
    memory.put<std::uintptr_t>(kHead, kNode1);
    put_node(memory, kNode1, kCreate1, kNames, kNode2);
    put_node(memory, kNode2, kCreate2, kNames + 0x40, 0);
    return memory;
}
} // namespace

TEST_CASE("list_interfaces walks the InterfaceReg list and decodes lea/ret create functions")
{
    const test::FakeMemory memory = make_module();
    const auto interfaces = game::list_interfaces(memory, kBase);
    REQUIRE(interfaces.has_value());
    REQUIRE(interfaces->size() == 2);
    CHECK((*interfaces)[0].name == "SchemaSystem_001");
    CHECK((*interfaces)[0].instance == kInstance1);
    CHECK((*interfaces)[1].name == "Other001");
    CHECK((*interfaces)[1].instance == 0);

    CHECK(game::find_interface(*interfaces, "SchemaSystem_001") == kInstance1);
    CHECK_FALSE(game::find_interface(*interfaces, "Other001").has_value()); // known name, unknown instance
    CHECK_FALSE(game::find_interface(*interfaces, "SchemaSystem_00").has_value());
}

TEST_CASE("list_interfaces: no CreateInterface, or an unexpected first instruction")
{
    test::FakeMemory no_export;
    test::map_fake_pe(no_export, kBase, {{"Something", 0x1000}});
    CHECK_FALSE(game::list_interfaces(no_export, kBase).has_value());

    test::FakeMemory memory = make_module();
    memory.put<std::uint8_t>(kCreateInterface, 0x48); // not mov r9, [rip+x]
    CHECK_FALSE(game::list_interfaces(memory, kBase).has_value());
}

TEST_CASE("list_interfaces: a loop stops at the limit, a broken link keeps what was read")
{
    test::FakeMemory looped = make_module();
    put_node(looped, kNode2, kCreate2, kNames + 0x40, kNode1); // node2 -> node1 -> node2 -> ...
    const auto many = game::list_interfaces(looped, kBase);
    REQUIRE(many.has_value());
    CHECK(many->size() == config::kMaxInterfaces);

    test::FakeMemory broken = make_module();
    put_node(broken, kNode1, kCreate1, kNames, 0x5000000); // next points at nothing
    const auto one = game::list_interfaces(broken, kBase);
    REQUIRE(one.has_value());
    REQUIRE(one->size() == 1);
    CHECK((*one)[0].name == "SchemaSystem_001");
}
