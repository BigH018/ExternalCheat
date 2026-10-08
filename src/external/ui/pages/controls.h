#pragma once

// Rows the feature pages share: enum choices, the shared team mode and the max-distance slider (keys:
// ui/keybind_widgets). Each returns true when the user changed the value.

#include <cstddef>
#include <span>

#include "settings/settings.h"
#include "ui/widgets.h"

namespace ui::pages
{
// An enum as a choice row (segmented buttons, or a dropdown when they don't fit); `names` in enumerator order.
template <class Enum, std::size_t N>
bool choice(const char* label, Enum& value, const char* const (&names)[N], const char* help = nullptr)
{
    int index = static_cast<int>(value);
    if (widgets::choice_row(label, &index, std::span<const char* const>(names, N), help))
    {
        value = static_cast<Enum>(index);
        return true;
    }
    return false;
}

// The team mode every feature shares (settings::GeneralSettings), with its help.
bool team_mode_row(settings::TeamMode& mode);

// A max-distance slider in metres; all the way left = no limit.
bool max_distance_row(float& metres, const char* help);
} // namespace ui::pages
