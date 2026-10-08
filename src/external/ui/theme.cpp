#include "ui/theme.h"

#include <optional>

namespace ui
{
namespace
{
Palette g_palette;
std::optional<settings::OverlaySettings> g_applied; // what g_palette was built from

constexpr float kAccentHoverMix = 0.25f; // towards the text colour
constexpr float kAccentSoftAlpha = 0.16f;

Palette build(const settings::ThemeColours& t, const Color& accent)
{
    Color opaque_accent = accent;
    opaque_accent.a = 1.0f;
    Palette p;
    p.window = to_imvec4(t.window);
    p.chrome = to_imvec4(t.chrome);
    p.panel = to_imvec4(t.panel);
    p.control = to_imvec4(t.control);
    p.control_hover = to_imvec4(t.control_hover);
    p.border = to_imvec4(t.border);
    p.text = to_imvec4(t.text);
    p.text_dim = to_imvec4(t.text_dim);
    p.text_faint = to_imvec4(t.text_faint);
    p.accent = to_imvec4(opaque_accent);
    p.accent_hover = to_imvec4(opaque_accent.lerp(t.text, kAccentHoverMix));
    p.accent_soft = to_imvec4(opaque_accent.faded(kAccentSoftAlpha));
    p.on_accent = to_imvec4(settings::text_on(opaque_accent, t));
    p.ok = to_imvec4(t.ok);
    p.warn = to_imvec4(t.warn);
    p.danger = to_imvec4(t.danger);
    return p;
}

void apply_colours(ImGuiStyle& style, const Palette& p)
{
    const ImVec4 clear(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = p.text;
    c[ImGuiCol_TextDisabled] = p.text_faint;
    c[ImGuiCol_WindowBg] = p.window;
    c[ImGuiCol_ChildBg] = clear;
    c[ImGuiCol_PopupBg] = p.chrome;
    c[ImGuiCol_Border] = p.border;
    c[ImGuiCol_BorderShadow] = clear;
    c[ImGuiCol_FrameBg] = p.control;
    c[ImGuiCol_FrameBgHovered] = p.control_hover;
    c[ImGuiCol_FrameBgActive] = p.control_hover;
    c[ImGuiCol_TitleBg] = p.chrome;
    c[ImGuiCol_TitleBgActive] = p.chrome;
    c[ImGuiCol_TitleBgCollapsed] = p.chrome;
    c[ImGuiCol_MenuBarBg] = p.chrome;
    c[ImGuiCol_ScrollbarBg] = clear;
    c[ImGuiCol_ScrollbarGrab] = p.control_hover;
    c[ImGuiCol_ScrollbarGrabHovered] = p.text_faint;
    c[ImGuiCol_ScrollbarGrabActive] = p.accent;
    c[ImGuiCol_CheckMark] = p.accent;
    c[ImGuiCol_SliderGrab] = p.accent;
    c[ImGuiCol_SliderGrabActive] = p.accent_hover;
    c[ImGuiCol_Button] = p.control;
    c[ImGuiCol_ButtonHovered] = p.control_hover;
    c[ImGuiCol_ButtonActive] = p.accent_soft;
    c[ImGuiCol_Header] = p.accent_soft;
    c[ImGuiCol_HeaderHovered] = p.control_hover;
    c[ImGuiCol_HeaderActive] = p.accent_soft;
    c[ImGuiCol_Separator] = p.border;
    c[ImGuiCol_SeparatorHovered] = p.accent;
    c[ImGuiCol_SeparatorActive] = p.accent;
    c[ImGuiCol_ResizeGrip] = clear;
    c[ImGuiCol_ResizeGripHovered] = p.accent_soft;
    c[ImGuiCol_ResizeGripActive] = p.accent;
    c[ImGuiCol_TableHeaderBg] = p.control;
    c[ImGuiCol_TableRowBg] = clear;
    c[ImGuiCol_TableRowBgAlt] = with_alpha(p.text, 0.03f);
    c[ImGuiCol_TableBorderLight] = p.border;
    c[ImGuiCol_TableBorderStrong] = p.border;
    c[ImGuiCol_TextSelectedBg] = with_alpha(p.accent, 0.30f);
    c[ImGuiCol_NavCursor] = p.accent;
    c[ImGuiCol_ModalWindowDimBg] = with_alpha(p.chrome, 0.6f);
}
} // namespace

const Palette& palette() noexcept
{
    return g_palette;
}

void use_theme(const settings::OverlaySettings& overlay)
{
    if (g_applied && g_applied->theme == overlay.theme && g_applied->accent == overlay.accent)
    {
        return;
    }
    g_applied = overlay;
    g_palette = build(settings::theme_colours(overlay.theme), overlay.accent);
    apply_colours(ImGui::GetStyle(), g_palette);
}

void apply_style(ImGuiStyle& style)
{
    style.WindowPadding = ImVec2(0.0f, 0.0f); // the menu lays out its own header, tab bar and page areas
    style.FramePadding = ImVec2(9.0f, 5.0f);
    style.ItemSpacing = ImVec2(8.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
    style.CellPadding = ImVec2(6.0f, 4.0f);
    style.ScrollbarSize = 8.0f;
    style.GrabMinSize = 10.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f; // panels have a hairline edge
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    style.WindowRounding = 10.0f;
    style.ChildRounding = 8.0f; // panels
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
}

ImVec4 to_imvec4(const Color& c) noexcept
{
    return ImVec4(c.r, c.g, c.b, c.a);
}

ImVec4 with_alpha(ImVec4 c, float alpha) noexcept
{
    c.w = alpha;
    return c;
}

ImU32 u32(const ImVec4& c) noexcept
{
    return ImGui::ColorConvertFloat4ToU32(c);
}

float scaled(float pixels)
{
    return pixels * ImGui::GetStyle().FontScaleMain;
}
} // namespace ui
