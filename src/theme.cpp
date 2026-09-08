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
Theme MakePrecisionTheme(ColorScheme scheme) {
    Theme t;
    t.scheme = scheme;
    bool d = scheme == ColorScheme::Dark;
    t.colors = {Hex(d ? 0x14161b : 0xedeef1),
                Hex(d ? 0x20232a : 0xf8f9fb),
                Hex(d ? 0x171a20 : 0xeff1f5),
                Hex(d ? 0x30343e : 0xffffff),
                Hex(d ? 0xeff0f4 : 0x242833),
                Hex(d ? 0xacb2bf : 0x636b7b),
                Hex(d ? 0x454d5c : 0xcbd0da),
                Hex(d ? 0xbba8ff : 0x6950b4),
                Hex(d ? 0x142031 : 0xffffff),
                {},
                {},
                Hex(d ? 0xf2a2a5 : 0xad3441),
                Hex(d ? 0x35191e : 0xffffff),
                Hex(d ? 0x83c6a1 : 0x276341),
                Hex(d ? 0xe4c17b : 0x805a12)};
    SetAccent(t, t.colors.accent);
    return t;
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
