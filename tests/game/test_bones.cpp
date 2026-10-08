#include <cstdint>
#include <limits>

#include <doctest.h>

#include "game/bones.h"
#include "game/offsets.h"
#include "game/schema.h"
#include "helpers/fake_memory.h"

namespace
{
namespace layout = game::offsets::layout;

constexpr std::uintptr_t kNode = 0x4DF57000000;
constexpr std::uintptr_t kBones = 0x4DF9CCDDA00;
constexpr maths::Vec3 kOrigin{278.5f, -874.9f, -163.2f};

// A scene node whose model state points at a bone array, each bone `i` at origin + (0, 0, i * 3).
test::FakeMemory make_bones()
{
    test::FakeMemory memory;
    memory.map(kNode, 0x400);
    memory.put<std::uintptr_t>(kNode + game::schema::CSkeletonInstance::m_modelState + layout::kModelStateBones,
                               kBones);
    memory.map(kBones, 32 * layout::kBoneStride);
    for (std::size_t i = 0; i < 32; ++i)
    {
        const std::uintptr_t bone = kBones + i * layout::kBoneStride;
        memory.put<maths::Vec3>(bone, kOrigin + maths::Vec3{0.0f, 0.0f, static_cast<float>(i) * 3.0f});
        memory.put<float>(bone + 12, 1.0f); // scale; the rotation quaternion follows
    }
    return memory;
}
} // namespace

TEST_CASE("read_bones: model state + 0x80 -> 32-byte bones, position first")
{
    const test::FakeMemory memory = make_bones();
    const auto bones = game::read_bones(memory, kNode, kOrigin);
    REQUIRE(bones.has_value());
    CHECK((*bones)[0] == kOrigin);
    CHECK((*bones)[maths::bone::kHead].z == doctest::Approx(kOrigin.z + 18.0f));
    CHECK((*bones)[maths::kBoneCount - 1].z == doctest::Approx(kOrigin.z + 66.0f));
    CHECK(memory.read_count() == 2); // the pointer, then the whole array
}

TEST_CASE("read_bones: garbage is rejected")
{
    SUBCASE("a bone far from the feet (a stale array)")
    {
        test::FakeMemory memory = make_bones();
        memory.put<maths::Vec3>(kBones + 3 * layout::kBoneStride, kOrigin + maths::Vec3{0.0f, 500.0f, 0.0f});
        CHECK_FALSE(game::read_bones(memory, kNode, kOrigin).has_value());
    }
    SUBCASE("a non-finite bone")
    {
        test::FakeMemory memory = make_bones();
        memory.put<float>(kBones + 5 * layout::kBoneStride, std::numeric_limits<float>::quiet_NaN());
        CHECK_FALSE(game::read_bones(memory, kNode, kOrigin).has_value());
    }
    SUBCASE("no array")
    {
        test::FakeMemory memory = make_bones();
        memory.put<std::uintptr_t>(kNode + game::schema::CSkeletonInstance::m_modelState + layout::kModelStateBones,
                                   0);
        CHECK_FALSE(game::read_bones(memory, kNode, kOrigin).has_value());
    }
    SUBCASE("an array that is too short")
    {
        test::FakeMemory memory;
        memory.map(kNode, 0x400);
        memory.put<std::uintptr_t>(kNode + game::schema::CSkeletonInstance::m_modelState + layout::kModelStateBones,
                                   kBones);
        memory.map(kBones, 4 * layout::kBoneStride);
        CHECK_FALSE(game::read_bones(memory, kNode, kOrigin).has_value());
    }
}
