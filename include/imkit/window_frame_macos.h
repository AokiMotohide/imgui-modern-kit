#pragma once

#include <imkit/window_frame.h>

namespace imkit {

// Borrows an NSWindow passed as void*. This header remains valid in C++ translation units.
class WindowFrameMacOSAdapter {
  public:
    WindowFrameMacOSAdapter() = default;
    ~WindowFrameMacOSAdapter() { Detach(); }
    WindowFrameMacOSAdapter(const WindowFrameMacOSAdapter&) = delete;
    WindowFrameMacOSAdapter& operator=(const WindowFrameMacOSAdapter&) = delete;

    bool Attach(void* nsWindow, WindowFramePreset preset);
    void Configure(WindowFramePreset preset);
    void Detach();
    [[nodiscard]] bool Attached() const noexcept { return window_ != nullptr; }
    [[nodiscard]] WindowFrameState State() const;
    void Execute(WindowFrameOperation operation);

  private:
    void* window_ = nullptr;
    unsigned long originalStyleMask_ = 0;
    bool originalTitlebarTransparent_ = false;
    bool originalTitleVisible_ = true;
    bool originalMovableByBackground_ = false;
};

} // namespace imkit
