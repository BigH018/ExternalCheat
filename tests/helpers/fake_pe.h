#pragma once

// A tiny PE32+ module mapped into a FakeMemory, for testing core/pe and game/interfaces without the game.
//
// Layout (RVAs): headers at 0, ".text" at 0x1000 (0x1000 bytes), ".rdata" at 0x2000 (0x2000 bytes) holding the export
// directory at 0x2000-0x2400, and free space for test data up to kImageSize.

#include <cstdint>
#include <string>
#include <vector>

#include "helpers/fake_memory.h"

namespace test
{
struct FakeExport
{
    std::string name;
    std::uint32_t rva = 0;
};

inline constexpr std::uint32_t kFakePeSize = 0x8000;
inline constexpr std::uint32_t kFakePeText = 0x1000;
inline constexpr std::uint32_t kFakePeTextSize = 0x1000;
inline constexpr std::uint32_t kFakePeExports = 0x2000;
inline constexpr std::uint32_t kFakePeExportsSize = 0x400;
inline constexpr std::uint32_t kFakePeData = 0x4000; // free for test data

// Maps kFakePeSize bytes at `base` and writes the headers, two sections and an export table with `exports`
// (sorted by the caller if it cares; find_export doesn't).
inline void map_fake_pe(FakeMemory& memory, std::uintptr_t base, const std::vector<FakeExport>& exports)
{
    memory.map(base, kFakePeSize);
    constexpr std::uintptr_t kNt = 0x80;
    constexpr std::uintptr_t kOptional = kNt + 24;
    constexpr std::uint16_t kOptionalSize = 0xF0;

    memory.put<std::uint16_t>(base, 0x5A4D);            // "MZ"
    memory.put<std::uint32_t>(base + 0x3C, kNt);         // e_lfanew
    memory.put<std::uint32_t>(base + kNt, 0x00004550);   // "PE\0\0"
    memory.put<std::uint16_t>(base + kNt + 4, 0x8664);   // Machine
    memory.put<std::uint16_t>(base + kNt + 6, 2);        // NumberOfSections
    memory.put<std::uint16_t>(base + kNt + 20, kOptionalSize);
    memory.put<std::uint16_t>(base + kOptional, 0x20B);  // PE32+
    memory.put<std::uint32_t>(base + kOptional + 56, kFakePeSize);
    memory.put<std::uint32_t>(base + kOptional + 108, 16); // NumberOfRvaAndSizes
    memory.put<std::uint32_t>(base + kOptional + 112, kFakePeExports);
    memory.put<std::uint32_t>(base + kOptional + 116, kFakePeExportsSize);

    const std::uintptr_t sections = base + kOptional + kOptionalSize;
    memory.put_string(sections, ".text");
    memory.put<std::uint32_t>(sections + 8, kFakePeTextSize);
    memory.put<std::uint32_t>(sections + 12, kFakePeText);
    memory.put_bytes(sections + 40, ".rdata\0\0", 8);
    memory.put<std::uint32_t>(sections + 40 + 8, 0x2000);
    memory.put<std::uint32_t>(sections + 40 + 12, 0x2000);

    // Export directory, then its three arrays, then the name strings.
    const std::uintptr_t directory = base + kFakePeExports;
    constexpr std::uint32_t kFunctions = kFakePeExports + 0x40;
    constexpr std::uint32_t kNames = kFakePeExports + 0x80;
    constexpr std::uint32_t kOrdinals = kFakePeExports + 0xC0;
    std::uint32_t next_string = kFakePeExports + 0x100;
    const auto count = static_cast<std::uint32_t>(exports.size());
    memory.put<std::uint32_t>(directory + 20, count); // NumberOfFunctions
    memory.put<std::uint32_t>(directory + 24, count); // NumberOfNames
    memory.put<std::uint32_t>(directory + 28, kFunctions);
    memory.put<std::uint32_t>(directory + 32, kNames);
    memory.put<std::uint32_t>(directory + 36, kOrdinals);
    for (std::uint32_t i = 0; i < count; ++i)
    {
        memory.put<std::uint32_t>(base + kFunctions + 4 * i, exports[i].rva);
        memory.put<std::uint32_t>(base + kNames + 4 * i, next_string);
        memory.put<std::uint16_t>(base + kOrdinals + 2 * i, static_cast<std::uint16_t>(i));
        memory.put_string(base + next_string, exports[i].name);
        next_string += static_cast<std::uint32_t>(exports[i].name.size()) + 1;
    }
}
} // namespace test
