#include <algorithm>
#include <format>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>

#include "config.h"
#include "input/actions.h"
#include "input/keys.h"
#include "settings/profile_store.h"
#include "settings/themes.h"
#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui::pages
{
namespace
{
// Sizes at UI scale 1.0.
constexpr float kProfileListHeight = 140.0f;
constexpr float kThemeTileHeight = 64.0f;
constexpr float kThemeTileMinWidth = 92.0f;

void request(app::AppState& app, app::ProfileOp op, std::string name = {})
{
    app.requests.profile = app::ProfileRequest{op, std::move(name)};
}

// "default" first, then the saved profiles. Click selects, double-click loads; the current one is marked.
void draw_profile_list(PageContext& ctx)
{
    app::AppState& app = ctx.app;
    MenuState& menu = ctx.menu;
    const std::vector<std::string>& names = app.profiles.names;
    if (std::find(names.begin(), names.end(), menu.selected_profile) == names.end())
    {
        menu.selected_profile = app.profiles.current;
    }
    ImGui::PushStyleColor(ImGuiCol_FrameBg, palette().window);
    const bool open = ImGui::BeginListBox("##profiles", ImVec2(-1.0f, scaled(kProfileListHeight)));
    ImGui::PopStyleColor();
    if (!open)
    {
        return;
    }
    for (const std::string& name : names)
    {
        const bool current = name == app.profiles.current;
        const std::string label = current ? name + "  (loaded)" : name;
        if (ImGui::Selectable(label.c_str(), name == menu.selected_profile, ImGuiSelectableFlags_AllowDoubleClick))
        {
            menu.selected_profile = name;
            menu.confirm_delete.clear();
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                request(app, app::ProfileOp::load, name);
            }
        }
    }
    ImGui::EndListBox();
}

void draw_profiles(PageContext& ctx)
{
    app::AppState& app = ctx.app;
    MenuState& menu = ctx.menu;
    const app::ProfileState& profiles = app.profiles;
    const bool current_read_only = settings::ProfileStore::is_read_only(profiles.current);
    const Palette& p = palette();

    widgets::panel_begin(ctx.fonts, "Profiles", nullptr,
                         "Every setting and keybind (and this look), saved as a JSON file. The last loaded or saved "
                         "profile loads again on the next start. \"default\" is the built-in defaults and can't be "
                         "overwritten: use Save as.");
    widgets::info_row("Current profile", profiles.current.c_str());
    if (app.unsaved_changes())
    {
        widgets::info_row("", "unsaved changes", p.warn);
    }

    draw_profile_list(ctx);

    // The selected profile: load or delete. The current one: save.
    if (widgets::button("Load", widgets::Tone::accent))
    {
        request(app, app::ProfileOp::load, menu.selected_profile);
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(current_read_only);
    if (widgets::button("Save"))
    {
        request(app, app::ProfileOp::save);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    const bool can_delete = !settings::ProfileStore::is_read_only(menu.selected_profile);
    ImGui::BeginDisabled(!can_delete);
    if (can_delete && menu.confirm_delete == menu.selected_profile)
    {
        if (widgets::button("Really delete?", widgets::Tone::danger))
        {
            request(app, app::ProfileOp::remove, menu.selected_profile);
            menu.confirm_delete.clear();
        }
    }
    else if (widgets::button("Delete", widgets::Tone::danger))
    {
        menu.confirm_delete = menu.selected_profile;
    }
    ImGui::EndDisabled();

    // A name: save as a new profile, or rename the current one.
    widgets::subheading("NEW NAME");
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float save_as = ImGui::CalcTextSize("Save as").x + 2.0f * ImGui::GetStyle().FramePadding.x;
    const float rename = ImGui::CalcTextSize("Rename").x + 2.0f * ImGui::GetStyle().FramePadding.x;
    ImGui::SetNextItemWidth(std::max(ImGui::GetContentRegionAvail().x - save_as - rename - 2.0f * spacing, 60.0f));
    ImGui::InputTextWithHint("##profile_name", "letters, digits, spaces, _ and -", menu.profile_name.data(),
                             menu.profile_name.size());
    const std::string name(menu.profile_name.data());
    const bool valid_name = settings::ProfileStore::clean_name(name).has_value();
    ImGui::SameLine();
    ImGui::BeginDisabled(!valid_name);
    if (widgets::button("Save as"))
    {
        request(app, app::ProfileOp::save_as, name);
        menu.profile_name.fill('\0');
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(current_read_only);
    if (widgets::button("Rename"))
    {
        request(app, app::ProfileOp::rename, name);
        menu.profile_name.fill('\0');
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Rename the current profile");
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();

    if (!profiles.message.empty())
    {
        widgets::notice(profiles.message.c_str(),
                        profiles.message_failed ? widgets::Notice::warn : widgets::Notice::info);
    }
    if (!profiles.warnings.empty())
    {
        const std::string title = std::to_string(profiles.warnings.size()) + " problem(s) in the loaded profile";
        ImGui::PushStyleColor(ImGuiCol_Text, p.warn);
        const bool open = ImGui::TreeNode(title.c_str());
        ImGui::PopStyleColor();
        if (open)
        {
            for (const std::string& warning : profiles.warnings)
            {
                ImGui::BulletText("%s", warning.c_str());
            }
            ImGui::TreePop();
        }
    }
    if (widgets::button("Reset to defaults"))
    {
        request(app, app::ProfileOp::reset);
    }
    ImGui::SameLine();
    widgets::help_marker("Every setting and keybind back to the built-in defaults. Nothing is saved until you Save "
                         "(or Save as).");
    ImGui::PushStyleColor(ImGuiCol_Text, p.text_faint);
    ImGui::TextWrapped("Folder: %s", profiles.folder.c_str());
    ImGui::PopStyleColor();
    widgets::panel_end();
}

// One clickable preview per theme: its background, a panel, a line of text, its accent and its name.
bool theme_tile(settings::MenuTheme theme, bool selected, float width)
{
    const settings::ThemeColours& t = settings::theme_colours(theme);
    const Palette& p = palette();
    const float height = scaled(kThemeTileHeight);
    const float rounding = ImGui::GetStyle().FrameRounding;
    const ImVec2 min = ImGui::GetCursorScreenPos();
    ImGui::PushID(static_cast<int>(theme));
    const bool clicked = ImGui::InvisibleButton("##theme", ImVec2(width, height));
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();
    const ImVec2 max(min.x + width, min.y + height);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(min, max, u32(to_imvec4(t.window)), rounding);

    // A tiny panel with a text line and an accent switch.
    const float pad = scaled(7.0f);
    const ImVec2 panel_min(min.x + pad, min.y + pad);
    const ImVec2 panel_max(max.x - pad, min.y + height * 0.52f);
    draw->AddRectFilled(panel_min, panel_max, u32(to_imvec4(t.panel)), rounding * 0.6f);
    const float mid = (panel_min.y + panel_max.y) * 0.5f;
    draw->AddLine(ImVec2(panel_min.x + pad * 0.8f, mid), ImVec2(panel_min.x + (panel_max.x - panel_min.x) * 0.45f, mid),
                  u32(to_imvec4(t.text_dim)), scaled(2.0f));
    const float knob = (panel_max.y - panel_min.y) * 0.22f;
    const ImVec2 track_max(panel_max.x - pad * 0.8f, mid + knob);
    const ImVec2 track_min(track_max.x - knob * 3.6f, mid - knob);
    draw->AddRectFilled(track_min, track_max, u32(to_imvec4(t.accent)), knob);

    const char* name = settings::theme_name(theme).data();
    const ImVec2 name_size = ImGui::CalcTextSize(name);
    draw->AddText(ImVec2(min.x + (width - name_size.x) * 0.5f, panel_max.y + (max.y - panel_max.y - name_size.y) * 0.5f),
                  u32(to_imvec4(t.text)), name);

    if (selected)
    {
        draw->AddRect(min, max, u32(p.accent), rounding, scaled(2.0f));
    }
    else
    {
        draw->AddRect(min, max, u32(hovered ? p.text_dim : p.border), rounding);
    }
    return clicked;
}

void draw_appearance(PageContext& ctx)
{
    settings::OverlaySettings& overlay = ctx.app.settings.overlay;
    widgets::panel_begin(ctx.fonts, "Appearance", nullptr,
                         "The menu's and the watermark's colours. Saved in the profile. The ESP, radar and panels on "
                         "the game keep their own colours (their tabs).");
    widgets::subheading("THEME");
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float available = ImGui::GetContentRegionAvail().x;
    const int count = static_cast<int>(settings::kMenuThemes.size());
    const int per_row = std::clamp(static_cast<int>((available + spacing) / (scaled(kThemeTileMinWidth) + spacing)),
                                   1, count);
    const float width = (available - spacing * static_cast<float>(per_row - 1)) / static_cast<float>(per_row);
    for (int i = 0; i < count; ++i)
    {
        if (i % per_row != 0)
        {
            ImGui::SameLine();
        }
        const settings::MenuTheme theme = settings::kMenuThemes[static_cast<std::size_t>(i)];
        if (theme_tile(theme, theme == overlay.theme, width))
        {
            overlay.theme = theme;
            overlay.accent = settings::theme_colours(theme).accent; // a new theme brings its own accent
        }
    }
    widgets::subheading("ACCENT");
    widgets::colour_row("Accent colour", overlay.accent, false,
                        "Switches that are on, the selected tab, sliders and the main buttons. Picking a theme sets "
                        "its own accent first.");
    const Color theme_accent = settings::theme_colours(overlay.theme).accent;
    ImGui::BeginDisabled(overlay.accent == theme_accent);
    if (widgets::button("Use the theme's accent"))
    {
        overlay.accent = theme_accent;
    }
    ImGui::EndDisabled();
    widgets::panel_end();
}

void draw_overlay(PageContext& ctx)
{
    settings::OverlaySettings& overlay = ctx.app.settings.overlay;
    widgets::panel_begin(ctx.fonts, "Overlay");
    widgets::switch_row("Watermark", &overlay.watermark,
                        "The logo and \"External Cheat by BigH\" in the top-left corner of the game, with the features "
                        "that are on listed under it.");
    widgets::switch_row("Frame outline", &overlay.frame_outline,
                        "A thin line along the overlay's edges. If the overlay covers the game exactly, the line sits "
                        "right on the edges of the game's picture, in windowed mode too.");
    widgets::panel_end();
}

// Exit straight away, or after one more click if there are unsaved changes.
void draw_exit(PageContext& ctx)
{
    app::AppState& app = ctx.app;
    MenuState& menu = ctx.menu;
    widgets::panel_begin(ctx.fonts, "Exit");
    widgets::hint(std::format("Closes the tool cleanly: lets go of anything it pressed in the game, removes the "
                              "overlay and closes the handle. The game keeps running, as if the tool was never "
                              "started. Same as the exit key ({}).",
                              input::key_name(app.settings.keybinds.bind(input::ActionId::exit).key))
                      .c_str());
    const bool unsaved = app.unsaved_changes();
    if (!unsaved)
    {
        menu.confirm_exit = false;
    }
    if (menu.confirm_exit)
    {
        if (widgets::button("Really exit? Unsaved changes are lost", widgets::Tone::danger))
        {
            app.requests.exit = true;
        }
        ImGui::SameLine();
        if (widgets::button("Cancel##exit"))
        {
            menu.confirm_exit = false;
        }
    }
    else if (widgets::button("Exit the tool", widgets::Tone::danger))
    {
        app.requests.exit = !unsaved;
        menu.confirm_exit = unsaved;
    }
    widgets::panel_end();
}
} // namespace

void draw_settings(PageContext& ctx)
{
    widgets::page_intro("Profiles, the menu's look and the overlay. Changes apply immediately.");
    widgets::Columns columns;
    draw_profiles(ctx);
    columns.next();
    draw_appearance(ctx);
    draw_overlay(ctx);
    draw_exit(ctx);
}
} // namespace ui::pages
