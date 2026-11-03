# Dependencies

[日本語](依存関係.md) · [Documentation index](../README.md) · [Documentation catalog](../reference/documentation-catalog.md)

ImKit is built on [Dear ImGui](https://github.com/ocornut/imgui). This page lists what ImKit depends on, which revisions are pinned, and how CMake resolves them.

## Built on Dear ImGui

ImKit would not exist without Dear ImGui. Thank you to Omar Cornut and to every contributor and sponsor of the project. ImKit is a layer on top of their work, not a replacement for it.

| Question | Answer |
| --- | --- |
| Does ImKit change Dear ImGui? | No. It uses only the public API, does not use `imgui_internal.h`, and ships no patch. |
| Does ImKit compile Dear ImGui? | No. Your application supplies the Dear ImGui target. The Gallery builds the unmodified upstream sources. |
| Which revision? | The `docking` branch at one pinned commit. `version.h` stops the build when the Dear ImGui version or its docking, viewport and dynamic-texture support does not match. |

## Pinned dependencies

| Dependency | Source | Version | Commit | License | Used for |
| --- | --- | --- | --- | --- | --- |
| Dear ImGui | [ocornut/imgui](https://github.com/ocornut/imgui) | `1.93.0 WIP docking` | `367b2c24f399988ddafc0bb4628da0106bcc09be` | MIT | Public widget types and API. The Gallery also uses the official GLFW/OpenGL3 and GLFW/Metal backends. |
| GLFW | [glfw/glfw](https://github.com/glfw/glfw) | `3.5.1` | `d9d6f0f1f967807ffade6598ea9a631ebaf37a56` | zlib/libpng | Gallery window, input and OpenGL context only |

Full license texts and hashes are in [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md).

## How CMake resolves them

| Build | What happens |
| --- | --- |
| Embedded with `add_subdirectory` | Set `IMKIT_IMGUI_TARGET` to a Dear ImGui target you already built. Nothing is downloaded, and the include directories and link requirements reach ImKit through that target. |
| Standalone, Gallery on | CMake fetches Dear ImGui and GLFW at the pinned commits. |
| Standalone, Gallery off | CMake fetches Dear ImGui only. GLFW is not fetched. |

- The Windows Gallery finds OpenGL with `FindOpenGL`. The macOS Gallery links the system Metal, MetalKit and Cocoa frameworks.
- The minimum is CMake 3.20, which covers the preset schema, `FetchContent_MakeAvailable` and target aliases that ImKit uses.

## Compatibility

ImKit 4.x requires exactly the Dear ImGui revision above. Build your application and every ImKit library with the same headers, and do not mix 3.x objects with 4.x structures.

| Package | Platforms |
| --- | --- |
| Windows | x64 and Arm64, MSVC |
| macOS | 15 or later: arm64, x86_64 and Universal 2, Apple Clang (unsigned, not notarized) |

Each package holds the ImKit libraries, the Gallery, the Node Editor Gallery, English and Japanese documents, `LICENSE` and the third-party notices. It does not contain Dear ImGui.

## Optional assets and tools

| Item | Role |
| --- | --- |
| Inter 4.1, Noto Sans JP 2.004 | Optional fonts under the SIL Open Font License 1.1. `imkit_copy_font_assets` copies the files beside your executable. Your application loads them into its own atlas. |
| Pillow, FFmpeg | Used only to make documentation media. They are not linked, installed or redistributed. |

## Developing ImKit beside your application

If you consume ImKit as a pinned Git submodule but also develop it next to your application, expose a CMake cache variable that you own and pass either the submodule or that explicit path to `add_subdirectory`. Keep the override out of project files and release manifests. Both routes must set `IMKIT_IMGUI_TARGET` before ImKit is added.
