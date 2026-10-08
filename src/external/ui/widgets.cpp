#include "ui/widgets.h"

#include <algorithm>
#include <cmath>

namespace ui::widgets
{
namespace
{
// Sizes at UI scale 1.0.
constexpr float kPanelPaddingX = 14.0f;
constexpr float kPanelPaddingY = 12.0f;
constexpr float kPanelGap = 12.0f;         // between panels in a column
constexpr float kColumnGap = 12.0f;        // between the two columns
constexpr float kTwoColumnMinWidth = 720.0f; // below this, one column (each panel gets the whole width)
constexpr float kLabelFraction = 0.46f;    // of a row's width
constexpr float kLabelMinWidth = 110.0f;
constexpr float kControlMinWidth = 120.0f;
constexpr float kHelpGap = 5.0f;
constexpr float kTooltipWidthInFonts = 28.0f;
constexpr float kSwitchHeightFraction = 0.68f; // of the frame height
constexpr float kSwitchAspect = 1.8f;
constexpr float kSwitchSpeed = 12.0f;          // animation: fraction of the way per second
constexpr float kSegmentInset = 2.0f;
constexpr float kChipPaddingX = 11.0f;
constexpr float kNoticeBar = 3.0f;
constexpr float kNoticePadding = 9.0f;
constexpr float kPillPaddingX = 8.0f;
constexpr float kPillPaddingY = 2.0f;
constexpr float kDotRadius = 4.0f;

const Palette& pal()
{
    return palette();
}

struct RowLayout
{
    float start_x;       // window-relative
    float available;     // the row's whole width
    float control_x;     // window-relative
    float control_width;
};

RowLayout layout_row()
{
    const float available = ImGui::GetContentRegionAvail().x;
    const float label = std::clamp(available * kLabelFraction, std::min(scaled(kLabelMinWidth), available),
                                   std::max(available - scaled(kControlMinWidth), 0.0f));
    const float start = ImGui::GetCursorPosX();
    return RowLayout{start, available, start + label, available - label};
}

float help_marker_width()
{
    return ImGui::GetFontSize() * 0.8f;
}

// The label and its help marker. Returns the label's own width plus the marker's.
float draw_label(const char* label, const char* help, const ImVec4& colour)
{
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, colour);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    float width = ImGui::GetItemRectSize().x;
    if (help != nullptr)
    {
        ImGui::SameLine(0.0f, scaled(kHelpGap));
        help_marker(help);
        width += scaled(kHelpGap) + help_marker_width();
    }
    return width;
}

void tooltip(const char* text)
{
    if (ImGui::BeginTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * kTooltipWidthInFonts);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

// Text centred in [min, max], clipped to it.
void centred_text(ImDrawList* draw, ImVec2 min, ImVec2 max, ImU32 colour, const char* text)
{
    const ImVec2 size = ImGui::CalcTextSize(text);
    const ImVec2 pos(std::max(min.x, (min.x + max.x - size.x) * 0.5f), (min.y + max.y - size.y) * 0.5f);
    draw->PushClipRect(min, max, true);
    draw->AddText(pos, colour, text);
    draw->PopClipRect();
}
} // namespace

// --- Layout --------------------------------------------------------------------------------------------------------

Columns::Columns()
{
    two_ = ImGui::GetContentRegionAvail().x >= scaled(kTwoColumnMinWidth);
    if (!two_)
    {
        return;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(scaled(kColumnGap) * 0.5f, 0.0f));
    two_ = ImGui::BeginTable("##columns", 2,
                             ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings |
                                 ImGuiTableFlags_NoPadOuterX);
    if (!two_)
    {
        ImGui::PopStyleVar();
        return;
    }
    ImGui::TableNextColumn();
}

Columns::~Columns()
{
    if (two_)
    {
        ImGui::EndTable();
        ImGui::PopStyleVar();
    }
}

void Columns::next()
{
    if (two_)
    {
        ImGui::TableNextColumn();
    }
}

void page_intro(const char* text)
{
    hint(text);
    ImGui::Dummy(ImVec2(0.0f, scaled(2.0f)));
}

void panel_begin(const Fonts& fonts, const char* title, bool* enabled, const char* help)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, pal().panel);
    ImGui::PushStyleColor(ImGuiCol_Border, pal().border);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(scaled(kPanelPaddingX), scaled(kPanelPaddingY)));
    ImGui::BeginChild(title, ImVec2(0.0f, 0.0f),
                      ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Title row: the title (and help) on the left, the main switch on the right.
    const float start = ImGui::GetCursorPosX();
    const float available = ImGui::GetContentRegionAvail().x;
    ImGui::PushFont(fonts.semibold, 0.0f);
    const ImVec4 title_colour = enabled != nullptr && !*enabled ? pal().text_dim : pal().text;
    draw_label(title, nullptr, title_colour);
    ImGui::PopFont();
    if (help != nullptr)
    {
        ImGui::SameLine(0.0f, scaled(kHelpGap));
        help_marker(help);
    }
    if (enabled != nullptr)
    {
        ImGui::SameLine(start + available - switch_width());
        toggle_switch("##enabled", enabled);
    }

    // A hairline under the title, edge to edge.
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetWindowPos();
    const float y = ImGui::GetCursorScreenPos().y + scaled(1.0f);
    draw->AddLine(ImVec2(pos.x + 1.0f, y), ImVec2(pos.x + ImGui::GetWindowSize().x - 1.0f, y), u32(pal().border));
    ImGui::Dummy(ImVec2(0.0f, scaled(4.0f)));
}

