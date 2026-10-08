#pragma once

// Which features are switched on, by name: one per line in the watermark, joined on one line on the Home page.
//
// PURE: no <Windows.h>, no ImGui.

#include <string>
#include <string_view>
#include <vector>

namespace features
{
// One flag per user-facing feature, in the order they are listed. app/frame fills it from the settings every frame
// once the features exist (ESP in Phase 4, aimbot + triggerbot in Phase 5, the misc ones in Phase 6); until then
// everything is off.
struct ActiveFeatures
{
    bool esp = false;
    bool aimbot = false;
    bool triggerbot = false;
    bool radar = false;
    bool bomb_timer = false;
    bool spectator_list = false;
};

// The names of the features that are on, in the fixed order above ({"ESP", "Aimbot"}). Empty when nothing is on.
// The views point at string literals, so they stay valid.
[[nodiscard]] std::vector<std::string_view> active_feature_names(const ActiveFeatures& active);

// The same names joined with " · " ("ESP · Aimbot"). Empty when nothing is on.
[[nodiscard]] std::string feature_summary(const ActiveFeatures& active);
} // namespace features
