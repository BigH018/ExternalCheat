#include <format>
#include <string>

#include <imgui.h>

#include "config.h"
#include "features/feature_summary.h"
#include "game/weapon.h"
#include "input/keys.h"
#include "settings/presets.h"
#include "ui/pages/pages.h"
#include "ui/widgets.h"

// Home: the dashboard. The match status, every feature's main switch, the presets, the match and the tool's status,
// and the keys that matter. (The logo and name are in the menu's header.)

namespace ui::pages
{
namespace
{
std::string module_text(const core::ModuleInfo& module)
{
    return std::format("0x{:X}", module.base);
}

std::string key_of(const app::AppState& app, input::ActionId id)
{
    return input::key_name(app.settings.keybinds.bind(id).key);
}

// One line under the name: where the tool stands, with a coloured dot.
void match_line(const app::AppState& app, std::string& text, ImVec4& colour)
{
    const Palette& p = palette();
    if (!app.pawn_read_ok)
    {
        text = "Can't read the local player";
        colour = p.warn;
    }
    else if (!app.in_match())
    {
        text = "Not in a match: join Practice with Bots";
        colour = p.text_faint;
    }
    else
    {
        const game::GameSnapshot& snapshot = app.snapshot;
        const std::string map = snapshot.globals.map_name.empty() ? "a match" : snapshot.globals.map_name;
        text = std::format("In a match on {}  ·  {} players", map, snapshot.players.size());
        colour = p.ok;
    }
}

// The page's first line: the match status with a coloured dot.
void draw_status_line(const app::AppState& app)
{
    std::string status;
    ImVec4 colour;
    match_line(app, status, colour);
    ImGui::AlignTextToFramePadding();
    widgets::dot(colour);
    ImGui::SameLine(0.0f, scaled(8.0f));
    ImGui::TextColored(palette().text_dim, "%s", status.c_str());
    ImGui::Dummy(ImVec2(0.0f, scaled(2.0f)));
}

void draw_features(PageContext& ctx)
{
    settings::Settings& s = ctx.app.settings;
    widgets::panel_begin(ctx.fonts, "Features", nullptr,
                         "Every feature's main switch. Their options are on their own tabs; a green dot on a tab means "
                         "that feature is on.");
    widgets::subheading("COMBAT");
    widgets::switch_row("Aimbot", &s.aimbot.enabled, "Turns your view towards a bot while the aim key is held.");
    widgets::switch_row("Triggerbot", &s.triggerbot.enabled, "Fires while an enemy is under your crosshair.");
    widgets::subheading("VISUALS");
    widgets::switch_row("ESP", &s.esp.enabled, "Boxes, names and health over the bots, through walls.");
    widgets::switch_row("Radar", &s.radar.enabled, "Our own radar in a corner of the screen.");
    widgets::switch_row("Bomb timer", &s.bomb_timer.enabled, "A countdown while a bomb is planted.");
    widgets::switch_row("Spectator list", &s.spectators.enabled, "Who is watching you.");
    widgets::panel_end();
}

void draw_presets(PageContext& ctx)
{
    widgets::panel_begin(ctx.fonts, "Presets", nullptr,
                         "A preset switches the features and sets their strength. Keybinds, colours, team mode, team "
                         "checks, max distances and where things sit stay as they are. Save the profile (Settings) to "
                         "keep the result. Keys: Keybinds tab, Presets.");
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float count = static_cast<float>(settings::kPresets.size());
    const float width = (ImGui::GetContentRegionAvail().x - spacing * (count - 1.0f)) / count;
    bool first = true;
    for (const settings::Preset preset : settings::kPresets)
    {
        if (!first)
        {
            ImGui::SameLine();
        }
        first = false;
        const std::string label = std::string(settings::preset_name(preset)) + "##preset";
        const widgets::Tone tone = preset == settings::Preset::rage ? widgets::Tone::danger : widgets::Tone::normal;
        if (widgets::button(label.c_str(), tone, width))
        {
            ctx.app.requests.preset = preset;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", settings::preset_description(preset).data());
        }
    }
    widgets::panel_end();
}

void draw_match(PageContext& ctx)
{
    const app::AppState& app = ctx.app;
    const Palette& p = palette();
    widgets::panel_begin(ctx.fonts, "Match");
    if (!app.in_match())
    {
        widgets::hint(app.pawn_read_ok ? "Not in a match. Start a Practice with Bots match (offline, -insecure)."
                                       : "The local player couldn't be read: is the game still loading?");
        widgets::panel_end();
        return;
    }
    const game::GameSnapshot& snapshot = app.snapshot;
    widgets::info_row("Map", snapshot.globals.map_name.empty() ? "?" : snapshot.globals.map_name.c_str());
    int alive = 0;
    for (const game::PlayerSnapshot& player : snapshot.players)
    {
        alive += player.alive ? 1 : 0;
    }
    widgets::info_row("Players", std::format("{} ({} alive)", snapshot.players.size(), alive).c_str());
    if (const game::PlayerSnapshot* local = snapshot.local(); local != nullptr)
    {
        widgets::info_row("You", std::format("{}  ({})", local->name, game::team_short_name(local->team)).c_str());
        if (local->alive)
        {
            const std::string_view weapon =
                local->weapon_id ? game::weapon_info(*local->weapon_id).name : std::string_view{};
            widgets::info_row("Health", std::to_string(local->health).c_str(), local->health > 30 ? p.ok : p.warn);
            widgets::info_row("Weapon", weapon.empty() ? "-" : std::string(weapon).c_str());
        }
        else
        {
            widgets::info_row("Health", "dead", p.text_faint);
        }
    }
    widgets::panel_end();
}

void draw_tool(PageContext& ctx)
{
    const app::AppState& app = ctx.app;
    const Palette& p = palette();
    widgets::panel_begin(ctx.fonts, "Tool");
    const app::OffsetReport& offsets = app.offsets;
    if (offsets.ok())
    {
        widgets::info_row("Offsets", std::format("build {}, {} checks OK", offsets.game_build, offsets.checks).c_str(),
                          p.ok);
    }
    else if (offsets.ran)
    {
        widgets::info_row("Offsets",
                          std::format("{} of {} failed (console)", offsets.failures, offsets.checks).c_str(), p.warn);
    }
    if (app.unsaved_changes())
    {
        widgets::info_row("Profile", std::format("{} (unsaved changes)", app.profiles.current).c_str(), p.warn);
    }
    else
    {
        widgets::info_row("Profile", app.profiles.current.c_str());
    }
    const std::string active = features::feature_summary(app.active);
    widgets::info_row("Active", active.empty() ? "nothing" : active.c_str(), active.empty() ? p.text_faint : p.text);
    widgets::info_row("Overlay",
                      std::format("{} x {}, {:.0f} FPS", app.overlay_width, app.overlay_height, app.fps).c_str());
    widgets::info_row("Game", std::format("cs2.exe, PID {}", app.game.pid).c_str());
    widgets::info_row("client.dll", module_text(app.game.client).c_str(), p.text_dim);
    widgets::info_row("engine2.dll", module_text(app.game.engine).c_str(), p.text_dim);
    widgets::panel_end();
}

void draw_keys(PageContext& ctx)
{
    const app::AppState& app = ctx.app;
    widgets::panel_begin(ctx.fonts, "Keys", nullptr,
                         "While the menu is open the game doesn't get your mouse or keyboard, so clicks here never "
                         "shoot. The overlay hides when the game loses focus (Alt+Tab). Every key can be changed on the "
                         "Keybinds tab.");
    widgets::info_row("Open / close the menu", key_of(app, input::ActionId::menu_toggle).c_str());
    widgets::info_row("Panic (every feature off)", key_of(app, input::ActionId::panic).c_str());
    widgets::info_row("Exit the tool", key_of(app, input::ActionId::exit).c_str());
    widgets::hint("Exit also: Settings tab, Ctrl+C in the console, or Alt+F4 with the menu open. The game keeps "
                  "running.");
    widgets::panel_end();
}
} // namespace

void draw_home(PageContext& ctx)
{
    draw_status_line(ctx.app);
    if (ctx.app.focus_warning)
    {
        widgets::notice("The menu couldn't take focus from the game: click it once so the game lets go of the mouse.",
                        widgets::Notice::warn);
        ImGui::Spacing();
    }
    widgets::Columns columns;
    draw_features(ctx);
    draw_presets(ctx);
    columns.next();
    draw_match(ctx);
    draw_tool(ctx);
    draw_keys(ctx);
}
} // namespace ui::pages
