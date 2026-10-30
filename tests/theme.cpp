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
ImVec4 Hex(unsigned x) {
    return {((x >> 16) & 255) / 255.f, ((x >> 8) & 255) / 255.f, (x & 255) / 255.f, 1};
}
bool Near(ImVec4 a, ImVec4 b, float tolerance = 1.f / 255.f) {
    return std::fabs(a.x - b.x) <= tolerance && std::fabs(a.y - b.y) <= tolerance &&
           std::fabs(a.z - b.z) <= tolerance && std::fabs(a.w - b.w) <= tolerance;
}
bool Contains(const std::string &text, const char *part) {
    return text.find(part) != std::string::npos;
}

// Generic sample palettes; none of them is tied to a product.
imkit::BrandPalette StickerPalette() {
    imkit::BrandPalette p;
    p.canvas = Hex(0xffe9a8); p.surface = Hex(0xfff6d6); p.text = Hex(0x4a2a18);
    p.accent = Hex(0xb2552f); p.border = Hex(0x4a2a18);
    return p;
}
imkit::BrandPalette PastelPalette() {
    imkit::BrandPalette p;
    p.canvas = Hex(0xfde8ef); p.surface = Hex(0xfff7fa); p.text = Hex(0x3b2a4a); p.accent = Hex(0xc2417c);
    return p;
}
imkit::BrandPalette BlueprintPalette() {
    imkit::BrandPalette p;
    p.canvas = Hex(0x0d2438); p.surface = Hex(0x123453); p.text = Hex(0xe8f3ff); p.accent = Hex(0x6cc4ff);
    return p;
}

int TestStyleProjection() {
    ImGui::CreateContext();
    int failure = 0;
    // Hairline themes (border <= 1) must keep their previous projection exactly.
    for (const auto &info : imkit::ThemePresets()) {
        const auto theme = imkit::MakeTheme(info.preset);
        imkit::ApplyTheme(theme, 1.f);
        const auto &style = ImGui::GetStyle();
        const bool highContrast = theme.contrast == imkit::ContrastMode::HighContrast;
        const auto &c = theme.colors;
        const ImVec4 hairline = highContrast ? c.border
            : ImVec4(c.surface.x + (c.border.x - c.surface.x) * .42f, c.surface.y + (c.border.y - c.surface.y) * .42f,
                     c.surface.z + (c.border.z - c.surface.z) * .42f, c.surface.w + (c.border.w - c.surface.w) * .42f);
        if (style.FrameBorderSize != theme.stroke.border || style.WindowBorderSize != theme.stroke.border ||
            style.ChildBorderSize != 0.f || style.TabBorderSize != 0.f ||
            style.SeparatorTextBorderSize != theme.stroke.border ||
            style.WindowRounding != theme.radius.control || style.ImageRounding != theme.radius.control ||
            !Near(style.Colors[ImGuiCol_Border], hairline, 1e-6f))
            failure = 20;
    }
    // A border of two pixels or more is an outline: full-strength colour, thick frames, thin dividers.
    auto outlined = imkit::MakeTheme(imkit::ThemePreset::WarmSand);
    outlined.stroke.border = 3.f; // Deliberately without ResolveTheme: the token alone must apply.
    imkit::ApplyTheme(outlined, 1.f);
    const auto &style = ImGui::GetStyle();
    if (style.FrameBorderSize != 3.f || style.WindowBorderSize != 3.f || style.PopupBorderSize != 3.f ||
        style.ChildBorderSize != 3.f || style.TabBorderSize != 3.f)
        failure = 21;
    if (style.SeparatorTextBorderSize != 1.f || style.TreeLinesSize != 1.f)
        failure = 22;
    if (!Near(style.Colors[ImGuiCol_Border], outlined.colors.border, 1e-6f))
        failure = 23;
    // Pill rounds the tallest control (44 px) to a capsule and nothing taller than that.
    imkit::SetCornerStyle(outlined, imkit::CornerStyle::Pill);
    imkit::ApplyTheme(outlined, 1.f);
    if (outlined.radius.control != 22.f || style.FrameRounding != 22.f || style.WindowRounding != 22.f)
        failure = 24;
    // A huge control radius set by hand must not turn windows and images into capsules.
    outlined.radius.control = 999.f;
    imkit::ApplyTheme(outlined, 1.f);
    if (style.FrameRounding < 900.f || style.WindowRounding != outlined.radius.overlay ||
        style.ImageRounding != outlined.radius.overlay)
        failure = 26;
    imkit::SetCornerStyle(outlined, imkit::CornerStyle::Sharp);
    if (outlined.radius.control != 2.f || outlined.metrics.radius != 2.f)
        failure = 25;
    ImGui::DestroyContext();
    return failure;
}

