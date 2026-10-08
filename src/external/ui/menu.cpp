#include "ui/menu.h"

#include <algorithm>
#include <array>

#include <imgui.h>

#include "config.h"
#include "ui/icons.h"
#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui
{
namespace
{
struct PageEntry
{
    Page page;
    const char* label;
    Icon icon;
    void (*draw)(pages::PageContext&);
};

constexpr std::array kPages = {
    PageEntry{Page::Home, "Home", Icon::home, &pages::draw_home},
    PageEntry{Page::Aimbot, "Aimbot", Icon::aimbot, &pages::draw_aimbot},
    PageEntry{Page::Triggerbot, "Triggerbot", Icon::triggerbot, &pages::draw_triggerbot},
    PageEntry{Page::Esp, "ESP", Icon::esp, &pages::draw_esp},
    PageEntry{Page::Misc, "Misc", Icon::misc, &pages::draw_misc},
    PageEntry{Page::Keybinds, "Keybinds", Icon::keybinds, &pages::draw_keybinds},
    PageEntry{Page::Settings, "Settings", Icon::settings, &pages::draw_settings},
};

// Sizes at UI scale 1.0.
constexpr float kEdge = 16.0f;         // left/right padding of the header, tabs and page
constexpr float kTabPadding = 11.0f;   // inside a tab, left and right
constexpr float kTabGap = 2.0f;        // between tabs
constexpr float kTabIconSize = 15.0f;
constexpr float kTabIconGap = 7.0f;
constexpr float kTabUnderline = 2.0f;
constexpr float kTabDotRadius = 2.5f;
constexpr float kCloseSize = 26.0f;

// The tab's feature (or any of them, for Misc) is switched on.
bool page_active(const app::AppState& app, Page page)
{
    const settings::Settings& s = app.settings;
    switch (page)
    {
    case Page::Aimbot: return s.aimbot.enabled;
    case Page::Triggerbot: return s.triggerbot.enabled;
    case Page::Esp: return s.esp.enabled;
    case Page::Misc: return s.radar.enabled || s.bomb_timer.enabled || s.spectators.enabled;
    default: return false;
    }
}

// The header and tab bar share one background, painted on the window with its rounded top corners.
void paint_chrome()
{
    const Palette& p = palette();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    const float bottom = pos.y + scaled(config::kHeaderHeight) + scaled(config::kTabBarHeight);
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, bottom), u32(p.chrome), ImGui::GetStyle().WindowRounding,
                        ImDrawFlags_RoundCornersTop);
    draw->AddLine(ImVec2(pos.x, bottom), ImVec2(pos.x + size.x, bottom), u32(p.border));
}

// A "x" drawn in a square button. Returns true when clicked.
bool close_button(ImVec2 min, float size)
{
    const Palette& p = palette();
    ImGui::SetCursorScreenPos(min);
    const bool clicked = ImGui::InvisibleButton("##close", ImVec2(size, size));
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 max(min.x + size, min.y + size);
    if (hovered)
    {
        draw->AddRectFilled(min, max, u32(p.control_hover), ImGui::GetStyle().FrameRounding);
        ImGui::SetTooltip("Close the menu (same as the menu key)");
    }
    const float arm = size * 0.2f;
    const ImVec2 c(min.x + size * 0.5f, min.y + size * 0.5f);
    const ImU32 colour = u32(hovered ? p.text : p.text_dim);
    draw->AddLine(ImVec2(c.x - arm, c.y - arm), ImVec2(c.x + arm, c.y + arm), colour, 1.6f);
    draw->AddLine(ImVec2(c.x - arm, c.y + arm), ImVec2(c.x + arm, c.y - arm), colour, 1.6f);
    return clicked;
}

void draw_header(const Fonts& fonts, ImTextureData* logo, app::AppState& app)
{
    const Palette& p = palette();
    const float header = scaled(config::kHeaderHeight);
    const float edge = scaled(kEdge);
    const float logo_size = scaled(config::kHeaderLogoSize);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetWindowSize().x;
    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Logo, name, author.
    ImGui::SetCursorScreenPos(ImVec2(origin.x + edge, origin.y + (header - logo_size) * 0.5f));
    widgets::image_rounded(logo, logo_size, logo_size * 0.28f);
    const float title_size = scaled(config::kTitleFontSize);
    const ImVec2 name_size = fonts.bold->CalcTextSizeA(title_size, 1e6f, 0.0f, config::kAppName);
    const float text_x = origin.x + edge + logo_size + scaled(12.0f);
    const float text_y = origin.y + (header - name_size.y) * 0.5f;
    draw->AddText(fonts.bold, title_size, ImVec2(text_x, text_y), u32(p.text), config::kAppName);
    const float small = ImGui::GetFontSize();
    const ImVec2 author_size = ImGui::CalcTextSize(config::kAppAuthor);
    draw->AddText(fonts.regular, small,
                  ImVec2(text_x + name_size.x + scaled(8.0f), text_y + name_size.y - author_size.y - scaled(1.0f)),
                  u32(p.text_dim), config::kAppAuthor);

    // Right side: the edition pill, then the close button.
    const float close = scaled(kCloseSize);
    const ImVec2 close_min(origin.x + width - edge - close, origin.y + (header - close) * 0.5f);
    if (close_button(close_min, close))
    {
        app.requests.close_menu = true;
    }
    const ImVec2 pill_text = ImGui::CalcTextSize(config::kAppEdition);
    const float pill_width = pill_text.x + 2.0f * scaled(8.0f);
    const float pill_height = pill_text.y + 2.0f * scaled(2.0f);
    ImGui::SetCursorScreenPos(
        ImVec2(close_min.x - scaled(10.0f) - pill_width, origin.y + (header - pill_height) * 0.5f));
    widgets::pill(config::kAppEdition, p.text_dim);

    ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + header));
}

