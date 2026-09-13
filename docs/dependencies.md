# Dependencies

## Pinned standalone dependencies

| Dependency | Official source | Version | Commit | License | Scope |
|---|---|---|---|---|---|
| Dear ImGui | https://github.com/ocornut/imgui | `1.93.0 WIP docking` | `367b2c24f399988ddafc0bb4628da0106bcc09be` | MIT | Public widget types and APIs; unmodified core, demo, and official GLFW/OpenGL3 or GLFW/Metal backends for standalone development |
| GLFW | https://github.com/glfw/glfw | `3.5.1` | `d9d6f0f1f967807ffade6598ea9a631ebaf37a56` | zlib/libpng | Gallery window, input, and OpenGL context only |

Full upstream license texts are preserved in `THIRD_PARTY_NOTICES.md`.

## Resolution rules

- A top-level standalone build without `IMKIT_IMGUI_TARGET` fetches both pinned revisions when Gallery is enabled. GLFW is not fetched when Gallery is disabled.
- A subdirectory build must set `IMKIT_IMGUI_TARGET` to a target that already exists. No dependency is downloaded in this mode.
- Dear ImGui include directories and link requirements reach consumers through the selected target. `imkit` never recompiles Dear ImGui.
- OpenGL is resolved with CMake's `FindOpenGL` module for the Windows Gallery. The macOS Gallery links the system Metal, MetalKit and Cocoa frameworks.

The minimum project version is CMake 3.20. This covers the selected preset schema, `FetchContent_MakeAvailable`, target aliases, and the dependency requirements used here without requiring the locally installed CMake release number.

`imkit` 3.0.0 requires the exact pinned docking ABI above. Older font/style/API contracts are not supported. Optional host assets include Inter 4.1 and Noto Sans JP 2.004; font provenance, hashes and OFL texts are in THIRD_PARTY_NOTICES.md. `imkit_copy_font_assets` can stage them beside a consumer target, while the host remains responsible for loading them into its atlas.
