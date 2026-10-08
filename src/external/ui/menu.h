#pragma once

// The menu window (Phase 10 design): a header (logo, name, the edition pill, a close button), a row of tabs with icons
// (a dot on the tabs whose feature is on) and the current page, laid out in two columns of panels. Drawn only while
// the menu is open. The menu never touches game memory: it edits app::AppState and app/frame applies it.

#include <array>
#include <string>

#include <imgui.h>

#include "app/state.h"
#include "ui/theme.h"

namespace ui
{
enum class Page
{
    Home,
    Aimbot,
    Triggerbot,
    Esp,
    Misc,
    Keybinds,
    Settings,
};

// UI-side state that survives between frames (which page is selected). Main thread only.
struct MenuState
{
    Page page = Page::Home;

    // Settings page, Profiles card.
    std::string selected_profile;        // in the list (empty = the current profile)
    std::string confirm_delete;          // "Really delete?" is showing for this profile
    std::array<char, 64> profile_name{}; // the name box (Save as, Rename)

    // Settings page, Exit card.
    bool confirm_exit = false; // "Really exit?" is showing (only with unsaved changes)
};

// `logo` may be null (then the logo spots stay empty).
void draw_menu(MenuState& state, const Fonts& fonts, ImTextureData* logo, app::AppState& app);
} // namespace ui
