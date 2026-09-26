# Documentation catalog

[日本語](文書カタログ.md) · [Table of Contents](../README.md)

This catalog outlines all public ImKit documentation, their intended audiences, roles, and implementation mappings.
Every document is maintained as an English and Japanese pair.

## Overview

Documentation is organized into five structured categories based on development phase and intent:

1. **Getting Started (`getting-started/`)**: Setup, design philosophy, and core principles.
2. **Tutorials (`tutorials/`)**: Step-by-step guides for building real application screens.
3. **Architecture (`architecture/`)**: Internal design, design system tokens, and dependencies.
4. **Components (`components/`)**: Component specifications and host ownership contracts.
5. **Reference (`reference/`)**: Complete API listings, validation records, migration, and troubleshooting.

## Document catalog by category

### 1. Getting Started

| Document (EN / JA) | Audience & Focus | Headers / Source |
|---|---|---|
| [Getting Started](../getting-started/getting-started.md) / [JA](../getting-started/導入ガイド.md) | CMake integration and minimal initialization | `imkit/imkit.h` |
| [How it Works](../getting-started/how-it-works.md) / [JA](../getting-started/仕組みと設計思想.md) | Dear ImGui frame loop and host-ownership mental model | Overall architecture |
| [Guide](../getting-started/guide.md) / [JA](../getting-started/利用ガイド.md) | Theme application and core component usage | `imkit/imkit.h` |
| [Gallery Guide](../getting-started/gallery.md) / [JA](../getting-started/ギャラリーガイド.md) | Interactive sample gallery startup, usage, and verification | `examples/gallery/` |
| [Examples & Recipes](../getting-started/examples-recipes.md) / [JA](../getting-started/実例とレシピ.md) | Module-by-module real-world code recipes | Various modules |

### 2. Tutorials

| Document (EN / JA) | Audience & Focus | Headers / Source |
|---|---|---|
| [Build First App](../tutorials/build-first-app.md) / [JA](../tutorials/最初のアプリの作成.md) | Creating a minimal standalone ImKit application | `imkit/imkit.h` |
| [Build Settings Screen](../tutorials/build-settings-screen.md) / [JA](../tutorials/設定画面の作成.md) | Property grids, toggle rows, and toast notifications | `imkit/workflow.h` |
| [Build Node Editor](../tutorials/build-node-editor.md) / [JA](../tutorials/ノードエディタの作成.md) | Snapshot/request node graph editing | `imkit/node_editor.h` |
| [Build Timeline](../tutorials/build-timeline.md) / [JA](../tutorials/タイムラインの作成.md) | Multi-track timeline, trimming, and fade transitions | `imkit/video.h` |
| [Custom Component](../tutorials/custom-component.md) / [JA](../tutorials/カスタムコンポーネントの作成.md) | Building custom widgets with design tokens | `imkit/imkit.h` |

### 3. Architecture

| Document (EN / JA) | Audience & Focus | Headers / Source |
|---|---|---|
| [Architecture](../architecture/architecture.md) / [JA](../architecture/アーキテクチャ.md) | Layer structure, module dependencies, and state boundaries | Overall design |
| [Design System](../architecture/design-system.md) / [JA](../architecture/デザインシステム.md) | Color, spacing, typography tokens, and accessibility | `imkit/theme.h` |
| [Themes](../architecture/themes.md) / [JA](../architecture/テーマ.md) | 13 preset themes, ThemeScope, and custom palette | `imkit/theme.h` |
| [Icons](../architecture/icons.md) / [JA](../architecture/アイコン.md) | Procedural icons, atlas management, and HiDPI drawing | `imkit/icons.h` |
| [Dependencies](../architecture/dependencies.md) / [JA](../architecture/依存関係.md) | Pinned Dear ImGui docking revision and CMake options | `CMakeLists.txt` |

### 4. Components

| Document (EN / JA) | Audience & Focus | Headers / Source |
|---|---|---|
| [Basic Components](../components/components.md) / [JA](../components/基本コンポーネント.md) | Buttons, badges, segments, switches, and core widgets | `imkit/imkit.h` |
| [Node Editor](../components/node-editor.md) / [JA](../components/ノードエディタ.md) | Node canvas, pins, links, selection, and gesture contracts | `imkit/node_editor.h` |
| [Timeline Editing](../components/timeline-editing.md) / [JA](../components/タイムライン編集.md) | Clip placement, ripple trims, roll, and slide editing | `imkit/video.h` |
| [Editor Suite](../components/editor-suite.md) / [JA](../components/エディタスイート.md) | 3D viewports, transform gizmos, outliners, and UV editor | `imkit/cg.h`, `imkit/preview.h` |
| [Workflow Components](../components/workflow-components.md) / [JA](../components/ワークフローコンポーネント.md) | Wizards, breadcrumbs, inspector cards, and side panels | `imkit/workflow.h` |
| [Shell Components](../components/shell-components.md) / [JA](../components/シェルコンポーネント.md) | App bars, workspace headers, action bars, and drawers | `imkit/shell.h` |
| [Window Frame](../components/gallery-window-frame.md) / [JA](../components/ウィンドウフレーム.md) | Custom title bars, snap layouts, and OS window adapters | `imkit/window_frame.h` |

### 5. Reference

| Document (EN / JA) | Audience & Focus | Headers / Source |
|---|---|---|
| [Public API Coverage](api-coverage.md) / [JA](公開API一覧.md) | Full table of wrapped Dear ImGui and ImKit public functions | `imkit/imkit.h` |
| [Editor API](editor-api.md) / [JA](エディタAPI.md) | Editor Suite public types, signatures, and provider contracts | `imkit/editor_core.h` |
| [Widget Inventory](widget-inventory.md) / [JA](ウィジェット一覧.md) | Component inventory and design audit decision logs | All modules |
| [Documentation Catalog](documentation-catalog.md) / [JA](文書カタログ.md) | This document (comprehensive documentation index) | - |
| [Validation](validation.md) / [JA](検証記録.md) | Platform, CI, and GPU smoke test verification records | Test suite |
| [Migration v3](migration-v3.md) / [JA](v3移行ガイド.md) | Step-by-step upgrade guide from v2 | - |
| [Troubleshooting](troubleshooting.md) / [JA](トラブルシューティング.md) | Solutions for common build issues and rendering anomalies | - |

## Archive and historical notes

Historical design proposals and audit notes are preserved in `docs/archive/`. They represent rationale from past milestones rather than active public API promises.
