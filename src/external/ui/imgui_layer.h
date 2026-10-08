#pragma once

// Dear ImGui lifetime and per-frame plumbing for the overlay window (Win32 + DX11 backends). Main thread only.
// It renders into the overlay's own device and swap chain (ui/overlay_window), never the game's.

#include <Windows.h>

#include <imgui.h>

#include "ui/theme.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace ui
{
class ImGuiLayer
{
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    // Create the context, load fonts, apply the theme and init both backends. Returns false (logged) on failure.
    bool init(HWND window, ID3D11Device* device, ID3D11DeviceContext* context);
    [[nodiscard]] bool ready() const noexcept { return ready_; }
    [[nodiscard]] const Fonts& fonts() const noexcept { return fonts_; }
    // The logo (ui/logo_pixels.h) as an ImGui texture, or nullptr before init.
    [[nodiscard]] ImTextureData* logo() const noexcept { return logo_; }

    // Menu open: ImGui gets the mouse and may change the cursor shape. Closed: it ignores the mouse entirely.
    void set_menu_open(bool open);

    void begin_frame();
    // ImGui's running average of frames per second (valid after begin_frame).
    [[nodiscard]] float framerate() const;
    // ImGui::Render + draw into the overlay's currently bound render target.
    void end_frame();

    // Safe to call more than once; the destructor calls it too. Must run before the D3D device is released.
    void shutdown() noexcept;

private:
    void load_fonts();
    void create_logo();

    bool ready_ = false;
    HWND window_ = nullptr;
    Fonts fonts_;
    ImTextureData* logo_ = nullptr; // a user texture: ImGui's DX11 backend uploads and releases it
};

// OverlayWindow's message hook: hands every message to ImGui's Win32 backend (does nothing without a context).
LRESULT imgui_message_hook(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
} // namespace ui
