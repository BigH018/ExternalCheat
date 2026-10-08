#include "app/frame.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>

#include "app/state.h"
#include "config.h"
#include "core/log.h"
#include "core/runtime.h"
#include "features/aimbot.h"
#include "features/bomb_timer.h"
#include "features/esp.h"
#include "features/radar.h"
#include "features/spectators.h"
#include "features/triggerbot.h"
#include "game/bomb.h"
#include "game/handle.h"
#include "game/offsets.h"
#include "game/player.h"
#include "game/writes.h"
#include "input/actions.h"
#include "input/key_poll.h"
#include "input/keybinds.h"
#include "input/keys.h"
#include "maths/vec.h"
#include "render/painter.h"
#include "render/primitives.h"
#include "settings/presets.h"
#include "settings/profile_store.h"
#include "ui/hud.h"
#include "ui/imgui_layer.h"
#include "ui/menu.h"
#include "ui/overlay_window.h"

namespace app
{
namespace
{
// The game window's client area in screen coordinates. false if it has none (minimised, zero-sized).
bool client_area(HWND window, RECT& area) noexcept
{
    RECT client{};
    POINT top_left{0, 0};
    if (!GetClientRect(window, &client) || !ClientToScreen(window, &top_left))
    {
        return false;
    }
    area = RECT{top_left.x, top_left.y, top_left.x + client.right, top_left.y + client.bottom};
    return client.right > 0 && client.bottom > 0;
}

// <folder of cs2_external.exe>\profiles.
std::filesystem::path profiles_folder()
{
    std::wstring path(MAX_PATH, L'\0');
    for (;;)
    {
        const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0)
        {
            return std::filesystem::path(config::kProfilesFolder); // next to wherever we were started from
        }
        if (length < path.size())
        {
            path.resize(length);
            break;
        }
        path.resize(path.size() * 2); // truncated: the exe's path is longer than MAX_PATH
    }
    return std::filesystem::path(path).parent_path() / config::kProfilesFolder;
}

std::string utf8(const std::filesystem::path& path)
{
    const std::u8string text = path.u8string();
    return std::string(text.begin(), text.end());
}

// Profile names are file names: Windows doesn't tell "Rage" and "rage" apart.
bool same_profile(const std::string& a, const std::string& b)
{
    const std::optional<std::string> clean = settings::ProfileStore::clean_name(a);
    return clean && clean->size() == b.size() &&
           std::equal(clean->begin(), clean->end(), b.begin(), [](char x, char y) {
               return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
           });
}

// Each preset's key (Keybinds page, Presets).
constexpr std::pair<input::ActionId, settings::Preset> kPresetActions[] = {
    {input::ActionId::preset_off, settings::Preset::off},
    {input::ActionId::preset_chill, settings::Preset::chill},
    {input::ActionId::preset_medium, settings::Preset::medium},
    {input::ActionId::preset_rage, settings::Preset::rage},
};

class Runner
{
public:
    explicit Runner(const Context& ctx) : ctx_(ctx)
    {
        state_.game = GameInfo{ctx.pid, ctx.client, ctx.engine};
        state_.offsets = ctx.offsets;
    }

