#pragma once

// Resolving the signatures in game/offsets.h (offsets::signatures) over a local copy of a module.
//
// PURE: no <Windows.h>.

#include <cstddef>
#include <cstdint>
#include <optional>

#include "core/pattern.h"
#include "core/pe.h"
#include "game/offsets.h"

namespace game
{
enum class SignatureStatus
{
    found,       // every match points at the same address
    not_found,   // no match: the code changed (update the pattern)
    ambiguous,   // matches that point at different addresses: the pattern is too loose
    bad_pattern, // the pattern text doesn't parse
};

struct SignatureResult
{
    SignatureStatus status = SignatureStatus::not_found;
    std::size_t hits = 0;
    std::uintptr_t rva = 0; // module RVA of the target (+ add), when found
};

// Scans `section` of the module copied in `module_copy` (base = module base) for `signature`.
[[nodiscard]] SignatureResult resolve_signature(const core::RemoteCopy& module_copy, const core::pe::Section& section,
                                                const offsets::Signature& signature);

[[nodiscard]] const char* to_string(SignatureStatus status) noexcept;
} // namespace game
