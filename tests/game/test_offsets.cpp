#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <doctest.h>

#include "core/pattern.h"
#include "game/offsets.h"
#include "game/schema.h"

// Consistency checks on the offset tables themselves (no game needed). The values are proven in-game by the
// startup diagnostic, not here.

namespace
{
std::vector<std::string_view> tokens(std::string_view text)
{
    std::vector<std::string_view> out;
    std::size_t i = 0;
    while (i < text.size())
    {
        const std::size_t end = std::min(text.find(' ', i), text.size());
        if (end > i)
        {
            out.push_back(text.substr(i, end - i));
        }
        i = end + 1;
    }
    return out;
}
} // namespace

TEST_CASE("every signature parses and wildcards its whole disp32")
{
    for (const game::offsets::Signature& signature : game::offsets::signatures::kClient)
    {
        CAPTURE(signature.name);
        const auto pattern = core::Pattern::parse(signature.pattern);
        REQUIRE(pattern.has_value());
        CHECK(signature.disp_offset + 4 <= signature.instruction_size);
        CHECK(signature.instruction_size <= pattern->size());
        // The displacement changes with every build, so it must never be part of what's matched.
        const auto parts = tokens(signature.pattern);
        for (std::size_t i = signature.disp_offset; i < signature.disp_offset + 4; ++i)
        {
            CHECK(parts[i] == "?");
        }
    }
}

TEST_CASE("signature and schema tables have no duplicates")
{
    std::set<std::string_view> signature_names;
    for (const game::offsets::Signature& signature : game::offsets::signatures::kClient)
    {
        CHECK(signature_names.insert(signature.name).second);
    }

    std::set<std::pair<std::string_view, std::string_view>> fields;
    for (const game::schema::Field& field : game::schema::kFields)
    {
        CAPTURE(field.field_name);
        CHECK(fields.insert({field.class_name, field.field_name}).second);
    }
}

TEST_CASE("derived values agree with the dump")
{
    using namespace game::offsets;
    CHECK(client::dwLocalPlayerPawn - client::dwPrediction == signatures::kPredictionLocalPawn);
    CHECK(client::dwGameEntitySystem == client::dwEntityList);
    CHECK(inputsystem::dwInputSystem == interfaces::InputSystemVersion001);
    // The weapon item definition index chain: 0x149A combined in build 14189 (proven live 2026-10-06: an AK-47 reads
    // 7 there). CLAUDE.md used to say 0x14FA, which was wrong.
    using namespace game::schema;
    CHECK(C_EconEntity::m_AttributeManager + C_AttributeContainer::m_Item + C_EconItemView::m_iItemDefinitionIndex ==
          0x149A);
}