    int run()
    {
        load_startup_profile(); // before anything reads the settings (the keys logged below come from it)
        if (!overlay_.create(&ui::imgui_message_hook) ||
            !imgui_.init(overlay_.hwnd(), overlay_.device(), overlay_.context()))
        {
            return 1;
        }
        const settings::KeybindSettings& keys = state_.settings.keybinds;
        logger::info("Overlay running. {} opens the menu, {} is panic. Exit: {}, the Exit button (Settings), Ctrl+C "
                     "here (or close this window), or Alt+F4 while the menu is open.",
                     input::key_name(menu_key()), input::key_name(keys.bind(input::ActionId::panic).key),
                     input::key_name(keys.bind(input::ActionId::exit).key));

        int exit_code = 1;
        try
        {
            exit_code = loop();
        }
        catch (const std::exception& e)
        {
            // Still shut down properly below: the attack button must not stay pressed in the game.
            logger::error("The overlay stopped on an error: {}", e.what());
        }

        stop_features(); // let go of attack if the triggerbot holds it
        if (state_.menu_open)
        {
            close_menu(true);
        }
        imgui_.shutdown(); // before the overlay releases the D3D device ImGui's objects live on
        overlay_.destroy();
        logger::info("Overlay removed");
        return exit_code;
    }

private:
    // Frames until something ends the run. Returns the exit code.
    int loop()
    {
        while (!core::shutdown_requested.load())
        {
            const ui::OverlayWindow::Events events = overlay_.pump();
            if (events.close_requested)
            {
                exit_requested_ = "Overlay closed (Alt+F4)";
            }
            if (!exit_requested_ && !core::is_running(ctx_.process))
            {
                logger::info("cs2.exe has closed: exiting");
                return 0;
            }
            if (!exit_requested_ && !frame(events))
            {
                return 1;
            }
            if (exit_requested_)
            {
                // You asked for it: no "Press Enter to exit" in the console afterwards (main.cpp).
                logger::info("{}: exiting", exit_requested_);
                core::shutdown_requested = true;
                return 0;
            }
        }
        return 0;
    }

    // One frame. false = the overlay can't go on (device lost).
    bool frame(const ui::OverlayWindow::Events& events)
    {
        state_.now_ms = GetTickCount64();
        if (std::exchange(state_.requests.exit, false))
        {
            exit_requested_ = "Exit button";
            return true;
        }
        // Taken every frame, also while hidden: presses made while we aren't listening are dropped, not saved up.
        const input::PressCounts presses = overlay_.take_presses();
        if (!find_game_window())
        {
            hide();
            return true;
        }

        const HWND foreground = GetForegroundWindow();
        const bool game_focused = foreground == game_window_;
        const bool overlay_focused = foreground == overlay_.hwnd();
        // While a bind is being captured the menu key is just a key (it may be the one being bound).
        const bool capturing = state_.capture.active();
        overlay_.enable_menu_hotkey((game_focused || overlay_focused) && !capturing, menu_key());
        state_.menu_key_failed = overlay_.menu_hotkey_failed();

        // The menu's close button works like the menu key.
        const bool close_clicked = std::exchange(state_.requests.close_menu, false);
        const bool menu_key_pressed = (events.menu_key && !capturing) || (close_clicked && state_.menu_open);
        if (menu_key_pressed && state_.menu_open)
        {
            close_menu(true);
        }
        else if (menu_key_pressed)
        {
            open_menu();
        }
        else if (state_.menu_open && overlay_had_focus_ && !overlay_focused)
        {
            close_menu(false); // focus moved on (Alt+Tab, a click on the game's title bar): leave it there
        }
        overlay_had_focus_ = GetForegroundWindow() == overlay_.hwnd();

        RECT area{};
        const bool ours = game_focused || overlay_focused || state_.menu_open;
        if (!ours || IsIconic(game_window_) || !client_area(game_window_, area))
        {
            hide();
            return true;
        }
        follow(area);
        run_keybinds(read_keys(game_focused || overlay_focused, presses)); // may turn features off or ask to exit
        if (exit_requested_)
        {
            return true;
        }
        run_requests(); // profile operations and presets the menu (or a preset key) asked for
        const settings::Settings& settings = state_.settings;
        state_.active.esp = settings.esp.enabled;
        state_.active.aimbot = settings.aimbot.enabled;
        state_.active.triggerbot = settings.triggerbot.enabled;
        state_.active.radar = settings.radar.enabled;
        state_.active.bomb_timer = settings.bomb_timer.enabled;
        state_.active.spectator_list = settings.spectators.enabled;
        read_game();

        // Aiming and firing only while you're playing: the game in front, the menu closed.
        const bool playing = game_focused && !state_.menu_open;
        const float frame_seconds = frame_time();
        run_aimbot(playing, frame_seconds);
        run_triggerbot(playing);

        imgui_.begin_frame();
        ui::use_theme(state_.settings.overlay); // the profile's theme and accent, before anything is drawn
        state_.fps = imgui_.framerate();
        draw_world();
        ui::draw_hud(imgui_.fonts(), imgui_.logo(), state_);
        if (state_.menu_open)
        {
            ui::draw_menu(menu_, imgui_.fonts(), imgui_.logo(), state_);
        }
        overlay_.begin_frame();
        imgui_.end_frame();
        return overlay_.present();
    }

