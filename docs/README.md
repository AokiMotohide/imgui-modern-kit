# ImKit documentation

[日本語](README.ja.md) · [Project overview](../README.md)

ImKit's documentation is organized in three stages, following the patterns of established GUI projects (Dear ImGui, GTK, Qt, egui, Flutter):

1. **Learn** — the mental model: ownership, the Dear ImGui frame, theming, and module layout.
2. **Build** — runnable codelabs, from your first themed window to a node editor, timeline, and custom component.
3. **Reference and verify** — the authoritative module contracts, API inventories, and dated validation evidence.

For the complete bilingual page map, audience, source/API mapping, and package paths, see the [documentation catalog](documentation-catalog.md). For practical, module-by-module examples, see [examples and recipes](examples-recipes.md).

## Learn the mental model

| Read | What it answers |
|---|---|
| [How ImKit works](how-it-works.md) | Who owns what, the Dear ImGui frame, and where ImKit fits |
| [Architecture](architecture.md) | The authoritative ownership model, module layout, and public contract |
| [Design system](design-system.md) | Design tokens, themes, and icon APIs |
| [Dependencies](dependencies.md) | The required Dear ImGui revision and external packages |
| [User guide](guide.md) | A short orientation to the host–library split and first calls |

## Build it (codelabs)

Step-by-step, host-owned builds you can port into your application. Each pairs with a reference page below that defines the full contract.

- [Build your first ImKit app](build-first-app.md) — context, theme, and your first themed controls.
- [Build a settings screen](build-settings-screen.md) — host state, setting rows, and save/validation/notify.
- [Build a node editor](build-node-editor.md) — graph snapshot in, edit requests out.
- [Build a timeline editor](build-timeline.md) — tracks, clips, fades, transitions, and ripple moves.
- [Author a custom component](custom-component.md) — `ThemeScope`, semantic colors, and reporting.

## Start using ImKit (by goal)

| Goal | Read |
|---|---|
| Add ImKit to an existing Dear ImGui application | [Getting started](getting-started.md) |
| Understand host and library ownership, themes, and module choices | [User guide](guide.md), [Architecture](architecture.md) |
| Build a common settings interface from reusable controls | [Components and recipes](components.md) |
| Explore the live examples | [Gallery guide](gallery.md) |
| Choose, customize, and apply a theme | [Themes](themes.md) |
| Diagnose configuration or runtime issues | [Troubleshooting](troubleshooting.md) |
| Upgrade an existing v2 consumer | [v3 migration](migration-v3.md) |
| Find the Gallery page, source, API, and next reading for a task | [Learning map and recipes](examples-recipes.md) |

## Integrate a specific module

| Module | Guide | Contract or deeper reference |
|---|---|---|
| Node Editor | [Node Editor integration](node-editor.md) | [Generated API inventory](node-editor-api.json), [recipe](examples-recipes.md) |
| Workflow and data components | [Workflow components](workflow-components.md) | [Shell components](shell-components.md), [recipes](examples-recipes.md) |
| Editor Core, Video, and CG | [Editor Suite](editor-suite.md) | [Editor API](editor-api.md), [Timeline editing](timeline-editing.md), [recipes](examples-recipes.md) |
| Window frame | [Gallery WindowFrame guide](gallery-window-frame.md) | [WindowFrame API inventory](window-frame-api-inventory.json), [recipe](examples-recipes.md) |
| Icons | [Icon reference](icons.md) | [recipe](examples-recipes.md) |

## Verify a claim or inspect the API

| Question | Canonical reference |
|---|---|
| What does ImKit own, and what stays in the host? | [Architecture](architecture.md) |
| Which Dear ImGui revision and external packages are required? | [Dependencies](dependencies.md) |
| Which Dear ImGui functions and overloads are exposed? | [Public API coverage](api-coverage.md), [API inventory](api-inventory.json) |
| What has been built or exercised, and what remains unverified? | [Validation](validation.md) |
| What does the widget catalog cover? | [Widget inventory](widget-inventory.md) |
| What are the design-system tokens and APIs? | [Design system](design-system.md), [Design-system API inventory](design-system-api.json) |

`architecture.md` and `validation.md` are the canonical ownership and evidence references. Validation is time- and revision-specific: a recorded pass does not automatically describe a later checkout or establish native OS input, accessibility, external-host, or physical-device acceptance.

## Maintainer notes and design records

These pages preserve project decisions, proposals, checklists, and review context. They are not consumer API promises; use the public headers and the contract references above when integrating ImKit.

Some records describe an earlier release or checkout. Their status tables are historical snapshots, not current TODO lists or publication authorization; use their stated baseline and date and confirm current behavior against the public contract pages.

- [Design proposals](design-proposals.md) · [Design refinements](design-refinements.md)
- [Development status](development-status.md) · [Editor refresh](editor-refresh.md) · [Editor validation](editor-validation.md)
- [Editor implementation checklist](editor-implementation-checklist.md) · [Node Editor review](node-editor-review.md)
- [GitHub profile copy](github-profile.md)
- [Performance evidence](evidence/)

## Documentation approach

The information flow follows patterns used by established GUI projects: Dear ImGui routes readers from setup to backend-specific examples and its live demo; GTK starts with a buildable first application; Qt separates tutorials/examples from API reference; egui pairs concise examples with an interactive demo. ImKit applies these structure choices to its host-owned, immediate-mode contract without copying their code or presentation. See [Dear ImGui Getting Started](https://github.com/ocornut/imgui/wiki/Getting-Started), [GTK Getting Started](https://docs.gtk.org/gtk4/getting_started.html), [Qt documentation categories](https://doc.qt.io/qt-6/qdoc-categories.html), and [egui](https://docs.rs/egui/latest/egui/).
