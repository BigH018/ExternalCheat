#include "core/pe.h"

#include <cstddef>

namespace core::pe
{
namespace
{
// The PE32+ format (winnt.h names in the comments). Offsets are from the start of each structure.
constexpr std::uint16_t kDosMagic = 0x5A4D;           // IMAGE_DOS_SIGNATURE "MZ"
constexpr std::uintptr_t kDosNewHeader = 0x3C;        // IMAGE_DOS_HEADER::e_lfanew
constexpr std::uint32_t kNtSignature = 0x00004550;    // IMAGE_NT_SIGNATURE "PE\0\0"
constexpr std::uint16_t kMachineAmd64 = 0x8664;       // IMAGE_FILE_MACHINE_AMD64
constexpr std::uint16_t kOptionalMagic64 = 0x20B;     // IMAGE_NT_OPTIONAL_HDR64_MAGIC
constexpr std::uintptr_t kFileHeader = 4;             // IMAGE_NT_HEADERS64::FileHeader
constexpr std::uintptr_t kOptionalHeader = 24;        // IMAGE_NT_HEADERS64::OptionalHeader
constexpr std::uintptr_t kNumberOfSections = 2;       // in IMAGE_FILE_HEADER
constexpr std::uintptr_t kSizeOfOptionalHeader = 16;  // in IMAGE_FILE_HEADER
constexpr std::uintptr_t kSizeOfImage = 56;           // in IMAGE_OPTIONAL_HEADER64
constexpr std::uintptr_t kNumberOfRvaAndSizes = 108;  // in IMAGE_OPTIONAL_HEADER64
constexpr std::uintptr_t kDataDirectory = 112;        // in IMAGE_OPTIONAL_HEADER64; entry 0 = exports
constexpr std::size_t kSectionHeaderSize = 40;        // IMAGE_SECTION_HEADER
constexpr std::size_t kSectionNameSize = 8;
constexpr std::uintptr_t kSectionVirtualSize = 8;
constexpr std::uintptr_t kSectionVirtualAddress = 12;
constexpr std::uintptr_t kExportNumberOfFunctions = 20; // IMAGE_EXPORT_DIRECTORY
constexpr std::uintptr_t kExportNumberOfNames = 24;
constexpr std::uintptr_t kExportAddressOfFunctions = 28;
constexpr std::uintptr_t kExportAddressOfNames = 32;
constexpr std::uintptr_t kExportAddressOfNameOrdinals = 36;
constexpr std::uint16_t kMaxSections = 96;     // the PE loader's own limit
constexpr std::uint32_t kMaxExports = 0x10000; // sanity limit
} // namespace

std::optional<Headers> read_headers(const Memory& memory, std::uintptr_t base)
{
    if (memory.read<std::uint16_t>(base) != kDosMagic)
    {
        return std::nullopt;
    }
    const auto new_header = memory.read<std::uint32_t>(base + kDosNewHeader);
    if (!new_header)
    {
        return std::nullopt;
    }
    const std::uintptr_t nt = base + *new_header;
    const std::uintptr_t file = nt + kFileHeader;
    const std::uintptr_t optional = nt + kOptionalHeader;
    const auto signature = memory.read<std::uint32_t>(nt);
    const auto machine = memory.read<std::uint16_t>(file);
    const auto section_count = memory.read<std::uint16_t>(file + kNumberOfSections);
    const auto optional_size = memory.read<std::uint16_t>(file + kSizeOfOptionalHeader);
    const auto magic = memory.read<std::uint16_t>(optional);
    const auto image_size = memory.read<std::uint32_t>(optional + kSizeOfImage);
    const auto directories = memory.read<std::uint32_t>(optional + kNumberOfRvaAndSizes);
    if (signature != kNtSignature || machine != kMachineAmd64 || magic != kOptionalMagic64 || !section_count ||
        *section_count > kMaxSections || !optional_size || !image_size || !directories)
    {
        return std::nullopt;
    }

    Headers headers;
    headers.size_of_image = *image_size;
    if (*directories > 0)
    {
        const auto export_rva = memory.read<std::uint32_t>(optional + kDataDirectory);
        const auto export_size = memory.read<std::uint32_t>(optional + kDataDirectory + 4);
        if (!export_rva || !export_size)
        {
            return std::nullopt;
        }
        headers.export_rva = *export_rva;
        headers.export_size = *export_size;
    }

    const std::uintptr_t first_section = optional + *optional_size;
    for (std::uint16_t i = 0; i < *section_count; ++i)
    {
        const std::uintptr_t at = first_section + i * kSectionHeaderSize;
        char name[kSectionNameSize]{};
        const auto size = memory.read<std::uint32_t>(at + kSectionVirtualSize);
        const auto rva = memory.read<std::uint32_t>(at + kSectionVirtualAddress);
        if (!memory.read_bytes(at, name, sizeof(name)) || !size || !rva)
        {
            return std::nullopt;
        }
        // Not NUL-terminated when the name uses all 8 characters.
        const std::string_view name_view(name, sizeof(name));
        headers.sections.push_back(Section{std::string(name_view.substr(0, name_view.find('\0'))), *rva, *size});
    }
    return headers;
}

std::optional<Section> find_section(const Headers& headers, std::string_view name)
{
    for (const Section& section : headers.sections)
    {
        if (section.name == name)
        {
            return section;
        }
    }
    return std::nullopt;
}

std::optional<std::uintptr_t> find_export(const Memory& memory, std::uintptr_t base, std::string_view name)
{
    const auto headers = read_headers(memory, base);
    if (!headers || headers->export_rva == 0)
    {
        return std::nullopt;
    }
    const std::uintptr_t directory = base + headers->export_rva;
    const auto function_count = memory.read<std::uint32_t>(directory + kExportNumberOfFunctions);
    const auto name_count = memory.read<std::uint32_t>(directory + kExportNumberOfNames);
    const auto functions = memory.read<std::uint32_t>(directory + kExportAddressOfFunctions);
    const auto names = memory.read<std::uint32_t>(directory + kExportAddressOfNames);
    const auto ordinals = memory.read<std::uint32_t>(directory + kExportAddressOfNameOrdinals);
    if (!function_count || !name_count || !functions || !names || !ordinals || *name_count > kMaxExports ||
        *function_count > kMaxExports)
    {
        return std::nullopt;
    }

    std::vector<std::uint32_t> name_rvas(*name_count);
    if (*name_count == 0 ||
        !memory.read_bytes(base + *names, name_rvas.data(), name_rvas.size() * sizeof(std::uint32_t)))
    {
        return std::nullopt;
    }
    for (std::uint32_t i = 0; i < *name_count; ++i)
    {
        // name.size() + 1 characters are enough to tell whether this one is `name`.
        const auto export_name = read_string(memory, base + name_rvas[i], name.size() + 1);
        if (!export_name || *export_name != name)
        {
            continue;
        }
        const auto ordinal = memory.read<std::uint16_t>(base + *ordinals + i * sizeof(std::uint16_t));
        if (!ordinal || *ordinal >= *function_count)
        {
            return std::nullopt;
        }
        const auto function_rva = memory.read<std::uint32_t>(base + *functions + *ordinal * sizeof(std::uint32_t));
        if (!function_rva)
        {
            return std::nullopt;
        }
        // An RVA inside the export directory is a forwarder string ("OTHER.Function"), not code.
        if (*function_rva >= headers->export_rva && *function_rva < headers->export_rva + headers->export_size)
        {
            return std::nullopt;
        }
        return base + *function_rva;
    }
    return std::nullopt;
}
} // namespace core::pe
