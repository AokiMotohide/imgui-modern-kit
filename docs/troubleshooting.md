# Troubleshooting

[日本語](troubleshooting.ja.md)

## `IMKIT_IMGUI_TARGET is required`

Create the host Dear ImGui CMake target before `add_subdirectory`, then set `IMKIT_IMGUI_TARGET` to that exact target name. ImKit intentionally does not inject another ImGui copy into an embedded project.

## Header rejects the ImGui version

Use the pinned supported revision. Relaxing the guard does not establish source or ABI compatibility because style fields and public signatures can differ.

## Installed SDK does not link

Confirm x64, MSVC toolset, Debug/Release CRT, Dear ImGui revision, compile definitions and `imconfig.h`. Use source integration if any binary setting differs.

## Theme looks correct but Japanese text is missing

Themes do not load fonts. Add the required glyph ranges to the host font atlas and provide platform IME callbacks when text input needs them. The Gallery's fallback font is an optional host asset, not library behavior.

## Theme size grows after repeated application

Pass the application scale only to `ApplyTheme` or `ThemeScope`. Do not also pre-scale `Theme::metrics`. ImKit always derives style from the stored unscaled metrics.

## Gallery build cannot overwrite the executable

Close the running `imkit_gallery.exe`, then run the same incremental target once. Windows prevents the linker from replacing a running executable.

## A screenshot is not proof of integration

Use [Validation](validation.md) to distinguish compile/link, public-IO interaction, GPU capture, installed consumer and native application acceptance.
