#include <imkit/theme.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <set>
#include <string>

namespace {
float Linear(float value) {
    return value <= .04045f ? value / 12.92f : std::pow((value + .055f) / 1.055f, 2.4f);
}
float Luminance(ImVec4 color) {
    return .2126f * Linear(color.x) + .7152f * Linear(color.y) + .0722f * Linear(color.z);
}
float Contrast(ImVec4 a, ImVec4 b) {
    const auto low = std::min(Luminance(a), Luminance(b));
    const auto high = std::max(Luminance(a), Luminance(b));
    return (high + .05f) / (low + .05f);
}
bool Valid(ImVec4 color) {
    const float channels[] = {color.x, color.y, color.z, color.w};
    return std::all_of(std::begin(channels), std::end(channels), [](float value) {
        return std::isfinite(value) && value >= 0 && value <= 1;
    }) && color.w == 1;
}
}

int main() {
    if (imkit::ThemeScaleDefault != 1.25f || imkit::ThemeScaleMinimum != .5f ||
        imkit::ThemeScaleMaximum != 2.5f)
        return 11;
    const auto presets = imkit::ThemePresets();
    if (presets.size() != 12)
        return 1;
    std::set<std::string> ids;
    for (const auto &info : presets) {
        if (info.id.empty() || info.displayName.empty() || !ids.emplace(info.id).second)
            return 2;
        const auto theme = imkit::MakeTheme(info.preset);
        if (theme.scheme != info.scheme)
            return 3;
        const auto &c = theme.colors;
        const std::array colors = {c.canvas,c.surface,c.input,c.raised,c.text,c.muted,c.border,c.accent,c.onAccent,
                                   c.selection,c.focus,c.destructive,c.onDestructive,c.success,c.warning};
        if (!std::all_of(colors.begin(), colors.end(), Valid))
            return 4;
        for (auto background : {c.canvas,c.surface,c.input,c.raised}) {
            if (Contrast(c.text, background) < 4.5f || Contrast(c.muted, background) < 3.f)
                return 5;
        }
        if (Contrast(c.onAccent, c.accent) < 4.5f || Contrast(c.onDestructive, c.destructive) < 4.5f)
            return 6;
        const auto &e = theme.editor;
        const std::array editor = {e.canvas,e.grid,e.ruler,e.trackHeader,e.videoClip,e.audioClip,e.captionClip,e.key,
                                   e.selectedKey,e.marker,e.snapGuide,e.axisX,e.axisY,e.axisZ,e.gizmo,e.scope,e.missing,
                                   e.proxy,e.error,e.locked,e.effectClip,e.adjustmentClip,e.groupClip};
        if (!std::all_of(editor.begin(), editor.end(), Valid))
            return 7;
        if (imkit::ThemePresetFromId(info.id) != info.preset)
            return 8;
    }
    if (imkit::ThemePresetFromId("Graphite") || imkit::ThemePresetFromId("unknown"))
        return 9;
    const auto light = imkit::MakeTheme(imkit::ThemePreset::PrecisionLight);
    const auto legacyLight = imkit::MakePrecisionTheme(imkit::ColorScheme::Light);
    const auto dark = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);
    const auto legacyDark = imkit::MakePrecisionTheme(imkit::ColorScheme::Dark);
    if (light.colors.canvas.x != legacyLight.colors.canvas.x || dark.colors.canvas.x != legacyDark.colors.canvas.x ||
        light.colors.accent.x != legacyLight.colors.accent.x || dark.colors.accent.x != legacyDark.colors.accent.x)
        return 10;

    ImGui::CreateContext();
    imkit::ApplyTheme(dark);
    if (ImGui::GetStyle().FontScaleMain != imkit::ThemeScaleDefault) {
        ImGui::DestroyContext();
        return 12;
    }
    for (const float scale : {imkit::ThemeScaleMinimum, imkit::ThemeScaleDefault,
                              imkit::ThemeScaleMaximum}) {
        imkit::ApplyTheme(dark, scale);
        if (ImGui::GetStyle().FontScaleMain != scale ||
            ImGui::GetStyle().FramePadding.x <= 0.0f) {
            ImGui::DestroyContext();
            return 13;
        }
    }
    ImGui::DestroyContext();
    std::puts("12 theme presets, stable IDs, scale bounds and legacy factories validated");
    return 0;
}
