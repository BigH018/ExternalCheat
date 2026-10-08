#pragma once

// Keybind controls shared by the Keybinds page and the feature pages. They edit app.settings.keybinds and start or
// cancel app.capture; app/frame polls the keys and stores a captured key.

#include "app/state.h"
#include "input/actions.h"

namespace ui::keybind
{
// The bound key, `width` wide. Click: "Press a key..." (the next key or mouse button becomes the bind, Esc clears it,
// clicking again cancels). Red with a tooltip when the same key is bound to other actions.
void key_button(app::AppState& app, input::ActionId id, float width);

// Hold | Toggle as segmented buttons, for actions that allow both (nothing for press-only actions).
void mode_selector(app::AppState& app, input::ActionId id, float width);

// A row: the label on the left, the key button (and the mode) in the control column.
void bind_row(app::AppState& app, input::ActionId id, const char* label, const char* help = nullptr);
} // namespace ui::keybind