void panel_end()
{
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    ImGui::Dummy(ImVec2(0.0f, scaled(kPanelGap) - ImGui::GetStyle().ItemSpacing.y));
}

void subheading(const char* text)
{
    ImGui::Dummy(ImVec2(0.0f, scaled(2.0f)));
    ImGui::PushFont(nullptr, ImGui::GetFontSize() - scaled(3.0f));
    ImGui::TextColored(pal().text_faint, "%s", text);
    ImGui::PopFont();
}

// --- Rows ----------------------------------------------------------------------------------------------------------

float row(const char* label, const char* help)
{
    const RowLayout layout = layout_row();
    const float label_width = draw_label(label, help, pal().text);
    if (layout.start_x + label_width + ImGui::GetStyle().ItemSpacing.x > layout.control_x)
    {
        ImGui::SetCursorPosX(layout.start_x); // no room: the control goes on its own line, full width
        return layout.available;
    }
    ImGui::SameLine(layout.control_x);
    return layout.control_width;
}

bool switch_row(const char* label, bool* value, const char* help)
{
    const RowLayout layout = layout_row();
    ImGui::PushID(label);
    draw_label(label, nullptr, pal().text);
    bool changed = false;
    if (ImGui::IsItemClicked())
    {
        *value = !*value;
        changed = true;
    }
    if (help != nullptr)
    {
        ImGui::SameLine(0.0f, scaled(kHelpGap));
        help_marker(help);
    }
    ImGui::SameLine(layout.start_x + layout.available - switch_width());
    changed |= toggle_switch("##switch", value);
    ImGui::PopID();
    return changed;
}

