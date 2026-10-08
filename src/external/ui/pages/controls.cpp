#include "ui/pages/controls.h"

#include "config.h"

namespace ui::pages
{
bool team_mode_row(settings::TeamMode& mode)
{
    static constexpr const char* kTeamModes[] = {"Teams", "Free for all"};
    return choice("Team mode", mode, kTeamModes,
                  "Who counts as an enemy, for every feature (ESP, aimbot, triggerbot, radar). Teams: the other team. "
                  "Free for all: everyone else (CS2 deathmatch is free for all).");
}

bool max_distance_row(float& metres, const char* help)
{
    return widgets::slider_row("Max distance", &metres, config::kMaxDistance.min, config::kMaxDistance.max,
                               metres <= 0.0f ? "no limit" : "%.0f m", help);
}
} // namespace ui::pages
