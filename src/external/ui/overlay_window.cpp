#include "ui/overlay_window.h"

#include <cstdint>
#include <iterator>

#include <dwmapi.h>

#include "config.h"
#include "core/log.h"
#include "input/keys.h"

namespace ui
{
namespace
{
constexpr int kMenuHotkeyId = 1;

// Menu closed: mouse input falls through to the game and the overlay can never become the active window.
constexpr LONG_PTR kClickThroughStyles = WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
// Always: on top of the game, layered (needed for WS_EX_TRANSPARENT to pass clicks through), not on the taskbar or in
// Alt+Tab.
constexpr DWORD kBaseStyles = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW;

// Present returned DXGI_STATUS_OCCLUDED (nothing of ours visible): Present doesn't wait for vsync then, so pause.
constexpr std::uint32_t kOccludedWaitMs = 16;

std::uint32_t hresult_bits(HRESULT hr)
{
    return static_cast<std::uint32_t>(hr);
}
} // namespace

OverlayWindow::~OverlayWindow()
{
    destroy();
}

bool OverlayWindow::create(MessageHook hook)
{
    hook_ = hook;
    const HINSTANCE instance = GetModuleHandleW(nullptr);

    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = &OverlayWindow::window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.lpszClassName = config::kOverlayClassName;
    if (RegisterClassExW(&window_class) == 0)
    {
        logger::error("RegisterClassExW failed (error {})", GetLastError());
        return false;
    }
    class_registered_ = true;

    // Starts hidden at 1x1; cover() moves it over the game and set_visible() shows it.
    hwnd_ = CreateWindowExW(kBaseStyles | static_cast<DWORD>(kClickThroughStyles), config::kOverlayClassName,
                            config::kOverlayTitle, WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, instance, this);
    if (hwnd_ == nullptr)
    {
        logger::error("CreateWindowExW failed (error {})", GetLastError());
        return false;
    }

    // Fully opaque as a layered window; the per-pixel transparency comes from DWM ("glass" over the whole client
    // area) and the alpha we clear the back buffer to.
    if (!SetLayeredWindowAttributes(hwnd_, 0, 255, LWA_ALPHA))
    {
        logger::error("SetLayeredWindowAttributes failed (error {})", GetLastError());
        return false;
    }
    const MARGINS glass{-1, -1, -1, -1};
    if (const HRESULT hr = DwmExtendFrameIntoClientArea(hwnd_, &glass); FAILED(hr))
    {
        logger::error("DwmExtendFrameIntoClientArea failed (0x{:08X})", hresult_bits(hr));
        return false;
    }
    raw_input_ = register_raw_input();
    return create_device();
}

bool OverlayWindow::register_raw_input() noexcept
{
    // Generic desktop page (0x01): mouse (0x02) and keyboard (0x06), delivered to this window in the background too.
    const RAWINPUTDEVICE devices[] = {
        {0x01, 0x02, RIDEV_INPUTSINK, hwnd_},
        {0x01, 0x06, RIDEV_INPUTSINK, hwnd_},
    };
    if (!RegisterRawInputDevices(devices, static_cast<UINT>(std::size(devices)), sizeof(RAWINPUTDEVICE)))
    {
        logger::warn("Raw input couldn't be registered (error {}): keys are read once per frame instead, so very "
                     "quick taps may be missed",
                     GetLastError());
        return false;
    }
    return true;
}

void OverlayWindow::on_raw_input(LPARAM lparam) noexcept
{
    RAWINPUT input{};
    UINT size = sizeof(input);
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) ==
        static_cast<UINT>(-1))
    {
        return;
    }
    if (input.header.dwType == RIM_TYPEKEYBOARD)
    {
        const RAWKEYBOARD& keyboard = input.data.keyboard;
        keys_.on_keyboard(keyboard.VKey, keyboard.MakeCode, keyboard.Flags);
    }
    else if (input.header.dwType == RIM_TYPEMOUSE)
    {
        keys_.on_mouse_buttons(input.data.mouse.usButtonFlags);
    }
}

