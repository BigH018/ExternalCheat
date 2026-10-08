#include "features/feature_summary.h"

#include <array>

namespace features
{
namespace
{
constexpr std::string_view kSeparator = " · ";

struct Entry
{
    bool ActiveFeatures::*flag;
    std::string_view name;
};

// Combat first, then visuals, then the rest.
constexpr std::array kEntries = {
    Entry{&ActiveFeatures::esp, "ESP"},
    Entry{&ActiveFeatures::aimbot, "Aimbot"},
    Entry{&ActiveFeatures::triggerbot, "Triggerbot"},
    Entry{&ActiveFeatures::radar, "Radar"},
    Entry{&ActiveFeatures::bomb_timer, "Bomb timer"},
    Entry{&ActiveFeatures::spectator_list, "Spectators"},
};
} // namespace

std::vector<std::string_view> active_feature_names(const ActiveFeatures& active)
{
    std::vector<std::string_view> names;
    for (const Entry& entry : kEntries)
    {
        if (active.*entry.flag)
        {
            names.push_back(entry.name);
        }
    }
    return names;
}

std::string feature_summary(const ActiveFeatures& active)
{
    std::string summary;
    for (const std::string_view name : active_feature_names(active))
    {
        if (!summary.empty())
        {
            summary += kSeparator;
        }
        summary += name;
    }
    return summary;
}
} // namespace features
