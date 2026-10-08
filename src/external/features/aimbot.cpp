#include "features/aimbot.h"

#include <algorithm>
#include <array>
#include <cstdint>

#include "features/targeting.h"
#include "maths/skeleton.h"

namespace features
{
namespace
{
// Without bones, the body point sits this far up the eye height (about the chest).
constexpr float kBodyHeightFraction = 0.7f;
// How far along the view the FOV circle's edge point is placed (any distance works; this keeps it well clear of the
// camera plane).
constexpr float kFovProbeDistance = 1000.0f;

constexpr std::array<std::uint8_t, 5> kNearestBones{maths::bone::kHeadCentre, maths::bone::kNeck,
                                                    maths::bone::kSpine3, maths::bone::kSpine1, maths::bone::kPelvis};

maths::Vec3 body_point(const game::PlayerSnapshot& target)
{
    if (target.bones)
    {
        return (*target.bones)[maths::bone::kSpine3];
    }
    return target.origin + target.view_offset * kBodyHeightFraction;
}

// Lower is better for every priority.
float priority_key(const AimCandidate& candidate, settings::AimPriority priority)
{
    switch (priority)
    {
    case settings::AimPriority::distance: return candidate.metres;
    case settings::AimPriority::lowest_health: return static_cast<float>(candidate.player->health);
    case settings::AimPriority::crosshair: break;
    }
    return candidate.fov_distance;
}
} // namespace

maths::Vec3 aim_point(const game::PlayerSnapshot& target, settings::AimTarget where, maths::Vec3 eye,
                      maths::Angles view)
{
    switch (where)
    {
    case settings::AimTarget::body: return body_point(target);
    case settings::AimTarget::nearest:
    {
        if (!target.bones)
        {
            const maths::Vec3 head = target.head_position();
            const maths::Vec3 body = body_point(target);
            return maths::angular_distance(view, maths::calc_aim_angles(eye, head)) <=
                           maths::angular_distance(view, maths::calc_aim_angles(eye, body))
                       ? head
                       : body;
        }
        const maths::Bones& bones = *target.bones;
        const auto closest = std::ranges::min(kNearestBones, {}, [&](std::uint8_t bone) {
            return maths::angular_distance(view, maths::calc_aim_angles(eye, bones[bone]));
        });
        return bones[closest];
    }
    case settings::AimTarget::head: break;
    }
    return target.head_position();
}

std::vector<AimCandidate> find_candidates(const game::GameSnapshot& game, const settings::AimbotSettings& settings,
                                          settings::TeamMode team_mode)
{
    std::vector<AimCandidate> candidates;
    const game::PlayerSnapshot* local = game.local();
    const auto& view = game.local_state.view_angles;
    if (!game.in_match || local == nullptr || !local->alive || !view)
    {
        return candidates;
    }
    const maths::Vec3 eye = local->eye_position();
    for (const game::PlayerSnapshot& player : game.players)
    {
        if (!is_live_target(player) || (settings.team_check && !is_enemy(player, *local, team_mode)))
        {
            continue;
        }
        const float metres = distance_metres(*local, player);
        if (!within_distance(metres, settings.max_distance) || (settings.visible_only && !is_visible_to(player, *local)))
        {
            continue;
        }
        const maths::Vec3 point = aim_point(player, settings.target, eye, *view);
        const maths::Angles angles = maths::calc_aim_angles(eye, point);
        const float fov_distance = maths::angular_distance(*view, angles);
        if (fov_distance <= settings.fov)
        {
            candidates.push_back(AimCandidate{&player, point, angles, fov_distance, metres});
        }
    }
    return candidates;
}

std::optional<AimCandidate> select_target(std::span<const AimCandidate> candidates, settings::AimPriority priority)
{
    if (candidates.empty())
    {
        return std::nullopt;
    }
    return *std::ranges::min_element(candidates, [&](const AimCandidate& a, const AimCandidate& b) {
        const float ka = priority_key(a, priority);
        const float kb = priority_key(b, priority);
        return ka != kb ? ka < kb : a.fov_distance < b.fov_distance;
    });
}

std::optional<maths::Angles> compute_aim(const game::GameSnapshot& game, const settings::AimbotSettings& settings,
                                         settings::TeamMode team_mode, float frame_seconds)
{
    const std::vector<AimCandidate> candidates = find_candidates(game, settings, team_mode);
    const auto target = select_target(candidates, settings.priority);
    if (!target)
    {
        return std::nullopt;
    }
    const float fraction = maths::smoothing_fraction(settings.smoothing, frame_seconds);
    return maths::step_towards(*game.local_state.view_angles, target->angles, fraction);
}

std::optional<render::Circle> fov_circle(const game::GameSnapshot& game, const settings::AimbotSettings& settings,
                                         maths::Vec2 screen)
{
    const game::PlayerSnapshot* local = game.local();
    const auto& view = game.local_state.view_angles;
    if (!game.in_match || !game.view || local == nullptr || !view)
    {
        return std::nullopt;
    }
    // A point `fov` degrees off the view (along pitch, where the angle is exact), measured from the crosshair.
    const maths::Vec3 eye = local->eye_position();
    const float pitch = view->pitch + settings.fov <= maths::kMaxPitch ? view->pitch + settings.fov
                                                                        : view->pitch - settings.fov;
    const auto centre = maths::world_to_screen(*game.view, eye + maths::forward(*view) * kFovProbeDistance, screen);
    const auto edge = maths::world_to_screen(
        *game.view, eye + maths::forward(maths::Angles{pitch, view->yaw}) * kFovProbeDistance, screen);
    if (!centre || !edge)
    {
        return std::nullopt;
    }
    return render::Circle{{screen.x * 0.5f, screen.y * 0.5f}, (*edge - *centre).length(), settings.fov_colour, 1.0f};
}
} // namespace features
