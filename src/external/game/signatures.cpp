#include "game/signatures.h"

#include <span>

#include "config.h"

namespace game
{
SignatureResult resolve_signature(const core::RemoteCopy& module_copy, const core::pe::Section& section,
                                  const offsets::Signature& signature)
{
    SignatureResult result;
    const auto pattern = core::Pattern::parse(signature.pattern);
    if (!pattern)
    {
        result.status = SignatureStatus::bad_pattern;
        return result;
    }
    const std::uintptr_t section_start = module_copy.base + section.rva;
    const std::span<const std::byte> code = module_copy.view(section_start, section.size);
    if (code.empty())
    {
        return result; // the section isn't in the copy: not found
    }

    const std::vector<std::size_t> hits = pattern->find_all(code, config::kMaxSignatureHits);
    result.hits = hits.size();
    std::optional<std::uintptr_t> target;
    for (const std::size_t hit : hits)
    {
        const auto this_target = core::rip_relative_target(module_copy, section_start + hit, signature.disp_offset,
                                                           signature.instruction_size);
        if (!this_target || (target && *target != *this_target))
        {
            result.status = SignatureStatus::ambiguous;
            return result;
        }
        target = this_target;
    }
    if (!target)
    {
        return result;
    }
    result.status = SignatureStatus::found;
    result.rva = *target - module_copy.base + signature.add;
    return result;
}

const char* to_string(SignatureStatus status) noexcept
{
    switch (status)
    {
    case SignatureStatus::found:
        return "found";
    case SignatureStatus::not_found:
        return "not found";
    case SignatureStatus::ambiguous:
        return "ambiguous";
    case SignatureStatus::bad_pattern:
        return "bad pattern";
    }
    return "?";
}
} // namespace game
