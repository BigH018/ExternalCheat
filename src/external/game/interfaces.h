#pragma once

// CreateInterface from outside the process. We can't call it, so we do what it does: every Source 2 module keeps a
// linked list of InterfaceReg { create function, name, next }, and CreateInterface(name) walks it and calls the
// matching create function. We read the list's head from CreateInterface's first instruction, walk it with RPM, and
// decode each create function (`lea rax, [rip + instance]; ret`) instead of calling it.
//
// PURE: no <Windows.h>. Layout constants in game/offsets.h (offsets::layout).

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/memory.h"

namespace game
{
struct Interface
{
    std::string name;            // "SchemaSystem_001"
    std::uintptr_t instance = 0; // what CreateInterface(name) returns; 0 if the create function isn't a lea/ret
};

// Every interface the module at `module_base` registers, in list order. nullopt if CreateInterface isn't exported or
// doesn't start with the expected instruction. The walk stops at config::kMaxInterfaces entries.
[[nodiscard]] std::optional<std::vector<Interface>> list_interfaces(const core::Memory& memory,
                                                                    std::uintptr_t module_base);

// The instance of the interface called exactly `name` in `interfaces`, or nullopt (also when its instance is 0).
[[nodiscard]] std::optional<std::uintptr_t> find_interface(const std::vector<Interface>& interfaces,
                                                           std::string_view name);
} // namespace game
