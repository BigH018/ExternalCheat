#include <doctest.h>

#include "maths/projection.h"
#include "maths/skeleton.h"

namespace
{
constexpr maths::Vec2 kScreen{1920.0f, 1080.0f};
constexpr maths::ViewMatrix kCamera{{0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                     0.0f, 0.0f, 0.0f}};

// Every bone at `x` units in front of the camera, at heights by index so each projects somewhere different.
maths::Bones bones_at(float x)
{
    maths::Bones bones{};
    for (std::size_t i = 0; i < bones.size(); ++i)
    {
        bones[i] = maths::Vec3{x, 0.0f, static_cast<float>(i)};
    }
    return bones;
}
} // namespace

TEST_CASE("skeleton links: 17 lines, all within the bones that are read")
{
    CHECK(maths::kSkeletonLinks.size() == 17);
    for (const maths::BoneLink link : maths::kSkeletonLinks)
    {
        CHECK(link.from < maths::kBoneCount);
        CHECK(link.to < maths::kBoneCount);
        CHECK(link.from != link.to);
    }
    CHECK(maths::bone::kHead == 6);       // proven live in build 14189
    CHECK(maths::bone::kHeadCentre == 7); // proven live 2026-10-07 (screenshot)
    CHECK(maths::bone::kHeadCentre < maths::kBoneCount);
}

TEST_CASE("project_skeleton: every link in front of the camera, none behind it")
{
    const auto lines = maths::project_skeleton(kCamera, bones_at(200.0f), kScreen);
    REQUIRE(lines.size() == maths::kSkeletonLinks.size());
    // The first link is head -> neck: z 6 -> 5 at 200 units, so the head is higher on screen.
    CHECK(lines[0].from.y < lines[0].to.y);

    CHECK(maths::project_skeleton(kCamera, bones_at(-200.0f), kScreen).empty());
}

TEST_CASE("project_skeleton: a link with one end behind the camera is left out")
{
    maths::Bones bones = bones_at(200.0f);
    bones[maths::bone::kLeftHand] = maths::Vec3{-10.0f, 0.0f, 0.0f};
    const auto lines = maths::project_skeleton(kCamera, bones, kScreen);
    CHECK(lines.size() == maths::kSkeletonLinks.size() - 1); // only elbow -> hand uses the hand
}