int TestBrandThemes() {
    struct Case { imkit::BrandPalette palette; imkit::ColorScheme scheme; };
    const Case cases[] = {{StickerPalette(), imkit::ColorScheme::Light}, {PastelPalette(), imkit::ColorScheme::Light},
                          {BlueprintPalette(), imkit::ColorScheme::Dark}};
    for (const auto &item : cases) {
        const auto theme = imkit::MakeBrandTheme(item.palette);
        const auto &c = theme.semantic;
        if (theme.scheme != item.scheme || !Near(c.canvas, item.palette.canvas, 0) ||
            !Near(c.surface, item.palette.surface, 0) || !Near(c.text, item.palette.text, 0) ||
            !Near(c.accent, item.palette.accent, 0) || !Near(c.control.focused, item.palette.accent, 0))
            return 30;
        const std::array colors = {c.canvas, c.surface, c.surfaceRaised, c.overlay, c.text, c.textSecondary, c.textDisabled,
                                   c.border, c.accent, c.onAccent, c.success, c.warning, c.error, c.control.rest,
                                   c.control.hover, c.control.pressed, c.control.focused, c.control.selected};
        if (!std::all_of(colors.begin(), colors.end(), Valid))
            return 31;
        // Derived colours must keep the contrast the validator asks for.
        if (!imkit::ValidateContrast(theme))
            return 32;
        // The projection is already resolved: colors mirrors semantic.
        if (!Near(theme.colors.canvas, c.canvas, 0) || !Near(theme.colors.accent, c.accent, 0) ||
            !Near(theme.colors.input, c.control.rest, 0))
            return 33;
    }
    // Explicit optional colours win over derived ones; an explicit error colour picks its own on-colour.
    auto custom = StickerPalette();
    custom.raised = Hex(0xffffff); custom.textSecondary = Hex(0x6b4a36); custom.error = Hex(0x7a1020);
    const auto themed = imkit::MakeBrandTheme(custom);
    if (!Near(themed.semantic.surfaceRaised, Hex(0xffffff), 0) || !Near(themed.semantic.textSecondary, Hex(0x6b4a36), 0) ||
        !Near(themed.semantic.error, Hex(0x7a1020), 0) || !Valid(themed.colors.onDestructive) ||
        Contrast(themed.colors.onDestructive, themed.semantic.error) < 4.5f)
        return 34;
    // Poor input is a warning (false), never a failure.
    imkit::BrandPalette poor = StickerPalette();
    poor.text = poor.canvas;
    if (imkit::ValidateContrast(imkit::MakeBrandTheme(poor)))
        return 35;
    // The coloured presets have their own canvas and surface, not just another accent, and stay legible.
    for (const auto preset : {imkit::ThemePreset::Graphite, imkit::ThemePreset::Midnight, imkit::ThemePreset::Ocean,
                              imkit::ThemePreset::Forest, imkit::ThemePreset::Violet, imkit::ThemePreset::WarmSand,
                              imkit::ThemePreset::Rose, imkit::ThemePreset::Solar}) {
        const auto theme = imkit::MakeTheme(preset);
        const auto base = imkit::MakePrecisionTheme(theme.scheme);
        if (Near(theme.semantic.canvas, base.semantic.canvas, 0) || Near(theme.semantic.surface, base.semantic.surface, 0))
            return 36;
        if (!imkit::ValidateContrast(theme))
            return 37;
    }
    return 0;
}

