#pragma once

// The game's own schema system, read from outside: which fields a class has and at which offsets. Used at startup to
// prove the values in game/schema.h against the running game (app/diagnostics).
//
// How the pieces fit (build 14189, layouts in game/offsets.h, offsets::layout):
//   - SchemaSystem_001 (schemasystem.dll, found with game/interfaces) keeps one type scope per module that declares
//     schema classes; the client's is called "client.dll".
//   - Each class's info (SchemaClassInfoData: name, size, field array, ...) is static data inside the declaring
//     module, and its first member points to itself. So one pass over a copy of client.dll finds every class without
//     having to walk the scope's internal hash table. Each info also points back to its type scope, which ties it to
//     what SchemaSystem_001 says.
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/memory.h"
#include "core/pattern.h"

namespace game
{
// The type scope SchemaSystem_001 keeps for `module_name` ("client.dll"), or nullopt.
[[nodiscard]] std::optional<std::uintptr_t> find_type_scope(const core::Memory& memory, std::uintptr_t schema_system,
                                                            std::string_view module_name);

// Every class info stored in `module_copy` whose module name is `schema_module` ("client"), by class name. A name
// that occurs twice is left out (it would be ambiguous).
[[nodiscard]] std::unordered_map<std::string, std::uintptr_t> index_classes(const core::RemoteCopy& module_copy,
                                                                            std::string_view schema_module);

struct SchemaField
{
    std::string name;
    std::uint32_t offset = 0;
};

struct SchemaClass
{
    std::uintptr_t type_scope = 0; // the type scope it belongs to
    std::int32_t size = 0;         // sizeof the class
    std::vector<SchemaField> fields; // the fields this class declares (not the inherited ones)

    // The offset of the field called `name`, if the class declares it.
    [[nodiscard]] std::optional<std::uint32_t> offset_of(std::string_view name) const;
};

// The class whose info is at `class_info`. nullopt if the info or its field array can't be read.
[[nodiscard]] std::optional<SchemaClass> read_class(const core::Memory& memory, std::uintptr_t class_info);
} // namespace game