bool slider_row(const char* label, float* value, float min, float max, const char* format, const char* help)
{
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(row(label, help));
    const bool changed = ImGui::SliderFloat("##slider", value, min, max, format, ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopID();
    return changed;
}

bool slider_row(const char* label, int* value, int min, int max, const char* format, const char* help)
{
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(row(label, help));
    const bool changed = ImGui::SliderInt("##slider", value, min, max, format, ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopID();
    return changed;
}

bool choice_row(const char* label, int* index, std::span<const char* const> names, const char* help)
{
    ImGui::PushID(label);
    const float width = row(label, help);
    float needed = 0.0f;
    for (const char* name : names)
    {
        needed += ImGui::CalcTextSize(name).x + 2.0f * ImGui::GetStyle().FramePadding.x;
    }
    bool changed = false;
    if (needed <= width)
    {
        changed = segmented("##choice", index, names, width);
    }
    else
    {
        ImGui::SetNextItemWidth(width);
        changed = ImGui::Combo("##choice", index, names.data(), static_cast<int>(names.size()));
    }
    ImGui::PopID();
    return changed;
}

bool colour_row(const char* label, Color& value, bool alpha, const char* help)
{
    static_assert(sizeof(Color) == 4 * sizeof(float), "ImGui edits a Color as float[4]");
    const RowLayout layout = layout_row();
    ImGui::PushID(label);
    draw_label(label, help, pal().text);
    ImGui::SameLine(layout.start_x + layout.available - ImGui::GetFrameHeight()); // the swatch is a frame-high square
    ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
    flags |= alpha ? ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf : ImGuiColorEditFlags_NoAlpha;
    const bool changed = ImGui::ColorEdit4("##colour", &value.r, flags);
    ImGui::PopID();
    return changed;
}

void info_row(const char* label, const char* value)
{
    info_row(label, value, pal().text);
}

void info_row(const char* label, const char* value, const ImVec4& value_colour)
{
    const RowLayout layout = layout_row();
    ImGui::TextColored(pal().text_dim, "%s", label);
    const float width = ImGui::CalcTextSize(value).x;
    const float right = layout.start_x + layout.available - width;
    ImGui::SameLine(std::max(right, layout.start_x + ImGui::GetItemRectSize().x + ImGui::GetStyle().ItemSpacing.x));
    ImGui::TextColored(value_colour, "%s", value);
}

// --- Controls ------------------------------------------------------------------------------------------------------

float switch_width()
{
    return ImGui::GetFrameHeight() * kSwitchHeightFraction * kSwitchAspect;
}

bool toggle_switch(const char* id, bool* value)
{
    const float frame = ImGui::GetFrameHeight();
    const float height = frame * kSwitchHeightFraction;
    const float width = height * kSwitchAspect;
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton(id, ImVec2(width, frame));
    if (clicked)
    {
        *value = !*value;
    }
    const bool hovered = ImGui::IsItemHovered();

    // Slide the knob instead of jumping (the position is kept per switch in ImGui's state storage).
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID key = ImGui::GetItemID();
    const float target = *value ? 1.0f : 0.0f;
    float t = storage->GetFloat(key, target);
    const float step = std::min(1.0f, ImGui::GetIO().DeltaTime * kSwitchSpeed);
    t += (target - t) * step;
    if (std::abs(target - t) < 0.01f)
    {
        t = target;
    }
    storage->SetFloat(key, t);

    const Palette& p = pal();
    const ImVec2 min(origin.x, origin.y + (frame - height) * 0.5f);
    const ImVec2 max(min.x + width, min.y + height);
    const ImVec4 off = hovered ? p.control_hover : p.control;
    const ImVec4 on = hovered ? p.accent_hover : p.accent;
    const ImVec4 track(off.x + (on.x - off.x) * t, off.y + (on.y - off.y) * t, off.z + (on.z - off.z) * t, 1.0f);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(min, max, u32(track), height * 0.5f);
    const float radius = height * 0.5f - scaled(2.5f);
    const float x = min.x + height * 0.5f + (width - height) * t;
    const ImVec4 knob_off = p.text_dim;
    const ImVec4 knob_on = p.on_accent;
    const ImVec4 knob(knob_off.x + (knob_on.x - knob_off.x) * t, knob_off.y + (knob_on.y - knob_off.y) * t,
                      knob_off.z + (knob_on.z - knob_off.z) * t, 1.0f);
    draw->AddCircleFilled(ImVec2(x, min.y + height * 0.5f), radius, u32(knob));
    return clicked;
}

bool segmented(const char* id, int* index, std::span<const char* const> names, float width)
{
    if (names.empty())
    {
        return false;
    }
    ImGui::PushID(id);
    const Palette& p = pal();
    const float height = ImGui::GetFrameHeight();
    const float rounding = ImGui::GetStyle().FrameRounding;
    const float inset = scaled(kSegmentInset);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), u32(p.control), rounding);

    const float segment = width / static_cast<float>(names.size());
    bool changed = false;
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        const ImVec2 min(origin.x + segment * static_cast<float>(i), origin.y);
        const ImVec2 max(min.x + segment, origin.y + height);
        ImGui::SetCursorScreenPos(min);
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::InvisibleButton("##segment", ImVec2(segment, height)) && *index != static_cast<int>(i))
        {
            *index = static_cast<int>(i);
            changed = true;
        }
        const bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();
        const bool selected = *index == static_cast<int>(i);
        const ImVec2 inner_min(min.x + inset, min.y + inset);
        const ImVec2 inner_max(max.x - inset, max.y - inset);
        if (selected)
        {
            draw->AddRectFilled(inner_min, inner_max, u32(p.accent), rounding - inset * 0.5f);
        }
        else if (hovered)
        {
            draw->AddRectFilled(inner_min, inner_max, u32(p.control_hover), rounding - inset * 0.5f);
        }
        const ImVec4 text = selected ? p.on_accent : (hovered ? p.text : p.text_dim);
        centred_text(draw, inner_min, inner_max, u32(text), names[i]);
    }
    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy(ImVec2(width, height));
    ImGui::PopID();
    return changed;
}

void chips(std::initializer_list<Chip> items)
{
    const Palette& p = pal();
    const float line_end = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
    const float height = ImGui::GetFrameHeight();
    bool first = true;
    for (const Chip& chip : items)
    {
        const ImVec2 size(ImGui::CalcTextSize(chip.label).x + 2.0f * scaled(kChipPaddingX), height);
        if (!first)
        {
            ImGui::SameLine();
            if (ImGui::GetCursorPosX() + size.x > line_end)
            {
                ImGui::NewLine();
            }
        }
        first = false;
        const ImVec2 min = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton(chip.label, size))
        {
            *chip.value = !*chip.value;
        }
        const bool hovered = ImGui::IsItemHovered();
        const bool on = *chip.value;
        const ImVec2 max(min.x + size.x, min.y + size.y);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(min, max, u32(on ? p.accent_soft : (hovered ? p.control_hover : p.control)), height * 0.5f);
        if (on)
        {
            draw->AddRect(min, max, u32(p.accent), height * 0.5f);
        }
        centred_text(draw, min, max, u32(on ? p.text : p.text_dim), chip.label);
    }
}