int TestThemeText() {
    auto original = imkit::MakeBrandTheme(StickerPalette());
    original.stroke.border = 3.f;
    original.elevation.hard = true;
    original.elevation.offset = {0.f, 3.f};
    original.semantic.shadow = Hex(0x4a2a18);
    imkit::SetCornerStyle(original, imkit::CornerStyle::Round);
    imkit::SetDensity(original, imkit::Density::Touch);
    const std::string text = imkit::SerializeTheme(original);

    // Fonts belong to the host and must survive reading. A fake pointer is enough: it is never dereferenced.
    auto *fakeFont = reinterpret_cast<ImFont *>(std::uintptr_t{64});
    auto restored = imkit::MakeTheme(imkit::ThemePreset::Graphite);
    restored.fonts.regular = fakeFont;
    const auto result = imkit::ParseTheme(text, restored);
    if (result.ignored != 0 || !result.warnings.empty() || result.applied < 40)
        return 40;
    if (imkit::SerializeTheme(restored) != text) // Reading what was written reproduces it exactly.
        return 41;
    if (restored.fonts.regular != fakeFont || restored.stroke.border != 3.f || !restored.elevation.hard ||
        restored.elevation.offset.y != 3.f || restored.density != imkit::Density::Touch ||
        restored.radius.control != original.radius.control || restored.metrics.controlHeight != 44.f ||
        !Near(restored.semantic.canvas, original.semantic.canvas) || !Near(restored.colors.accent, original.semantic.accent))
        return 42;

    // A partial file changes only what it names, on top of the chosen base preset.
    auto partial = imkit::MakeTheme(imkit::ThemePreset::PrecisionLight);
    partial.fonts.display = fakeFont;
    const auto diff = imkit::ParseTheme("preset = warm-sand\nstroke.border = 3\ncorner = pill\n", partial);
    const auto warmSand = imkit::MakeTheme(imkit::ThemePreset::WarmSand);
    if (diff.applied != 3 || diff.ignored != 0 || !Near(partial.semantic.accent, warmSand.semantic.accent, 0) ||
        partial.stroke.border != 3.f || partial.radius.control != 22.f || partial.fonts.display != fakeFont ||
        partial.metrics.border != 3.f)
        return 43;

    // Bad lines are skipped one by one with a line-numbered warning; the good ones still apply.
    auto tolerant = imkit::MakeTheme(imkit::ThemePreset::PrecisionDark);
    const char *messy = "\xEF\xBB\xBF# comment\r\n\r\n"
                        "stroke.border = 2\r\n"
                        "semantic.accent = blue\r\n"
                        "no equals sign here\r\n"
                        "radius.control = -4\r\n"
                        "future.token = 1\r\n"
                        "preset = nonsense\r\n"
                        "elevation.offset = 1 2\r\n";
    const auto messyResult = imkit::ParseTheme(messy, tolerant);
    if (messyResult.applied != 2 || messyResult.ignored != 5 || tolerant.stroke.border != 2.f ||
        tolerant.elevation.offset.x != 1.f || tolerant.elevation.offset.y != 2.f ||
        !Contains(messyResult.warnings, "line 4:") || !Contains(messyResult.warnings, "line 5:") ||
        !Contains(messyResult.warnings, "line 6:") || !Contains(messyResult.warnings, "line 7:") ||
        !Contains(messyResult.warnings, "line 8:") || Contains(messyResult.warnings, "line 3:"))
        return 44;

    // brand.* rebuilds colours only; shape stays. Without the four required colours nothing is applied.
    auto branded = imkit::MakeTheme(imkit::ThemePreset::PrecisionLight);
    imkit::ParseTheme("corner = round\nbrand.canvas = #ffe9a8\nbrand.surface = #fff6d6\nbrand.text = #4a2a18\nbrand.accent = #b2552f\n",
                      branded);
    if (!Near(branded.semantic.canvas, Hex(0xffe9a8)) || !Near(branded.colors.accent, Hex(0xb2552f)) ||
        branded.radius.control != 16.f || !imkit::ValidateContrast(branded))
        return 45;
    auto incomplete = imkit::MakeTheme(imkit::ThemePreset::PrecisionLight);
    const auto before = imkit::SerializeTheme(incomplete);
    const auto incompleteResult = imkit::ParseTheme("brand.canvas = #ffe9a8\nbrand.text = #4a2a18\n", incomplete);
    if (incompleteResult.ignored != 1 || imkit::SerializeTheme(incomplete) != before)
        return 46;
    // A lone accent keeps text on it readable, and a misspelt brand key is reported.
    auto accented = imkit::MakeTheme(imkit::ThemePreset::PrecisionLight);
    const auto accentResult = imkit::ParseTheme("semantic.accent = #ffee00\nbrand.textsecondary = #888888\n", accented);
    if (accentResult.ignored != 1 || Contrast(accented.semantic.onAccent, accented.semantic.accent) < 4.5f ||
        !Near(accented.semantic.control.focused, Hex(0xffee00)))
        return 47;
    return 0;
}

int TestWorlds() {
    if (const int failure = TestStyleProjection())
        return failure;
    if (const int failure = TestBrandThemes())
        return failure;
    return TestThemeText();
}
}

int main() {
    if (imkit::ThemeScaleDefault != 1.25f || imkit::ThemeScaleMinimum != .5f ||
        imkit::ThemeScaleMaximum != 2.5f)
        return 11;
    const auto presets = imkit::ThemePresets();
    if (presets.size() != 13)
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
    const auto slate = imkit::MakeTheme(imkit::ThemePreset::Slate);
    if (slate.radius.control <= dark.radius.control ||
        slate.colors.canvas.x >= slate.colors.surface.x ||
        imkit::ThemePresetFromId("slate") != imkit::ThemePreset::Slate)
        return 14;
    if (const int failure = TestWorlds())
        return failure;
    std::puts("13 theme presets, stable IDs, scale bounds, legacy factories, brand themes, outlines and theme text validated");
    return 0;
}
