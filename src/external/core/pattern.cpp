#include "core/pattern.h"

#include <algorithm>
#include <cstring>

namespace core
{
namespace
{
std::optional<std::uint8_t> hex_digit(char c) noexcept
{
    if (c >= '0' && c <= '9')
    {
        return static_cast<std::uint8_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f')
    {
        return static_cast<std::uint8_t>(c - 'a' + 10);
    }
    if (c >= 'A' && c <= 'F')
    {
        return static_cast<std::uint8_t>(c - 'A' + 10);
    }
    return std::nullopt;
}

bool is_space(char c) noexcept
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}
} // namespace

std::optional<Pattern> Pattern::parse(std::string_view text)
{
    Pattern pattern;
    std::size_t i = 0;
    while (i < text.size())
    {
        if (is_space(text[i]))
        {
            ++i;
            continue;
        }
        std::size_t end = i;
        while (end < text.size() && !is_space(text[end]))
        {
            ++end;
        }
        const std::string_view token = text.substr(i, end - i);
        i = end;

        if (token == "?" || token == "??")
        {
            pattern.bytes_.push_back(kWildcard);
            continue;
        }
        const auto high = token.size() == 2 ? hex_digit(token[0]) : std::nullopt;
        const auto low = token.size() == 2 ? hex_digit(token[1]) : std::nullopt;
        if (!high || !low)
        {
            return std::nullopt;
        }
        pattern.bytes_.push_back(static_cast<std::int16_t>(*high * 16 + *low));
    }

    const auto anchor = std::find_if(pattern.bytes_.begin(), pattern.bytes_.end(),
                                     [](std::int16_t b) { return b != kWildcard; });
    if (anchor == pattern.bytes_.end())
    {
        return std::nullopt; // empty, or nothing but wildcards
    }
    pattern.anchor_ = static_cast<std::size_t>(anchor - pattern.bytes_.begin());
    return pattern;
}

bool Pattern::matches_at(std::span<const std::byte> data, std::size_t position) const noexcept
{
    if (position > data.size() || bytes_.size() > data.size() - position)
    {
        return false;
    }
    for (std::size_t i = 0; i < bytes_.size(); ++i)
    {
        if (bytes_[i] != kWildcard && static_cast<std::int16_t>(data[position + i]) != bytes_[i])
        {
            return false;
        }
    }
    return true;
}

std::vector<std::size_t> Pattern::find_all(std::span<const std::byte> data, std::size_t max_hits) const
{
    std::vector<std::size_t> hits;
    if (bytes_.empty() || data.size() < bytes_.size())
    {
        return hits;
    }
    const auto anchor_value = static_cast<int>(bytes_[anchor_]);
    const std::size_t last_start = data.size() - bytes_.size(); // the last position a match can start at
    std::size_t start = 0;
    while (start <= last_start && hits.size() < max_hits)
    {
        // The next place the anchor byte occurs, as a match start.
        const std::byte* from = data.data() + start + anchor_;
        const std::size_t remaining = last_start - start + 1;
        const void* found = std::memchr(from, anchor_value, remaining);
        if (found == nullptr)
        {
            break;
        }
        start = static_cast<std::size_t>(static_cast<const std::byte*>(found) - data.data()) - anchor_;
        if (matches_at(data, start))
        {
            hits.push_back(start);
        }
        ++start;
    }
    return hits;
}

RemoteCopy copy_remote(const Memory& memory, std::uintptr_t base, std::size_t size, std::size_t chunk_size)
{
    RemoteCopy copy;
    copy.base = base;
    copy.bytes.resize(size);
    chunk_size = std::max(chunk_size, config::kPageSize);

    for (std::size_t offset = 0; offset < size; offset += chunk_size)
    {
        const std::size_t chunk = std::min(chunk_size, size - offset);
        if (memory.read_bytes(base + offset, copy.bytes.data() + offset, chunk))
        {
            continue;
        }
        // Something in this chunk isn't readable: page by page, along the target's real page boundaries.
        std::size_t page = offset;
        while (page < offset + chunk)
        {
            const std::uintptr_t address = base + page;
            const std::size_t length = std::min(config::kPageSize - address % config::kPageSize, offset + chunk - page);
            if (!memory.read_bytes(address, copy.bytes.data() + page, length))
            {
                std::fill_n(copy.bytes.data() + page, length, std::byte{0});
                ++copy.unreadable_pages;
            }
            page += length;
        }
    }
    return copy;
}

std::optional<std::uintptr_t> rip_relative_target(const RemoteCopy& copy, std::uintptr_t instruction,
                                                  std::uint32_t disp_offset, std::uint32_t instruction_size) noexcept
{
    const std::span<const std::byte> disp_bytes = copy.view(instruction + disp_offset, sizeof(std::int32_t));
    if (disp_bytes.empty())
    {
        return std::nullopt;
    }
    std::int32_t disp = 0;
    std::memcpy(&disp, disp_bytes.data(), sizeof(disp));
    return rip_relative(instruction, instruction_size, disp);
}
} // namespace core
