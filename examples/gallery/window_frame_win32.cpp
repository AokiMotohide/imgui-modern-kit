#include <imkit/imkit.h>
#include "window_frame_win32.h"
#include <commctrl.h>
#include <dwmapi.h>
#include <windowsx.h>
#include <algorithm>

namespace imkit::gallery {
namespace {
constexpr UINT_PTR subclassId=0x494d4b46;
int ButtonIndex(int hit) { return hit==HTMINBUTTON?0:hit==HTMAXBUTTON?1:hit==HTCLOSE?2:-1; }
POINT ScreenPoint(LPARAM p) { return {GET_X_LPARAM(p),GET_Y_LPARAM(p)}; }
}
bool Win32WindowFrame::Attach(HWND window) {
    if(window_ || !IsWindow(window)) return false;
    window_=window;
    if(!SetWindowSubclass(window_,Procedure,subclassId,reinterpret_cast<DWORD_PTR>(this))) {
        window_=nullptr; return false;
    }
    // Keep caption/thickframe/system-menu styles so the OS retains snap/resize semantics.
    MARGINS margins{0,0,1,0};
    DwmExtendFrameIntoClientArea(window_,&margins);
    SetWindowPos(window_,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
    return true;
}
void Win32WindowFrame::Detach() {
    if(!window_) return;
    auto window=window_;
    RemoveWindowSubclass(window,Procedure,subclassId);
    window_=nullptr;
    MARGINS margins{};
    DwmExtendFrameIntoClientArea(window,&margins);
    SetWindowPos(window,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
}
FrameLayout Win32WindowFrame::Layout() const {
    RECT client{}; GetClientRect(window_,&client);
    return LayoutWindowFrame(float(client.right),float(GetDpiForWindow(window_))/96.f);
}
int Win32WindowFrame::HitTest(POINT screen) const {
    RECT rect{}; GetWindowRect(window_,&rect);
    const UINT dpi=GetDpiForWindow(window_);
    const int border=GetSystemMetricsForDpi(SM_CXSIZEFRAME,dpi)+GetSystemMetricsForDpi(SM_CXPADDEDBORDER,dpi);
    if(!IsZoomed(window_)) {
        const bool left=screen.x<rect.left+border,right=screen.x>=rect.right-border;
        const bool top=screen.y<rect.top+border,bottom=screen.y>=rect.bottom-border;
        if(top) return left?HTTOPLEFT:right?HTTOPRIGHT:HTTOP;
        if(bottom) return left?HTBOTTOMLEFT:right?HTBOTTOMRIGHT:HTBOTTOM;
        if(left) return HTLEFT;
        if(right) return HTRIGHT;
    }
    ScreenToClient(window_,&screen);
    const ImVec2 p{float(screen.x),float(screen.y)};
    const auto layout=Layout();
    for(int i=0;i<3;++i) if(layout.buttons[i].Contains(p)) return i==0?HTMINBUTTON:i==1?HTMAXBUTTON:HTCLOSE;
    if(layout.icon.Contains(p)) return HTSYSMENU;
    return layout.title.Contains(p)?HTCAPTION:HTCLIENT;
}
FrameState Win32WindowFrame::State() {
    POINT cursor{}; GetCursorPos(&cursor);
    FrameState state;
    state.active=GetForegroundWindow()==window_;
    state.maximized=IsZoomed(window_)!=FALSE;
    state.hovered=WindowFromPoint(cursor)==window_?ButtonIndex(HitTest(cursor)):-1;
    state.pressed=pressed_;
    state.requested=pending_; pending_=FrameAction::None;
    return state;
}
void Win32WindowFrame::Execute(FrameAction action) {
    if(!window_ || action==FrameAction::None) return;
    const WPARAM command=action==FrameAction::Minimize?SC_MINIMIZE:
        action==FrameAction::Close?SC_CLOSE:IsZoomed(window_)?SC_RESTORE:SC_MAXIMIZE;
    // SC_CLOSE reaches GLFW's WM_CLOSE path and existing should-close callback.
    SendMessageW(window_,WM_SYSCOMMAND,command,0);
}
LRESULT CALLBACK Win32WindowFrame::Procedure(HWND window,UINT message,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR data) {
    auto* self=reinterpret_cast<Win32WindowFrame*>(data);
    if(message==WM_NCDESTROY) {
        RemoveWindowSubclass(window,Procedure,subclassId);
        self->window_=nullptr;
        return DefSubclassProc(window,message,w,l);
    }
    return self->Message(message,w,l);
}
LRESULT Win32WindowFrame::Message(UINT message,WPARAM w,LPARAM l) {
    switch(message) {
    case WM_SYSCHAR:
        // GLFW normally consumes WM_SYSCHAR/SC_KEYMENU. Preserve Alt+Space
        // without changing its treatment of other keyboard/IME messages.
        if(w==VK_SPACE) return DefWindowProcW(window_,message,w,l);
        break;
    case WM_SYSCOMMAND:
        if((w&0xfff0)==SC_KEYMENU && l==VK_SPACE)
            return DefWindowProcW(window_,message,w,l);
        break;
    case WM_GETMINMAXINFO: {
        const auto result=DefSubclassProc(window_,message,w,l);
        auto* limits=reinterpret_cast<MINMAXINFO*>(l);
        const UINT dpi=GetDpiForWindow(window_);
        limits->ptMinTrackSize.x=std::max(limits->ptMinTrackSize.x,LONG(MulDiv(320,dpi,96)));
        limits->ptMinTrackSize.y=std::max(limits->ptMinTrackSize.y,LONG(MulDiv(200,dpi,96)));
        return result;
    }
    case WM_NCCALCSIZE: {
        auto* rect=w?&reinterpret_cast<NCCALCSIZE_PARAMS*>(l)->rgrc[0]:reinterpret_cast<RECT*>(l);
        if(IsZoomed(window_)) {
            const UINT dpi=GetDpiForWindow(window_);
            const int x=GetSystemMetricsForDpi(SM_CXSIZEFRAME,dpi)+GetSystemMetricsForDpi(SM_CXPADDEDBORDER,dpi);
            const int y=GetSystemMetricsForDpi(SM_CYSIZEFRAME,dpi)+GetSystemMetricsForDpi(SM_CXPADDEDBORDER,dpi);
            InflateRect(rect,-x,-y);
        }
        return 0;
    }
    case WM_NCHITTEST: return HitTest(ScreenPoint(l));
    case WM_NCPAINT: return 0;
    case WM_NCACTIVATE: return DefWindowProcW(window_,message,w,-1);
    case WM_NCMOUSEMOVE: {
        // Let DWM observe HTMAXBUTTON for the Windows 11 snap layout flyout.
        LRESULT result=0;
        DwmDefWindowProc(window_,message,w,l,&result);
        if(ButtonIndex(int(w))>=0) return 0;
        break;
    }
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
        if(const int index=ButtonIndex(int(w));index>=0) {
            pressed_=index; SetCapture(window_); return 0;
        }
        break;
    case WM_LBUTTONUP:
    case WM_NCLBUTTONUP:
        if(pressed_>=0) {
            POINT p=ScreenPoint(l);
            if(message==WM_LBUTTONUP) ClientToScreen(window_,&p);
            const int index=pressed_; pressed_=-1;
            if(GetCapture()==window_) ReleaseCapture();
            if(ButtonIndex(HitTest(p))==index) pending_=static_cast<FrameAction>(index+1);
            return 0;
        }
        break;
    case WM_CANCELMODE:
        pressed_=-1;
        if(GetCapture()==window_) ReleaseCapture();
        break;
    case WM_CAPTURECHANGED: pressed_=-1; break;
    }
    return DefSubclassProc(window_,message,w,l);
}
}
