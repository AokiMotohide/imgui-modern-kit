#pragma once
#include <imkit/workflow.h>
#include <array>

namespace imkit {
enum class ToastPosition { TopLeft, TopCenter, TopRight, BottomLeft, BottomCenter, BottomRight };
enum class ToastPhase { Message, Loading };
enum class ToastEventKind { Dismiss, Expired, Action };
struct ToastView {
    StableId id=0;
    const char* title="";
    const char* description="";
    FeedbackKind kind=FeedbackKind::Info;
    // Negative selects the kind default; zero persists; positive is seconds.
    double duration=-1;
    bool dismissible=true;
    const char* action="";
    bool actionDisabled=false;
    ToastPhase phase=ToastPhase::Message;
    // Negative or nonfinite means indeterminate. Otherwise clamped to [0,1].
    float progress=-1;
};
struct ToastEvent { StableId id=0; ToastEventKind kind=ToastEventKind::Dismiss; };
struct ToastEventBuffer {
    std::span<ToastEvent> storage{};
    std::size_t count=0;
    bool overflow=false;
    bool Push(ToastEvent event);
};
struct ToastViewportOptions {
    ToastPosition position=ToastPosition::TopRight;
    float width=0; // Zero uses font-relative width.
    float margin=-1, gap=-1; // Negative uses theme/style spacing.
    std::size_t maximum=4;
};
// Host-owned, bounded storage; one instance per viewport/context. No strings,
// ImGui context or application work are retained. Reset when reusing a context.
struct ToastViewportState {
    static constexpr std::size_t Capacity=64;
    struct Entry {
        StableId id=0;
        std::uint64_t order=0;
        FeedbackKind kind=FeedbackKind::Info;
        ToastPhase phase=ToastPhase::Message;
        double duration=-1, remaining=0, shownSeconds=0;
        bool visible=false, paused=false, retired=false, pendingExpiry=false;
    };
    std::array<Entry,Capacity> entries{};
    double lastTime=0;
    std::uint64_t nextOrder=0;
    bool hasTime=false, overflow=false;
    void Reset();
};
// Update once per frame using monotonic host time. Last duplicate ID wins.
// Paused IDs stop expiry. Hidden entries never consume duration. Overflowed
// expiry requests are retried until delivered; remove expired IDs in the host.
void UpdateToastViewport(std::span<const ToastView> items, ToastViewportState& state,
                         double now, std::size_t maximum, ToastEventBuffer& events,
                         std::span<const StableId> paused={});
// Call once per frame inside the host window whose viewport should contain the
// overlay. Layout may reduce maximum to fit the work area. Actions are requests,
// never callbacks. Remove IDs only after processing their returned events.
// Covering host windows should use NoBringToFrontOnFocus; ordinary ImGui
// window/modal ordering is retained, without internal display-order APIs.
void ToastViewport(const char* id, std::span<const ToastView> items,
                   ToastViewportState& state, double now, ToastEventBuffer& events,
                   ToastViewportOptions layout={}, ComponentOptions options={});
} // namespace imkit
