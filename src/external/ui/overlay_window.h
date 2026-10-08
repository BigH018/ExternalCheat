#pragma once

// Our own transparent, topmost Win32 window laid over the game's client area, with its own DX11 device and swap chain.
// The game's window, device and swap chain are never touched (no hook of the game's Present, CLAUDE.md §10).
//
// - Transparency: a layered popup window whose client area is "all glass" (DwmExtendFrameIntoClientArea with -1
//   margins). We clear to (0,0,0,0) every frame, so only what ImGui draws is visible.
// - Click-through: WS_EX_TRANSPARENT (+ WS_EX_NOACTIVATE) while the menu is closed, so mouse input and focus go to the
//   game underneath. set_interactive(true) drops both while the menu is open: the overlay then takes every click over
//   the game's client area, so a click on (or next to) the menu never reaches the game.
// - Raw input: the window registers for keyboard and mouse raw input in the background (RIDEV_INPUTSINK) and counts
//   every key press (input::KeyTracker), so app/frame sees taps shorter than a frame. Not a hook: Windows only tells
//   us about input it delivers to the game anyway.
// - The menu key (a keybind, input::ActionId::menu_toggle) is a RegisterHotKey on this window. Receiving it counts as
//   user input to our process, which is what lets SetForegroundWindow take focus from the game when the menu opens.
//
// Main thread only (the window's messages are pumped on the thread that created it).

#include <cstdint>

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>

#include "input/key_tracker.h"

namespace ui
{
class OverlayWindow
{
public:
    // Every window message goes here first (ImGui's Win32 backend). Non-zero = handled, return that value.
    using MessageHook = LRESULT (*)(HWND, UINT, WPARAM, LPARAM);

    // What pump() saw since the last call.
    struct Events
    {
        bool menu_key = false;        // the menu hotkey was pressed
        bool close_requested = false; // Alt+F4 on the overlay (or WM_QUIT)
    };

    OverlayWindow() = default;
    ~OverlayWindow();

    OverlayWindow(const OverlayWindow&) = delete;
    OverlayWindow& operator=(const OverlayWindow&) = delete;

    // Register the class, create the (hidden, click-through) window, the D3D11 device and the swap chain.
    // Returns false (logged) on failure.
    bool create(MessageHook hook);
    // Safe to call more than once; the destructor calls it too.
    void destroy() noexcept;

    [[nodiscard]] HWND hwnd() const noexcept { return hwnd_; }
    [[nodiscard]] ID3D11Device* device() const noexcept { return device_.Get(); }
    [[nodiscard]] ID3D11DeviceContext* context() const noexcept { return context_.Get(); }

    // Dispatch every pending window message of this thread.
    Events pump();
    // Sleep until a message arrives or `timeout_ms` passes (used while the overlay is hidden). Raw input (every mouse
    // move) doesn't wake it; those messages wait for the next pump.
    void wait(std::uint32_t timeout_ms) const noexcept;

    // Key presses since the last call (from raw input, pumped by pump()). Call every frame, so presses made while the
    // tool isn't listening are dropped.
    [[nodiscard]] input::PressCounts take_presses() noexcept { return keys_.take_presses(); }
    // Whether raw input is registered; if not, app/frame falls back to presses from polling.
    [[nodiscard]] bool raw_input() const noexcept { return raw_input_; }

    // Register the menu hotkey (`vk`) while the game (or we) have focus, unregister otherwise, so the key still works
    // normally in every other program. A new `vk` re-registers it.
    void enable_menu_hotkey(bool enable, std::uint32_t vk) noexcept;
    // The last registration of the current key failed (another program holds it as a hotkey).
    [[nodiscard]] bool menu_hotkey_failed() const noexcept { return hotkey_failed_vk_ != 0; }

    // Move and size the overlay to `area` (screen coordinates of the game's client area).
    void cover(const RECT& area) noexcept;
    void set_visible(bool visible) noexcept;
    [[nodiscard]] bool visible() const noexcept { return visible_; }
    // true = takes mouse input and can be focused (menu open); false = click-through, never activated (menu closed).
    void set_interactive(bool interactive) noexcept;

    // Bind and clear the back buffer to fully transparent. Applies a pending resize first.
    void begin_frame();
    // Present with vsync. false (logged) when the device is lost; the caller then shuts down.
    bool present();

private:
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    // `hwnd` is passed in: hwnd_ is still null for the messages sent during CreateWindowExW.
    LRESULT handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    bool register_raw_input() noexcept;
    void on_raw_input(LPARAM lparam) noexcept;
    bool create_device();
    bool create_render_target();
    void resize_buffers();

    HWND hwnd_ = nullptr;
    bool class_registered_ = false;
    MessageHook hook_ = nullptr;
    bool visible_ = false;
    bool interactive_ = false;
    bool hotkey_registered_ = false;
    std::uint32_t hotkey_vk_ = 0;        // the key registered now
    std::uint32_t hotkey_failed_vk_ = 0; // the key whose registration failed (logged once), 0 = none
    RECT area_{};
    bool resize_pending_ = false;
    bool close_requested_ = false;
    bool device_lost_logged_ = false;
    bool raw_input_ = false;
    input::KeyTracker keys_;

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_;
};
} // namespace ui
