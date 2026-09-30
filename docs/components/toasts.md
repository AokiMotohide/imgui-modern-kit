# Toasts

[日本語](トースト.md) · [Workflow components](workflow-components.md)

## Colored cards

Include `imkit/toast.h` or `imkit/imkit.h`. `ToastViewport` draws colored cards in the current host window's viewport without taking focus on appearance. Existing `ToastRegion` overloads remain unchanged. The host owns state, content, context, fonts, renderer and application operations. No React dependency, global registry, worker or persistence is introduced.

```cpp
// Host-owned values, retained across frames.
imkit::ToastViewportState state;
std::vector<imkit::ToastView> queue;
queue.push_back({1, "Files loaded", "12 files loaded", imkit::FeedbackKind::Success});
// Once per frame inside the host window and theme scope:
std::array<imkit::ToastEvent, 16> storage{};
imkit::ToastEventBuffer events{storage};
imkit::ToastViewportOptions layout;
layout.position = imkit::ToastPosition::TopRight;
imkit::ToastViewport("notifications", queue, state, ImGui::GetTime(), events,
                     layout, {&theme});
for (std::size_t i = 0; i < events.count; ++i) {
    const auto event = storage[i];
    if (event.kind == imkit::ToastEventKind::Action) {
        // Execute the host operation identified by event.id.
    } else {
        std::erase_if(queue, [&](const auto& item) { return item.id == event.id; });
    }
}
```

Include `<array>`, `<vector>` and `<algorithm>` for this example. Strings are borrowed UTF-8: their storage must remain valid during drawing. Use unique nonzero IDs. Use one state per viewport/context and `Reset()` before context reuse. Calls run on the UI thread; the host marshals worker results.

Create covering host windows with `ImGuiWindowFlags_NoBringToFrontOnFocus`, for example `ImGui::Begin("App", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus)`, before drawing toasts. Otherwise clicking a host window can raise it above existing toast windows. Toasts use ordinary public ImGui windows; they do not force display order through internal APIs or steal focus. Newly created overlapping windows and modal dialogs retain Dear ImGui's ordering/input rules.

## Duration and queue

Negative `duration` selects defaults: success/info use five seconds; warning/error persist. Zero persists; positive values specify seconds. Loading always persists. Hover or keyboard focus pauses expiry. Waiting cards do not age. Draw once per frame. `UpdateToastViewport` is available for nonvisual timing; do not also call it when drawing the same state in that frame.

Set `phase=ToastPhase::Loading` for progress. Negative or nonfinite `progress` uses a spinner; other values are clamped to 0–1. Update the same ID to `phase=Message` and success/error when work completes. Changing phase, kind or duration restarts timing without changing insertion order. Text/progress updates do not restart it.

Newest cards appear closest to the selected edge; excess cards wait. Last duplicate ID wins. State holds up to 64 IDs, including expired IDs awaiting host removal. `state.overflow` reports untracked IDs; they can enter when slots are freed. Expired IDs stay hidden until removed or explicitly updated. `events.overflow` reports a full event buffer. Undelivered expiry requests retry in subsequent frames; action/dismiss overflow requires another activation. Reset buffer count each frame.

## Placement and customization

`ToastPosition` supports top/bottom × left/center/right; default is `TopRight`, with `maximum=4`. Width zero uses font-relative width; negative margin/gap use theme-derived current style spacing. Cards and visible count are bounded by the viewport work area; individually oversized cards scroll vertically.

Colors derive from semantic surface/status tokens. Text contrast is checked against the tinted background. Theme controls radius, entry fade and reduced motion; reduced motion stops fade and spinner rotation. Pass `ComponentOptions` for theme, locale and semantic publication. The `dismiss` locale key supplies the close button's accessible name. Titles, descriptions and action labels come from the host. Buttons support native keyboard navigation; no global shortcut or automatic focus transfer is installed.

`action` supplies a button, `actionDisabled` disables it, and `dismissible` controls the close button. `ToastEvent` distinguishes `Action`, `Dismiss` and `Expired`. The host decides whether an action removes or updates a toast; cancelling work must be handled by the host.

## Gallery and validation

Open **Toasts** (page 20): six positions, timing overrides, queue bursts, loading completion, determinate/indeterminate progress and English/Japanese examples. **Appearance** changes theme, density, contrast and reduced motion.

<img src="../../images/v3-toasts.gif" alt="Theme-colored toast cards move between six screen positions" width="960">

`imkit.toast` covers timing, pauses, queues, updates, buffer bounds, duplicate IDs, layout and public mouse requests. `imkit.toast_api_compile` and the external consumer cover public signatures. Gallery GPU capture is separate from native OS/IME, screen-reader, multi-monitor DPI and macOS acceptance.
