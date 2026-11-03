---
title: "How ImKit works"
description: "ImKit is a layer on top of Dear ImGui. It adds themes, controls and editor widgets that you call from your own Dear ImGui frame."
---

ImKit is a layer on top of Dear ImGui. It adds themes, controls and editor widgets that you call from your own Dear ImGui frame. To use it correctly you only need to know how a frame runs and who owns what. This page covers both. [Architecture](../../guide/architecture/) is the formal reference.

## What ImKit does and does not do

| ImKit does | ImKit does not |
| --- | --- |
| Wrap Dear ImGui widgets with themed versions (`imkit::imkit`) | Create or own the Dear ImGui context, backend, renderer or frame loop |
| Provide composite widgets: workflow panels, charts, a node editor, a timeline, a show module | Decode or play media, or run simulations |
| Provide named themes, 321 icons and a token model for building your own look | Keep your data, selection, Undo history or saved files |

Because ImKit owns none of the Dear ImGui machinery, you can add it to an application that already has a context without changing how that application works.

## One Dear ImGui frame

Dear ImGui is an immediate-mode library: the interface is rebuilt every frame.

1. Your loop calls `ImGui::NewFrame()`.
2. Your code calls widget functions in any order. ImKit functions are widget functions too.
3. Your code calls `ImGui::Render()`, which hands the draw lists to your backend.
4. The next frame starts from an empty state.

No widget objects survive between frames. Text, selections and flags live in your application's data, and each frame reads and updates them.

## Who owns what

![ImKit draws and reports; the application owns data, state and the Dear ImGui setup](/imgui-modern-kit/docs-images/ownership-en.svg)

| Item | Owner | Notes |
| --- | --- | --- |
| Dear ImGui context, backend, renderer, frame loop | Your application | ImKit never creates or destroys them |
| Fonts and the font atlas | Your application | `Theme::fonts` holds borrowed pointers |
| Documents, scenes, graphs, media, settings | Your application | ImKit does not copy or keep them |
| Selection, Undo/Redo, saving | Your application | |
| The `Theme` value and when it is applied | Your application | |
| What appears on screen | ImKit | Drawn from the data you pass in |
| What a click, drag or connection means | Your application | ImKit returns a request; you validate and apply it |

When ImKit returns something, such as a pressed button, a dragged clip or a connection request, it is a request. Your code decides what it means and updates your data.

## One frame with ImKit

The Before / After page of the Gallery edits the same values with plain Dear ImGui on the left and ImKit on the right.

![Plain Dear ImGui and ImKit editing the same values side by side](/imgui-modern-kit/docs-images/v4-compare-poster.png)

[Open native GIF](https://raw.githubusercontent.com/AokiMotohide/imgui-modern-kit/main/docs/images/v4-compare.gif)

```cpp
#include <imkit/imkit.h>

// Once, outside the frame loop
imkit::Theme theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);

// Your application state
bool verbose = false;

// Every frame
imkit::ApplyTheme(theme);          // before ImGui::NewFrame()
ImGui::NewFrame();

if (imkit::Begin("Settings")) {
    imkit::Toggle("Verbose", &verbose);                        // reads and writes your bool
    if (imkit::ActionButton("Save", imkit::ActionVariant::Primary)) {
        SaveSettings(verbose);                                 // your code decides what Save does
    }
}
imkit::End();

ImGui::Render();
```

- `ApplyTheme` writes the theme into the current context's style. Call it before `NewFrame`. It rebuilds the style from unscaled tokens, so calling it every frame does not compound sizes.
- ImKit widgets read values you own (`&verbose`) and return results you handle (`ActionButton` returns `bool`).
- `Begin` and `End` follow Dear ImGui's rule: always call `End`, even when `Begin` returns false.

## Summary

- ImKit draws inside your Dear ImGui frame and never creates or owns the context.
- Every frame is redrawn from your data. Your application keeps all state.
- ImKit returns requests, and your code applies them.
- Apply the theme before `NewFrame`, and keep `Begin` and `End` paired.

## Next

- [Getting started](../../getting-started/): set up a project.
- [Build your first app](../../tutorials/first-app/): a complete walkthrough.
- [Architecture](../../guide/architecture/): ownership rules and module boundaries.
