#include "game/schema_system.h"

#include <algorithm>
#include <cstring>
#include <span>
#include <unordered_set>
#include <utility>

#include "config.h"
#include "game/offsets.h"

namespace game
{
namespace
{
namespace layout = offsets::layout;

constexpr std::int32_t kMaxTypeScopes = 256; // build 14189 has 20

// A T stored at `offset` in `bytes` (which the caller has made big enough).
template <core::RemoteValue T>
T load(std::span<const std::byte> bytes, std::size_t offset) noexcept
{
    T value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(T));
    return value;
}

// The NUL-terminated string at `address` inside the copy (at most config::kMaxNameLength characters), if it's there.
std::optional<std::string_view> string_in_copy(const core::RemoteCopy& copy, std::uintptr_t address) noexcept
{
    if (!copy.contains(address, 1))
    {
        return std::nullopt;
    }
    const std::size_t available = copy.bytes.size() - (address - copy.base);
    const std::span<const std::byte> bytes = copy.view(address, std::min(available, config::kMaxNameLength + 1));
    const auto* text = reinterpret_cast<const char*>(bytes.data());
    const std::size_t length = std::string_view(text, bytes.size()).find('\0');
    if (length == std::string_view::npos || length == 0)
    {
        return std::nullopt;
    }
    return std::string_view(text, length);
}
} // namespace

std::optional<std::uintptr_t> find_type_scope(const core::Memory& memory, std::uintptr_t schema_system,
                                              std::string_view module_name)
{
    const auto count = memory.read<std::int32_t>(schema_system + layout::kSchemaSystemScopeCount);
    const auto data = memory.read<std::uintptr_t>(schema_system + layout::kSchemaSystemScopeData);
    if (!count || !data || *count <= 0 || *count > kMaxTypeScopes)
    {
        return std::nullopt;
    }
    std::vector<std::uintptr_t> scopes(static_cast<std::size_t>(*count));
    if (!memory.read_bytes(*data, scopes.data(), scopes.size() * sizeof(std::uintptr_t)))
    {
        return std::nullopt;
    }
    for (const std::uintptr_t scope : scopes)
    {
        const auto name = core::read_string(memory, scope + layout::kTypeScopeName, layout::kTypeScopeNameSize);
        if (name && *name == module_name)
        {
            return scope;
        }
    }
    return std::nullopt;
}

std::unordered_map<std::string, std::uintptr_t> index_classes(const core::RemoteCopy& module_copy,
                                                              std::string_view schema_module)
{
    std::unordered_map<std::string, std::uintptr_t> classes;
    std::unordered_set<std::string> ambiguous;
    const std::span<const std::byte> bytes(module_copy.bytes);
    constexpr std::size_t kStep = sizeof(std::uintptr_t);
    if (bytes.size() < layout::kClassInfoReadSize)
    {
        return classes;
    }

    // Module bases are page aligned, so offset % 8 == address % 8.
    for (std::size_t offset = 0; offset + layout::kClassInfoReadSize <= bytes.size(); offset += kStep)
    {
        const std::uintptr_t address = module_copy.base + offset;
        if (load<std::uintptr_t>(bytes, offset + layout::kClassInfoSelf) != address)
        {
            continue; // not a self-pointer: not a class info (this rules out nearly everything, cheaply)
        }
        const auto module = string_in_copy(module_copy, load<std::uintptr_t>(bytes, offset + layout::kClassInfoModule));
        const auto name = string_in_copy(module_copy, load<std::uintptr_t>(bytes, offset + layout::kClassInfoName));
        if (!module || !name || *module != schema_module)
        {
            continue;
        }
        std::string key(*name);
        if (ambiguous.contains(key))
        {
            continue;
        }
        if (classes.contains(key))
        {
            classes.erase(key);
            ambiguous.insert(std::move(key));
            continue;
        }
        classes.emplace(std::move(key), address);
    }
    return classes;
}

std::optional<std::uint32_t> SchemaClass::offset_of(std::string_view name) const
{
    for (const SchemaField& field : fields)
    {
        if (field.name == name)
        {
            return field.offset;
        }
    }
    return std::nullopt;
}

std::optional<SchemaClass> read_class(const core::Memory& memory, std::uintptr_t class_info)
{
    std::byte info[layout::kClassInfoReadSize]{};
    if (!memory.read_bytes(class_info, info, sizeof(info)))
    {
        return std::nullopt;
    }
    const std::span<const std::byte> bytes(info);
    const auto field_count = load<std::int16_t>(bytes, layout::kClassInfoFieldCount);
    const auto fields = load<std::uintptr_t>(bytes, layout::kClassInfoFields);
    if (load<std::uintptr_t>(bytes, layout::kClassInfoSelf) != class_info || field_count < 0 ||
        static_cast<std::size_t>(field_count) > config::kMaxSchemaFields)
    {
        return std::nullopt;
    }

    SchemaClass result;
    result.type_scope = load<std::uintptr_t>(bytes, layout::kClassInfoTypeScope);
    result.size = load<std::int32_t>(bytes, layout::kClassInfoSize);
    if (field_count == 0)
    {
        return result;
    }

    std::vector<std::byte> entries(static_cast<std::size_t>(field_count) * layout::kFieldStride);
    if (!memory.read_bytes(fields, entries.data(), entries.size()))
    {
        return std::nullopt;
    }
    result.fields.reserve(static_cast<std::size_t>(field_count));
    for (std::size_t i = 0; i < static_cast<std::size_t>(field_count); ++i)
    {
        const std::size_t entry = i * layout::kFieldStride;
        auto name = core::read_string(memory, load<std::uintptr_t>(entries, entry + layout::kFieldName),
                                      config::kMaxNameLength);
        if (!name)
        {
            return std::nullopt;
        }
        const auto offset = load<std::int32_t>(entries, entry + layout::kFieldOffset);
        result.fields.push_back(SchemaField{std::move(*name), static_cast<std::uint32_t>(offset)});
    }
    return result;
}
} // namespace game
