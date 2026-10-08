#include "ui/keybind_widgets.h"

#include <algorithm>
#include <array>
#include <span>
#include <string>

#include <imgui.h>

#include "input/keybinds.h"
#include "input/keys.h"
#include "ui/theme.h"
#include "ui/widgets.h"

namespace ui::keybind
{
namespace
{
constexpr float kKeyMinFraction = 0.4f; // of the control column, when a mode selector sits next to the key

constexpr char kCaptureText[] = "Press a key...";
constexpr char kHelp[] = "Click, then press a key or mouse button (for Mouse 1, click outside the menu). Esc clears "
                         "the bind, clicking again cancels.";
constexpr char kMenuKeyHelp[] = "Click, then press a keyboard key. The menu key is a Windows hotkey, so it can't be a "
                                "mouse button or Shift / Ctrl / Alt on its own, and it can't be cleared. Esc or "
                                "clicking again cancels.";

// The other actions bound to this action's key, as one line ("ESP on / off, Radar on / off"), or empty.
std::string conflicting_with(const app::AppState& app, input::ActionId id)
{
    const std::uint32_t key = app.settings.keybinds.bind(id).key;
    std::string others;
    for (const input::Conflict& conflict : input::find_conflicts(app.settings.keybinds.binds))
    {
        if (conflict.key != key)
        {
            continue;
        }
        for (const input::ActionId other : conflict.actions)
        {
            if (other != id)
            {
                others += (others.empty() ? "" : ", ") + std::string(input::action(other).label);
            }
        }
    }
    return others;
}
} // namespace

void key_button(app::AppState& app, input::ActionId id, float width)
{
    const Palette& p = palette();
    const input::Bind& bind = app.settings.keybinds.bind(id);
    const bool capturing = app.capture.capturing(id);
    const std::string others = capturing ? std::string() : conflicting_with(app, id);
    const std::string label = (capturing ? std::string(kCaptureText) : input::key_name(bind.key)) + "##key";

    ImGui::PushID(static_cast<int>(input::index_of(id)));
    int colours = 0;
    if (capturing)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, p.accent);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, p.accent_hover);
        ImGui::PushStyleColor(ImGuiCol_Text, p.on_accent);
        colours = 3;
    }
    else if (!others.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, with_alpha(p.danger, 0.16f));
        ImGui::PushStyleColor(ImGuiCol_Text, p.danger);
        colours = 2;
    }
    else if (bind.key == input::kUnbound)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, p.text_faint);
        colours = 1;
    }
    if (ImGui::Button(label.c_str(), ImVec2(width, 0.0f)))
    {
        if (capturing)
        {
            app.capture.cancel();
        }
        else
        {
            app.capture.start(id, app.now_ms);
        }
    }
    ImGui::PopStyleColor(colours);
    if (ImGui::IsItemHovered())
    {
        if (others.empty())
        {
            ImGui::SetTooltip("%s", input::action(id).hotkey ? kMenuKeyHelp : kHelp);
        }
        else
        {
            ImGui::SetTooltip("%s is also bound to: %s", input::key_name(bind.key).c_str(), others.c_str());
        }
    }
    ImGui::PopID();
}

void mode_selector(app::AppState& app, input::ActionId id, float width)
{
    const std::span<const input::BindMode> modes = input::allowed_modes(id);
    if (modes.size() < 2)
    {
        return;
    }
    input::Bind& bind = app.settings.keybinds.bind(id);
    std::array<const char*, 3> names{};
    int index = 0;
    const std::size_t count = std::min(modes.size(), names.size());
    for (std::size_t i = 0; i < count; ++i)
    {
        names[i] = input::mode_name(modes[i]).data(); // string literals: null-terminated
        index = modes[i] == bind.mode ? static_cast<int>(i) : index;
    }
    ImGui::PushID(static_cast<int>(input::index_of(id)));
    if (widgets::segmented("##mode", &index, std::span<const char* const>(names.data(), count), width))
    {
        bind.mode = modes[static_cast<std::size_t>(index)];
    }
    ImGui::PopID();
}

void bind_row(app::AppState& app, input::ActionId id, const char* label, const char* help)
{
    ImGui::PushID(label);
    const float width = widgets::row(label, help);
    const std::span<const input::BindMode> modes = input::allowed_modes(id);
    if (modes.size() > 1)
    {
        // The mode selector gets what its names need; the key button the rest (but never less than its share).
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        float widest = 0.0f; // segments are equally wide, so each needs room for the longest name
        for (const input::BindMode mode : modes)
        {
            widest = std::max(widest, ImGui::CalcTextSize(input::mode_name(mode).data()).x);
        }
        const float mode_needed =
            static_cast<float>(modes.size()) * (widest + 2.0f * ImGui::GetStyle().FramePadding.x);
        const float key = std::max(width - spacing - mode_needed, (width - spacing) * kKeyMinFraction);
        key_button(app, id, key);
        ImGui::SameLine();
        mode_selector(app, id, width - spacing - key);
    }
    else
    {
        key_button(app, id, width);
    }
    ImGui::PopID();
}
} // namespace ui::keybind
