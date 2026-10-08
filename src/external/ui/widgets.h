#pragma once

// The menu's building blocks (Phase 10 design). Every page is made of panels laid out in two columns; inside a panel
// every setting is a row: its label (and a "?" with help) on the left, its control in a right-hand column that lines
// up in every row of the panel. Sizes go through ui::scaled(), colours through ui::palette().

#include <initializer_list>
#include <span>

#include <imgui.h>

#include "color.h"
#include "ui/theme.h"

namespace ui::widgets
{
// --- Layout --------------------------------------------------------------------------------------------------------

// Two columns of panels when the page is wide enough, one otherwise. Construct, put the left column's panels, call
// next(), put the right column's; the destructor closes the layout. Not nestable.
class Columns
{
public:
    Columns();
    ~Columns();
    Columns(const Columns&) = delete;
    Columns& operator=(const Columns&) = delete;

    void next();

private:
    bool two_ = false;
};

// One dimmed line under the tab bar saying what the page is for.
void page_intro(const char* text);

// A panel: a rounded box with a title row. `enabled` puts an on/off switch at the right of the title (the feature's
// main switch); `help` a "?" after the title. The title is also the panel's ID. Always pair with panel_end().
void panel_begin(const Fonts& fonts, const char* title, bool* enabled = nullptr, const char* help = nullptr);
void panel_end();

// A small dimmed heading between groups of rows inside a panel.
void subheading(const char* text);

// --- Rows ----------------------------------------------------------------------------------------------------------

// Starts a row: the label (and its help) on the left, then the cursor moves to the control column. Returns the width
// the control may take. A label too long for the label column puts the control on the next line, full width.
float row(const char* label, const char* help = nullptr);

// Clicking the label flips the switch too.
bool switch_row(const char* label, bool* value, const char* help = nullptr);
bool slider_row(const char* label, float* value, float min, float max, const char* format, const char* help = nullptr);
bool slider_row(const char* label, int* value, int min, int max, const char* format, const char* help = nullptr);
// Segmented buttons when the names fit the control column, a dropdown otherwise.
bool choice_row(const char* label, int* index, std::span<const char* const> names, const char* help = nullptr);
bool colour_row(const char* label, Color& value, bool alpha = true, const char* help = nullptr);
// "label ......... value", the value right-aligned (dimmed label).
void info_row(const char* label, const char* value);
void info_row(const char* label, const char* value, const ImVec4& value_colour);

// --- Controls ------------------------------------------------------------------------------------------------------

// An animated on/off switch at the cursor. Returns true when flipped.
bool toggle_switch(const char* id, bool* value);
[[nodiscard]] float switch_width();

// Side-by-side buttons, one per name, the selected one filled with the accent.
bool segmented(const char* id, int* index, std::span<const char* const> names, float width);

// Toggle chips that wrap onto new lines ("Pistols", "SMGs"...): filled while on.
struct Chip
{
    const char* label;
    bool* value;
};
void chips(std::initializer_list<Chip> items);

enum class Tone
{
    normal,
    accent, // filled with the accent: the main action
    danger, // red text: deletes or exits
};
bool button(const char* label, Tone tone = Tone::normal, float width = 0.0f);

// A small "?" in a circle; hovering it shows `text`.
void help_marker(const char* text);

// Dimmed, wrapped explanatory text.
void hint(const char* text);

enum class Notice
{
    info,
    warn,
    danger,
};
// A message box with a coloured bar on the left.
void notice(const char* text, Notice kind);

// A small rounded label ("External") in `colour`.
void pill(const char* text, const ImVec4& colour);

// A coloured status dot on the current line.
void dot(const ImVec4& colour);

// An image with rounded corners at the cursor (the logo). Only reserves the space if `texture` is null.
void image_rounded(ImTextureData* texture, float size, float rounding);
} // namespace ui::widgets
