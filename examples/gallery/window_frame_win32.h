#pragma once
#include <windows.h>
#include "window_frame.h"

namespace imkit::gallery {
// Borrowed HWND, explicit attachment lifetime. Does not own GLFW or ImGui.
class Win32WindowFrame {
public:
    bool Attach(HWND window);
    void Detach();
    ~Win32WindowFrame() { Detach(); }
    Win32WindowFrame()=default;
    Win32WindowFrame(const Win32WindowFrame&)=delete;
    Win32WindowFrame& operator=(const Win32WindowFrame&)=delete;
    bool Enabled() const { return window_!=nullptr; }
    FrameLayout Layout() const;
    FrameState State();
    void Execute(FrameAction action);
private:
    static LRESULT CALLBACK Procedure(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
    LRESULT Message(UINT,WPARAM,LPARAM);
    int HitTest(POINT screen) const;
    HWND window_=nullptr;
    int pressed_=-1;
    FrameAction pending_=FrameAction::None;
};
}