void draw_tabs(MenuState& state, const Fonts& fonts, const app::AppState& app)
{
    const Palette& p = palette();
    const float height = scaled(config::kTabBarHeight);
    const float padding = scaled(kTabPadding);
    const float icon = scaled(kTabIconSize);
    const float icon_gap = scaled(kTabIconGap);
    const float gap = scaled(kTabGap);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float available = ImGui::GetWindowSize().x - 2.0f * scaled(kEdge);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGui::PushFont(fonts.semibold, 0.0f);

    // Every tab with its label if they fit; otherwise only the selected one keeps its label (the rest show it as a
    // tooltip).
    float full = 0.0f;
    for (const PageEntry& entry : kPages)
    {
        full += 2.0f * padding + icon + icon_gap + ImGui::CalcTextSize(entry.label).x + gap;
    }
    const bool compact = full > available;

    float x = origin.x + scaled(kEdge) - padding;
    for (const PageEntry& entry : kPages)
    {
        const bool selected = state.page == entry.page;
        const bool labelled = !compact || selected;
        const ImVec2 label_size = ImGui::CalcTextSize(entry.label);
        const float width = 2.0f * padding + icon + (labelled ? icon_gap + label_size.x : 0.0f);
        ImGui::SetCursorScreenPos(ImVec2(x, origin.y));
        ImGui::PushID(static_cast<int>(entry.page));
        if (ImGui::InvisibleButton("##tab", ImVec2(width, height)))
        {
            state.page = entry.page;
        }
        const bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();
        if (hovered && !labelled)
        {
            ImGui::SetTooltip("%s", entry.label);
        }

        const ImU32 colour = u32(selected ? p.text : (hovered ? p.text : p.text_dim));
        const ImVec2 icon_centre(x + padding + icon * 0.5f, origin.y + height * 0.5f);
        draw_icon(draw, entry.icon, icon_centre, icon, u32(selected ? p.accent : (hovered ? p.text : p.text_dim)));
        if (page_active(app, entry.page))
        {
            draw->AddCircleFilled(ImVec2(icon_centre.x + icon * 0.55f, icon_centre.y - icon * 0.55f),
                                  scaled(kTabDotRadius), u32(p.ok));
        }
        if (labelled)
        {
            draw->AddText(ImVec2(x + padding + icon + icon_gap, origin.y + (height - label_size.y) * 0.5f), colour,
                          entry.label);
        }
        if (selected)
        {
            draw->AddRectFilled(ImVec2(x + padding * 0.5f, origin.y + height - scaled(kTabUnderline)),
                                ImVec2(x + width - padding * 0.5f, origin.y + height), u32(p.accent),
                                scaled(kTabUnderline));
        }
        x += width + gap;
    }
    ImGui::PopFont();
    ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + height));
}

void draw_page(MenuState& state, pages::PageContext& ctx)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(scaled(kEdge), scaled(kEdge) * 0.8f));
    ImGui::BeginChild("##page", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
    for (const PageEntry& entry : kPages)
    {
        if (entry.page == state.page)
        {
            entry.draw(ctx);
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}
} // namespace

void draw_menu(MenuState& state, const Fonts& fonts, ImTextureData* logo, app::AppState& app)
{
    // First appearance: centred, at the configured size but never bigger than the game window. Movable, resizable.
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 size(std::min(scaled(config::kMenuWidth), display.x * config::kMenuMaxScreenFraction),
                      std::min(scaled(config::kMenuHeight), display.y * config::kMenuMaxScreenFraction));
    ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(std::min(scaled(config::kMenuMinWidth), display.x), std::min(scaled(config::kMenuMinHeight), display.y)),
        ImVec2(display.x, display.y));

    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::Begin("External Cheat##menu", nullptr, kFlags))
    {
        paint_chrome();
        draw_header(fonts, logo, app);
        draw_tabs(state, fonts, app);
        pages::PageContext ctx{fonts, app, logo, state};
        draw_page(state, ctx);
    }
    ImGui::End();
}
} // namespace ui
