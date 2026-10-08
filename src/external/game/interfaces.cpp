#include "game/interfaces.h"

#include <array>
#include <utility>

#include "config.h"
#include "core/pattern.h"
#include "core/pe.h"
#include "game/offsets.h"

namespace game
{
namespace
{
namespace layout = offsets::layout;

// The `opcode disp32` at `address`, if it starts with `opcode`.
template <std::size_t N>
std::optional<std::int32_t> read_disp32(const core::Memory& memory, std::uintptr_t address,
                                        const std::array<std::uint8_t, N>& opcode)
{
    std::array<std::uint8_t, N> bytes{};
    std::int32_t disp = 0;
    if (!memory.read_bytes(address, bytes.data(), N) || bytes != opcode || !memory.safe_read(address + N, disp))
    {
        return std::nullopt;
    }
    return disp;
}

// The instance a create function returns, if it is `lea rax, [rip + instance]; ret`. 0 otherwise.
std::uintptr_t decode_create_function(const core::Memory& memory, std::uintptr_t create)
{
    const auto disp = read_disp32(memory, create, layout::kCreateFnOpcode);
    if (!disp || memory.read<std::uint8_t>(create + layout::kCreateFnInstructionSize) != layout::kCreateFnReturn)
    {
        return 0;
    }
    return core::rip_relative(create, layout::kCreateFnInstructionSize, *disp);
}
} // namespace

std::optional<std::vector<Interface>> list_interfaces(const core::Memory& memory, std::uintptr_t module_base)
{
    const auto create_interface = core::pe::find_export(memory, module_base, "CreateInterface");
    if (!create_interface)
    {
        return std::nullopt;
    }
    // mov r9, [rip + s_pInterfaceRegs]
    const auto disp = read_disp32(memory, *create_interface, layout::kCreateInterfaceOpcode);
    if (!disp)
    {
        return std::nullopt;
    }
    const std::uintptr_t head_slot = core::rip_relative(*create_interface, layout::kCreateInterfaceInstructionSize,
                                                        *disp);
    auto node = memory.read<std::uintptr_t>(head_slot);
    if (!node)
    {
        return std::nullopt;
    }

    std::vector<Interface> interfaces;
    while (*node != 0 && interfaces.size() < config::kMaxInterfaces)
    {
        const auto create = memory.read<std::uintptr_t>(*node + layout::kInterfaceRegCreate);
        const auto name_ptr = memory.read<std::uintptr_t>(*node + layout::kInterfaceRegName);
        const auto next = memory.read<std::uintptr_t>(*node + layout::kInterfaceRegNext);
        if (!create || !name_ptr || !next)
        {
            break; // a broken link: keep what we have
        }
        auto name = core::read_string(memory, *name_ptr, config::kMaxNameLength);
        if (name)
        {
            interfaces.push_back(Interface{std::move(*name), decode_create_function(memory, *create)});
        }
        node = next;
    }
    return interfaces;
}

std::optional<std::uintptr_t> find_interface(const std::vector<Interface>& interfaces, std::string_view name)
{
    for (const Interface& entry : interfaces)
    {
        if (entry.name == name && entry.instance != 0)
        {
            return entry.instance;
        }
    }
    return std::nullopt;
}
} // namespace game
