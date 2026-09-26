# How ImKit works

[日本語](how-it-works.ja.md) · [Documentation index](README.md) · [User guide](guide.md)

ImKit sits **on top of** Dear ImGui, not beside it and not instead of it. The single most useful thing for using ImKit correctly is the mental model: how a Dear ImGui frame works, and **who owns what**. This page explains that model in approachable terms; [Architecture](architecture.md) is the authoritative, formal reference for ownership and design.

## What ImKit is (and is not)

ImKit is a C++20 static library. It:

- Reuses Dear ImGui's own widgets and extends them with themed variants (`imkit::imkit`).
- Adds higher-level composites: the node editor, video timelines, workflow panels, and application chrome.
- Provides named themes, an icon set, and a design-system token model.

It does **not**:

- Create or own a Dear ImGui context, backend, renderer, or frame loop.
- Decode or play media, run simulations, or own your application's data.
- Persist anything. Every piece of state lives in your application.

That separation is deliberate. Because ImKit never owns the Dear ImGui machinery, it can be dropped into an application that already has a context, and your ownership, undo, and persistence semantics are untouched.

## How a Dear ImGui frame works

Dear ImGui is **immediate mode**. Every frame, your application redraws the entire interface from scratch. The pattern is:

1. Your frame loop calls `ImGui::NewFrame()`.
2. Your code issues drawing calls in any order; ImKit calls are just more of these.
3. Your code calls `ImGui::Render()`, which hands the accumulated drawing to the backend.
4. The next frame starts again, from a clean slate.

There is no persistent widget tree. Every call re-creates the widget. State (text, selections, toggles) must live **outside** the frame loop, in your application's model, and be read by each frame.

## Where ImKit fits

![Native Gallery overview](images/gallery-overview.gif)

Think of ImKit as a thin layer of themed widgets and composites drawn inside your existing Dear ImGui frame:

```
Your application
  └── Dear ImGui context + backend (GLFW/OpenGL, Metal, ...)
       └── Your frame loop
            ├── ImGui::NewFrame()
            │    ├── plain Dear ImGui widgets
            │    └── ImKit themed widgets and composites
            ├── ImGui::Render()
            └── next frame
```

ImKit calls operate on the **current** Dear ImGui context. They never create one, and they never outlive the frame in which they are drawn.

## Who owns what (the ownership model)

This is the rule to memorize. **ImKit returns requests; it never acts on your data by itself.**

| Thing | Owner |
|---|---|
| Dear ImGui context, backend, renderer, frame loop | **You (the host)** |
| Fonts and the font atlas | **You** |
| The data you display and edit (documents, scene, media, graph) | **You** |
| Selection, undo/redo, persistence, settings | **You** |
| The `Theme` value and when it is applied | **You** |
| The drawing that appears on screen | ImKit, *given* your context and data |
| The result of a button press, drag, or selection | **You**, via the returned request |

In one line: **you own everything; ImKit only draws and reports.** When ImKit gives you something back — a clicked button ID, a dragged clip, a requested edit — it is **your** code that decides what it means and what to do with it.

## A frame with ImKit, line by line

```cpp
// One-time setup, outside the frame loop:
auto theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);

// Per frame:
imkit::ApplyTheme(theme, 1.0f);          // before ImGui::NewFrame()
ImGui::NewFrame();

// Draw your UI. ImKit calls are just more drawing calls.
if (imkit::Begin("Settings")) {
    imkit::ToggleButton("Verbose", &verbose);             // reads/writes your bool
    imkit::ActionButton("Save", imkit::ActionVariant::Primary);  // true when pressed
}
imkit::End();

ImGui::Render();
```

Three things to notice:

- `ApplyTheme` is applied to the **current** context, once, before `NewFrame`.
- ImKit widgets **read** values you own (`&verbose`) and **return** results you handle (`ActionButton` returns a `bool`).
- `Begin`/`End` bracket a group of ImKit calls; they are a drawing scope, not an object with a lifetime.

## Key takeaways

- ImKit draws inside your Dear ImGui frame; it never creates or owns the context.
- Every frame is redrawn from scratch; your model owns all state.
- ImKit returns requests — your code is the one that acts on them.
- Keep theme application before `NewFrame` and `Begin`/`End` balanced.

## Next

- [Getting started](getting-started.md) — set up your first project.
- [Build your first ImKit app](build-first-app.md) — a full, runnable walkthrough.
- [Architecture](architecture.md) — the authoritative ownership and design reference.
