#include <cstddef>
#include <string>
#include <vector>

#include <imgui.h>

#include "input/actions.h"
#include "input/keybinds.h"
#include "input/keys.h"
#include "ui/keybind_widgets.h"
#include "ui/pages/pages.h"
#include "ui/widgets.h"

// Keybinds page: every action from the registry (input/actions), grouped by category, with its key, its mode and a
// warning for keys bound to several actions. Built from the registry, so a new action shows up here with no UI work.

namespace ui::pages
{
namespace
{
void draw_conflicts(const app::AppState& app)
{
    const std::vector<input::Conflict> conflicts = input::find_conflicts(app.settings.keybinds.binds);
    if (conflicts.empty())
    {
        return;
    }
    std::string text = "One key, several actions (each of them fires):";
    for (const input::Conflict& conflict : conflicts)
    {
        text += "\n" + input::key_name(conflict.key) + ": ";
        for (std::size_t i = 0; i < conflict.actions.size(); ++i)
        {
            text += (i == 0 ? "" : ", ") + std::string(input::action(conflict.actions[i]).label);
        }
    }
    widgets::notice(text.c_str(), widgets::Notice::danger);
    ImGui::Spacing();
}

void draw_category(PageContext& ctx, input::Category category)
{
    const std::string title(input::category_name(category));
    widgets::panel_begin(ctx.fonts, title.c_str());
    for (const input::ActionDef& def : input::actions())
    {
        if (def.category == category)
        {
            const std::string label(def.label);
            keybind::bind_row(ctx.app, def.id, label.c_str());
        }
    }
    widgets::panel_end();
}
} // namespace

void draw_keybinds(PageContext& ctx)
{
    widgets::page_intro("Every action you can put on a key or mouse button. Keys work while the game is in front.");
    app::AppState& app = ctx.app;
    if (app.menu_key_failed)
    {
        widgets::notice("The menu key couldn't be registered: another program (or Windows) uses it as a hotkey. Pick "
                        "another one below.",
                        widgets::Notice::warn);
        ImGui::Spacing();
    }
    draw_conflicts(app);
    {
        // Alternate the categories between the columns, in registry order.
        widgets::Columns columns;
        for (std::size_t i = 0; i < input::kCategories.size(); i += 2)
        {
            draw_category(ctx, input::kCategories[i]);
        }
        columns.next();
        for (std::size_t i = 1; i < input::kCategories.size(); i += 2)
        {
            draw_category(ctx, input::kCategories[i]);
        }
    }
    widgets::hint("Click a key button, then press any key or mouse button (Esc clears it, clicking again cancels, after "
                  "6 seconds it gives up). For Mouse 1, click outside the menu. Hold = on while held, Toggle = each "
                  "press switches it, the rest fire once per press. While this menu is open only the menu key, panic "
                  "and exit work.");
    if (widgets::button("Reset every keybind", widgets::Tone::danger))
    {
        app.capture.cancel();
        app.settings.keybinds = settings::KeybindSettings{};
    }
    ImGui::SameLine();
    widgets::help_marker("Back to the defaults: INSERT menu, END panic, DELETE exit, Mouse 1 aim (hold), Mouse 4 "
                         "trigger (hold), every on / off key unbound.");
}
} // namespace ui::pages
