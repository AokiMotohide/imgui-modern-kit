#include <imkit/window_frame_win32.h>

#include <algorithm>
#include <commctrl.h>
#include <dwmapi.h>
#include <windowsx.h>

namespace imkit {
namespace {
constexpr UINT_PTR kSubclassId = 0x494d4b46;
int ButtonIndex(int hit) { return hit == HTMINBUTTON ? 0 : hit == HTMAXBUTTON ? 1 : hit == HTCLOSE ? 2 : -1; }
POINT ScreenPoint(LPARAM value) { return {GET_X_LPARAM(value), GET_Y_LPARAM(value)}; }
WindowFrameOperation OperationForButton(int index) {
    return index == 0 ? WindowFrameOperation::Minimize
                      : index == 1 ? WindowFrameOperation::MaximizeRestore
                                   : index == 2 ? WindowFrameOperation::Close : WindowFrameOperation::None;
}
} // namespace

bool WindowFrameWin32Adapter::Attach(HWND window) {
    if (window_ || !IsWindow(window)) return false;
    window_ = window;
    if (!SetWindowSubclass(window_, Procedure, kSubclassId, reinterpret_cast<DWORD_PTR>(this))) {
        window_ = nullptr;
        return false;
    }
    MARGINS margins{0, 0, 1, 0};
    DwmExtendFrameIntoClientArea(window_, &margins);
    SetWindowPos(window_, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    return true;
}

void WindowFrameWin32Adapter::Detach() {
    if (!window_) return;
    HWND window = window_;
    RemoveWindowSubclass(window, Procedure, kSubclassId);
    window_ = nullptr;
    pressedButton_ = -1;
    pendingEvent_ = {};
    MARGINS margins{};
    DwmExtendFrameIntoClientArea(window, &margins);
    SetWindowPos(window, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

int WindowFrameWin32Adapter::HitTest(POINT screen) const {
    RECT rect{};
    GetWindowRect(window_, &rect);
    const UINT dpi = GetDpiForWindow(window_);
    const int border = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) +
                       GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    if (!IsZoomed(window_)) {
        const bool left = screen.x < rect.left + border, right = screen.x >= rect.right - border;
        const bool top = screen.y < rect.top + border, bottom = screen.y >= rect.bottom - border;
        if (top) return left ? HTTOPLEFT : right ? HTTOPRIGHT : HTTOP;
        if (bottom) return left ? HTBOTTOMLEFT : right ? HTBOTTOMRIGHT : HTBOTTOM;
        if (left) return HTLEFT;
        if (right) return HTRIGHT;
    }
    ScreenToClient(window_, &screen);
    const ImVec2 point{static_cast<float>(screen.x), static_cast<float>(screen.y)};
    if (layout_.workspaceSwitcher.Contains(point)) return HTCLIENT;
    if (layout_.minimize.Contains(point)) return HTMINBUTTON;
    if (layout_.maximizeRestore.Contains(point)) return HTMAXBUTTON;
    if (layout_.close.Contains(point)) return HTCLOSE;
    if (layout_.icon.Contains(point)) return HTSYSMENU;
    return layout_.titleBar.Contains(point) ? HTCAPTION : HTCLIENT;
}

WindowFrameState WindowFrameWin32Adapter::State() {
    WindowFrameState state;
    if (!window_) return state;
    state.dpiScale = static_cast<float>(GetDpiForWindow(window_)) / 96.f;
    state.active = GetForegroundWindow() == window_;
    state.maximized = IsZoomed(window_) != FALSE;
    POINT cursor{};
    GetCursorPos(&cursor);
    state.hoveredButton = WindowFromPoint(cursor) == window_ ? ButtonIndex(HitTest(cursor)) : -1;
    state.pressedButton = pressedButton_;
    state.pendingEvent = pendingEvent_;
    pendingEvent_ = {};
    return state;
}

void WindowFrameWin32Adapter::Execute(WindowFrameOperation operation) {
    if (!window_ || operation == WindowFrameOperation::None) return;
    WPARAM command = 0;
    switch (operation) {
    case WindowFrameOperation::Minimize: command = SC_MINIMIZE; break;
    case WindowFrameOperation::MaximizeRestore: command = IsZoomed(window_) ? SC_RESTORE : SC_MAXIMIZE; break;
    case WindowFrameOperation::Close: command = SC_CLOSE; break;
    case WindowFrameOperation::SystemMenu: command = SC_KEYMENU; break;
    default: return;
    }
    SendMessageW(window_, WM_SYSCOMMAND, command, operation == WindowFrameOperation::SystemMenu ? VK_SPACE : 0);
}

LRESULT CALLBACK WindowFrameWin32Adapter::Procedure(HWND window, UINT message, WPARAM w, LPARAM l,
                                                     UINT_PTR, DWORD_PTR data) {
    auto* self = reinterpret_cast<WindowFrameWin32Adapter*>(data);
    if (message == WM_NCDESTROY) {
        RemoveWindowSubclass(window, Procedure, kSubclassId);
        self->window_ = nullptr;
        return DefSubclassProc(window, message, w, l);
    }
    return self->Message(message, w, l);
}

LRESULT WindowFrameWin32Adapter::Message(UINT message, WPARAM w, LPARAM l) {
    switch (message) {
    case WM_SYSCHAR:
        if (w == VK_SPACE) return DefWindowProcW(window_, message, w, l);
        break;
    case WM_SYSCOMMAND:
        if ((w & 0xfff0) == SC_KEYMENU && l == VK_SPACE) return DefWindowProcW(window_, message, w, l);
        break;
    case WM_GETMINMAXINFO: {
        const auto result = DefSubclassProc(window_, message, w, l);
        auto* limits = reinterpret_cast<MINMAXINFO*>(l);
        const UINT dpi = GetDpiForWindow(window_);
        limits->ptMinTrackSize.x = std::max(limits->ptMinTrackSize.x, LONG(MulDiv(320, dpi, 96)));
        limits->ptMinTrackSize.y = std::max(limits->ptMinTrackSize.y, LONG(MulDiv(200, dpi, 96)));
        return result;
    }
    case WM_NCCALCSIZE: {
        auto* rect = w ? &reinterpret_cast<NCCALCSIZE_PARAMS*>(l)->rgrc[0] : reinterpret_cast<RECT*>(l);
        if (IsZoomed(window_)) {
            const UINT dpi = GetDpiForWindow(window_);
            const int x = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
            const int y = GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
            InflateRect(rect, -x, -y);
        }
        return 0;
    }
    case WM_NCHITTEST: return HitTest(ScreenPoint(l));
    case WM_NCPAINT: return 0;
    case WM_NCACTIVATE: return DefWindowProcW(window_, message, w, -1);
    case WM_NCMOUSEMOVE: {
        LRESULT result = 0;
        DwmDefWindowProc(window_, message, w, l, &result);
        if (ButtonIndex(static_cast<int>(w)) >= 0) return 0;
        break;
    }
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
        if (const int index = ButtonIndex(static_cast<int>(w)); index >= 0) {
            pressedButton_ = index;
            SetCapture(window_);
            return 0;
        }
        break;
    case WM_LBUTTONUP:
    case WM_NCLBUTTONUP:
        if (pressedButton_ >= 0) {
            POINT point = ScreenPoint(l);
            if (message == WM_LBUTTONUP) ClientToScreen(window_, &point);
            const int index = pressedButton_;
            pressedButton_ = -1;
            if (GetCapture() == window_) ReleaseCapture();
            if (ButtonIndex(HitTest(point)) == index)
                pendingEvent_ = {WindowFrameEventType::Operation, OperationForButton(index), 0};
            return 0;
        }
        break;
    case WM_CANCELMODE:
        pressedButton_ = -1;
        if (GetCapture() == window_) ReleaseCapture();
        break;
    case WM_CAPTURECHANGED: pressedButton_ = -1; break;
    }
    return DefSubclassProc(window_, message, w, l);
}

} // namespace imkit