bool OverlayWindow::create_device()
{
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // width/height 0 = the window's client size
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd_;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    // The blt model: DWM composites its alpha channel through the extended frame. Flip-model swap chains on an HWND
    // ignore alpha (per-pixel transparency there needs DirectComposition).
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    constexpr D3D_FEATURE_LEVEL kLevels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL level{};
    const auto create_with = [&](D3D_DRIVER_TYPE driver) {
        return D3D11CreateDeviceAndSwapChain(nullptr, driver, nullptr, 0, kLevels, static_cast<UINT>(std::size(kLevels)),
                                             D3D11_SDK_VERSION, &desc, swap_chain_.ReleaseAndGetAddressOf(),
                                             device_.ReleaseAndGetAddressOf(), &level,
                                             context_.ReleaseAndGetAddressOf());
    };
    HRESULT hr = create_with(D3D_DRIVER_TYPE_HARDWARE);
    if (hr == DXGI_ERROR_UNSUPPORTED)
    {
        logger::warn("No hardware D3D11 device: the overlay falls back to WARP (software rendering)");
        hr = create_with(D3D_DRIVER_TYPE_WARP);
    }
    if (FAILED(hr))
    {
        logger::error("D3D11CreateDeviceAndSwapChain failed (0x{:08X})", hresult_bits(hr));
        return false;
    }

    // Alt+Enter must never turn the overlay into an exclusive-fullscreen window.
    Microsoft::WRL::ComPtr<IDXGIFactory> factory;
    if (SUCCEEDED(swap_chain_->GetParent(IID_PPV_ARGS(&factory))))
    {
        factory->MakeWindowAssociation(hwnd_, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);
    }
    logger::info("Overlay D3D11 device ready (feature level {}.{})", (static_cast<int>(level) >> 12) & 0xF,
                 (static_cast<int>(level) >> 8) & 0xF);
    return create_render_target();
}

bool OverlayWindow::create_render_target()
{
    Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer;
    HRESULT hr = swap_chain_->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    if (SUCCEEDED(hr))
    {
        hr = device_->CreateRenderTargetView(back_buffer.Get(), nullptr, render_target_.ReleaseAndGetAddressOf());
    }
    if (FAILED(hr))
    {
        logger::error("Creating the overlay's render target failed (0x{:08X})", hresult_bits(hr));
        return false;
    }
    return true;
}

void OverlayWindow::resize_buffers()
{
    // Every reference to the back buffer must be gone before ResizeBuffers, including the one the context holds.
    context_->OMSetRenderTargets(0, nullptr, nullptr);
    render_target_.Reset();
    if (const HRESULT hr = swap_chain_->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0); FAILED(hr))
    {
        logger::error("Resizing the overlay's swap chain failed (0x{:08X})", hresult_bits(hr));
        return;
    }
    create_render_target();
}

