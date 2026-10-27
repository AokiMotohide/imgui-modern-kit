#include "gallery.h"
#include "node_launcher.h"
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <string_view>
#include <sstream>
#include <iomanip>
#include <vector>
namespace imkit::gallery {
void Record(GalleryState &s, const char *name) {
    s.probes[name] = {GetItemRectMin(), GetItemRectMax()};
}
void SelectFramePreset(GalleryState& s, WindowFramePreset preset) {
    s.framePresetStyles[static_cast<std::size_t>(s.framePreset)]=s.frameStyle;
    s.framePreset=preset;
    s.frameStyle=s.framePresetStyles[static_cast<std::size_t>(preset)];
}
void RegenerateFrameColors(GalleryState& s) {
    const auto generated=MakeWindowFrameStyle(s.framePreset,s.theme);
    s.frameStyle.activeBackground=generated.activeBackground;
    s.frameStyle.inactiveBackground=generated.inactiveBackground;
    s.frameStyle.border=generated.border;
    s.frameStyle.titleText=generated.titleText;
    s.frameStyle.auxiliaryText=generated.auxiliaryText;
    s.frameStyle.icon=generated.icon;
    s.frameStyle.buttonText=generated.buttonText;
    s.frameStyle.buttonHover=generated.buttonHover;
    s.frameStyle.buttonPressed=generated.buttonPressed;
    s.frameStyle.closeButtonHover=generated.closeButtonHover;
    s.frameStyle.closeButtonPressed=generated.closeButtonPressed;
}
void ResetFramePreset(GalleryState& s) {
    s.frameStyle=MakeWindowFrameStyle(s.framePreset,s.theme);
    s.framePresetStyles[static_cast<std::size_t>(s.framePreset)]=s.frameStyle;
}
namespace {
void Heading(GalleryState &s, const char *text) {
    PushFont(s.fonts.emphasis, s.theme.metrics.headingSize);
    TextUnformatted(text);
    PopFont();
    Spacing();
}
void StartCard(GalleryState &s, int page, const char *eyebrow, const char *title,
               const char *description, const char *action) {
    PushID(page);
    PushStyleColor(ImGuiCol_ChildBg, s.theme.semantic.surfaceRaised);
    PushStyleVar(ImGuiStyleVar_ChildRounding, s.theme.radius.overlay);
    PushStyleVar(ImGuiStyleVar_WindowPadding, {16.f * s.scale, 14.f * s.scale});
    const bool routeVisible = BeginChild("route", {0, 150 * s.scale}, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    PopStyleVar(2);
    PopStyleColor();
    if (routeVisible) {
        TextDisabled("%s", eyebrow);
        PushFont(s.fonts.emphasis, s.theme.metrics.headingSize);
        TextUnformatted(title);
        PopFont();
        TextWrapped("%s", description);
        if (ActionButton(action, ActionVariant::Secondary, {}, {&s.theme, &s.animation}))
            s.page = page;
        Record(s, (std::string("start-") + action).c_str());
    }
    EndChild();
    PopID();
}
// One heading per page: the title, then a single line saying what the page demonstrates.
void PageHeader(GalleryState &s, const char *title, const char *summary, bool compact = false) {
    PushFont(s.fonts.emphasis, s.theme.metrics.headingSize * (compact ? 1.f : 1.15f));
    TextUnformatted(title);
    PopFont();
    if (summary && *summary) {
        // Dense editor workspaces keep the summary on the title line to save vertical room.
        if (compact)
            SameLine();
        TextDisabled("%s", summary);
    }
    if (!compact)
        Spacing();
}
const char *PageSummary(int page) {
    switch (page) {
    case 0: return "Buttons, selection controls and text. Standard Dear ImGui behaviour with a modern look.";
    case 1: return "Scalar, vector and unit-bearing numeric input.";
    case 2: return "Text, colour, image and plot widgets.";
    case 3: return "Trees, tabs, tables and drag and drop.";
    case 4: return "Popups, modals, toolbars, scroll regions and docking.";
    case 5: return "Segmented controls, badges, notifications and Japanese text.";
    case 6: return "Searchable icon catalogue with copyable C++ calls.";
    case 7: return "Timeline ruler, transport and curve editing.";
    case 8: return "Media bin, monitors and inspector for video editing.";
    case 9: return "Views, scene properties and gizmos for CG editing.";
    case 10: return "Density, contrast, typography and motion tokens.";
    case 11: return "Rows, toolbars, command palette, dialogs and a 100,000 row data table.";
    case 12: return "Navigation, command and data patterns.";
    case 13: return "The semantic tree published for assistive technology.";
    case 14: return "Layouts that adapt from narrow to wide.";
    case 15: return "A full workspace assembled from public components.";
    case 16: return "Progress, stages, alerts and empty, error and retry states.";
    case 17: return "Resizable preview tiles with host-owned state.";
    case 18: return "Window frame presets with live colour and metric editing.";
    case 20: return "Stacked notifications with progress, actions and timers.";
    case 21: return "The same ImGui calls with and without an ImKit theme.";
    case 22: return "Tabs, hierarchy actions, settings and choices in one screen.";
    case 23: return "Donut, bar, gauge, sparkline and progress components.";
    case 24: return "Pan, tilt and zoom control with presets, driven against a simulated camera.";
    case 25: return "Faders, knobs, pads, meters, clocks and hold-to-confirm controls for a live operator.";
    case 26: return "Cue list, clip launcher, mixer, transition, effects, modulator, mappings and device health.";
    case 27: return "Build if / then / else rules without code, explain them in a sentence and test them against live values.";
    case 28: return "Calendar, resource timeline, recurrence, cron, sunrise triggers and weekly on-air hours with conflict detection.";
    case 29: return "Beat clock, decks, scrolling waveforms, hot cues, channel strips, pitch, step sequencer and spectrum.";
    case 30: return "Paste a script, see the problems line by line, get controls for its parameters, and import a pasted table.";
    default: return "";
    }
}
void Start(GalleryState &s) {
    Heading(s, "Modern UI. Familiar workflow.");
    TextWrapped("Modernize your existing Dear ImGui app with a theme. Add useful controls as your tool grows. "
                "Your context, values and frame loop stay yours.");
    if (ActionButton("Open live comparison", ActionVariant::Primary, {}, {&s.theme, &s.animation}))
        { s.page = 21; s.comparison.open = true; }
    Record(s, "start-comparison");
    if(GetContentRegionAvail().x > 650.f) SameLine();
    if (Button("Explore components"))
        s.page = 0;
    Record(s, "start-components");
    if(GetContentRegionAvail().x > 650.f) SameLine();
    if (Button("Circular progress"))
        s.page = 16;
    Record(s, "start-circular-progress");
    SeparatorText("Choose a route");
    if (BeginTable("start-routes", GetContentRegionAvail().x < 680.f ? 1 : 2, ImGuiTableFlags_SizingStretchSame)) {
        TableNextRow();
        TableNextColumn();
        StartCard(s, 21, "01 / THEME ONLY", "Modernize an existing UI", "The same ImGui calls, the same values. Compare a theme with the default style.", "Compare themes");
        TableNextColumn();
        StartCard(s, 6, "02 / ICON LIBRARY", "Find your next icon", "288 icons, 19 categories and seven sizes. Search, select and copy the C++ call.", "Explore icons");
        TableNextRow();
        TableNextColumn();
        StartCard(s, 22, "03 / NEW IN 3.2", "Build a useful workspace", "Tabs, hierarchy actions, settings, three-axis editing and choices in one live screen.", "Open 3.2 workspace");
        TableNextColumn();
        StartCard(s, 8, "04 / EDITING", "Scale into timelines", "Explore Video editing UI. Media, project data and Undo stay in the host.", "Open timeline");
        EndTable();
    }
    SeparatorText("Try it here");
    Toggle("Enable preview", &s.toggle, {&s.theme, &s.animation});
    SameLine();
    if(ActionButton("Apply settings", ActionVariant::Primary)) ++s.clicks;
    SameLine(); Text("Applied %d times", s.clicks);
    TextDisabled("Learning map: docs/getting-started/examples-recipes.md (English) / docs/getting-started/実例とレシピ.md (日本語)");
    SeparatorText("What remains yours");
    TextWrapped("Your Dear ImGui context, renderer, font atlas, data, undo history and persistence stay in the host. "
                "ImKit supplies visual structure and reusable controls without taking those responsibilities.");
    if (ActionButton("Open Frame Lab", ActionVariant::Ghost, {}, {&s.theme, &s.animation}))
        s.page = 18;
    Record(s, "start-frame-lab");
}
class DefaultStyleScope {
  public:
    DefaultStyleScope() : previous_(ImGui::GetStyle()) {
        ImGui::GetStyle() = ImGuiStyle{};
        ImGui::StyleColorsDark(&ImGui::GetStyle());
        ImGui::GetStyle().FontSizeBase=previous_.FontSizeBase;
        ImGui::GetStyle().FontScaleMain=previous_.FontScaleMain;
        ImGui::GetStyle().FontScaleDpi=previous_.FontScaleDpi;
        ImGui::GetStyle().ScaleAllSizes(ImGui::GetIO().DisplayFramebufferScale.x);
        ImGui::GetStyle().Colors[ImGuiCol_ChildBg].w = 1.f;
    }
    ~DefaultStyleScope() { ImGui::GetStyle() = previous_; }
    DefaultStyleScope(const DefaultStyleScope &) = delete;
    DefaultStyleScope &operator=(const DefaultStyleScope &) = delete;

  private:
    ImGuiStyle previous_;
};
void DefaultComparisonSpecimen(GalleryState &s, bool modern=false) {
    TextUnformatted(modern ? "ImKit theme" : "Dear ImGui default");
    TextDisabled("Identical ImGui:: calls / shared application values");
    ImGui::BeginDisabled(s.comparison.disabled);
    Separator();
    if(!modern && s.comparison.focusDefault) { ImGui::SetKeyboardFocusHere(); s.comparison.focusDefault=false; }
    if (ImGui::Button("Apply"))
        ++s.comparison.applyCount;
    Record(s, modern ? "comparison-imkit-apply" : "comparison-default-apply");
    ImGui::SameLine();
    ImGui::Text("Applied: %d", s.comparison.applyCount);
    imkit::Checkbox("Enabled", &s.comparison.enabled);
    Record(s, modern ? "comparison-imkit-enabled" : "comparison-default-enabled");
    ImGui::SetNextItemWidth(-ImGui::CalcTextSize("Level").x-ImGui::GetStyle().ItemInnerSpacing.x);
    imkit::SliderFloat("Level", &s.comparison.level, 0, 1, "%.2f");
    Record(s, modern ? "comparison-imkit-level" : "comparison-default-level");
    ImGui::InputText("Name", s.comparison.name, sizeof(s.comparison.name));
    Record(s, modern ? "comparison-imkit-name" : "comparison-default-name");
    const char *quality[] = {"Draft", "Balanced", "Final"};
    imkit::Combo("Quality", &s.comparison.quality, quality, 3);
    Record(s, modern ? "comparison-imkit-quality" : "comparison-default-quality");
    ImGui::EndDisabled();
}
void ImKitComparisonSpecimen(GalleryState &s) {
    TextUnformatted("ImKit");
    TextDisabled("Additional components / shared application values");
    ImGui::BeginDisabled(s.comparison.disabled);
    Separator();
    if (ActionButton("Apply", ActionVariant::Primary, {}, {&s.theme, &s.animation}))
        ++s.comparison.applyCount;
    Record(s, "comparison-imkit-apply");
    SameLine();
    Text("Applied: %d", s.comparison.applyCount);
    Toggle("Enabled", &s.comparison.enabled, {&s.theme, &s.animation});
    Record(s, "comparison-imkit-enabled");
    SetNextItemWidth(-ImGui::CalcTextSize("Level").x-ImGui::GetStyle().ItemInnerSpacing.x);
    SliderFloat("Level", &s.comparison.level, 0, 1, "%.2f");
    Record(s, "comparison-imkit-level");
    InputText("Name", s.comparison.name, sizeof(s.comparison.name));
    Record(s, "comparison-imkit-name");
    const char *quality[] = {"Draft", "Balanced", "Final"};
    Combo("Quality", &s.comparison.quality, quality, 3);
    Record(s, "comparison-imkit-quality");
    ImGui::EndDisabled();
}
void Icons(GalleryState &s) {
    TextDisabled("%d preset icons · %d atlas sizes · searchable catalog",
                 static_cast<int>(GetIconCatalog().size()), static_cast<int>(IconPixelSizes.size()));
    SetNextItemWidth(210);
    InputText("Search", s.iconSearch, sizeof(s.iconSearch));
    Record(s, "icon-search");
    SameLine();
    static const auto categories=[] {
        std::vector<const char*> values{"All"};
        for (const auto &icon:GetIconCatalog())
            if (std::none_of(values.begin(),values.end(),[&](const char *value){return std::string_view(value)==icon.category;}))
                values.push_back(icon.category);
        return values;
    }();
    SetNextItemWidth(140);
    Combo("Category", &s.iconCategory, categories.data(), static_cast<int>(categories.size()));
    SameLine();
    const char *sizes[] = {"12", "16", "20", "24", "32", "48", "64"};
    SetNextItemWidth(CalcTextSize("64").x+GetFrameHeight()+GetStyle().FramePadding.x*2);
    Combo("Size", &s.iconSizeIndex, sizes, 7);
    Checkbox("Custom icon color", &s.iconCustomColor);
    SameLine();
    SetNextItemWidth(230);
    ColorEdit4("Tint", &s.iconColor.x, ImGuiColorEditFlags_NoInputs);
    const IconOptions options{static_cast<float>(IconPixelSizes[s.iconSizeIndex]) * s.scale,
                              s.iconCustomColor ? std::optional<ImVec4>(s.iconColor) : std::nullopt};
    const auto selected = static_cast<IconId>(s.selectedIcon);
    const auto *info = GetIconInfo(selected);
    Separator();
    Icon(s.icons, selected, {48 * s.scale, options.color});
    SameLine();
    Text("%s / %s", info->name, info->category);
    char example[256];
    std::snprintf(example, sizeof(example), "imkit::Icon(icons, imkit::IconId::%s, {.size = %d});",
                  info->name, IconPixelSizes[s.iconSizeIndex]);
    TextUnformatted(example);
    SameLine();
    if (Button("Copy code"))
        SetClipboardText(example);
    if (s.iconFocus) {
        SetKeyboardFocusHere();
        s.iconFocus = false;
    }
    if (IconButton("icon-example", s.icons, selected, info->name, options))
        ++s.iconClicks;
    Record(s, "icon-button");
    s.iconFocused = IsItemFocused();
    SameLine();
    if (IconLabelButton("icon-label-example", s.icons, selected, "Run action", options))
        ++s.iconClicks;
    Record(s, "icon-label-button");
    SameLine();
    BeginDisabled();
    if (IconLabelButton("icon-disabled-example", s.icons, selected, "Unavailable", options))
        ++s.iconClicks;
    Record(s, "icon-disabled");
    EndDisabled();
    SameLine();
    Text("Activated: %d", s.iconClicks);
    Separator();
    std::string query = s.iconSearch;
    auto fold = [](std::string &value) {
        for (auto &c : value)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    };
    fold(query);
    const int columns = std::max(1, static_cast<int>(GetContentRegionAvail().x / (210 * s.scale)));
    if (BeginTable("Icon catalogue", columns, ImGuiTableFlags_SizingStretchSame)) {
        for (const auto &item : GetIconCatalog()) {
            if (s.iconCategory && std::string_view(item.category) != categories[s.iconCategory])
                continue;
            std::string searchable = std::string(item.name) + " " + item.category;
            fold(searchable);
            if (searchable.find(query) == std::string::npos)
                continue;
            TableNextColumn();
            PushID(static_cast<int>(item.id));
            if (IconLabelButton("pick", s.icons, item.id, item.name, options))
                s.selectedIcon = static_cast<int>(item.id);
            Record(s,(std::string("icon-pick-")+std::string(item.name)).c_str());
            PopID();
        }
        EndTable();
    }
}
} // namespace
// Responsive masonry of cards: each card goes into the currently shortest column.
namespace {
struct GridLayout {
    ImVec2 origin{};
    float columnWidth = 0, gap = 0, width = 0;
    int columns = 1, current = 0;
    std::array<float, 4> bottom{};
} grid;
} // namespace
bool BeginGrid(GalleryState &s, const char *id, int maxColumns) {
    grid.gap = 12.f * s.scale;
    grid.width = std::max(1.f, GetContentRegionAvail().x);
    const float minimum = 440.f * s.scale;
    grid.columns = std::clamp(static_cast<int>((grid.width + grid.gap) / (minimum + grid.gap)), 1, std::min(maxColumns, 4));
    grid.columnWidth = (grid.width - grid.gap * static_cast<float>(grid.columns - 1)) / static_cast<float>(grid.columns);
    grid.origin = GetCursorScreenPos();
    grid.bottom.fill(grid.origin.y);
    PushID(id);
    return true;
}
void EndGrid() {
    const float bottom = *std::max_element(grid.bottom.begin(), grid.bottom.begin() + grid.columns);
    SetCursorScreenPos({grid.origin.x, bottom});
    Dummy({grid.width, 0});
    PopID();
}
// One titled card in the grid. Always pair with EndSection, including a false return.
bool BeginSection(GalleryState &s, const char *title, const char *hint) {
    grid.current = static_cast<int>(std::min_element(grid.bottom.begin(), grid.bottom.begin() + grid.columns) - grid.bottom.begin());
    SetCursorScreenPos({grid.origin.x + static_cast<float>(grid.current) * (grid.columnWidth + grid.gap), grid.bottom[grid.current]});
    PushStyleColor(ImGuiCol_ChildBg, s.theme.semantic.surfaceRaised);
    PushStyleVar(ImGuiStyleVar_ChildRounding, s.theme.radius.overlay);
    PushStyleVar(ImGuiStyleVar_WindowPadding, {16.f * s.scale, 14.f * s.scale});
    const bool visible = BeginChild(title, {grid.columnWidth, 0}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding,
                                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    PopStyleVar(2);
    PopStyleColor();
    if (visible) {
        PushFont(s.fonts.emphasis, s.theme.metrics.bodySize * 1.08f);
        TextUnformatted(title);
        PopFont();
        if (hint)
            TextDisabled("%s", hint);
        Spacing();
        PushItemWidth(std::min(GetContentRegionAvail().x * .62f, 380.f * s.scale));
    }
    return visible;
}
void EndSection(bool visible) {
    if (visible)
        PopItemWidth();
    EndChild();
    grid.bottom[grid.current] = GetItemRectMax().y + grid.gap;
}
namespace {
void Basic(GalleryState &s) {
    if (BeginGrid(s, "basic-grid")) {
        const bool actions = BeginSection(s, "Actions", "Variants, states and icon buttons");
        if (actions) {
            if (s.focusApply) {
                SetKeyboardFocusHere();
                s.focusApply = false;
            }
            if (ActionButton("Apply", ActionVariant::Primary, {}, {&s.theme, &s.animation}))
                ++s.clicks;
            Record(s, "apply");
            s.applyFocused = IsItemFocused();
            SameLine();
            ActionButton("Secondary", ActionVariant::Secondary, {}, {&s.theme, &s.animation});
            SameLine();
            ActionButton("Ghost", ActionVariant::Ghost, {}, {&s.theme, &s.animation});
            SameLine();
            ActionButton("Delete", ActionVariant::Destructive, {}, {&s.theme, &s.animation});
            BeginDisabled();
            if (Button("Disabled"))
                ++s.clicks;
            Record(s, "disabled");
            EndDisabled();
            SameLine();
            SmallButton("Inline");
            SameLine();
            ArrowButton("Next", ImGuiDir_Right);
            SameLine();
            IconButton("Up", ImGuiDir_Up, "Move up");
            TextDisabled("Applied: %d", s.clicks);
        }
        EndSection(actions);
        const bool selection = BeginSection(s, "Selection", "Checks, switches, choices and lists");
        if (selection) {
            Checkbox("Enabled", &s.checked);
            Record(s, "checkbox");
            SameLine();
            Toggle("Switch", &s.toggle, {&s.theme, &s.animation});
            Record(s, "toggle");
            IndeterminateCheckbox("Mixed selection", &s.mixed);
            Record(s, "mixed");
            RadioButton("Low", &s.radio, 0);
            SameLine();
            RadioButton("High", &s.radio, 1);
            Record(s, "radio");
            Selectable("Selected row marks its leading edge", &s.selected);
            Record(s, "selectable");
            static const char *items[] = {"Low", "Medium", "High"};
            Combo("Quality", &s.combo, items, 3);
            Record(s, "combo");
            if (BeginListBox("List", {0, 96 * s.scale})) {
                for (int i = 0; i < 3; ++i) {
                    PushID(i);
                    if (Selectable(items[i], s.combo == i))
                        s.combo = i;
                    PopID();
                }
                EndListBox();
            }
        }
        EndSection(selection);
        const bool text = BeginSection(s, "Text and links", "Hierarchy comes from size, colour and weight");
        if (text) {
            TextUnformatted("Precision Layers / 表示設定");
            TextColored(s.theme.colors.success, "Ready");
            TextDisabled("Unavailable");
            TextWrapped("A neutral palette separates the canvas, panels, inputs and overlays. Standard Dear ImGui "
                        "behavior is retained.");
            LabelText("Version", "%s", IMKIT_VERSION);
            BulletText("Keyboard focus and selection are different states.");
            TextLink("Local link action");
        }
        EndSection(text);
        const bool identity = BeginSection(s, "ID stack check", "Identical labels stay distinct under PushID");
        if (identity) {
            PushID("first");
            if (Button("Same label"))
                ++s.clicks;
            Record(s, "same-first");
            PopID();
            SameLine();
            PushID("second");
            if (Button("Same label"))
                s.clicks += 10;
            Record(s, "same-second");
            PopID();
        }
        EndSection(identity);
        EndGrid();
    }
}
void Numeric(GalleryState &s) {
    if (BeginGrid(s, "numeric-grid")) {
        const bool scalars = BeginSection(s, "Scalars", "Drag, slide and type values");
        if (scalars) {
            DragFloat("Value", &s.scalar, .01f, 0, 1);
            Record(s, "drag");
            SliderFloat("Level", &s.scalar, 0, 1);
            Record(s, "slider");
            RangeSlider("Range", &s.lower, &s.upper, 0, 100, "%.0f", {}, {&s.theme, &s.animation});
            Record(s, "range");
            InputScalar("64-bit integer", ImGuiDataType_S64, &s.integer64);
            Record(s, "int64");
            InputDouble("Precision", &s.precise);
            InputFloat("Float", &s.scalar);
            InputInt("Count", &s.integers[0]);
        }
        EndSection(scalars);
        const bool vectors = BeginSection(s, "Vectors", "Two to four components");
        if (vectors) {
            DragFloat2("Drag 2", s.vector);
            DragFloat3("Drag 3", s.vector);
            DragFloat4("Drag 4", s.vector);
            SliderInt4("Int 4", s.integers, 0, 100);
            InputInt3("Input 3", s.integers);
        }
        EndSection(vectors);
        const bool units = BeginSection(s, "Units", "Values with a visible unit; narrow widths stack the axes");
        if (units) {
            InputScalarWithUnit("Distance", ImGuiDataType_Double, &s.precise, "mm");
            InputVector3WithUnit("Position", s.vector, "mm");
            DragVector3WithUnit("Offset", s.vector, "mm", .1f, -100.f, 100.f, "%.2f");
            SetNextItemWidth(160 * s.scale);
            DragVector3WithUnit("Narrow offset", s.vector, "mm", .1f, -100.f, 100.f, "%.2f");
            DragFloatWithUnit("Angle", &s.vector[0], "deg", .1f, -180, 180);
        }
        EndSection(units);
        const bool level = BeginSection(s, "Level", "Vertical slider and progress share the value");
        if (level) {
            VSliderFloat("##vertical", {44 * s.scale, 96 * s.scale}, &s.scalar, 0, 1, "%.2f");
            SameLine();
            ProgressBar(s.scalar, {GetContentRegionAvail().x, 0}, "Level");
        }
        EndSection(level);
        EndGrid();
    }
}
int TextCallback(ImGuiInputTextCallbackData *data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit)
        ++static_cast<GalleryState *>(data->UserData)->callbackCount;
    return 0;
}
int ResizeCallback(ImGuiInputTextCallbackData *data) {
    auto *s = static_cast<GalleryState *>(data->UserData);
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        s->growing.resize(data->BufSize);
        data->Buf = s->growing.data();
    }
    return 0;
}
void InputColor(GalleryState &s) {
    if (BeginGrid(s, "input-grid")) {
        const bool inputs = BeginSection(s, "Text input", "Native editing with validation");
        if (inputs) {
            InputText("Name / 名前", s.name, sizeof(s.name), ImGuiInputTextFlags_CallbackEdit, TextCallback, &s);
            Record(s, "text");
            InputText("Resizable", s.growing.data(), s.growing.size(), ImGuiInputTextFlags_CallbackResize,
                      ResizeCallback, &s);
            Record(s, "resize-text");
            InputTextWithHint("Required", "Enter a name", s.validation, sizeof(s.validation));
            Record(s, "validation");
            ValidationMessage("A name is required", s.validation[0] == 0, ComponentOptions{&s.theme});
            InputTextMultiline("Notes", s.notes, sizeof(s.notes), {0, 90 * s.scale});
            Record(s, "multiline");
        }
        EndSection(inputs);
        const bool color = BeginSection(s, "Colour", "Edit and pick");
        if (color) {
            ColorEdit4("Tint", &s.color.x);
            Record(s, "color");
            ColorPicker4("Picker", &s.color.x, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs);
            Record(s, "picker");
        }
        EndSection(color);
        const bool image = BeginSection(s, "Image", "Host-owned texture");
        if (image) {
            Image(s.texture, {180 * s.scale, 100 * s.scale});
            ImageWithBg(s.texture, {180 * s.scale, 48 * s.scale}, {0, 0}, {1, 1}, s.theme.colors.input, s.color);
            s.imageActivated = ImageButton("Texture action", s.texture, {40 * s.scale, 40 * s.scale}) || s.imageActivated;
            Record(s, "image-button");
        }
        EndSection(image);
        const bool plots = BeginSection(s, "Plots", "Dear ImGui plots; see Charts for ImKit chart components");
        if (plots) {
            const float values[] = {.1f, .4f, .8f, .5f, .3f, .9f, .65f};
            PlotLines("Signal", values, 7, 0, nullptr, 0, 1, {GetContentRegionAvail().x * .7f, 70 * s.scale});
            PlotHistogram("Histogram", values, 7, 0, nullptr, 0, 1, {GetContentRegionAvail().x * .7f, 70 * s.scale});
        }
        EndSection(plots);
        EndGrid();
    }
}
void Hierarchy(GalleryState &s) {
    if (BeginGrid(s, "hierarchy-grid")) {
        const bool tree = BeginSection(s, "Tree and tabs", "Hierarchy, headers and drag and drop");
        if (tree) {
            bool open = TreeNode("Settings");
            Record(s, "tree");
            s.treeOpen = open;
            if (open) {
                TreeNodeEx("Display", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                TreePop();
            }
            if (SectionHeader("quality-options", "Quality options", s.qualityOpen, {&s.theme, &s.animation}))
                Checkbox("Enabled in hierarchy", &s.checked);
            if (BeginTabBar("Tabs", ImGuiTabBarFlags_Reorderable)) {
                if (BeginTabItem("Display")) {
                    Record(s, "tab-display");
                    s.tab = 0;
                    TextUnformatted("Display content");
                    EndTabItem();
                } else
                    Record(s, "tab-display");
                if (BeginTabItem("Quality")) {
                    Record(s, "tab-quality");
                    s.tab = 1;
                    TextUnformatted("Quality content");
                    EndTabItem();
                } else
                    Record(s, "tab-quality");
                if (s.tabOpen) {
                    bool active = BeginTabItem("Closable", &s.tabOpen);
                    Record(s, "tab-close");
                    if (active) {
                        TextUnformatted("Close with the tab x");
                        EndTabItem();
                    }
                }
                TabItemButton("+");
                EndTabBar();
            }
            Button("Drag source");
            if (BeginDragDropSource()) {
                SetDragDropPayload("IMKIT_SAMPLE", &s.selectedRow, sizeof(s.selectedRow));
                TextUnformatted("Display");
                EndDragDropSource();
            }
            SameLine();
            Button("Drop target");
            if (BeginDragDropTarget()) {
                if (auto *p = AcceptDragDropPayload("IMKIT_SAMPLE"))
                    s.selectedRow = *static_cast<const int *>(p->Data);
                EndDragDropTarget();
            }
            TextDisabled("Selected %d / inline actions %d", s.selectedRow + 1, s.inlineActions);
        }
        EndSection(tree);
        const bool table = BeginSection(s, "Table", "Sortable, resizable and scrollable");
        if (table) {
            if (BeginTable("Items", 3,
                           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable |
                               ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_ScrollY,
                           {0, 280 * s.scale})) {
                TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort);
                TableSetupColumn("Quality");
                TableSetupColumn("Action", ImGuiTableColumnFlags_NoSort);
                TableSetupScrollFreeze(0, 1);
                TableNextRow(ImGuiTableRowFlags_Headers);
                TableSetColumnIndex(0);
                TableHeader("Name");
                Record(s, "header-name");
                TableSetColumnIndex(1);
                TableHeader("Quality");
                Record(s, "sort-quality");
                TableSetColumnIndex(2);
                TableHeader("Action");
                Record(s, "header-action");
                if (auto *sort = TableGetSortSpecs(); sort && sort->SpecsDirty) {
                    std::sort(s.rows.begin(), s.rows.end());
                    if (sort->SpecsCount && sort->Specs[0].SortDirection == ImGuiSortDirection_Descending)
                        std::reverse(s.rows.begin(), s.rows.end());
                    sort->SpecsDirty = false;
                }
                for (int i = 0; i < 30; ++i) {
                    PushID(i);
                    TableNextRow();
                    TableNextColumn();
                    char name[32];
                    std::snprintf(name, sizeof(name), "Display %d", i + 1);
                    if (Selectable(name, s.selectedRow == i))
                        s.selectedRow = i;
                    Record(s, ("row-" + std::to_string(i)).c_str());
                    TableNextColumn();
                    Text("%d", s.rows[i % s.rows.size()]);
                    TableNextColumn();
                    if (SmallButton("Edit"))
                        ++s.inlineActions;
                    Record(s, ("edit-" + std::to_string(i)).c_str());
                    PopID();
                }
                s.tableScroll = GetScrollY();
                EndTable();
            }
        }
        EndSection(table);
        EndGrid();
    }
}
void Overlay(GalleryState &s) {
    if (BeginMenuBar()) {
        if (BeginMenu("Settings")) {
            MenuItem("Enabled", nullptr, &s.checked);
            if (BeginMenu("Quality")) {
                MenuItem("High");
                EndMenu();
            }
            EndMenu();
        }
        EndMenuBar();
    }
    if (BeginGrid(s, "overlay-grid")) {
        const bool popups = BeginSection(s, "Popups and modal", "Overlays carry a soft shadow");
        if (popups) {
            if (Button("Open popup"))
                OpenPopup("Actions");
            Record(s, "popup");
            s.popupVisible = BeginPopup("Actions");
            if (s.popupVisible) {
                OverlayDecoration(s.theme, &s.animation);
                if (MenuItem("Apply"))
                    ++s.clicks;
                Record(s, "popup-apply");
                BeginDisabled();
                MenuItem("Unavailable");
                EndDisabled();
                EndPopup();
            }
            SameLine();
            if (Button("Open modal"))
                OpenPopup("Apply settings?");
            Record(s, "modal");
            s.modalLauncherFocused = IsItemFocused();
            Button("Context menu (right click)");
            if (BeginPopupContextItem()) {
                MenuItem("Reset");
                EndPopup();
            }
            Button("Hover for tooltip");
            if (BeginItemTooltip()) {
                TextUnformatted("A raised, compact tooltip.");
                EndTooltip();
            }
        // The modal must live in the same ID scope as the OpenPopup call above.
        bool modalOpen = true;
        s.modalVisible = BeginPopupModal("Apply settings?", &modalOpen, ImGuiWindowFlags_AlwaysAutoResize);
        if (s.modalVisible) {
            OverlayDecoration(s.theme, &s.animation);
            TextUnformatted("Apply the current display settings?");
            const char *values[] = {"Low", "Medium", "High"};
            Combo("Quality##modal", &s.combo, values, 3);
            Record(s, "modal-combo");
            if (ActionButton("Apply##modal", ActionVariant::Primary, {}, {&s.theme})) {
                ++s.clicks;
                CloseCurrentPopup();
            }
            SameLine();
            if (Button("Cancel") || IsKeyPressed(ImGuiKey_Escape))
                CloseCurrentPopup();
            Record(s, "cancel");
            EndPopup();
        }
        }
        EndSection(popups);
        const bool toolbar = BeginSection(s, "Toolbar and settings row", "Compact chrome around related controls");
        if (toolbar) {
            if (BeginToolbar("Toolbar")) {
                Button("New");
                SameLine();
                Button("Save");
                SameLine();
                TextDisabled("Session only");
            }
            EndToolbar();
            if (BeginSettingRow("setting", "Display")) {
                Checkbox("Enabled##setting", &s.checked);
                EndSettingRow();
            }
        }
        EndSection(toolbar);
        const bool scroll = BeginSection(s, "Scroll region", "A bordered child with its own scroll");
        if (scroll) {
            if (BeginChild("Scroll", {0, 140 * s.scale}, ImGuiChildFlags_Borders)) {
                for (int i = 0; i < 12; ++i)
                    Text("Scrollable content %02d", i);
            }
            EndChild();
        }
        EndSection(scroll);
        const bool dock = BeginSection(s, "Docking surface", "The host owns the layout");
        if (dock) {
            const ImVec2 origin = GetCursorScreenPos();
            const ImVec2 extent{GetContentRegionAvail().x, 180 * s.scale};
            DockSpace(GetID("CatalogDock"), {0, extent.y}, ImGuiDockNodeFlags_None);
            // A dashed outline marks the region the host hands to its docking layout.
            auto *draw = GetWindowDrawList();
            const ImU32 dash = GetColorU32(ImGuiCol_TextDisabled);
            const ImVec2 corner{origin.x + extent.x, origin.y + extent.y};
            for (float x = origin.x; x < corner.x; x += 12.f) {
                draw->AddLine({x, origin.y}, {std::min(x + 6.f, corner.x), origin.y}, dash, 1.f);
                draw->AddLine({x, corner.y}, {std::min(x + 6.f, corner.x), corner.y}, dash, 1.f);
            }
            for (float y = origin.y; y < corner.y; y += 12.f) {
                draw->AddLine({origin.x, y}, {origin.x, std::min(y + 6.f, corner.y)}, dash, 1.f);
                draw->AddLine({corner.x, y}, {corner.x, std::min(y + 6.f, corner.y)}, dash, 1.f);
            }
            const char *label = "Dock host area";
            const ImVec2 labelSize = CalcTextSize(label);
            draw->AddText({origin.x + (extent.x - labelSize.x) * .5f, origin.y + (extent.y - labelSize.y) * .5f}, dash, label);
        }
        EndSection(dock);
        EndGrid();
    }
}
void Composites(GalleryState &s) {
    if (BeginGrid(s, "composites-grid")) {
        const bool choices = BeginSection(s, "Choices", "Segmented control and searchable combo");
        if (choices) {
            static const char *labels[] = {"Low", "Medium", "High"};
            Segmented("segments", &s.segment, labels, {&s.theme, &s.animation});
            Record(s, "segments");
            static const bool disabled[] = {false, true, false};
            SearchableCombo("Search quality", &s.combo, labels, s.search, sizeof(s.search), disabled);
            Record(s, "search-combo");
        }
        EndSection(choices);
        const bool status = BeginSection(s, "Status", "Badges and notification cards");
        if (status) {
            StatusBadge("Ready / 準備完了", StatusKind::Success, ComponentOptions{&s.theme});
            SameLine();
            StatusBadge("Warning / 注意", StatusKind::Warning, ComponentOptions{&s.theme});
            SameLine();
            StatusBadge("Error / エラー", StatusKind::Error, ComponentOptions{&s.theme});
            if (s.notice &&
                NotificationCard({"notice", "Settings saved / 設定を保存しました", StatusKind::Success, 0}, GetTime(),
                                 ComponentOptions{&s.theme}))
                s.notice = false;
        }
        EndSection(status);
        const bool japanese = BeginSection(s, "Japanese text", "Fallback font coverage and mixed scripts");
        if (japanese) {
            TextUnformatted("表示設定 — ひらがな・カタカナ・漢字・半角ｶﾅ。");
            InputText("名前 / Name", s.name, sizeof(s.name));
            Toggle("表示を有効にする / Enabled", &s.toggle, {&s.theme, &s.animation});
            InputScalarWithUnit("位置", ImGuiDataType_Double, &s.precise, "mm");
        }
        EndSection(japanese);
        const bool actions = BeginSection(s, "Actions", "Localised labels");
        if (actions) {
            ActionButton("適用 / Apply", ActionVariant::Primary, {}, {&s.theme, &s.animation});
            SameLine();
            BeginDisabled();
            ActionButton("削除できません", ActionVariant::Destructive, {}, {&s.theme});
            EndDisabled();
            TextWrapped("Values and notifications belong to the host. Font files are optional catalog assets. No "
                        "operating-system font discovery or hidden context is used.");
        }
        EndSection(actions);
        EndGrid();
    }
}
void FrameLab(GalleryState& s) {
    const char* presets[]={"Native","Studio","Workspace","Tool"};
    int preset=static_cast<int>(s.framePreset);
    struct ColorEntry { const char* name; const char* field; ImVec4* value; };
    ColorEntry colors[]={{"Active background","activeBackground",&s.frameStyle.activeBackground},
        {"Inactive background","inactiveBackground",&s.frameStyle.inactiveBackground},{"Border","border",&s.frameStyle.border},
        {"Title text","titleText",&s.frameStyle.titleText},{"Auxiliary text","auxiliaryText",&s.frameStyle.auxiliaryText},
        {"Icon","icon",&s.frameStyle.icon},{"Button text","buttonText",&s.frameStyle.buttonText},
        {"Button hover","buttonHover",&s.frameStyle.buttonHover},{"Button pressed","buttonPressed",&s.frameStyle.buttonPressed},
        {"Close hover","closeButtonHover",&s.frameStyle.closeButtonHover},{"Close pressed","closeButtonPressed",&s.frameStyle.closeButtonPressed}};
    auto& m=s.frameStyle.metrics;
    auto& f=s.frameStyle.features;
    if(BeginGrid(s,"frame-grid")) {
        const bool presetCard=BeginSection(s,"Preset","Selecting a preset does not overwrite edited values");
        if(presetCard) {
            if(Combo("Preset",&preset,presets,4)) SelectFramePreset(s,static_cast<WindowFramePreset>(preset));
            TextWrapped("Native detaches the OS adapter; other presets use the public drawing API.");
            if(Button("Regenerate colors from selected Theme")) RegenerateFrameColors(s);
            if(Button("Reset selected preset")) ResetFramePreset(s);
        }
        EndSection(presetCard);
        const bool featureCard=BeginSection(s,"Metrics and features","Sizes are in DIP");
        if(featureCard) {
            DragFloat("Height",&m.height,.25f,0,96,"%.2f");
            DragFloat("Icon area width",&m.iconAreaWidth,.25f,0,160,"%.2f");
            DragFloat("Title left padding",&m.titlePaddingLeft,.25f,0,80,"%.2f");
            DragFloat("Title right padding",&m.titlePaddingRight,.25f,0,80,"%.2f");
            DragFloat("Button width",&m.buttonWidth,.25f,0,160,"%.2f");
            DragFloat("Border width",&m.borderWidth,.1f,0,8,"%.2f");
            Checkbox("Icon",&f.icon);SameLine();Checkbox("Application name",&f.applicationName);
            Checkbox("Project name",&f.projectName);SameLine();Checkbox("Unsaved indicator",&f.unsavedIndicator);
            Checkbox("Workspace switcher",&f.workspaceSwitcher);SameLine();Checkbox("Minimize",&f.minimize);
            Checkbox("Maximize / restore",&f.maximizeRestore);SameLine();Checkbox("Close",&f.close);
            Checkbox("Unsaved state",&s.frameUnsaved);
        }
        EndSection(featureCard);
        const bool colorCard=BeginSection(s,"Colors","Alpha is editable");
        if(colorCard) for(auto& entry:colors) ColorEdit4(entry.name,&entry.value->x,ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_AlphaBar);
        EndSection(colorCard);
        const bool codeCard=BeginSection(s,"Contrast and code","Validated against the active theme");
        if(codeCard) {
            const auto contrast=ValidateWindowFrameContrast(s.frameStyle);
            PushStyleColor(ImGuiCol_Text,contrast.valid?s.theme.colors.success:s.theme.colors.warning);
            TextWrapped("%s (title %.2f, inactive %.2f, auxiliary %.2f, icon %.2f, buttons %.2f, close %.2f)",
                        contrast.valid?"PASS":"WARNING: one or more colors are below the threshold",
                        contrast.activeTitle,contrast.inactiveTitle,contrast.auxiliary,contrast.icon,contrast.button,contrast.closeButton);
            PopStyleColor();
            std::ostringstream code;
            code<<std::fixed<<std::setprecision(3);
            code<<"auto style = imkit::MakeWindowFrameStyle(imkit::WindowFramePreset::"<<presets[preset]<<", theme);\n";
            for(const auto& entry:colors) code<<"style."<<entry.field<<" = {"<<entry.value->x<<"f, "<<entry.value->y<<"f, "<<entry.value->z<<"f, "<<entry.value->w<<"f};\n";
            code<<"style.metrics = {"<<m.height<<"f, "<<m.iconAreaWidth<<"f, "<<m.titlePaddingLeft<<"f, "
                <<m.titlePaddingRight<<"f, "<<m.buttonWidth<<"f, "<<m.borderWidth<<"f};\n";
            code<<"style.features = {"<<f.icon<<", "<<f.applicationName<<", "<<f.projectName<<", "
                <<f.unsavedIndicator<<", "<<f.workspaceSwitcher<<", "<<f.minimize<<", "
                <<f.maximizeRestore<<", "<<f.close<<"};\n";
            const auto snippet=code.str();
            BeginChild("##frame-snippet",{0,150*s.scale},ImGuiChildFlags_Borders);TextUnformatted(snippet.c_str());EndChild();
            if(Button("Copy C++ snippet")) SetClipboardText(snippet.c_str());
        }
        EndSection(codeCard);
        EndGrid();
    }
}
} // namespace
void DrawComparison(GalleryState &s) {
    if(!s.comparison.open) return;
    const char* modes[]={"Theme only", "Components"};
    Segmented("comparison-mode", &s.comparison.mode, modes);
    Record(s,"comparison-mode");
    SameLine(); Checkbox("Disabled", &s.comparison.disabled);
    Record(s,"comparison-disabled");
    TextWrapped(s.comparison.mode==0
        ? "One theme. The same ImGui:: calls. Both sides edit the same application-owned values."
        : "Familiar immediate-mode controls plus optional ImKit components. Both sides edit the same values.");
    TextDisabled("Same host font and scale. Default colors AND metrics restored; style is scoped to each specimen.");
    const bool narrow=GetContentRegionAvail().x<720.f;
    if(BeginTable("comparison-columns",narrow?1:2,ImGuiTableFlags_SizingStretchSame)) {
        for(int i=0;i<2;++i) {
            TableNextColumn(); PushID(i);
            if(i==0) {
                DefaultStyleScope style;
                ImGui::GetStyle().ScaleAllSizes(s.scale / ImGui::GetIO().DisplayFramebufferScale.x);
                if(BeginChild("specimen",{0,330*s.scale},ImGuiChildFlags_Borders)) DefaultComparisonSpecimen(s);
                EndChild();
            } else {
                ThemeScope style(s.theme,s.scale);
                PushStyleColor(ImGuiCol_ChildBg,s.theme.colors.surface);
                if(BeginChild("specimen",{0,330*s.scale},ImGuiChildFlags_Borders)) {
                    if(s.comparison.mode==0) DefaultComparisonSpecimen(s,true);
                    else ImKitComparisonSpecimen(s);
                }
                EndChild(); PopStyleColor();
            }
            PopID();
        }
        EndTable();
    }
    SeparatorText("Try the integration");
    TextUnformatted("auto theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionLight);\nimkit::ApplyTheme(theme, 1.0f); // before ImGui::NewFrame()\n// Keep drawing your existing ImGui:: widgets.");
    TextWrapped("Guide: docs/getting-started/getting-started.md | Components: docs/components/components.md");
}
void Show(GalleryState &s) {
    s.probes.clear();
    s.theme.fonts = s.fonts;
    s.animation.Prune(GetFrameCount());
    ThemeScope scope(s.theme, s.scale);
    ImGuiWindowFlags windowFlags=ImGuiWindowFlags_NoSavedSettings;
    if(s.floatingComparison) {
        constexpr float margin=24.f;
        const auto display=ImGui::GetIO().DisplaySize;
        const float top=s.windowFrameHeight+margin;
        const bool sideBySide=display.x>=1200.f;
        const float width=sideBySide
            ? std::clamp(display.x-622.f,560.f,1120.f)
            : std::max(320.f,display.x-margin*2.f);
        const float height=std::max(320.f,display.y-top-margin);
        SetNextWindowPos({margin,top},ImGuiCond_FirstUseEver);
        SetNextWindowSize({width,height},ImGuiCond_FirstUseEver);
    } else {
        SetNextWindowPos({0, s.windowFrameHeight});
        SetNextWindowSize({ImGui::GetIO().DisplaySize.x,
                          std::max(1.f,ImGui::GetIO().DisplaySize.y-s.windowFrameHeight)});
        windowFlags|=ImGuiWindowFlags_NoDecoration;
    }
    if(s.page==20) windowFlags|=ImGuiWindowFlags_NoBringToFrontOnFocus;
    const bool visible=Begin("ImKit Gallery - Precision Layers",nullptr,windowFlags);
    if(s.floatingComparison) {
        s.floatingCatalogPosition=GetWindowPos();
        s.floatingCatalogSize=GetWindowSize();
    }
    if(visible) {
        // One table drives the sidebar, the narrow-width combo and the category groups.
        struct PageEntry {int id; const char* name; int group;};
        static constexpr PageEntry table[]={
            {19,"Start",0},{0,"Components: Basic",1},{1,"Numeric / Units",1},{2,"Input / Media",1},{3,"Hierarchy / Table",1},
            {4,"Overlay / Layout",1},{5,"Composites",1},{6,"Icons",1},{23,"Charts",1},{24,"PTZ Camera",1},
            {7,"Editor Core",2},{8,"Video",2},{9,"CG",2},{10,"Foundations",3},{11,"Components",3},{12,"Patterns",3},
            {13,"Accessibility",3},{14,"Responsive",3},{15,"Generic Workspace",2},{16,"Feedback / States",1},
            {17,"Preview Tiles",2},{18,"Frame Lab",3},{20,"Toasts",1},{21,"Compare",0},{22,"New in 3.2",0},
            {25,"Show: Controls",4},{26,"Show: Live",4},{27,"Show: Rules",4},{28,"Show: Schedule",4},{29,"Show: Beat",4},{30,"Show: Script",4}};
        constexpr int pageCount=static_cast<int>(std::size(table));
        int pageIds[pageCount]; const char* pages[pageCount];
        for(int i=0;i<pageCount;++i) {pageIds[i]=table[i].id; pages[i]=table[i].name;}
        int pageIndex=0;
        for(int i=0;i<pageCount;++i) if(pageIds[i]==s.page) {pageIndex=i;break;}
        const bool wide=GetContentRegionAvail().x>=1100.f*std::max(1.f,ImGui::GetStyle().FontScaleDpi);
        PushFont(s.fonts.emphasis,s.theme.metrics.bodySize*1.1f); TextUnformatted("ImKit Gallery"); PopFont();
        SameLine(); TextDisabled("3.3   |   %d themes   |   %d icons",static_cast<int>(ThemePresets().size()),static_cast<int>(GetIconCatalog().size()));
        if(!wide) {
            SetNextItemWidth(std::min(260.f,GetContentRegionAvail().x*.45f));
            if(Combo("##section",&pageIndex,pages,pageCount)) s.page=pageIds[pageIndex];
            Record(s,"gallery-section");
            SameLine();
        }
        if(Button("Appearance")) OpenPopup("appearance");
        Record(s,"appearance");
        if(GetContentRegionAvail().x>280.f) SameLine();
        if(Button("Compare")) {s.page=21;s.comparison.open=true;}
        Record(s,"comparison-open");
        static NodeLauncher nodeLauncher;
        if(GetContentRegionAvail().x>160.f) SameLine();
        if(Button("Node Editor")) nodeLauncher.Open();
        Record(s,"node-editor-open");
        if(nodeLauncher.error) TextWrapped("Cannot open Node Editor (Windows error %lu). Place imkit_node_editor_gallery.exe beside this Gallery.", nodeLauncher.error);
        if(s.floatingComparison) {SameLine();Checkbox("Dear ImGui Demo",&s.showDearImGuiDemo);}
        if(BeginPopup("appearance")) {
            const bool themePickerOpen=BeginCombo("Theme",ThemePresets()[s.presetIndex].displayName.data());
            Record(s,"appearance-theme-picker");
            if(themePickerOpen) {
                for(int i=0;i<static_cast<int>(ThemePresets().size());++i) {
                    if(Selectable(ThemePresets()[i].displayName.data(),s.presetIndex==i)) {
                        s.presetIndex=i;
                        s.theme=MakeTheme(ThemePresets()[i].preset);
                        s.theme.fonts=s.fonts;
                        s.dark=s.theme.scheme==ColorScheme::Dark;
                        s.design.contrast=static_cast<int>(s.theme.contrast);
                        s.design.density=static_cast<int>(s.theme.density);
                    }
                    Record(s,(std::string("appearance-theme-")+std::to_string(i)).c_str());
                }
                EndCombo();
            }
            if(Checkbox("Dark",&s.dark)) { auto fonts=s.theme.fonts; s.theme=MakeTheme(s.dark?ColorScheme::Dark:ColorScheme::Light,s.theme.contrast,s.theme.density); s.theme.fonts=fonts; }
            const char* densities[]={"Compact","Comfortable","Touch"};
            if(Combo("Density",&s.design.density,densities,3)) SetDensity(s.theme,static_cast<Density>(s.design.density));
            const char* contrasts[]={"Standard","High contrast"};
            if(Combo("Contrast",&s.design.contrast,contrasts,2)) { auto fonts=s.theme.fonts; s.theme=MakeTheme(s.theme.scheme,static_cast<ContrastMode>(s.design.contrast),s.theme.density); s.theme.fonts=fonts; }
            Checkbox("Reduced motion",&s.theme.motion.reducedMotion);
            SliderFloat("Scale",&s.scale,ThemeScaleMinimum,ThemeScaleMaximum,"%.2f",ImGuiSliderFlags_AlwaysClamp);
            EndPopup();
        }
        Spacing();
        if(wide) {
            BeginChild("categories",{192*s.scale*std::max(1.f,ImGui::GetStyle().FontScaleDpi),0},ImGuiChildFlags_Borders);
            const char* groups[]={"Get started","Controls","Workspaces","Reference","Show"};
            for(int g=0;g<5;++g) {
                if(g) Spacing();
                TextDisabled("%s",groups[g]);
                for(int i=0;i<pageCount;++i) {
                    const int id=pageIds[i];
                    if(table[i].group!=g) continue;
                    if(Selectable(pages[i],s.page==id)) {s.page=id;if(id==21)s.comparison.open=true;}
                }
            }
            EndChild(); SameLine();
        }
        BeginChild("Component panel", {GetContentRegionAvail().x, 0}, ImGuiChildFlags_AlwaysUseWindowPadding,
                   s.page == 4 ? ImGuiWindowFlags_MenuBar : 0);
        int active=0;
        for(int i=0;i<pageCount;++i) if(pageIds[i]==s.page) active=i;
        if(s.page!=19) PageHeader(s,pages[active],PageSummary(s.page),s.page>=7&&s.page<=9);
        PushItemWidth(std::min(420 * s.scale,GetContentRegionAvail().x*.70f));
        switch (s.page) {
        case 21: s.comparison.open=true; DrawComparison(s); break;
        case 23: s.charts.Show(s); break;
        case 24: s.ptz.Show(s); break;
        case 25: case 26: case 27: case 28: case 29: case 30: s.show.Show(s.page,s); break;
        case 22: s.workflow.Workbench(s); break;
        case 20: s.toasts.Show(s); break;
        case 19: Start(s); break;
        case 18: FrameLab(s); break;
        case 15: case 16: case 17: s.workflow.Show(s.page,s); break;
        case 10: case 11: case 12: case 13: case 14:
            s.design.Show(s.page,s); break;
        case 0:
            Basic(s);
            break;
        case 1:
            Numeric(s);
            break;
        case 2:
            InputColor(s);
            break;
        case 3:
            Hierarchy(s);
            break;
        case 4:
            Overlay(s);
            break;
        case 5:
            Composites(s);
            break;
        case 6:
            Icons(s);
            break;
        case 7:
            s.editors.icons = &s.icons;
            CoreWorkspace(s.editors, s.theme);
            break;
        case 8:
            s.editors.icons = &s.icons;
            VideoWorkspace(s.editors, s.theme, s.texture);
            break;
        case 9:
            s.editors.icons = &s.icons;
            CGWorkspace(s.editors, s.theme, s.texture);
            break;
        }
        PopItemWidth();
        EndChild();
    }
    End();
    if (s.palette) {
        SetNextWindowSize({380, 620}, ImGuiCond_FirstUseEver);
        if (Begin("Theme palette", &s.palette)) {
            ImVec4 accent = s.theme.colors.accent;
            if (ColorEdit4("Accent", &accent.x))
                SetAccent(s.theme, accent);
            struct Entry {
                const char *name;
                ImVec4 *color;
            };
            auto &c = s.theme.colors;
            Entry entries[] = {{"Canvas", &c.canvas},
                               {"Surface", &c.surface},
                               {"Input", &c.input},
                               {"Raised", &c.raised},
                               {"Text", &c.text},
                               {"Muted", &c.muted},
                               {"Border", &c.border},
                               {"On accent", &c.onAccent},
                               {"Selection", &c.selection},
                               {"Focus", &c.focus},
                               {"Destructive", &c.destructive},
                               {"On destructive", &c.onDestructive},
                               {"Success", &c.success},
                               {"Warning", &c.warning}};
            for (auto &e : entries)
                ColorEdit4(e.name, &e.color->x);
            TextWrapped(
                "Theme is a host-owned value. Copy it to save a session preset, then apply it explicitly.");
        }
        End();
    }
}
void ShowComparison(GalleryState &) {
    // Comparison is rendered once inside the Gallery page, never over another UI.
}
} // namespace imkit::gallery
