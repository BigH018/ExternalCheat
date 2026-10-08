#include "input/keys.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <format>
#include <vector>

namespace input
{
namespace
{
// Every bindable key with a name of its own. Letters, digits, numpad digits and F-keys are added in build_table().
constexpr std::array<KeyInfo, 42> kNamed = {{
    {0x01, "Mouse 1"},   {0x02, "Mouse 2"},  {0x04, "Mouse 3"},  {0x05, "Mouse 4"},   {0x06, "Mouse 5"},
    {0x08, "BACKSPACE"}, {0x09, "TAB"},      {0x0D, "ENTER"},    {0x14, "CAPSLOCK"},  {0x20, "SPACE"},
    {0x21, "PAGEUP"},    {0x22, "PAGEDOWN"}, {0x23, "END"},      {0x24, "HOME"},      {0x25, "LEFT"},
    {0x26, "UP"},        {0x27, "RIGHT"},    {0x28, "DOWN"},     {0x2D, "INSERT"},    {0x2E, "DELETE"},
    {0x6A, "NUM*"},      {0x6B, "NUM+"},     {0x6D, "NUM-"},     {0x6E, "NUM."},      {0x6F, "NUM/"},
    {0xA0, "LSHIFT"},    {0xA1, "RSHIFT"},   {0xA2, "LCTRL"},    {0xA3, "RCTRL"},     {0xA4, "LALT"},
    {0xA5, "RALT"},      {0xBA, ";"},        {0xBB, "="},        {0xBC, ","},         {0xBD, "-"},
    {0xBE, "."},         {0xBF, "/"},        {0xC0, "`"},        {0xDB, "["},         {0xDC, "\\"},
    {0xDD, "]"},         {0xDE, "'"},
}};

constexpr std::array<std::string_view, 10> kDigits = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
constexpr std::array<std::string_view, 26> kLetters = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
                                                       "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
constexpr std::array<std::string_view, 10> kNumpad = {"NUM0", "NUM1", "NUM2", "NUM3", "NUM4",
                                                      "NUM5", "NUM6", "NUM7", "NUM8", "NUM9"};
constexpr std::array<std::string_view, 24> kFKeys = {"F1",  "F2",  "F3",  "F4",  "F5",  "F6",  "F7",  "F8",
                                                     "F9",  "F10", "F11", "F12", "F13", "F14", "F15", "F16",
                                                     "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24"};

std::vector<KeyInfo> build_table()
{
    std::vector<KeyInfo> table(kNamed.begin(), kNamed.end());
    for (std::size_t i = 0; i < kDigits.size(); ++i)
    {
        table.push_back({0x30 + static_cast<std::uint32_t>(i), kDigits[i]});
    }
    for (std::size_t i = 0; i < kNumpad.size(); ++i)
    {
        table.push_back({0x60 + static_cast<std::uint32_t>(i), kNumpad[i]});
    }
    for (std::size_t i = 0; i < kLetters.size(); ++i)
    {
        table.push_back({0x41 + static_cast<std::uint32_t>(i), kLetters[i]});
    }
    for (std::size_t i = 0; i < kFKeys.size(); ++i)
    {
        table.push_back({0x70 + static_cast<std::uint32_t>(i), kFKeys[i]});
    }
    std::sort(table.begin(), table.end(), [](const KeyInfo& a, const KeyInfo& b) { return a.vk < b.vk; });
    return table;
}

const std::vector<KeyInfo>& table()
{
    static const std::vector<KeyInfo> keys = build_table();
    return keys;
}

bool equal_ignoring_case(std::string_view a, std::string_view b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::toupper(static_cast<unsigned char>(x)) == std::toupper(static_cast<unsigned char>(y));
           });
}
} // namespace

std::span<const KeyInfo> bindable_keys()
{
    return table();
}

bool is_bindable(std::uint32_t vk)
{
    const std::vector<KeyInfo>& keys = table();
    const auto it = std::lower_bound(keys.begin(), keys.end(), vk,
                                     [](const KeyInfo& key, std::uint32_t code) { return key.vk < code; });
    return it != keys.end() && it->vk == vk;
}

std::string key_name(std::uint32_t vk)
{
    if (vk == kUnbound)
    {
        return std::string(kUnboundName);
    }
    for (const KeyInfo& key : table())
    {
        if (key.vk == vk)
        {
            return std::string(key.name);
        }
    }
    return std::format("VK 0x{:02X}", vk);
}

std::optional<std::uint32_t> vk_from_name(std::string_view name)
{
    for (const KeyInfo& key : table())
    {
        if (equal_ignoring_case(key.name, name))
        {
            return key.vk;
        }
    }
    return std::nullopt; // includes "Unbound" and ""
}
} // namespace input
