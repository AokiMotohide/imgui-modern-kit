#include <imkit/window_frame_macos.h>
#import <Cocoa/Cocoa.h>

namespace imkit {
bool WindowFrameMacOSAdapter::Attach(void* nsWindow, WindowFramePreset preset) {
    if (window_ || !nsWindow) return false;
    NSWindow* window = (__bridge NSWindow*)nsWindow;
    window_ = nsWindow;
    originalStyleMask_ = static_cast<unsigned long>(window.styleMask);
    originalTitlebarTransparent_ = window.titlebarAppearsTransparent;
    originalTitleVisible_ = window.titleVisibility == NSWindowTitleVisible;
    originalMovableByBackground_ = window.movableByWindowBackground;
    Configure(preset);
    return true;
}
void WindowFrameMacOSAdapter::Configure(WindowFramePreset preset) {
    if (!window_) return;
    NSWindow* window = (__bridge NSWindow*)window_;
    if (preset == WindowFramePreset::Native) {
        window.styleMask = static_cast<NSWindowStyleMask>(originalStyleMask_);
        window.titlebarAppearsTransparent = originalTitlebarTransparent_;
        window.titleVisibility = originalTitleVisible_ ? NSWindowTitleVisible : NSWindowTitleHidden;
        window.movableByWindowBackground = originalMovableByBackground_;
        return;
    }
    window.styleMask |= NSWindowStyleMaskFullSizeContentView;
    window.titlebarAppearsTransparent = YES;
    window.titleVisibility = NSWindowTitleHidden;
    window.movableByWindowBackground = YES;
}
void WindowFrameMacOSAdapter::Detach() {
    if (!window_) return;
    Configure(WindowFramePreset::Native);
    window_ = nullptr;
}
WindowFrameState WindowFrameMacOSAdapter::State() const {
    WindowFrameState state;
    if (!window_) return state;
    NSWindow* window = (__bridge NSWindow*)window_;
    // Cocoa/GLFW window coordinates are points (DIP); framebuffer scaling is renderer-owned.
    state.dpiScale = 1.f;
    state.active = window.isKeyWindow;
    state.maximized = (window.styleMask & NSWindowStyleMaskFullScreen) != 0;
    state.systemCaptionButtons = true;
    state.leadingSystemAreaDip = 78.f;
    return state;
}
void WindowFrameMacOSAdapter::Execute(WindowFrameOperation operation) {
    if (!window_) return;
    NSWindow* window = (__bridge NSWindow*)window_;
    switch (operation) {
    case WindowFrameOperation::Minimize: [window miniaturize:nil]; break;
    case WindowFrameOperation::MaximizeRestore: [window zoom:nil]; break;
    case WindowFrameOperation::Close: [window performClose:nil]; break;
    default: break;
    }
}
} // namespace imkit
