#include "ui/imgui_layer.h"

#include <cstddef>
#include <cstring>
#include <string>

#include <d3d11.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "config.h"
#include "core/log.h"
#include "ui/logo_pixels.h"

// ImGui::RegisterUserTexture (for the logo) is declared in imgui_internal.h ("experimental" in ImGui 1.92).
#include <imgui_internal.h>

// Declared in imgui_impl_win32.h only inside "#if 0" (so the header doesn't need <Windows.h>): declare it ourselves.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace ui
{
namespace
{
std::string windows_fonts_dir()
{
    char buffer[MAX_PATH] = {};
    const UINT length = GetWindowsDirectoryA(buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
    {
        return {};
    }
    return std::string(buffer, length) + "\\Fonts\\";
}

// AddFontFromFileTTF asserts on a missing file, so check first.
ImFont* add_font_if_present(ImGuiIO& io, const std::string& path)
{
    if (path.empty() || GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        return nullptr;
    }
    return io.Fonts->AddFontFromFileTTF(path.c_str(), config::kFontSize);
}

constexpr ImGuiConfigFlags kMenuClosedFlags = ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange;
} // namespace

ImGuiLayer::~ImGuiLayer()
{
    shutdown();
}

bool ImGuiLayer::init(HWND window, ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (ready_)
    {
        return true;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // no imgui.ini next to the exe (our settings live in profiles)
    io.LogFilename = nullptr;
    io.ConfigFlags |= kMenuClosedFlags; // the menu starts closed

    ImGuiStyle& style = ImGui::GetStyle();
    apply_style(style);
    use_theme(settings::OverlaySettings{}); // the colours; app/frame switches to the profile's theme every frame
    style.FontSizeBase = config::kFontSize;
    load_fonts();
    create_logo();

    if (!ImGui_ImplWin32_Init(window))
    {
        logger::error("ImGui_ImplWin32_Init failed");
        ImGui::DestroyContext();
        return false;
    }
    if (!ImGui_ImplDX11_Init(device, context))
    {
        logger::error("ImGui_ImplDX11_Init failed");
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        return false;
    }
    window_ = window;
    ready_ = true;
    logger::info("ImGui {} ready", IMGUI_VERSION);
    return true;
}

void ImGuiLayer::load_fonts()
{
    ImGuiIO& io = ImGui::GetIO();
    const std::string dir = windows_fonts_dir();
    fonts_.regular = add_font_if_present(io, dir.empty() ? dir : dir + "segoeui.ttf");
    fonts_.semibold = add_font_if_present(io, dir.empty() ? dir : dir + "seguisb.ttf");
    fonts_.bold = add_font_if_present(io, dir.empty() ? dir : dir + "segoeuib.ttf");
    if (fonts_.regular == nullptr)
    {
        fonts_.regular = io.Fonts->AddFontDefaultVector();
        logger::info("Segoe UI not found: using ImGui's built-in font");
    }
    if (fonts_.bold == nullptr)
    {
        fonts_.bold = fonts_.regular;
    }
    if (fonts_.semibold == nullptr)
    {
        fonts_.semibold = fonts_.bold;
    }
    io.FontDefault = fonts_.regular;
}

void ImGuiLayer::create_logo()
{
    // The logo's pixels are compiled into the exe (tools/make_logo_header.py), so no image decoder and no file are
    // needed. Registered as an ImGui "user texture", ImGui's DX11 backend uploads it like the font atlas (on the first
    // render) and releases it in its shutdown.
    logo_ = IM_NEW(ImTextureData)();
    logo_->Create(ImTextureFormat_RGBA32, logo::kWidth, logo::kHeight);
    static_assert(sizeof(logo::kPixels) == static_cast<std::size_t>(logo::kWidth) * logo::kHeight * 4);
    std::memcpy(logo_->GetPixels(), logo::kPixels, sizeof(logo::kPixels));
    logo_->UseColors = true;
    ImGui::RegisterUserTexture(logo_);
}

void ImGuiLayer::set_menu_open(bool open)
{
    if (!ready_)
    {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    if (open)
    {
        io.ConfigFlags &= ~kMenuClosedFlags;
        return;
    }
    io.ConfigFlags |= kMenuClosedFlags;
    // While closed the (click-through) overlay gets no mouse messages, so reset what ImGui knows: the backend's "mouse
    // is over the window" tracking (a fake WM_MOUSELEAVE only resets its own state), held buttons, keys, and capture.
    ImGui_ImplWin32_WndProcHandler(window_, WM_MOUSELEAVE, 0, 0);
    io.ClearInputMouse();
    io.ClearInputKeys();
    if (GetCapture() == window_)
    {
        ReleaseCapture();
    }
}

void ImGuiLayer::begin_frame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame(); // display size from the overlay's client rect, so game resolution changes just work
    ImGui::NewFrame();
}

float ImGuiLayer::framerate() const
{
    return ready_ ? ImGui::GetIO().Framerate : 0.0f;
}

void ImGuiLayer::end_frame()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::shutdown() noexcept
{
    if (!ready_)
    {
        return;
    }
    ImGui_ImplDX11_Shutdown(); // releases ImGui's D3D objects (shaders, buffers, font texture, logo texture)
    ImGui_ImplWin32_Shutdown();
    if (logo_ != nullptr)
    {
        ImGui::UnregisterUserTexture(logo_);
        logo_->DestroyPixels();
        IM_DELETE(logo_);
        logo_ = nullptr;
    }
    ImGui::DestroyContext();
    ready_ = false;
    window_ = nullptr;
    fonts_ = {};
    logger::info("ImGui shut down");
}

LRESULT imgui_message_hook(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    return ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam);
}
} // namespace ui
