# ImKit documentation

[日本語](目次.md) · [Project overview](../README.md) · [Changelog](../CHANGELOG.md)

ImKit is a C++20 static library of themes, controls and editor components for Dear ImGui applications. Every page here exists in English and Japanese with the same content. The [documentation catalog](reference/documentation-catalog.md) lists each page with its audience and source headers.

## Start here

| If you want to | Read |
| --- | --- |
| See what ImKit looks like before you integrate it | [Gallery guide](getting-started/gallery.md) |
| Add ImKit to a CMake project and draw a first frame | [Getting started](getting-started/getting-started.md) |
| Understand the frame model and who owns what | [How ImKit works](getting-started/how-it-works.md) |
| Build something step by step | [Tutorials](#tutorials) |
| Find the header and target for a feature | [Examples and recipes](getting-started/examples-recipes.md) |
| Give your application its own look | [Themes](architecture/themes.md), then [Make your own look](architecture/make-your-own-look.md) |
| Upgrade from an earlier version | [Migration to 4.0](reference/migration-v4.md) |
| Fix a build or runtime problem | [Troubleshooting](reference/troubleshooting.md) |

## Documents

### Getting started

| Page | Content |
| --- | --- |
| [Getting started](getting-started/getting-started.md) | Requirements, CMake integration, the first frame |
| [How ImKit works](getting-started/how-it-works.md) | The immediate-mode frame and the ownership model |
| [User guide](getting-started/guide.md) | Applying a theme, the control families, linking |
| [Gallery guide](getting-started/gallery.md) | Running the Gallery and finding each page's source |
| [Examples and recipes](getting-started/examples-recipes.md) | Header, target and Gallery page for each area |

### Tutorials

| Page | You build |
| --- | --- |
| [Build your first app](tutorials/build-first-app.md) | A one-frame program with a theme, a toggle and a button |
| [Build a settings screen](tutorials/build-settings-screen.md) | A form with validation, saving and a notification |
| [Build a node editor](tutorials/build-node-editor.md) | A graph model that applies edit requests, with Undo |
| [Build a timeline](tutorials/build-timeline.md) | Tracks and clips with move, trim and split |
| [Build a custom component](tutorials/custom-component.md) | A themed control written the way ImKit's own are |

### Architecture

| Page | Content |
| --- | --- |
| [Architecture](architecture/architecture.md) | Modules, ownership and the platform boundary |
| [Design system](architecture/design-system.md) | Semantic tokens, typography and contrast |
| [Themes](architecture/themes.md) | The 13 presets, customization and `ThemeScope` |
| [Make your own look](architecture/make-your-own-look.md) | Brand colours, theme files, outlines, shadows, skins and sprites |
| [Icons](architecture/icons.md) | 321 icons, atlases and GPU upload |
| [Dependencies](architecture/dependencies.md) | Dear ImGui and GLFW, the pinned revision, CMake resolution |

### Components

| Page | Content |
| --- | --- |
| [Components](components/components.md) | Themed controls and a full settings panel |
| [Charts](components/charts.md) | Donut, bar, gauge and sparkline |
| [PTZ and range sliders](components/ptz.md) | Camera control and two-handle sliders |
| [Toasts](components/toasts.md) | Notification cards, positions, actions and progress |
| [Workflow components](components/workflow-components.md) | Navigation, hierarchy, feedback, progress and an image viewer |
| [Shell components](components/shell-components.md) | App bars, workspace headers and drawers |
| [Node editor](components/node-editor.md) | The snapshot and request model for graphs |
| [Timeline editing](components/timeline-editing.md) | Fades, transitions, selection and track moves |
| [Editor suite](components/editor-suite.md) | Editor core, video and CG surfaces |
| [Show module](components/show.md) | Live, rules, schedule, surface, LED and tile map |
| [Window frame](components/gallery-window-frame.md) | A themed title bar and OS adapters |

### Reference

| Page | Content |
| --- | --- |
| [Public API coverage](reference/api-coverage.md) | Every wrapped Dear ImGui overload and ImKit function |
| [Editor API](reference/editor-api.md) | Editor suite types, signatures and provider contracts |
| [Widget inventory](reference/widget-inventory.md) | The component list and the audit decisions |
| [4.0 additions](reference/v4.0-api.md) | What 4.0 added, by header |
| [3.3 additions](reference/v3.3-api.md) and [3.2 additions](reference/v3.2-api.md) | Progress tracks, stages, task cards, workspace parts |
| [Migration to 4.0](reference/migration-v4.md), [from v2](reference/migration-v3.md) | Rebuild notes, changed defaults, deprecated spellings |
| [Validation](reference/validation.md) | What was verified, and what was not |
| [Troubleshooting](reference/troubleshooting.md) | Build, link and runtime problems |

## Versions

| Version | Where |
| --- | --- |
| In development (`main`) | [Changelog, Unreleased](../CHANGELOG.md): tile map, sprites, adjustable control sizes, PTZ capabilities |
| 4.0 | [4.0 additions](reference/v4.0-api.md), [migration guide](reference/migration-v4.md) |
| 3.3 and 3.2 | [3.3 additions](reference/v3.3-api.md), [3.2 additions](reference/v3.2-api.md) |

`docs/archive/` keeps design notes and audits from earlier milestones. They explain past decisions and do not promise any API.
