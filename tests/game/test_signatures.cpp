#include <cstdint>
#include <cstring>

#include <doctest.h>

#include "core/pattern.h"
#include "core/pe.h"
#include "game/signatures.h"

namespace
{
constexpr std::uintptr_t kBase = 0x7FFD62930000;
const core::pe::Section kText{".text", 0x1000, 0x100};

// A module copy whose .text has `mov rax, [rip+x]; mov [r14+...], edi` at the given RVAs, each pointing at its target.
struct FakeModule
{
    core::RemoteCopy copy;

    FakeModule()
    {
        copy.base = kBase;
        copy.bytes.resize(0x2000);
    }

    void put_mov(std::uint32_t rva, std::uint32_t target_rva)
    {
        const std::uint8_t code[] = {0x48, 0x8B, 0x05, 0, 0, 0, 0, 0x41, 0x89, 0xBE};
        std::memcpy(copy.bytes.data() + rva, code, sizeof(code));
        const auto disp = static_cast<std::int32_t>(static_cast<std::int64_t>(target_rva) - (rva + 7));
        std::memcpy(copy.bytes.data() + rva + 3, &disp, sizeof(disp));
    }
};

constexpr game::offsets::Signature kSignature{"dwTest", "48 8B 05 ? ? ? ? 41 89 BE", 3, 7, 0, 0x5000};
} // namespace

TEST_CASE("resolve_signature: one match")
{
    FakeModule module;
    module.put_mov(0x1010, 0x5000);
    const game::SignatureResult result = game::resolve_signature(module.copy, kText, kSignature);
    CHECK(result.status == game::SignatureStatus::found);
    CHECK(result.hits == 1);
    CHECK(result.rva == 0x5000);

    game::offsets::Signature with_add = kSignature;
    with_add.add = 0xF8; // a field inside the global (dwLocalPlayerPawn = prediction + 0xF8)
    CHECK(game::resolve_signature(module.copy, kText, with_add).rva == 0x50F8);
}

TEST_CASE("resolve_signature: several matches must agree")
{
    FakeModule module;
    module.put_mov(0x1010, 0x5000);
    module.put_mov(0x1080, 0x5000);
    const game::SignatureResult same = game::resolve_signature(module.copy, kText, kSignature);
    CHECK(same.status == game::SignatureStatus::found);
    CHECK(same.hits == 2);
    CHECK(same.rva == 0x5000);

    module.put_mov(0x1080, 0x6000);
    CHECK(game::resolve_signature(module.copy, kText, kSignature).status == game::SignatureStatus::ambiguous);
}

TEST_CASE("resolve_signature: only inside the section; missing and bad patterns")
{
    FakeModule module;
    CHECK(game::resolve_signature(module.copy, kText, kSignature).status == game::SignatureStatus::not_found);

    module.put_mov(0x1800, 0x5000); // in the copy, but past the end of .text
    const game::SignatureResult outside = game::resolve_signature(module.copy, kText, kSignature);
    CHECK(outside.status == game::SignatureStatus::not_found);
    CHECK(outside.hits == 0);

    game::offsets::Signature bad = kSignature;
    bad.pattern = "48 8B 0";
    CHECK(game::resolve_signature(module.copy, kText, bad).status == game::SignatureStatus::bad_pattern);

    const core::pe::Section beyond{".text", 0x1F00, 0x1000}; // the section runs past the copy
    CHECK(game::resolve_signature(module.copy, beyond, kSignature).status == game::SignatureStatus::not_found);
}