    bool find_game_window()
    {
        if (game_window_ != nullptr && IsWindow(game_window_))
        {
            return true;
        }
        if (game_window_ != nullptr)
        {
            logger::warn("The game window went away; waiting for a new one");
            game_window_ = nullptr;
            if (state_.menu_open)
            {
                close_menu(false);
            }
        }
        game_window_ = core::find_main_window(ctx_.pid);
        if (game_window_ != nullptr)
        {
            logger::info("Game window 0x{:X}", reinterpret_cast<std::uintptr_t>(game_window_));
            waiting_logged_ = false;
        }
        else if (!waiting_logged_)
        {
            logger::info("Waiting for the game window...");
            waiting_logged_ = true;
        }
        return game_window_ != nullptr;
    }

    // The game lost focus or is minimised: nothing on screen, and nothing to do but wait.
    void hide()
    {
        stop_features();
        if (state_.menu_open)
        {
            close_menu(false);
        }
        overlay_.set_visible(false);
        overlay_.enable_menu_hotkey(false, menu_key());
        overlay_.wait(config::kHiddenPollIntervalMs);
    }

    void follow(const RECT& area)
    {
        if (!EqualRect(&area, &last_area_))
        {
            last_area_ = area;
            state_.overlay_width = area.right - area.left;
            state_.overlay_height = area.bottom - area.top;
            logger::info("Overlay covers {}x{} at ({}, {})", state_.overlay_width, state_.overlay_height, area.left,
                         area.top);
        }
        overlay_.cover(area);
        overlay_.set_visible(true);
    }

    // The game snapshot: every frame while a feature draws from it, otherwise at ~4 Hz for the Home page.
    void read_game()
    {
        const std::uint64_t now = GetTickCount64();
        const bool every_frame = state_.active.esp || state_.active.aimbot || state_.active.triggerbot ||
                                 state_.active.radar || state_.active.bomb_timer || state_.active.spectator_list;
        if (!every_frame && now < next_status_ms_)
        {
            return;
        }
        next_status_ms_ = now + config::kStatusIntervalMs;
        const bool was_in_match = state_.in_match();
        const auto pawn = game::read_local_pawn(ctx_.memory, ctx_.client.base);
        state_.pawn_read_ok = pawn.has_value();
        state_.local_pawn = pawn.value_or(0);
        state_.snapshot = game::read_game(ctx_.memory, ctx_.client.base);
        if (state_.active.bomb_timer)
        {
            if (const auto entity_system = game::read_entity_system(ctx_.memory, ctx_.client.base))
            {
                state_.snapshot.bomb = game::read_bomb(ctx_.memory, ctx_.client.base, *entity_system);
            }
        }
        if (state_.in_match() != was_in_match)
        {
            if (state_.in_match())
            {
                logger::info("In a match (local pawn 0x{:X})", state_.local_pawn);
            }
            else
            {
                logger::info("Not in a match");
            }
        }
    }

    [[nodiscard]] std::uint32_t menu_key() const noexcept
    {
        return state_.settings.keybinds.bind(input::ActionId::menu_toggle).key;
    }

