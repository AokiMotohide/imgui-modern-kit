---
title: "Build a custom component"
description: "ImKit's own controls are small compositions: a Dear ImGui item for the interaction, theme tokens for colour and size, a value that the caller owns, an…"
---

ImKit's own controls are small compositions: a Dear ImGui item for the interaction, theme tokens for colour and size, a value that the caller owns, and a returned result. A control you write the same way follows the theme without any change to the library. This tutorial builds a pill-shaped toggle, `ChipToggle`.

![Picking a theme in the Gallery recolours every component that reads semantic colours](/imgui-modern-kit/docs-images/v4-themes-poster.png)

[Open native GIF](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-themes.gif)

## The pattern

1. **Let Dear ImGui handle the interaction.** Use an item such as `InvisibleButton`. Hover, focus, keyboard navigation, IDs and disabled state then stay native.
2. **Read colours from `theme.semantic`.** The control then follows light and dark themes, contrast and custom accents. Take sizes from the themed frame height, not from literals.
3. **The caller owns the state.** The control edits a value through a pointer and never stores it.
4. **Return the result.** Return true when the user changed the value. The caller decides what that means.

## Before you start

A Dear ImGui context and window, a `Theme` from `MakeTheme`, and `imkit::imkit` linked.

## Step 1: Write the control

```cpp
#include <imgui.h>
#include <imkit/imkit.h>

namespace a11y = imkit::accessibility;

// A pill-shaped toggle. It edits *on and returns true on the frame the user changes it.
bool ChipToggle(const char* label, bool* on, const imkit::Theme& theme,
                a11y::AccessibilityFrame* access = nullptr, a11y::StableId id = 0) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const imkit::SemanticColors& color = theme.semantic;

    // Size from the themed frame height, so the chip follows density and scale.
    const float height = ImGui::GetFrameHeight();
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const ImVec2 size{height + textSize.x + style.FramePadding.x, height};

    // Dear ImGui provides the hit test, hover, focus and disabled behaviour.
    const bool pressed = ImGui::InvisibleButton(label, size);
    if (pressed) *on = !*on;

    if (access) {                                    // optional: describe the item to assistive technology
        a11y::SemanticNode node;
        node.id = id;
        node.role = a11y::SemanticRole::Toggle;
        node.name = label;
        node.state.checked = *on;
        node.actions = a11y::SemanticAction::Toggle;
        a11y::AnnotateLastItem(*access, node);
    }

    // Draw with semantic colours only.
    const ImVec2 min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
    const ImVec4 fill = *on ? color.accent : (ImGui::IsItemHovered() ? color.surfaceRaised : color.surface);
    const ImVec4 ink = *on ? color.onAccent : color.text;
    const float round = height * 0.5f;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(min, max, ImGui::GetColorU32(fill), round);
    draw->AddRect(min, max, ImGui::GetColorU32(color.border), round);
    draw->AddCircleFilled({min.x + round, min.y + round}, height * 0.14f, ImGui::GetColorU32(ink));
    draw->AddText({min.x + height, min.y + (height - textSize.y) * 0.5f}, ImGui::GetColorU32(ink), label);
    if (ImGui::IsItemFocused())
        draw->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_NavCursor), round, ImDrawFlags_None, 2.0f);
    return pressed;
}
```

| Part | Why |
| --- | --- |
| `ImGui::GetFrameHeight()` | The height follows the theme's density and the application scale |
| `ImGui::InvisibleButton(label, size)` | Gives the item an ID, hit test, hover, focus and disabled behaviour |
| `color.accent`, `onAccent`, `surface`, `surfaceRaised`, `text`, `border` | Semantic colours, so a theme change recolours the chip |
| `ImGui::GetColorU32(ImVec4)` | Applies the style alpha, so the chip dims inside `BeginDisabled` |
| `ImGui::IsItemFocused()` | Draws a focus ring for keyboard and gamepad navigation |

The label must not contain `##`. To draw a different label from the ID, build the text yourself before the call.

## Step 2: Use it

```cpp
    bool notifications = true;   // your value

    // Inside the frame
    if (ChipToggle("Notifications", &notifications, theme)) {
        SaveNotificationSetting(notifications);   // your code decides what the change means
    }
```

Wrap the call in `imkit::ThemeScope` when one panel should use another theme. Call `imkit::BeginDisabled(true)` before the chip to disable it. The chip then ignores input and draws dimmed.

## Step 3: Describe it to assistive technology (optional)

`AnnotateLastItem` attaches a semantic node to the item you just drew. It submits no new item. The host owns the node storage and publishes the frame to its platform bridge.

```cpp
std::array<imkit::accessibility::SemanticNode, 64> storage;   // host-owned
imkit::accessibility::AccessibilityFrame access(storage);

access.Begin(generation);                                    // once per frame, before drawing
ChipToggle("Notifications", &notifications, theme, &access, 1001);
access.Publish(sink);                                        // after drawing, to your platform bridge
```

Give each control a nonzero `StableId` that you choose. Do not use `ImGui::GetID` for it, because the identity must stay the same between frames. The node's text is borrowed until you publish the frame.

## Rules

- Take colours from `theme.semantic` and sizes from the themed frame. Do not write colour literals into a control.
- Keep interaction in Dear ImGui items so focus, IDs, `Begin`/`End` pairing and disabled state stay native.
- The caller owns the value. The control must not store it, allocate, or keep pointers across frames.
- Return a result and let the caller act.
- Use a stable, nonzero `StableId` for accessibility.

## Next

- [Components](../../features/components/): the library's own controls.
- [Design system](../../features/design-system/): the tokens in detail.
- [Architecture](../../guide/architecture/)
