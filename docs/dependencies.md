# Dependencies

## Pinned standalone dependencies

| Dependency | Official source | Version | Commit | License | Scope |
|---|---|---|---|---|---|
| Dear ImGui | https://github.com/ocornut/imgui | `v1.92.9b-docking` | `b48d1afbe8ee8b238e2961dc363a949dd7304e23` | MIT | Public widget types and APIs; bundled core, demo, and official GLFW/OpenGL3 backend for standalone development |
| GLFW | https://github.com/glfw/glfw | `3.5.1` | `d9d6f0f1f967807ffade6598ea9a631ebaf37a56` | zlib/libpng | Gallery window, input, and OpenGL context only |

Full upstream license texts are preserved in `THIRD_PARTY_NOTICES.md`.

## Resolution rules

- A top-level standalone build without `IMKIT_IMGUI_TARGET` fetches both pinned revisions when Gallery is enabled. GLFW is not fetched when Gallery is disabled.
- A subdirectory build must set `IMKIT_IMGUI_TARGET` to a target that already exists. No dependency is downloaded in this mode.
- Dear ImGui include directories and link requirements reach consumers through the selected target. `imkit` never recompiles Dear ImGui.
- OpenGL is resolved with CMake's `FindOpenGL` module only for Gallery builds.

The minimum project version is CMake 3.20. This covers the selected preset schema, `FetchContent_MakeAvailable`, target aliases, and the dependency requirements used here without requiring the locally installed CMake release number.