void OverlayWindow::destroy() noexcept
{
    if (raw_input_)
    {
        const RAWINPUTDEVICE devices[] = {
            {0x01, 0x02, RIDEV_REMOVE, nullptr},
            {0x01, 0x06, RIDEV_REMOVE, nullptr},
        };
        RegisterRawInputDevices(devices, static_cast<UINT>(std::size(devices)), sizeof(RAWINPUTDEVICE));
        raw_input_ = false;
    }
    if (hotkey_registered_)
    {
        UnregisterHotKey(hwnd_, kMenuHotkeyId);
        hotkey_registered_ = false;
    }
    if (context_)
    {
        context_->ClearState();
        context_->Flush();
    }
    render_target_.Reset();
    swap_chain_.Reset();
    context_.Reset();
    device_.Reset();
    if (hwnd_ != nullptr)
    {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    if (class_registered_)
    {
        UnregisterClassW(config::kOverlayClassName, GetModuleHandleW(nullptr));
        class_registered_ = false;
    }
    visible_ = false;
    interactive_ = false;
}

OverlayWindow::Events OverlayWindow::pump()
{
    Events events;
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
        {
            events.close_requested = true;
            continue;
        }
        if (message.message == WM_HOTKEY && message.wParam == kMenuHotkeyId)
        {
            events.menu_key = true;
            continue;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (close_requested_)
    {
        events.close_requested = true;
        close_requested_ = false;
    }
    return events;
}

void OverlayWindow::wait(std::uint32_t timeout_ms) const noexcept
{
    MsgWaitForMultipleObjects(0, nullptr, FALSE, timeout_ms, QS_ALLINPUT & ~QS_RAWINPUT);
}

void OverlayWindow::enable_menu_hotkey(bool enable, std::uint32_t vk) noexcept
{
    if (hwnd_ == nullptr)
    {
        return;
    }
    if (hotkey_registered_ && (!enable || vk != hotkey_vk_))
    {
        UnregisterHotKey(hwnd_, kMenuHotkeyId);
        hotkey_registered_ = false;
    }
    if (!enable || hotkey_registered_)
    {
        return;
    }
    if (RegisterHotKey(hwnd_, kMenuHotkeyId, MOD_NOREPEAT, vk))
    {
        hotkey_registered_ = true;
        hotkey_vk_ = vk;
        hotkey_failed_vk_ = 0;
    }
    else if (hotkey_failed_vk_ != vk)
    {
        logger::warn("Couldn't register {} as the menu key (error {}): another program (or Windows) is using it as a "
                     "hotkey. Pick another key on the Keybinds page.",
                     input::key_name(vk), GetLastError());
        hotkey_failed_vk_ = vk;
    }
}

void OverlayWindow::cover(const RECT& area) noexcept
{
    if (EqualRect(&area, &area_))
    {
        return;
    }
    area_ = area;
    // WM_SIZE arrives inside this call; the swap chain is resized at the next begin_frame().
    SetWindowPos(hwnd_, HWND_TOPMOST, area.left, area.top, area.right - area.left, area.bottom - area.top,
                 SWP_NOACTIVATE);
}

void OverlayWindow::set_visible(bool visible) noexcept
{
    if (visible == visible_)
    {
        return;
    }
    visible_ = visible;
    if (visible)
    {
        ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
        SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    else
    {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

void OverlayWindow::set_interactive(bool interactive) noexcept
{
    if (interactive == interactive_ || hwnd_ == nullptr)
    {
        return;
    }
    interactive_ = interactive;
    LONG_PTR styles = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);
    styles = interactive ? (styles & ~kClickThroughStyles) : (styles | kClickThroughStyles);
    SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, styles);
    SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void OverlayWindow::begin_frame()
{
    if (resize_pending_)
    {
        resize_pending_ = false;
        resize_buffers();
    }
    if (!render_target_)
    {
        return;
    }
    constexpr float kTransparent[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    ID3D11RenderTargetView* const targets[] = {render_target_.Get()};
    context_->OMSetRenderTargets(1, targets, nullptr);
    context_->ClearRenderTargetView(render_target_.Get(), kTransparent);
}

bool OverlayWindow::present()
{
    const HRESULT hr = swap_chain_->Present(1, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        if (!device_lost_logged_)
        {
            logger::error("The overlay's D3D11 device was lost (0x{:08X}, reason 0x{:08X})", hresult_bits(hr),
                          hresult_bits(device_->GetDeviceRemovedReason()));
            device_lost_logged_ = true;
        }
        return false;
    }
    if (hr == DXGI_STATUS_OCCLUDED)
    {
        wait(kOccludedWaitMs);
    }
    return true;
}

LRESULT CALLBACK OverlayWindow::window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    auto* self = reinterpret_cast<OverlayWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self == nullptr)
    {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }
    if (self->hook_ != nullptr)
    {
        if (const LRESULT handled = self->hook_(hwnd, message, wparam, lparam); handled != 0)
        {
            return handled;
        }
    }
    if (message == WM_NCDESTROY)
    {
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
    }
    return self->handle_message(hwnd, message, wparam, lparam);
}

LRESULT OverlayWindow::handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
    case WM_SIZE:
        if (wparam != SIZE_MINIMIZED)
        {
            resize_pending_ = true;
        }
        return 0;
    case WM_CLOSE:
        // Alt+F4 while the menu has focus: exit the tool (app/frame shuts down and destroys the window).
        close_requested_ = true;
        return 0;
    case WM_ERASEBKGND:
        return 1; // the swap chain paints everything
    case WM_INPUT:
        on_raw_input(lparam);
        break; // DefWindowProc cleans the raw input up
    case WM_SYSCOMMAND:
        if ((wparam & 0xFFF0) == SC_KEYMENU)
        {
            return 0; // Alt alone would open the (non-existent) window menu and stall the message loop
        }
        break;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}
} // namespace ui