    // This frame's keys, only while the game or the overlay is in front (GetAsyncKeyState and raw input see every
    // program): held keys from polling, presses from raw input (every tap, however short the frame), or from comparing
    // two polls if raw input isn't available.
    input::KeyFrame read_keys(bool focused, const input::PressCounts& raw_presses)
    {
        input::KeyFrame keys;
        if (!focused)
        {
            return keys;
        }
        keys.down = input::poll_keys();
        keys.presses = overlay_.raw_input() ? raw_presses : input::presses_from_edges(previous_down_, keys.down);
        previous_down_ = keys.down;
        return keys;
    }

    // Feed a running bind capture, then the keybind engine, then act on what fired. While a capture runs nothing
    // fires; while the menu is open only panic and exit do. The menu key itself is the overlay's hotkey (frame()), not
    // an engine action.
    void run_keybinds(const input::KeyFrame& keys)
    {
        const bool was_capturing = state_.capture.active();
        if (was_capturing)
        {
            input::KeySet down = keys.down_or_pressed();
            if (ImGui::GetIO().WantCaptureMouse)
            {
                down.reset(input::kVkMouse1); // a click in the menu is a click: Mouse 1 is bound outside the menu
            }
            if (const auto result = state_.capture.update(down, state_.now_ms))
            {
                state_.settings.keybinds.bind(result->action).key = result->key;
                logger::info("{}: {}", input::action(result->action).label, input::key_name(result->key));
            }
        }
        // The frame a capture ends still counts as capturing, so the new key doesn't fire its action straight away.
        const input::Suspension suspension = was_capturing      ? input::Suspension::capture
                                             : state_.menu_open ? input::Suspension::menu_open
                                                                : input::Suspension::none;
        actions_ = keybinds_.update(keys, state_.settings.keybinds.binds, suspension);
        handle_actions();
    }

    void handle_actions()
    {
        if (actions_.did_fire(input::ActionId::exit))
        {
            exit_requested_ = "Exit key";
            return;
        }
        if (actions_.did_fire(input::ActionId::panic))
        {
            panic();
            return;
        }
        settings::Settings& settings = state_.settings;
        flip(input::ActionId::aimbot_enable, settings.aimbot.enabled, "Aimbot");
        flip(input::ActionId::triggerbot_enable, settings.triggerbot.enabled, "Triggerbot");
        flip(input::ActionId::esp_enable, settings.esp.enabled, "ESP");
        flip(input::ActionId::radar_enable, settings.radar.enabled, "Radar");
        flip(input::ActionId::bomb_timer_enable, settings.bomb_timer.enabled, "Bomb timer");
        flip(input::ActionId::spectators_enable, settings.spectators.enabled, "Spectator list");
        for (const auto& [id, preset] : kPresetActions)
        {
            if (actions_.did_fire(id))
            {
                state_.requests.preset = preset;
            }
        }
    }

    // --- Profiles and presets (Phase 8) ----------------------------------------------------------------------------

    // The last used profile (or "default") becomes the settings.
    void load_startup_profile()
    {
        profile_store_.emplace(profiles_folder());
        state_.profiles.folder = utf8(profile_store_->folder());
        use_profile(profile_store_->load_startup());
        refresh_profile_list();
        logger::info("Profile \"{}\" loaded ({} warnings) from {}", state_.profiles.current,
                     state_.profiles.warnings.size(), state_.profiles.folder);
    }

    // Make `loaded` the current profile: its settings apply now and are remembered as saved.
    void use_profile(settings::LoadedProfile loaded)
    {
        for (const std::string& warning : loaded.warnings)
        {
            logger::warn("Profile {}: {}", loaded.name, warning);
        }
        state_.settings = loaded.settings;
        state_.profiles.saved = std::move(loaded.settings);
        state_.profiles.current = std::move(loaded.name);
        state_.profiles.warnings = std::move(loaded.warnings);
        settings_replaced();
    }

