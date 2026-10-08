#include <cstddef>
#include <cstdint>
#include <set>
#include <string>

#include <doctest.h>

#include "input/keys.h"

TEST_CASE("input keys: the table is sorted, unique and has no Escape or generic modifiers")
{
    const auto keys = input::bindable_keys();
    REQUIRE(keys.size() > 100);
    std::set<std::string> names;
    for (std::size_t i = 0; i < keys.size(); ++i)
    {
        if (i > 0)
        {
            CHECK(keys[i - 1].vk < keys[i].vk); // sorted, so no duplicate codes
        }
        CHECK(keys[i].vk < 256);                // fits a KeySet
        CHECK(names.insert(std::string(keys[i].name)).second);
        CHECK(input::is_bindable(keys[i].vk));
    }
    CHECK_FALSE(input::is_bindable(input::kVkEscape)); // reserved: "clear this bind"
    CHECK_FALSE(input::is_bindable(0x10));             // generic Shift: use LSHIFT / RSHIFT
    CHECK_FALSE(input::is_bindable(0x11));
    CHECK_FALSE(input::is_bindable(0x12));
    CHECK_FALSE(input::is_bindable(input::kUnbound));
    CHECK_FALSE(input::is_bindable(0x03)); // VK_CANCEL
}

TEST_CASE("input keys: names")
{
    CHECK(input::key_name(input::kUnbound) == "Unbound");
    CHECK(input::key_name(0x01) == "Mouse 1");
    CHECK(input::key_name(0x05) == "Mouse 4");
    CHECK(input::key_name(input::kVkInsert) == "INSERT");
    CHECK(input::key_name(0x41) == "A");
    CHECK(input::key_name(0x35) == "5");
    CHECK(input::key_name(0x61) == "NUM1");
    CHECK(input::key_name(0x74) == "F5");
    CHECK(input::key_name(0xA0) == "LSHIFT");
    CHECK(input::key_name(0xFF) == "VK 0xFF");
}

TEST_CASE("input keys: every name maps back to its code, ignoring case")
{
    for (const input::KeyInfo& key : input::bindable_keys())
    {
        CHECK(input::vk_from_name(key.name) == key.vk);
    }
    CHECK(input::vk_from_name("insert") == input::kVkInsert);
    CHECK(input::vk_from_name("mouse 4") == input::kVkMouse4);
    CHECK_FALSE(input::vk_from_name("Unbound").has_value());
    CHECK_FALSE(input::vk_from_name("").has_value());
    CHECK_FALSE(input::vk_from_name("NOPE").has_value());
}

TEST_CASE("input keys: mouse buttons, modifiers and what can be a hotkey")
{
    for (const std::uint32_t vk : {0x01u, 0x02u, 0x04u, 0x05u, 0x06u})
    {
        CHECK(input::is_mouse_button(vk));
        CHECK_FALSE(input::can_be_hotkey(vk)); // RegisterHotKey takes keyboard keys only
    }
    CHECK_FALSE(input::is_mouse_button(0x03));
    CHECK_FALSE(input::is_mouse_button(input::kVkInsert));
    for (const std::uint32_t vk : {0xA0u, 0xA1u, 0xA2u, 0xA3u, 0xA4u, 0xA5u})
    {
        CHECK(input::is_modifier(vk));
        CHECK_FALSE(input::can_be_hotkey(vk));
    }
    CHECK(input::can_be_hotkey(input::kVkInsert));
    CHECK(input::can_be_hotkey(0x74)); // F5
    CHECK(input::can_be_hotkey(0x41)); // A
    CHECK_FALSE(input::can_be_hotkey(input::kUnbound));
    CHECK_FALSE(input::can_be_hotkey(input::kVkEscape));
}
