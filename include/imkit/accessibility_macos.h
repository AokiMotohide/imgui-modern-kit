#pragma once
#include <imkit/accessibility.h>

namespace imkit::accessibility {

// Borrows an NSView. The host publishes immutable snapshots and remains responsible
// for dispatching actions on the UI thread.
class MacOSAccessibilityAdapter {
  public:
    MacOSAccessibilityAdapter() = default;
    ~MacOSAccessibilityAdapter() { Detach(); }
    MacOSAccessibilityAdapter(const MacOSAccessibilityAdapter &) = delete;
    MacOSAccessibilityAdapter &operator=(const MacOSAccessibilityAdapter &) = delete;

    bool Attach(void *nsView, NativeActionSink actions);
    bool Publish(const AccessibilityTree &tree);
    void Detach();
    [[nodiscard]] bool Attached() const noexcept { return bridge_ != nullptr; }

  private:
    void *bridge_ = nullptr;
};

} // namespace imkit::accessibility