    // The whole settings changed at once (a profile, a reset): no toggle key stays on, no capture keeps running.
    void settings_replaced()
    {
        keybinds_.reset_toggles();
        state_.capture.cancel();
    }

    void refresh_profile_list() { state_.profiles.names = profile_store_->list(); }

    // The result line on the Settings page, and the console.
    void report(const settings::ProfileStatus& status, const std::string& success_text)
    {
        state_.profiles.message = status.ok ? success_text : status.message;
        state_.profiles.message_failed = !status.ok;
        if (status.ok)
        {
            logger::info("{}", state_.profiles.message);
        }
        else
        {
            logger::warn("{}", state_.profiles.message);
        }
    }

    void run_requests()
    {
        if (const std::optional<ProfileRequest> request = std::exchange(state_.requests.profile, std::nullopt))
        {
            run_profile_request(*request);
        }
        if (const std::optional<settings::Preset> preset = std::exchange(state_.requests.preset, std::nullopt))
        {
            settings::apply_preset(state_.settings, *preset);
            report(settings::ProfileStatus::success(),
                   std::format("Preset {} applied (Save to keep it)", settings::preset_name(*preset)));
        }
    }

    void run_profile_request(const ProfileRequest& request)
    {
        settings::ProfileStore& store = *profile_store_;
        ProfileState& profiles = state_.profiles;
        switch (request.op)
        {
        case ProfileOp::load:
        {
            settings::LoadedProfile loaded;
            const settings::ProfileStatus status = store.load(request.name, loaded);
            if (!status.ok)
            {
                report(status, {});
                break;
            }
            const std::size_t warnings = loaded.warnings.size();
            use_profile(std::move(loaded));
            store.set_last_profile(profiles.current);
            report(status, warnings == 0 ? std::format("Profile \"{}\" loaded", profiles.current)
                                         : std::format("Profile \"{}\" loaded with {} warnings (listed below)",
                                                       profiles.current, warnings));
            break;
        }
        case ProfileOp::save:
        {
            const settings::ProfileStatus status = store.save(profiles.current, state_.settings);
            if (status.ok)
            {
                profiles.saved = state_.settings;
                store.set_last_profile(profiles.current);
            }
            report(status, std::format("Profile \"{}\" saved", profiles.current));
            break;
        }
        case ProfileOp::save_as:
        {
            const std::optional<std::string> name = settings::ProfileStore::clean_name(request.name);
            const settings::ProfileStatus status =
                name && store.exists(*name)
                    ? settings::ProfileStatus::failure("Profile \"" + *name + "\" already exists: load it and Save")
                    : store.save(request.name, state_.settings);
            if (status.ok)
            {
                profiles.current = *name;
                profiles.saved = state_.settings;
                profiles.warnings.clear();
                store.set_last_profile(profiles.current);
            }
            report(status, std::format("Saved as \"{}\"", profiles.current));
            break;
        }
        case ProfileOp::rename:
        {
            const settings::ProfileStatus status = store.rename(profiles.current, request.name);
            if (status.ok)
            {
                profiles.current = *settings::ProfileStore::clean_name(request.name);
            }
            report(status, std::format("Renamed to \"{}\"", profiles.current));
            break;
        }
        case ProfileOp::remove:
        {
            const settings::ProfileStatus status = store.remove(request.name);
            if (status.ok && same_profile(request.name, profiles.current))
            {
                // The loaded profile is gone: the settings stay, but now belong to nothing saved.
                profiles.current = config::kDefaultProfile;
                profiles.saved = settings::Settings{};
                profiles.warnings.clear();
                store.set_last_profile(config::kDefaultProfile);
            }
            report(status, std::format("Profile \"{}\" deleted", request.name));
            break;
        }
        case ProfileOp::reset:
            state_.settings = settings::Settings{};
            settings_replaced();
            report(settings::ProfileStatus::success(), "Settings reset to the defaults (not saved)");
            break;
        }
        refresh_profile_list();
    }

