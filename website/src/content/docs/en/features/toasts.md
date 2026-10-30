---
title: "Toast notifications"
---

`ToastViewport` draws colored cards in the current host window's viewport, without taking focus when a card appears. Include `imkit/toast.h` or `imkit/imkit.h`. The existing `ToastRegion` overloads are unchanged. The host owns state, content, context, fonts, renderer and application operations; no React dependency, global registry, worker or persistence is introduced.

## Colored cards

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

The example needs `<array>`, `<vector>` and `<algorithm>`. Rules for using it:

- Strings are borrowed UTF-8; keep their storage valid while drawing.
- Use unique nonzero IDs.
- Keep one state per viewport and context, and call `Reset()` before reusing a context.
- Call on the UI thread. The host passes worker results across.
- Create covering host windows with `ImGuiWindowFlags_NoBringToFrontOnFocus`, for example `ImGui::Begin("App", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus)`, before drawing toasts. Otherwise a click on a host window can raise it above existing toasts.
- Toasts are ordinary public ImGui windows. They do not force the display order through internal APIs or steal focus, and new overlapping windows and modal dialogs keep Dear ImGui's ordering and input rules.

## Duration and queue

| Case | Behavior |
| --- | --- |
| `duration` negative | Defaults: success and info last five seconds; warning and error persist |
| `duration` zero | Persists |
| `duration` positive | That many seconds |
| `Loading` phase | Always persists |
| Hover or keyboard focus | Pauses expiry; waiting cards do not age |

Draw once per frame. `UpdateToastViewport` advances timing without drawing; do not call it for a state you also draw in the same frame.

**Progress.** Set `phase=ToastPhase::Loading`. The toast uses the shared thin rounded track, with `stage`, the status, and a percentage or `progressText` outside the bar. A negative or nonfinite `progress` shows a short moving segment while the status is `Running`; paused and terminal unknown states stay still. Other values are clamped to 0–1.

**Completion.** Update the same ID to `phase=Message`, `Succeeded` or `Failed`, with a success or error kind. Changing the phase, kind or duration restarts the timer without changing the insertion order. Text and progress updates do not restart it.

The trailing `ToastView` fields `stage`, `progressText` and `status` have defaults, so existing aggregate initializers keep their field order and behavior.

```cpp
toast.phase = imkit::ToastPhase::Loading;
toast.stage = "Inspecting files";
toast.progress = fraction; // Negative or nonfinite means indeterminate.
toast.status = imkit::ProgressStatus::Running;
toast.action = "View details"; // Returns an Action event for this toast ID.
// On completion, update this same ID and set phase=Message, status=Succeeded/Failed.
```

**Queue limits**

- The newest card appears closest to the selected edge; extra cards wait.
- The last duplicate ID wins.
- State holds up to 64 IDs, including expired IDs awaiting host removal. `state.overflow` reports untracked IDs, which enter when slots free up.
- An expired ID stays hidden until it is removed or explicitly updated.
- `events.overflow` reports a full event buffer. Undelivered expiry requests retry in later frames; an action or dismiss that did not fit needs another activation.
- Reset the buffer count every frame.

## Placement and customization

| Setting | Behavior |
| --- | --- |
| `ToastPosition` | Top or bottom, times left, center or right. Default `TopRight`, with `maximum=4` |
| Width zero | Font-relative width |
| Negative margin or gap | Uses the theme-derived current style spacing |
| Size limits | Cards and the visible count stay within the viewport work area; an oversized card scrolls vertically |

**Look.** Cards use the raised surface with a status accent bar and icon. The status colour comes from semantic tokens, and the text keeps the theme's text contrast. A hairline along the bottom edge shows the remaining display time and pauses on hover or focus. The theme controls the radius, the entry slide and fade, and reduced motion. Reduced motion stops the slide, the fade, the countdown tween, the indeterminate segment and the spinner.

**Host input.** Pass `ComponentOptions` for the theme, locale and semantic publication. The `dismiss` locale key supplies the close button's accessible name. Titles, descriptions and action labels come from the host. Buttons support native keyboard navigation, and no global shortcut or automatic focus transfer is installed.

**Actions.** `action` supplies a button, `actionDisabled` disables it, and `dismissible` controls the close button. `ToastEvent` distinguishes `Action`, `Dismiss` and `Expired`. The host decides whether an action removes or updates a toast, and cancelling work is the host's job.

## Gallery and validation

Open **Toasts** (page 20): six positions, timing overrides, queue bursts, same-ID success and error completion, determinate and indeterminate progress, a details action, and English and Japanese examples. **Appearance** changes theme, density, contrast and reduced motion.

![Native Loading to Success toast interaction](/imgui-modern-kit/docs-images/v4-toasts-poster.png)

[Open native GIF](/imgui-modern-kit/docs-images/v4-toasts.gif)

`imkit.toast` covers timing, pauses, queues, updates, buffer bounds, duplicate IDs, layout and public mouse requests. `imkit.toast_api_compile` and the external consumer cover public signatures. Gallery GPU capture is separate from native OS/IME, screen-reader, multi-monitor DPI and macOS acceptance.
