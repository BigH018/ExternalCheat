#include "features/targeting.h"

#include "game/visibility.h"
#include "maths/vec.h"

namespace features
{
bool is_enemy(const game::PlayerSnapshot& player, const game::PlayerSnapshot& local, settings::TeamMode mode) noexcept
{
    if (player.controller == local.controller)
    {
        return false;
    }
    return mode == settings::TeamMode::free_for_all || player.team != local.team;
}

bool is_live_target(const game::PlayerSnapshot& player) noexcept
{
    return !player.is_local && player.pawn != 0 && player.alive && !player.dormant;
}

bool is_visible_to(const game::PlayerSnapshot& player, const game::PlayerSnapshot& local) noexcept
{
    return game::is_spotted_by(player.spotted_by_mask, local.slot());
}

float distance_metres(const game::PlayerSnapshot& a, const game::PlayerSnapshot& b) noexcept
{
    return maths::units_to_metres(a.origin.distance_to(b.origin));
}
} // namespace features