    // An "on / off" action: each press flips the feature's Enabled switch (two quick presses in one frame: back
    // where it was).
    void flip(input::ActionId id, bool& enabled, const char* name)
    {
        if (actions_.press_count(id) % 2 == 1)
        {
            enabled = !enabled;
            logger::info("{} {}", name, enabled ? "on" : "off");
        }
    }

    // Every feature off, every toggle key off, nothing pressed, the menu closed.
    void panic()
    {
        settings::Settings& settings = state_.settings;
        settings.esp.enabled = false;
        settings.aimbot.enabled = false;
        settings.triggerbot.enabled = false;
        settings.radar.enabled = false;
        settings.bomb_timer.enabled = false;
        settings.spectators.enabled = false;
        keybinds_.reset_toggles();
        actions_ = {};
        // A preset or profile load clicked just before would otherwise switch features straight back on.
        state_.requests.preset.reset();
        if (state_.requests.profile && state_.requests.profile->op == ProfileOp::load)
        {
            state_.requests.profile.reset();
        }
        stop_features();
        if (state_.menu_open)
        {
            close_menu(true);
        }
        logger::info("Panic: every feature off");
    }

    // Seconds since the previous frame (0 on the first), for the aimbot's frame-rate-independent smoothing.
    float frame_time()
    {
        const auto now = std::chrono::steady_clock::now();
        const float seconds = last_frame_ ? std::chrono::duration<float>(now - *last_frame_).count() : 0.0f;
        last_frame_ = now;
        return seconds;
    }

    // One smoothing step towards the target, written to the game's view angles.
    void run_aimbot(bool playing, float frame_seconds)
    {
        const settings::AimbotSettings& aim = state_.settings.aimbot;
        if (!aim.enabled)
        {
            state_.aim_status = {};
            return;
        }
        const bool active = playing && actions_.is_active(input::ActionId::aimbot_activate);
        state_.aim_status = {active, false};
        if (!active)
        {
            return;
        }
        const auto angles =
            features::compute_aim(state_.snapshot, aim, state_.settings.general.team_mode, frame_seconds);
        state_.aim_status.has_target = angles.has_value();
        if (angles && !game::write_view_angles(ctx_.memory, ctx_.client.base, *angles))
        {
            warn_write_failed("view angles");
        }
    }

    void run_triggerbot(bool playing)
    {
        const settings::TriggerbotSettings& trigger = state_.settings.triggerbot;
        if (!trigger.enabled)
        {
            triggerbot_.reset();
            state_.trigger_status = {};
            set_attack(false);
            return;
        }
        const bool allowed = trigger.activation == settings::TriggerActivation::always ||
                             actions_.is_active(input::ActionId::triggerbot_activate);
        const bool active = allowed && playing;
        const features::TriggerBlock block =
            features::trigger_block(state_.snapshot, trigger, state_.settings.general.team_mode);
        state_.trigger_status = {active, block};
        set_attack(triggerbot_.update(GetTickCount64(), active, block == features::TriggerBlock::none, trigger));
    }

    // Presses or releases the game's attack button, only on a change. A release is skipped while you hold the
    // attack key yourself, so the triggerbot never cancels your own shooting.
    void set_attack(bool down)
    {
        if (down == attack_down_)
        {
            return;
        }
        attack_down_ = down;
        if (!down && input::is_key_down(input::kVkMouse1))
        {
            return;
        }
        if (!game::set_button(ctx_.memory, ctx_.client.base, game::offsets::buttons::attack, down))
        {
            warn_write_failed("attack button");
        }
    }

    // The game lost focus, the overlay hides or the tool exits: nothing may stay pressed. Toggle keys keep their state
    // (a toggled-on aim key is still on after Alt+Tab); hold keys read released because nothing is polled.
    void stop_features()
    {
        triggerbot_.reset();
        set_attack(false);
        last_frame_.reset();
    }