bool button(const char* label, Tone tone, float width)
{
    const Palette& p = pal();
    int colours = 0;
    if (tone == Tone::accent)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, p.accent);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, p.accent_hover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, p.accent);
        ImGui::PushStyleColor(ImGuiCol_Text, p.on_accent);
        colours = 4;
    }
    else if (tone == Tone::danger)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, p.danger);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, with_alpha(p.danger, 0.18f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, with_alpha(p.danger, 0.28f));
        colours = 3;
    }
    const bool pressed = ImGui::Button(label, ImVec2(width, 0.0f));
    ImGui::PopStyleColor(colours);
    return pressed;
}

void help_marker(const char* text)
{
    // Always used on a line of framed controls (rows, titles, after a button): centred on the frame height.
    const float size = help_marker_width();
    const float frame = ImGui::GetFrameHeight();
    const ImVec2 min = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(size, frame));
    const bool hovered = ImGui::IsItemHovered();
    const ImU32 colour = u32(hovered ? pal().text : pal().text_faint);
    const ImVec2 centre(min.x + size * 0.5f, min.y + frame * 0.5f);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddCircle(centre, size * 0.5f, colour, 0, 1.2f);
    const float font = ImGui::GetFontSize() * 0.72f;
    const ImVec2 mark = ImGui::GetFont()->CalcTextSizeA(font, 100.0f, 0.0f, "?");
    draw->AddText(ImGui::GetFont(), font, ImVec2(centre.x - mark.x * 0.5f, centre.y - mark.y * 0.5f), colour, "?");
    if (hovered)
    {
        tooltip(text);
    }
}

void hint(const char* text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, pal().text_dim);
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

void notice(const char* text, Notice kind)
{
    const Palette& p = pal();
    const ImVec4 colour = kind == Notice::danger ? p.danger : (kind == Notice::warn ? p.warn : p.accent);
    const float padding = scaled(kNoticePadding);
    const float bar = scaled(kNoticeBar);
    const float width = ImGui::GetContentRegionAvail().x;
    const float wrap = std::max(width - 2.0f * padding - bar, 1.0f);
    const ImVec2 text_size = ImGui::CalcTextSize(text, nullptr, false, wrap);
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 max(min.x + width, min.y + text_size.y + 2.0f * padding);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const float rounding = ImGui::GetStyle().FrameRounding;
    draw->AddRectFilled(min, max, u32(with_alpha(colour, 0.10f)), rounding);
    draw->AddRectFilled(min, ImVec2(min.x + bar, max.y), u32(colour), rounding, ImDrawFlags_RoundCornersLeft);
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(min.x + bar + padding, min.y + padding),
                  u32(p.text), text, nullptr, wrap);
    ImGui::Dummy(ImVec2(width, max.y - min.y));
}

void pill(const char* text, const ImVec4& colour)
{
    const ImVec2 padding(scaled(kPillPaddingX), scaled(kPillPaddingY));
    const ImVec2 text_size = ImGui::CalcTextSize(text);
    const ImVec2 size(text_size.x + 2.0f * padding.x, text_size.y + 2.0f * padding.y);
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 max(min.x + size.x, min.y + size.y);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(min, max, u32(with_alpha(colour, 0.12f)), size.y * 0.5f);
    draw->AddRect(min, max, u32(with_alpha(colour, 0.6f)), size.y * 0.5f);
    draw->AddText(ImVec2(min.x + padding.x, min.y + padding.y), u32(colour), text);
    ImGui::Dummy(size);
}

void dot(const ImVec4& colour)
{
    const float radius = scaled(kDotRadius);
    const float frame = ImGui::GetFrameHeight();
    const ImVec2 min = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(2.0f * radius, frame));
    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(min.x + radius, min.y + frame * 0.5f), radius, u32(colour));
}

void image_rounded(ImTextureData* texture, float size, float rounding)
{
    const ImVec2 min = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(size, size));
    if (texture == nullptr)
    {
        return;
    }
    ImGui::GetWindowDrawList()->AddImageRounded(texture->GetTexRef(), min, ImVec2(min.x + size, min.y + size),
                                                ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32_WHITE, rounding);
}
} // namespace ui::widgets
