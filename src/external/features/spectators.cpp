#include "features/spectators.h"

#include <format>

#include "config.h"
#include "features/esp.h"
#include "features/targeting.h"
#include "render/panel.h"

namespace features
{
namespace
{
constexpr Color kTitle = Color::rgb(0x8CC4CF);
constexpr Color kEnemy = Color::rgb(0xF25C5C);
constexpr Color kTeam = Color::rgb(0x5CA8F2);
} // namespace

std::uintptr_t watched_pawn(const game::GameSnapshot& game) noexcept
{
    const game::PlayerSnapshot* local = game.local();
    if (local == nullptr)
    {
        return 0;
    }
    if (local->alive)
    {
        return local->pawn;
    }
    return game::watches_target(local->observer_mode) ? local->observer_target : 0;
}

std::optional<SpectatorInfo> spectator_info(const game::GameSnapshot& game, settings::TeamMode team_mode)
{
    const std::uintptr_t watched = watched_pawn(game);
    const game::PlayerSnapshot* local = game.local();
    if (watched == 0 || local == nullptr)
    {
        return std::nullopt;
    }

    SpectatorInfo info;
    for (const game::PlayerSnapshot& player : game.players)
    {
        if (player.is_local)
        {
            continue;
        }
        if (player.alive && player.pawn == watched) // you're dead and watch them
        {
            info.watched = display_name(player.name);
        }
        else if (!player.alive && game::watches_target(player.observer_mode) && player.observer_target == watched)
        {
            info.spectators.push_back(
                Spectator{display_name(player.name), player.observer_mode, is_enemy(player, *local, team_mode)});
        }
    }
    return info;
}

const char* observer_mode_name(game::ObserverMode mode) noexcept
{
    switch (mode)
    {
    case game::ObserverMode::in_eye: return "1st person";
    case game::ObserverMode::chase: return "3rd person";
    case game::ObserverMode::none:
    case game::ObserverMode::fixed:
    case game::ObserverMode::roaming: break;
    }
    return "";
}

std::vector<render::Primitive> build_spectator_list(const game::GameSnapshot& game,
                                                    const settings::SpectatorSettings& settings,
                                                    settings::TeamMode team_mode, maths::Vec2 screen,
                                                    float line_height)
{
    std::vector<render::Primitive> out;
    const auto info = settings.enabled ? spectator_info(game, team_mode) : std::nullopt;
    if (!info || (settings.hide_when_empty && info->spectators.empty()))
    {
        return out;
    }
    const float left = settings.side == settings::PanelSide::left
                           ? config::kSpectatorMargin
                           : screen.x - config::kSpectatorPanelWidth - config::kSpectatorMargin;
    const float top = config::kSpectatorTop.clamp(settings.top);
    render::PanelWriter panel(out, {left, top}, config::kSpectatorPanelWidth, line_height);

    const std::string title = info->watched.empty() ? "Spectators" : "Watching " + info->watched;
    panel.text(title, kTitle, std::format("{}", info->spectators.size()), kTitle);
    if (info->spectators.empty())
    {
        panel.text("Nobody", render::kPanelDim);
    }
    for (const Spectator& spectator : info->spectators)
    {
        panel.text(spectator.name, spectator.enemy ? kEnemy : kTeam,
                   settings.show_mode ? observer_mode_name(spectator.mode) : "", render::kPanelDim);
    }
    panel.finish();
    return out;
}
} // namespace features
