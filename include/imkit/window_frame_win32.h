#pragma once

#include <imkit/window_frame.h>
#include <windows.h>

namespace imkit {

// Borrows an HWND. The host must Detach before destroying the window or adapter.
class WindowFrameWin32Adapter {
  public:
    WindowFrameWin32Adapter() = default;
    ~WindowFrameWin32Adapter() { Detach(); }
    WindowFrameWin32Adapter(const WindowFrameWin32Adapter&) = delete;
    WindowFrameWin32Adapter& operator=(const WindowFrameWin32Adapter&) = delete;

    bool Attach(HWND window);
    void Detach();
    [[nodiscard]] bool Attached() const noexcept { return window_ != nullptr; }
    void SetLayout(const WindowFrameLayout& layout) noexcept { layout_ = layout; }
    [[nodiscard]] WindowFrameState State();
    void Execute(WindowFrameOperation operation);

  private:
    static LRESULT CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    LRESULT Message(UINT, WPARAM, LPARAM);
    int HitTest(POINT screen) const;

    HWND window_ = nullptr;
    WindowFrameLayout layout_{};
    int pressedButton_ = -1;
    WindowFrameEvent pendingEvent_{};
};

} // namespace imkit