    void warn_write_failed(const char* what)
    {
        if (!write_warned_)
        {
            logger::warn("Writing the {} failed (is the handle missing write access?). Further failures aren't logged.",
                         what);
            write_warned_ = true;
        }
    }

    // Under the watermark and the menu, on ImGui's background draw list: the ESP, the aimbot's FOV circle, the radar,
    // the bomb timer and the spectator list.
    void draw_world()
    {
        const maths::Vec2 screen{static_cast<float>(state_.overlay_width), static_cast<float>(state_.overlay_height)};
        const float font_size = ui::scaled(config::kEspFontSize);
        std::vector<render::Primitive> primitives = features::build_esp(
            state_.snapshot, state_.settings.esp, state_.settings.general.team_mode, screen, font_size);
        const settings::AimbotSettings& aim = state_.settings.aimbot;
        if (aim.enabled && aim.draw_fov)
        {
            if (const auto circle = features::fov_circle(state_.snapshot, aim, screen))
            {
                primitives.push_back(*circle);
            }
        }
        const std::vector<render::Primitive> radar =
            features::build_radar(state_.snapshot, state_.settings.radar, state_.settings.general.team_mode, screen);
        primitives.insert(primitives.end(), radar.begin(), radar.end());
        const std::vector<render::Primitive> bomb =
            features::build_bomb_timer(state_.snapshot, state_.settings.bomb_timer, screen, font_size);
        primitives.insert(primitives.end(), bomb.begin(), bomb.end());
        const std::vector<render::Primitive> spectators = features::build_spectator_list(
            state_.snapshot, state_.settings.spectators, state_.settings.general.team_mode, screen, font_size);
        primitives.insert(primitives.end(), spectators.begin(), spectators.end());
        render::paint(*ImGui::GetBackgroundDrawList(), primitives, imgui_.fonts().regular, font_size);
    }

    void open_menu()
    {
        state_.menu_open = true;
        overlay_.set_interactive(true);
        imgui_.set_menu_open(true);
        // Taking focus is what makes the game let go of the mouse: the cursor shows, and clicks and keys come to the
        // menu instead of the game. Allowed because the menu hotkey was input to our process.
        state_.focus_warning = SetForegroundWindow(overlay_.hwnd()) == FALSE;
        if (state_.focus_warning)
        {
            logger::warn("The menu couldn't take focus from the game: click it once");
        }
    }

    void close_menu(bool give_focus_back)
    {
        state_.capture.cancel();
        state_.menu_open = false;
        state_.focus_warning = false;
        imgui_.set_menu_open(false);
        overlay_.set_interactive(false);
        if (give_focus_back && game_window_ != nullptr && IsWindow(game_window_))
        {
            SetForegroundWindow(game_window_);
        }
    }

    const Context& ctx_;
    std::optional<settings::ProfileStore> profile_store_; // profiles next to the exe (load_startup_profile)
    ui::OverlayWindow overlay_;
    ui::ImGuiLayer imgui_;
    AppState state_;
    ui::MenuState menu_;
    HWND game_window_ = nullptr;
    bool waiting_logged_ = false;
    bool overlay_had_focus_ = false;
    RECT last_area_{};
    std::uint64_t next_status_ms_ = 0;
    input::KeybindEngine keybinds_;
    input::KeySet previous_down_; // last frame's held keys (presses without raw input)
    input::ActionStates actions_; // this frame's keybind states
    const char* exit_requested_ = nullptr; // what asked to exit (the exit key, the Exit button, Alt+F4); null = run on
    features::Triggerbot triggerbot_;
    bool attack_down_ = false; // what we last wrote to the attack button
    bool write_warned_ = false;
    std::optional<std::chrono::steady_clock::time_point> last_frame_;
};
} // namespace

int run(const Context& ctx)
{
    Runner runner(ctx);
    return runner.run();
}
} // namespace app
