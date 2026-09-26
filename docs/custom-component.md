# Author a custom component

[日本語](custom-component.ja.md) · [Documentation index](README.md) · [How ImKit works](how-it-works.md)

ImKit components are small compositions over Dear ImGui behavior: a `ThemeScope`, semantic colors from your theme, host-owned state, and a returned request. Once you follow that shape, you can add a control that fits the design system without touching the library. This page shows the rules and a complete, minimal custom control — a "chip" toggle.

![Themed panels in the Gallery](images/gallery-overview.gif)

## The core pattern

Every custom component obeys the same four moves:

1. **Wrap in a `ThemeScope`.** The control is drawn in the active theme and restores the previous context when the block ends.
2. **Read semantic colors, not raw hex.** Pull colors from `theme.semantic.*` so the control automatically follows light/dark, contrast, and custom accents.
3. **Own the state on the host.** The control reads/writes a value your model holds. ImKit never retains it.
4. **Report, don't act.** Return a request (`true` when something changed). Your code decides what the change means.

## What you'll build

- A small toggle chip: a label, an on/off dot, and a click.
- A `ThemeScope` and semantic colors so it themes itself.
- A host `bool` and a `StableId` for focus.
- A returned toggle request.

## What you need

1. A Dear ImGui context and window.
2. A `Theme` (from `MakeTheme`/`MakePrecisionTheme`).
3. CMake linking the `imkit::imkit` target (and `accessibility` for the optional a11y step).

## Own the state

```cpp
#include <imkit/imkit.h>
#include <imkit/accessibility.h>

struct ChipState {
    bool on = false;
    const char *label = "Notifications";
    imkit::StableId id = 1001;   // nonzero: focus + accessibility identity
    bool disabled = false;
};
```

`id` is a 64-bit stable value you assign (never `ImGui::GetID` for the a11y path, which must survive frame churn). Keep it alive for the control's lifetime.

## Theme it and draw it

The control is a `ThemeScope` + a few semantic colors + Dear ImGui primitives. Notice the colors come from `theme.semantic`, so the chip re-skins itself when the theme changes.

```cpp
bool DrawChip(const ChipState &s,
              const imkit::Theme &theme,
              imkit::ComponentOptions options = {}) {
    imkit::ThemeScope scope(theme);                 // 1. theme; restores context

    // 2. semantic colors (auto light/dark, contrast, custom accent)
    ImVec4 bg   = s.on ? theme.semantic.accent : theme.semantic.surfaceRaised;
    ImVec4 ink  = s.disabled ? theme.semantic.textDisabled : theme.semantic.text;
    float r      = theme.metrics.radius;
    ImVec2 size  = { 96.f, theme.metrics.controlHeight };   // from theme metrics

    bool pressed = !s.disabled && ImGui::InvisibleButton("##chip", size);
    if (pressed) s.on = !s.on;                              // 3. host value flips

    // 4. draw the pill + label with theme colors only
    ImGuiDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetItemRectMin();
    dl->AddRoundedRect(p, p + size, r,
                       (ImU32)ImGui::ColorImVec4(theme.semantic.border), bg);
    ImGui::SetCursorScreenPos(p + ImVec2(size.x * .36f, size.y * .5f));
    ImGui::PushStyleColorV3(ImGuiCol_Text, ink);
    ImGui::TextUnformatted(s.label);
    ImGui::PopStyleColorV3(1);
    // A dot: dl->AddCircle(p + ImVec2(size.x * .24f, size.y * .5f), 4.f,
    //                     s.on ? theme.semantic.onAccent : theme.semantic.border, 32);

    // Advanced: read options.accessibility / options.parent / options.locale /
    // options.animation here for focus, a11y, localized strings, motion.
    (void)options;

    return pressed;                    // 5. report; the caller decides what it means
}
```

The visual details (pill, dot, label) are up to you. The important parts are that **all colors come from the theme** and **all state is the host's**. Swap in `theme.semantic.success` for a "ready" chip, `theme.semantic.error` for a fault indicator, and `ThemeScope` keeps the surrounding context untouched.

The visual details (pill, dot, label) are up to you. The important part is that **all colors come from the theme** and **all state is the host's**. Swap in `theme.semantic.success` for a "ready" chip, `theme.semantic.error` for a fault indicator, and `ThemeScope` keeps the surrounding context untouched.

## Report, don't act

```cpp
// In your frame, after NewFrame():
if (DrawChip(chip, theme, { .disabledReason = "Locked" })) {
    chip.on ? PushEvent(chip.id, "on") : PushEvent(chip.id, "off"); // your code
}
```

The returned `true` means "user clicked." Your code decides what that means — persisting, toggling a global, firing an event. The `ComponentOptions` struct carries the extras a control may need: `theme`, `animation`, `accessibility`, `parent` (StableId), `locale`, and `disabledReason`.

## Optional: accessibility annotations

For larger controls, expose them to screen readers and native a11y. Build an `AccessibilityFrame` once, then annotate the item you just drew:

```cpp
imkit::accessibility::AccessibilityFrame frame(storage, actions);   // host-owned storage/queue
frame.Begin(generation);
// ... draw the control ...
frame.Add({
    .id = s.id,
    .parent = parentId,
    .role = imkit::accessibility::SemanticRole::Toggle,
    .name = s.label,
    .value = s.on ? "on" : "off",
    .state = { .checked = s.on, .disabled = s.disabled, .focused = false },
    .actions = imkit::accessibility::SemanticAction::Press,
});
frame.Publish(sink);
```

The node fields are borrowed until the host publishes the frame. Use `AnnotateLastItem` when attaching to an immediately-previous ImGui item. This is how you make a custom control participate in keyboard focus, screen readers, and high-contrast mode.

## How to run it

- Drop `DrawChip` into the Gallery or your own app and call it in a frame. It themes itself against the active `Theme`.
- In `tests/design_system.cpp`-style fixtures you can assert the chip's colors follow `ThemePresets()` and that `ThemeScope` restores the prior context.

## The rules that keep it right

- **ThemeScope always.** No component escapes the active theme.
- **Semantic colors only.** Never bake `ImVec4{...}` hex literals into a control; read `theme.semantic.*`.
- **Host owns state.** The control writes your value and returns a request; it never persists, allocates, or keeps pointers across frames.
- **Stable `id`.** Use a nonzero `StableId` for focus and a11y. Don't reuse a value from `ImGui::GetID`.
- **Gate on `disabled`.** Skip input when `disabled` is set, and surface `disabledReason` to the user.
- **Balance every scope.** `ThemeScope` is a RAII block; `Begin`/`End` pairs close even when they returned false.

## Next

- [Components and recipes (per-control reference)](components.md)
- [Design system](design-system.md)
- [Architecture](architecture.md)
