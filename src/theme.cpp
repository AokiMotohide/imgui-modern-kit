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
Palette Colors(unsigned canvas, unsigned surface, unsigned input, unsigned raised, unsigned text, unsigned muted,
               unsigned border, unsigned accent, unsigned destructive, unsigned onDestructive, unsigned success,
               unsigned warning) {
    return {Hex(canvas), Hex(surface), Hex(input), Hex(raised), Hex(text), Hex(muted), Hex(border),
            Hex(accent), {}, {}, {}, Hex(destructive), Hex(onDestructive), Hex(success), Hex(warning)};
}
Theme BuildTheme(ColorScheme scheme, Palette colors) {
    Theme t;
    t.scheme = scheme;
    t.colors = colors;
    SetAccent(t, t.colors.accent);
    const bool dark = scheme == ColorScheme::Dark;
    t.editor = {t.colors.canvas,t.colors.border,t.colors.muted,t.colors.surface,
                t.colors.accent,t.colors.success,t.colors.warning,t.colors.text,t.colors.warning,
                t.colors.warning,t.colors.focus,Hex(0xdb6565),Hex(0x5cab72),Hex(0x6699e0),
                t.colors.accent,t.colors.success,t.colors.destructive,t.colors.warning,
                t.colors.destructive,t.colors.muted,
                Hex(dark ? 0xc791c9 : 0x864c8e),Hex(dark ? 0x72b7c6 : 0x347080),
                Hex(dark ? 0x9ca2b0 : 0x626979)};
    return t;
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
    s.ChildBorderSize = m.border;
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
    s.DisabledAlpha = .45f;
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
    c[ImGuiCol_FrameBgHovered] = Mix(p.input, p.accent, .10f);
    c[ImGuiCol_FrameBgActive] = Mix(p.input, p.accent, .17f);
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
Theme MakeTheme(ThemePreset preset) {
    switch (preset) {
    case ThemePreset::PrecisionLight:
        return BuildTheme(ColorScheme::Light, Colors(0xedeef1,0xf8f9fb,0xeff1f5,0xffffff,
                         0x242833,0x636b7b,0xcbd0da,0x6950b4,0xad3441,0xffffff,0x276341,0x805a12));
    case ThemePreset::PrecisionDark:
        return BuildTheme(ColorScheme::Dark, Colors(0x14161b,0x20232a,0x171a20,0x30343e,
                         0xeff0f4,0xacb2bf,0x454d5c,0xbba8ff,0xf2a2a5,0x35191e,0x83c6a1,0xe4c17b));
    case ThemePreset::Graphite:
        return BuildTheme(ColorScheme::Dark, Colors(0x101214,0x1b1e21,0x14171a,0x292d31,
                         0xf1f3f5,0xaeb5bd,0x454b52,0x78d6c6,0xff9ca6,0x281014,0x7fd8a6,0xf2c879));
    case ThemePreset::Midnight:
        return BuildTheme(ColorScheme::Dark, Colors(0x0b1020,0x141b30,0x10172a,0x202b48,
                         0xf1f5ff,0xaab6d1,0x3b4968,0x8eafff,0xff9daa,0x2b1018,0x73d4b1,0xf3c86e));
    case ThemePreset::Ocean:
        return BuildTheme(ColorScheme::Dark, Colors(0x07191f,0x10272e,0x0b2027,0x19363e,
                         0xeaf8fa,0xa5c5cb,0x35606a,0x56d3e5,0xffa0aa,0x2a1116,0x77d9ad,0xf0ca78));
    case ThemePreset::Forest:
        return BuildTheme(ColorScheme::Dark, Colors(0x0d1712,0x17261e,0x111e18,0x24382d,
                         0xf0f8f2,0xa9c1b0,0x3d5c49,0x83d69c,0xffa1a8,0x2b1114,0x79d6a1,0xebc979));
    case ThemePreset::WarmSand:
        return BuildTheme(ColorScheme::Light, Colors(0xeee9df,0xfaf7f0,0xf2ede3,0xffffff,
                         0x302a23,0x6e6255,0xcfc5b5,0x9b5c2e,0xa52f3e,0xffffff,0x2c6a48,0x7b5510));
    case ThemePreset::Rose:
        return BuildTheme(ColorScheme::Light, Colors(0xf2e8eb,0xfff8fa,0xf6edf0,0xffffff,
                         0x34252b,0x735d66,0xd7c5cc,0xa9466b,0xa52e42,0xffffff,0x286746,0x79540d));
    case ThemePreset::Violet:
        return BuildTheme(ColorScheme::Dark, Colors(0x15101f,0x221a30,0x1a1427,0x332642,
                         0xf5f0fb,0xbcaecb,0x554568,0xc19af5,0xff9ba8,0x2b1016,0x81d2a4,0xe8c572));
    case ThemePreset::Solar:
        return BuildTheme(ColorScheme::Light, Colors(0xeee8d5,0xfdf6e3,0xf4edda,0xffffff,
                         0x2b3436,0x5f6b6d,0xc9c1ad,0x856500,0xa72e36,0xffffff,0x2f6d49,0x76540b));
    case ThemePreset::HighContrastLight:
        return BuildTheme(ColorScheme::Light, Colors(0xffffff,0xffffff,0xf5f5f5,0xffffff,
                         0x000000,0x4a4a4a,0x767676,0x0037c1,0x9c001f,0xffffff,0x006b34,0x6b4b00));
    case ThemePreset::HighContrastDark:
        return BuildTheme(ColorScheme::Dark, Colors(0x000000,0x090909,0x000000,0x161616,
                         0xffffff,0xc7c7c7,0x8a8a8a,0x6eb4ff,0xff8b9a,0x260008,0x72e0a3,0xffd36e));
    }
    IM_ASSERT(false && "Unknown ThemePreset");
    return MakeTheme(ThemePreset::PrecisionLight);
}
Theme MakePrecisionTheme(ColorScheme scheme) {
    return MakeTheme(scheme == ColorScheme::Dark ? ThemePreset::PrecisionDark : ThemePreset::PrecisionLight);
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
float AnimationState::Update(ImGuiID id, float target, float dt, float seconds, int frame, bool enabled) {
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
        *slot = {id, target, frame, true};
        return target;
    }
    if (slot->frame != frame) {
        float step = seconds > 0 ? std::max(0.f, dt) / seconds : 1;
        slot->value =
            !enabled || seconds <= 0 ? target : slot->value + std::clamp(target - slot->value, -step, step);
        slot->frame = frame;
    }
    if (!enabled)
        slot->value = target;
    return slot->value;
}
} // namespace imkit
