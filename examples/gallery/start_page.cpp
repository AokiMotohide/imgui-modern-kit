#include "gallery.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <optional>
#include <string>
namespace imkit::gallery {
namespace {
ImU32 Color(const ImVec4 &c, float alpha = 1.f) {
    return ColorConvertFloat4ToU32({c.x, c.y, c.z, c.w * alpha});
}
ImFont *DisplayFont(const GalleryState &s) {
    return s.fonts.display ? s.fonts.display : s.fonts.emphasis;
}
void Section(GalleryState &s, const char *title) {
    Spacing();
    PushFont(s.fonts.emphasis, s.theme.metrics.headingSize);
    TextUnformatted(title);
    PopFont();
    Spacing();
}
// The same calls under both styles; only the style differs. Both specimens edit StartPage's values.
void Specimen(GalleryState &s, StartPage &p, bool plain) {
    TextUnformatted("Display settings");
    TextDisabled("The same calls, one theme.");
    Separator();
    if (plain ? ImGui::Button("Apply") : Button("Apply")) ++p.applied;
    Record(s, plain ? "start-default-apply" : "start-imkit-apply");
    SameLine();
    Text("Applied %d", p.applied);
    Checkbox("Enabled", &p.enabled);
    SetNextItemWidth(-CalcTextSize("Level").x - ImGui::GetStyle().ItemInnerSpacing.x);
    SliderFloat("Level", &p.level, 0, 1, "%.2f");
    Record(s, plain ? "start-default-level" : "start-imkit-level");
    if (plain) ImGui::InputText("Name", p.name, sizeof(p.name));
    else InputText("Name", p.name, sizeof(p.name));
    const char *quality[] = {"Draft", "Balanced", "Final"};
    Combo("Quality", &p.quality, quality, 3);
}
void Hero(GalleryState &s, StartPage &p) {
    const bool wide = GetContentRegionAvail().x >= 920.f * s.scale;
    if (!BeginTable("start-hero", wide ? 2 : 1, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX)) return;
    if (wide) {
        TableSetupColumn("copy", ImGuiTableColumnFlags_WidthStretch, 1.15f);
        TableSetupColumn("specimen", ImGuiTableColumnFlags_WidthStretch, 1.f);
    }
    TableNextRow();
    TableNextColumn();
    TextDisabled("IMKIT %s", IMKIT_VERSION);
    PushFont(DisplayFont(s), s.theme.typography.title * 1.6f);
    TextUnformatted("Modern UI.");
    TextUnformatted("Familiar workflow.");
    PopFont();
    Spacing();
    PushTextWrapPos(GetCursorPos().x + std::min(GetContentRegionAvail().x, 560.f * s.scale));
    TextUnformatted("Give your Dear ImGui app a modern look with one theme call. Add richer controls as the tool grows. "
                    "Your context, values and frame loop stay yours.");
    PopTextWrapPos();
    Spacing();
    if (ActionButton("Open live comparison", ActionVariant::Primary, {}, {&s.theme, &s.animation})) {
        s.page = PageCompare;
        s.comparison.open = true;
    }
    Record(s, "start-comparison");
    SameLine();
    if (ActionButton("Explore components", ActionVariant::Secondary, {}, {&s.theme, &s.animation})) s.page = PageBasic;
    Record(s, "start-components");
    Spacing();
    const std::string icons = std::to_string(GetIconCatalog().size()) + " icons";
    const std::string themes = std::to_string(ThemePresets().size()) + " themes";
    StatusBadge(icons.c_str(), StatusKind::Neutral, ComponentOptions{&s.theme});
    SameLine();
    StatusBadge(themes.c_str(), StatusKind::Neutral, ComponentOptions{&s.theme});
    SameLine();
    StatusBadge("C++20", StatusKind::Neutral, ComponentOptions{&s.theme});
    SameLine();
    StatusBadge("MIT", StatusKind::Success, ComponentOptions{&s.theme});
    TableNextColumn();
    const char *styles[] = {"Dear ImGui", "ImKit"};
    Segmented("start-style", &p.style, styles, {&s.theme, &s.animation});
    Record(s, "start-style");
    const float height = 226.f * s.scale;
    if (p.style == 0) {
        DefaultStyleScope plain;
        ImGui::GetStyle().ScaleAllSizes(s.scale / ImGui::GetIO().DisplayFramebufferScale.x);
        if (BeginChild("start-specimen", {0, height}, ImGuiChildFlags_Borders)) Specimen(s, p, true);
        EndChild();
    } else {
        PushStyleColor(ImGuiCol_ChildBg, s.theme.colors.surface);
        if (BeginChild("start-specimen", {0, height}, ImGuiChildFlags_Borders)) Specimen(s, p, false);
        EndChild();
        PopStyleColor();
    }
    EndTable();
}
// One pill per theme, drawn in that theme's own colours. Picking one re-themes the whole Gallery.
bool WorldPill(GalleryState &s, int index, const ThemePresetInfo &info, const SemanticColors &c, bool selected, ImVec2 size) {
    PushID(index);
    const ImVec2 min = GetCursorScreenPos();
    const ImVec2 max{min.x + size.x, min.y + size.y};
    const bool pressed = InvisibleButton("world", size);
    const bool hovered = IsItemHovered();
    Record(s, (std::string("start-world-") + std::to_string(index)).c_str());
    auto *draw = GetWindowDrawList();
    const float radius = s.theme.radius.control + 2.f * s.scale;
    draw->AddRectFilled(min, max, Color(c.canvas), radius);
    draw->AddRect(min, max, Color(selected ? s.theme.semantic.accent : (hovered ? c.accent : c.border), selected || hovered ? 1.f : .9f),
                  radius, 0, (selected ? 2.f : 1.f) * s.scale);
    const float dot = size.y * .17f;
    const ImVec2 centre{min.x + size.y * .5f, min.y + size.y * .5f};
    draw->AddCircleFilled(centre, dot, Color(c.accent));
    draw->AddCircleFilled({centre.x + dot * 1.5f, centre.y}, dot * .6f, Color(c.surface));
    draw->AddText({min.x + size.y * .5f + dot * 3.f, min.y + (size.y - GetFontSize()) * .5f}, Color(c.text), info.displayName.data());
    PopID();
    return pressed;
}
void Worlds(GalleryState &s) {
    Section(s, "Wear a world");
    TextDisabled("Every theme is four colours plus a few tokens. Pick one and the whole Gallery follows.");
    Spacing();
    static constexpr std::array<ThemePreset, 8> worlds{ThemePreset::PrecisionLight, ThemePreset::Graphite, ThemePreset::Midnight,
                                                       ThemePreset::Ocean,          ThemePreset::Forest,   ThemePreset::Violet,
                                                       ThemePreset::WarmSand,       ThemePreset::Rose};
    // Preset colours do not change; compute them once.
    static const auto colors = [] {
        std::array<SemanticColors, worlds.size()> out{};
        for (std::size_t i = 0; i < worlds.size(); ++i) out[i] = MakeTheme(worlds[i]).semantic;
        return out;
    }();
    const ImVec2 size{140.f * s.scale, 40.f * s.scale};
    const float gap = ImGui::GetStyle().ItemSpacing.x, width = GetContentRegionAvail().x;
    float used = 0;
    for (std::size_t i = 0; i < worlds.size(); ++i) {
        const int index = static_cast<int>(worlds[i]);
        // Items return to the line start, so the row width is tracked here.
        if (i > 0 && used + gap + size.x <= width) {
            SameLine();
            used += gap + size.x;
        } else {
            used = size.x;
        }
        if (WorldPill(s, index, ThemePresets()[index], colors[i], s.presetIndex == index, size)) ApplyThemePreset(s, index);
    }
    Spacing();
    if (ActionButton("Design your own", ActionVariant::Ghost, {}, {&s.theme, &s.animation})) s.page = PageLookStudio;
    Record(s, "start-studio");
}
struct Feature {
    IconId icon;
    const char *eyebrow, *title, *body;
    PageId page;
    const char *probe;
};
void Card(GalleryState &s, const Feature &f, float height) {
    PushID(f.probe);
    PushStyleColor(ImGuiCol_ChildBg, s.theme.semantic.surfaceRaised);
    PushStyleVar(ImGuiStyleVar_ChildRounding, s.theme.radius.overlay);
    PushStyleVar(ImGuiStyleVar_WindowPadding, {16.f * s.scale, 14.f * s.scale});
    const bool visible = BeginChild("card", {0, height}, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    PopStyleVar(2);
    PopStyleColor();
    if (visible) {
        Icon(s.icons, f.icon, {26.f * s.scale, s.theme.semantic.accent});
        SameLine();
        TextDisabled("%s", f.eyebrow);
        PushFont(s.fonts.emphasis, s.theme.metrics.headingSize);
        TextUnformatted(f.title);
        PopFont();
        TextWrapped("%s", f.body);
        SetCursorPosY(std::max(GetCursorPosY(), height - GetFrameHeight() - 14.f * s.scale));
        if (ActionButton("Open", ActionVariant::Ghost, {}, {&s.theme, &s.animation})) s.page = f.page;
        Record(s, f.probe);
    }
    EndChild();
    PopID();
}
void Features(GalleryState &s) {
    Section(s, "What is in the box");
    const std::string iconTitle = std::to_string(GetIconCatalog().size()) + " icons, seven sizes";
    const Feature features[] = {
        {IconId::Palette, "LOOK", "One colour. A whole world.",
         "Build a theme from four colours, then add corners, an outline and a shadow. Or replace component frames with your own skin.",
         PageLookStudio, "start-look"},
        {IconId::Layers, "WORKSPACES", "Tools, not just widgets.",
         "Tabs, hierarchy, inspector, timeline and a node editor. Your data and undo history stay in your app.",
         PageWorkbench, "start-workspaces"},
        {IconId::Fader, "SHOW", "Run a live show.",
         "Cue list and GO, mixer, control surface, LED and address tools, scheduling and beat tools for media servers.",
         PageShowLive, "start-show"},
        {IconId::Waveform, "DATA", "Charts and camera control.",
         "Donut, bar, gauge and sparkline charts, plus a pan, tilt and zoom pad driven against a simulated camera.",
         PageCharts, "start-data"},
        {IconId::Bell, "FEEDBACK", "Say what is happening.",
         "Toasts, progress tracks, stages and task cards that report long work without owning it.",
         PageFeedback, "start-feedback"},
        {IconId::Grid, "ICONS", iconTitle.c_str(),
         "Search the catalogue and copy the C++ call. Image-backed artwork is included for production tools.",
         PageIcons, "start-icons"},
    };
    const int columns = std::clamp(static_cast<int>(GetContentRegionAvail().x / (330.f * s.scale)), 1, 3);
    if (BeginTable("start-cards", columns, ImGuiTableFlags_SizingStretchSame)) {
        for (const auto &f : features) {
            TableNextColumn();
            Card(s, f, 168.f * s.scale);
        }
        EndTable();
    }
}
// Draws one code line; text after // is muted.
void CodeLine(const char *line) {
    const char *comment = std::strstr(line, "//");
    if (!comment) {
        TextUnformatted(line);
        return;
    }
    const std::string code(line, comment);
    TextUnformatted(code.c_str());
    SameLine(0, 0);
    TextDisabled("%s", comment);
}
void CodeBlock(GalleryState &s, const char *id, const char *title, std::initializer_list<const char *> lines) {
    PushID(id);
    PushStyleColor(ImGuiCol_ChildBg, s.theme.semantic.surfaceRaised);
    PushStyleVar(ImGuiStyleVar_ChildRounding, s.theme.radius.overlay);
    PushStyleVar(ImGuiStyleVar_WindowPadding, {16.f * s.scale, 12.f * s.scale});
    const bool visible = BeginChild("code", {0, 0}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
    PopStyleVar(2);
    PopStyleColor();
    if (visible) {
        TextDisabled("%s", title);
        PushTextRole(s.theme, TextRole::Mono);
        std::string all;
        for (const char *line : lines) {
            CodeLine(line);
            all += line;
            all += '\n';
        }
        PopTextRole();
        Spacing();
        if (ActionButton("Copy", ActionVariant::Secondary, {}, {&s.theme, &s.animation})) SetClipboardText(all.c_str());
        Record(s, (std::string("start-copy-") + id).c_str());
    }
    EndChild();
    PopID();
}
void Quickstart(GalleryState &s) {
    Section(s, "Add it to your app");
    TextDisabled("Your Dear ImGui context, backends, renderer, fonts, data, undo history and persistence stay yours.");
    Spacing();
    const bool wide = GetContentRegionAvail().x >= 900.f * s.scale;
    if (BeginTable("start-code", wide ? 2 : 1, ImGuiTableFlags_SizingStretchSame)) {
        TableNextColumn();
        CodeBlock(s, "cmake", "CMake",
                  {"set(IMKIT_IMGUI_TARGET host_imgui)", "add_subdirectory(external/imgui-modern-kit)",
                   "target_link_libraries(your_app PRIVATE imkit::imkit)"});
        TableNextColumn();
        CodeBlock(s, "cpp", "C++",
                  {"auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);",
                   "imkit::ApplyTheme(theme, 1.0f); // before ImGui::NewFrame()",
                   "imkit::Toggle(\"Enabled\", &enabled); // inside any window"});
        EndTable();
    }
    Spacing();
    StatusBadge("Context", StatusKind::Neutral, ComponentOptions{&s.theme});
    for (const char *kept : {"Backend", "Renderer", "Fonts", "Data", "Undo", "Persistence"}) {
        SameLine();
        StatusBadge(kept, StatusKind::Neutral, ComponentOptions{&s.theme});
    }
    SameLine();
    TextDisabled("stay in your app");
    Spacing();
    TextDisabled("Next: docs/getting-started/getting-started.md  ·  docs/getting-started/導入ガイド.md");
}
} // namespace
void StartPage::Show(GalleryState &s) {
    Hero(s, *this);
    Worlds(s);
    Features(s);
    Quickstart(s);
}
} // namespace imkit::gallery
