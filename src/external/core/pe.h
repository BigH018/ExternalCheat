#pragma once

// A loaded module's PE headers and export table, read from another process through core::Memory. This is the image
// as Windows mapped it (RVAs are offsets from the module base), not the file on disk.
//
// PURE: no <Windows.h>. The few PE format offsets this needs are spelled out in pe.cpp.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/memory.h"

namespace core::pe
{
struct Section
{
    std::string name;      // ".text"
    std::uint32_t rva = 0; // VirtualAddress
    std::uint32_t size = 0; // VirtualSize
};

struct Headers
{
    std::uint32_t size_of_image = 0;
    std::uint32_t export_rva = 0; // 0 = no export table
    std::uint32_t export_size = 0;
    std::vector<Section> sections;
};

// The headers of the x64 module loaded at `base`. nullopt if they can't be read or aren't a PE32+ image.
[[nodiscard]] std::optional<Headers> read_headers(const Memory& memory, std::uintptr_t base);

// The section called `name`, if the module has one.
[[nodiscard]] std::optional<Section> find_section(const Headers& headers, std::string_view name);

// Address of the function exported as `name` by the module at `base`. nullopt if there is no such export, the export
// is forwarded to another DLL, or the export table can't be read.
[[nodiscard]] std::optional<std::uintptr_t> find_export(const Memory& memory, std::uintptr_t base,
                                                        std::string_view name);
} // namespace core::pe
