#include <imkit/theme.h>
#include <algorithm>
#include <cmath>

namespace imkit {
namespace {
ImVec4 Hex(unsigned x) {
    return {((x >> 16) & 255) / 255.f, ((x >> 8) & 255) / 255.f, (x & 255) / 255.f, 1};
}
ImVec4 Mix(ImVec4 a, ImVec4 b, float x) {
    return {a.x + (b.x - a.x) * x, a.y + (b.y - a.y) * x, a.z + (b.z - a.z) * x, a.w + (b.w - a.w) * x};
}
ImVec4 Alpha(ImVec4 c, float a) {
    c.w = a;
    return c;
}
ImGuiStyle Style(const Theme &t, float scale) {
    IM_ASSERT(scale > 0 && std::isfinite(scale));
    ImGuiStyle s; // Always derive from unscaled values, never the last application.
    const auto &m = t.metrics;
    const auto &p = t.colors;
    s.WindowPadding = {12, 12};
    s.FramePadding = {m.horizontalPadding, std::max(3.f, (m.controlHeight - m.bodySize) / 2)};
    s.ItemSpacing = {m.spacing, m.spacing};
    s.ItemInnerSpacing = {m.spacing, m.spacing};
    s.CellPadding = {m.spacing, 4};
    s.WindowRounding = m.radius;
    s.ChildRounding = m.radius;
    s.PopupRounding = m.radius + 2;
    s.FrameRounding = m.radius;
    s.GrabRounding = m.radius;
    s.ScrollbarRounding = m.radius;
    s.WindowBorderSize = m.border;
    s.ChildBorderSize = 0;
    s.PopupBorderSize = m.border;
    s.FrameBorderSize = m.border;
    s.IndentSpacing = 18;
    s.ScrollbarSize = 12;
    s.GrabMinSize = 10;
    s.TabRounding = m.radius;
    s.TabBorderSize = 0;
    s.TabBarBorderSize = 1;
    s.TabBarOverlineSize = 0;
    s.SeparatorTextBorderSize = m.border;
    s.SeparatorTextPadding = {0, 6};
    s.SeparatorTextAlign = {0, .5f};
    s.ImageRounding = m.radius;
    s.ImageBorderSize = m.border;
    s.MenuItemRounding = m.radius;
    s.TreeLinesSize = m.border;
    s.TreeLinesRounding = 2;
    s.TreeLinesFlags = ImGuiTreeNodeFlags_DrawLinesToNodes;
    s.DragDropTargetBorderSize = m.focusWidth;
    s.DragDropTargetRounding = m.radius;
    s.InputTextCursorSize = 1.5f;
    s.DisabledAlpha = t.opacity.disabled;
    s.DockingSeparatorSize = 4;
    auto *c = s.Colors;
    c[ImGuiCol_Text] = p.text;
    c[ImGuiCol_TextDisabled] = p.muted;
    c[ImGuiCol_WindowBg] = p.canvas;
    c[ImGuiCol_ChildBg] = p.surface;
    c[ImGuiCol_PopupBg] = p.raised;
    c[ImGuiCol_Border] = p.border;
    c[ImGuiCol_BorderShadow] = {0, 0, 0, 0};
    c[ImGuiCol_FrameBg] = p.input;
    c[ImGuiCol_FrameBgHovered] = t.semantic.control.hover;
    c[ImGuiCol_FrameBgActive] = t.semantic.control.pressed;
    c[ImGuiCol_TitleBg] = p.surface;
    c[ImGuiCol_TitleBgActive] = p.raised;
    c[ImGuiCol_TitleBgCollapsed] = p.surface;
    c[ImGuiCol_MenuBarBg] = p.surface;
    c[ImGuiCol_ScrollbarBg] = p.input;
    c[ImGuiCol_ScrollbarGrab] = p.border;
    c[ImGuiCol_ScrollbarGrabHovered] = p.muted;
    c[ImGuiCol_ScrollbarGrabActive] = p.accent;
    c[ImGuiCol_CheckMark] = p.accent;
    c[ImGuiCol_CheckboxSelectedBg] = p.selection;
    c[ImGuiCol_SliderGrab] = p.accent;
    c[ImGuiCol_SliderGrabActive] = p.focus;
    c[ImGuiCol_Button] = p.raised;
    c[ImGuiCol_ButtonHovered] = Mix(p.raised, p.accent, .15f);
    c[ImGuiCol_ButtonActive] = Mix(p.raised, p.accent, .25f);
    c[ImGuiCol_Header] = p.selection;
    c[ImGuiCol_HeaderHovered] = Mix(p.surface, p.accent, .20f);
    c[ImGuiCol_HeaderActive] = Mix(p.surface, p.accent, .30f);
    c[ImGuiCol_Separator] = p.border;
    c[ImGuiCol_SeparatorHovered] = p.focus;
    c[ImGuiCol_SeparatorActive] = p.accent;
    c[ImGuiCol_ResizeGrip] = p.border;
    c[ImGuiCol_ResizeGripHovered] = p.focus;
    c[ImGuiCol_ResizeGripActive] = p.accent;
    c[ImGuiCol_InputTextCursor] = p.focus;
    c[ImGuiCol_Tab] = p.surface;
    c[ImGuiCol_TabHovered] = p.selection;
    c[ImGuiCol_TabSelected] = p.raised;
    c[ImGuiCol_TabSelectedOverline] = p.accent;
    c[ImGuiCol_TabDimmed] = p.surface;
    c[ImGuiCol_TabDimmedSelected] = p.input;
    c[ImGuiCol_TabDimmedSelectedOverline] = p.muted;
    c[ImGuiCol_DockingPreview] = Alpha(p.accent, .35f);
    c[ImGuiCol_DockingEmptyBg] = p.canvas;
    c[ImGuiCol_PlotLines] = p.accent;
    c[ImGuiCol_PlotLinesHovered] = p.focus;
    c[ImGuiCol_PlotHistogram] = p.accent;
    c[ImGuiCol_PlotHistogramHovered] = p.focus;
    c[ImGuiCol_TableHeaderBg] = p.input;
    c[ImGuiCol_TableBorderStrong] = p.border;
    c[ImGuiCol_TableBorderLight] = Alpha(p.border, .5f);
    c[ImGuiCol_TableRowBg] = {0, 0, 0, 0};
    c[ImGuiCol_TableRowBgAlt] = Alpha(p.muted, .045f);
    c[ImGuiCol_TextLink] = p.accent;
    c[ImGuiCol_TextSelectedBg] = p.selection;
    c[ImGuiCol_TreeLines] = p.border;
    c[ImGuiCol_DragDropTarget] = p.focus;
    c[ImGuiCol_DragDropTargetBg] = Alpha(p.selection, .35f);
    c[ImGuiCol_UnsavedMarker] = p.warning;
    c[ImGuiCol_NavCursor] = p.focus;
    c[ImGuiCol_NavWindowingHighlight] = p.focus;
    c[ImGuiCol_NavWindowingDimBg] = Alpha(p.canvas, .65f);
    c[ImGuiCol_ModalWindowDimBg] = {.04f, .05f, .07f, .50f};
    s.ScaleAllSizes(scale);
    s.FontSizeBase = m.bodySize;
    s.FontScaleMain = scale;
    return s;
}
} // namespace
std::span<const ThemePresetInfo> ThemePresets() noexcept {
    static constexpr std::array presets = {
        ThemePresetInfo{ThemePreset::PrecisionLight, "precision-light", "Precision Light", ColorScheme::Light},
        ThemePresetInfo{ThemePreset::PrecisionDark, "precision-dark", "Precision Dark", ColorScheme::Dark},
        ThemePresetInfo{ThemePreset::Graphite, "graphite", "Graphite", ColorScheme::Dark},
        ThemePresetInfo{ThemePreset::Midnight, "midnight", "Midnight", ColorScheme::Dark},
        ThemePresetInfo{ThemePreset::Ocean, "ocean", "Ocean", ColorScheme::Dark},
        ThemePresetInfo{ThemePreset::Forest, "forest", "Forest", ColorScheme::Dark},
        ThemePresetInfo{ThemePreset::WarmSand, "warm-sand", "Warm Sand", ColorScheme::Light},
        ThemePresetInfo{ThemePreset::Rose, "rose", "Rose", ColorScheme::Light},
        ThemePresetInfo{ThemePreset::Violet, "violet", "Violet", ColorScheme::Dark},
        ThemePresetInfo{ThemePreset::Solar, "solar", "Solar", ColorScheme::Light},
        ThemePresetInfo{ThemePreset::HighContrastLight, "high-contrast-light", "High Contrast Light", ColorScheme::Light},
        ThemePresetInfo{ThemePreset::HighContrastDark, "high-contrast-dark", "High Contrast Dark", ColorScheme::Dark},
    };
    return presets;
}
std::optional<ThemePreset> ThemePresetFromId(std::string_view id) noexcept {
    const auto presets = ThemePresets();
    const auto it = std::find_if(presets.begin(), presets.end(), [id](const ThemePresetInfo& info) {
        return info.id == id;
    });
    return it == presets.end() ? std::nullopt : std::optional<ThemePreset>(it->preset);
}
Theme MakeTheme(ThemePreset preset) {
    const auto info = ThemePresets()[static_cast<std::size_t>(preset)];
    if (preset == ThemePreset::HighContrastLight || preset == ThemePreset::HighContrastDark)
        return MakeTheme(info.scheme, ContrastMode::HighContrast);
    auto theme = MakePrecisionTheme(info.scheme);
    constexpr std::array accents = {0x005ca8u, 0x85c5ffu, 0x78d6c6u, 0x8eafffu,
                                    0x56d3e5u, 0x83d69cu, 0x9b5c2eu, 0xa9466bu,
                                    0xc19af5u, 0x856500u};
    SetAccent(theme, Hex(accents[static_cast<std::size_t>(preset)]));
    return theme;
}
Theme MakePrecisionTheme(ColorScheme scheme) {
    Theme t;
    t.scheme = scheme;
    bool d = scheme == ColorScheme::Dark;
    t.colors = {Hex(d ? 0x14161b : 0xedeef1),
                Hex(d ? 0x20232a : 0xf8f9fb),
                Hex(d ? 0x171a20 : 0xeff1f5),
                Hex(d ? 0x30343e : 0xffffff),
                Hex(d ? 0xeff0f4 : 0x242833),
                Hex(d ? 0xacb2bf : 0x505968),
                Hex(d ? 0x8b95a5 : 0x606a78),
                Hex(d ? 0x85c5ff : 0x005ca8),
                Hex(d ? 0x142031 : 0xffffff),
                {},
                {},
                Hex(d ? 0xf2a2a5 : 0xad3441),
                Hex(d ? 0x35191e : 0xffffff),
                Hex(d ? 0x83c6a1 : 0x276341),
                Hex(d ? 0xe4c17b : 0x805a12)};
    SetAccent(t, t.colors.accent);
    t.editor = {t.colors.canvas,t.colors.border,t.colors.muted,t.colors.surface,
                t.colors.accent,t.colors.success,t.colors.warning,t.colors.text,t.colors.warning,
                t.colors.warning,t.colors.focus,Hex(0xdb6565),Hex(0x5cab72),Hex(0x6699e0),
                t.colors.accent,t.colors.success,t.colors.destructive,t.colors.warning,
                t.colors.destructive,t.colors.muted,
                Hex(d ? 0xc791c9 : 0x864c8e),Hex(d ? 0x72b7c6 : 0x347080),Hex(d ? 0x9ca2b0 : 0x626979)};
    auto& c=t.semantic;
    c={t.colors.canvas,t.colors.surface,t.colors.raised,t.colors.raised,t.colors.text,t.colors.muted,
       t.colors.muted,t.colors.border,t.colors.accent,t.colors.onAccent,t.colors.success,t.colors.warning,
       t.colors.destructive,{t.colors.input,Mix(t.colors.input,t.colors.accent,.10f),
       Mix(t.colors.input,t.colors.accent,.17f),t.colors.focus,t.colors.selection,t.colors.input}};
    return t;
}
Theme MakeTheme(ColorScheme scheme, ContrastMode contrast, Density density) {
    auto t=MakePrecisionTheme(scheme); t.contrast=contrast;
    if(contrast==ContrastMode::HighContrast) {
        bool dark=scheme==ColorScheme::Dark;
        auto bg=Hex(dark?0x000000:0xffffff), fg=Hex(dark?0xffffff:0x000000);
        auto& c=t.semantic;
        c.canvas=c.surface=c.surfaceRaised=c.overlay=c.control.rest=c.control.disabled=bg;
        c.text=c.textSecondary=c.textDisabled=c.border=fg;
        c.accent=c.control.focused=Hex(dark?0xffdc60:0x004c8c);
        c.onAccent=bg; c.control.hover=c.control.pressed=c.control.selected=bg;
    }
    SetDensity(t,density); return t;
}
void SetDensity(Theme& t,Density density) { t.density=density; ResolveTheme(t); }
void ResolveTheme(Theme& t) {
    const auto& c=t.semantic;
    t.colors={c.canvas,c.surface,c.control.rest,c.surfaceRaised,c.text,c.textSecondary,c.border,
        c.accent,c.onAccent,c.control.selected,c.control.focused,c.error,t.colors.onDestructive,
        c.success,c.warning};
    t.metrics.controlHeight=t.density==Density::Compact?24.f:t.density==Density::Touch?44.f:28.f;
    t.metrics.horizontalPadding=t.density==Density::Compact?6.f:t.density==Density::Touch?16.f:10.f;
    t.metrics.spacing=t.density==Density::Compact?4.f:t.density==Density::Touch?8.f:6.f;
    t.metrics.bodySize=t.typography.body; t.metrics.headingSize=t.typography.heading;
    t.metrics.radius=t.radius.control; t.metrics.border=t.stroke.border; t.metrics.focusWidth=t.stroke.focus;
    t.metrics.elevation=t.elevation.overlay;
    t.editor.canvas=c.canvas; t.editor.grid=c.border; t.editor.ruler=c.textSecondary;
    t.editor.trackHeader=c.surface; t.editor.videoClip=c.accent; t.editor.audioClip=c.success;
    t.editor.captionClip=c.warning; t.editor.key=c.text; t.editor.selectedKey=c.warning;
    t.editor.snapGuide=c.control.focused; t.editor.error=c.error; t.editor.locked=c.textDisabled;
}
float ContrastRatio(ImVec4 foreground,ImVec4 background) {
    auto lum=[](ImVec4 c) { auto linear=[](float x) { x=std::clamp(x,0.f,1.f);
        return x<=.04045f?x/12.92f:std::pow((x+.055f)/1.055f,2.4f); };
        return .2126f*linear(c.x)+.7152f*linear(c.y)+.0722f*linear(c.z); };
    foreground=Mix(background,foreground,std::clamp(foreground.w,0.f,1.f));
    float a=lum(foreground),b=lum(background); return (std::max(a,b)+.05f)/(std::min(a,b)+.05f);
}
bool ValidateContrast(const Theme& t) {
    const auto& c=t.semantic;
    for(auto bg:{c.canvas,c.surface,c.surfaceRaised,c.overlay,c.control.rest,c.control.hover,
                 c.control.pressed,c.control.selected,c.control.disabled}) {
        if(ContrastRatio(c.text,bg)<4.5f || ContrastRatio(c.textSecondary,bg)<4.5f ||
           ContrastRatio(c.control.focused,bg)<3.f || ContrastRatio(c.border,bg)<3.f) return false;
    }
    return ContrastRatio(c.textDisabled,c.control.disabled)>=4.5f && ContrastRatio(c.onAccent,c.accent)>=4.5f;
}
void SetAccent(Theme &t, ImVec4 accent) {
    t.colors.accent = accent;
    t.colors.focus = accent;
    const auto linear = [](float v) {
        return v <= .04045f ? v / 12.92f : std::pow((v + .055f) / 1.055f, 2.4f);
    };
    float luminance = .2126f * linear(accent.x) + .7152f * linear(accent.y) + .0722f * linear(accent.z);
    t.colors.onAccent = Hex(luminance > .179f ? 0x142031 : 0xffffff);
    t.colors.selection = Mix(t.colors.surface, accent, t.scheme == ColorScheme::Dark ? .16f : .09f);
    t.semantic.accent=accent; t.semantic.onAccent=t.colors.onAccent;
    t.semantic.control.focused=accent; t.semantic.control.selected=t.colors.selection;
}
void ApplyTheme(const Theme &t, float scale) {
    auto next = Style(t, scale);
    // These are host/window multipliers, not design tokens. In particular a
    // temporary theme must not undo a surrounding BeginDisabled() alpha.
    next.Alpha = ImGui::GetStyle().Alpha;
    next.FontScaleDpi = ImGui::GetStyle().FontScaleDpi;
    ImGui::GetStyle() = next;
}
ThemeScope::ThemeScope(const Theme &t, float scale)
    : context_(ImGui::GetCurrentContext()), previous_(ImGui::GetStyle()) {
    ApplyTheme(t, scale);
    ImGui::PushFont(t.fonts.regular, t.metrics.bodySize);
}
ThemeScope::~ThemeScope() {
    IM_ASSERT(ImGui::GetCurrentContext() == context_);
    ImGui::PopFont();
    ImGui::GetStyle() = previous_;
}
void AnimationState::Reset(std::uint64_t generation) {
    entries_ = {};
    generation_ = generation;
}
void AnimationState::Prune(int frame, int age) {
    for (auto &e : entries_)
        if (e.used && frame - e.frame > age)
            e = {};
}
float AnimationState::Update(ImGuiID id, float target, float dt, float seconds, int frame, bool enabled, Easing easing) {
    Entry *slot = nullptr;
    for (auto &e : entries_)
        if (e.used && e.id == id) {
            slot = &e;
            break;
        }
    if (!slot) {
        for (auto &e : entries_)
            if (!e.used) {
                slot = &e;
                break;
            }
        if (!slot)
            slot = &*std::min_element(entries_.begin(), entries_.end(),
                                      [](auto &a, auto &b) { return a.frame < b.frame; });
        *slot = {id, target, frame, true, target, target, 0};
        return target;
    }
    if (slot->frame != frame) {
        if(slot->target!=target) { slot->from=slot->value; slot->target=target; slot->elapsed=0; }
        slot->elapsed+=std::max(0.f,dt);
        float step=seconds>0?std::clamp(slot->elapsed/seconds,0.f,1.f):1.f;
        if(easing==Easing::EaseOut) step=1-(1-step)*(1-step);
        if(easing==Easing::EaseInOut) step=step*step*(3-2*step);
        slot->value=!enabled || seconds<=0?target:slot->from+(target-slot->from)*step;
        slot->frame = frame;
    }
    if (!enabled) { slot->value=slot->from=slot->target=target; slot->elapsed=0; }
    return slot->value;
}
} // namespace imkit
