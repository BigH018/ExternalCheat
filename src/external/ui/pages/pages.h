#pragma once

// One function per sidebar page, each in its own file in ui/pages/. Pages edit app::AppState; they never touch game
// memory (app/frame applies changes on the next frame).

#include <imgui.h>

#include "app/state.h"
#include "ui/menu.h"
#include "ui/theme.h"

namespace ui::pages
{
struct PageContext
{
    const Fonts& fonts;
    app::AppState& app;
    ImTextureData* logo; // may be null
    MenuState& menu;
};

void draw_home(PageContext& ctx);
void draw_aimbot(PageContext& ctx);
void draw_triggerbot(PageContext& ctx);
void draw_esp(PageContext& ctx);
void draw_misc(PageContext& ctx);
void draw_keybinds(PageContext& ctx);
void draw_settings(PageContext& ctx);
} // namespace ui::pages
