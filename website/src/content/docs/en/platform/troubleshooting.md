---
title: "Troubleshooting"
description: "Find the symptom, then read the cause and the fix."
---

Find the symptom, then read the cause and the fix.

## Build and link

| Symptom | Cause | Fix |
| --- | --- | --- |
| CMake stops with `IMKIT_IMGUI_TARGET is required` | The Dear ImGui target does not exist before `add_subdirectory`, or the name is wrong | Create the Dear ImGui CMake target first, then set `IMKIT_IMGUI_TARGET` to that exact name. ImKit never adds a second copy of Dear ImGui to an embedded project. |
| A header stops with an error about the Dear ImGui version | The Dear ImGui version, or its docking, viewport and dynamic-texture support, does not match | Use the pinned `docking` commit (see [Dependencies](../dependencies/)). Removing the guard does not make other revisions compatible, because style fields and public signatures differ. |
| CMake stops asking you to confirm the SDK ABI | The installed SDK needs `IMKIT_SDK_ABI_CONFIRMED=ON` | Check the architecture, MSVC toolset, Debug or Release CRT, Dear ImGui revision, compile definitions and `imconfig.h`, then set the flag. If any of them differs, build from source. |
| The Gallery build cannot overwrite `imkit_gallery.exe` | The Gallery is still running, and Windows will not let the linker replace a running executable | Close the Gallery and run the same incremental build again. |

## Runtime

```cpp
// Assertion: "Backend does not support ImGuiBackendFlags_RendererHasTextures, and font atlas is not built!"
```

| Symptom | Cause | Fix |
| --- | --- | --- |
| The assertion above fires at `NewFrame` in a program with no renderer backend | Nothing built the font atlas | Call `io.Fonts->GetTexDataAsRGBA32(...)` once, as in [Build your first app](../../tutorials/first-app/). A real backend does this for you. |
| An assertion fires when a `ThemeScope` is destroyed | The scope outlived its context, or was destroyed on another context | End every scope before `DestroyContext()`, on the context it was created on. |
| The theme looks right, but Japanese text is missing | Themes do not load fonts, so the atlas has no Japanese glyphs | Add the glyph ranges to your font atlas, and provide platform IME callbacks if text input needs them. The Gallery's fallback font is an optional asset, not library behaviour. |
| Sizes grow each time the theme is applied | You scaled `Theme::metrics` yourself and also passed a scale | Pass the application scale only to `ApplyTheme(theme, scale)` or `ThemeScope(theme, scale)`. ImKit always rebuilds the style from unscaled values. |
| Changes you made to `ImGuiStyle` disappear | `ApplyTheme` replaces the whole style, except `Alpha` and `FontScaleDpi` | Make the change in the `Theme`, as described in [Themes](../../features/themes/). |
| A control draws but nothing happens when you click it | The returned result is a request, and your code did not apply it | Read the return value or the returned request and update your own data. |

## Checking the result

A screenshot does not prove that an integration works. [Validation](https://github.com/AokiMotohide/imgui-modern-kit/blob/main/docs/reference/validation.md) separates compile and link checks, public-IO interaction, GPU capture, installed consumers and native application acceptance, so you can tell which of them a claim rests on.
