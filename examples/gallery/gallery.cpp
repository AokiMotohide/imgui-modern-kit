#include "gallery.h"
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <string_view>
#include <vector>
namespace imkit::gallery {
void Record(GalleryState &s, const char *name) {
    s.probes[name] = {GetItemRectMin(), GetItemRectMax()};
}
namespace {
void Heading(GalleryState &s, const char *text) {
    PushFont(s.fonts.emphasis, s.theme.metrics.headingSize);
    TextUnformatted(text);
    PopFont();
    Separator();
}
void Icons(GalleryState &s) {
    Heading(s, "Icons / Generated outline glyphs");
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
    const char *sizes[] = {"16", "20", "24", "32", "48", "64"};
    SetNextItemWidth(CalcTextSize("64").x+GetFrameHeight()+GetStyle().FramePadding.x*2);
    Combo("Size", &s.iconSizeIndex, sizes, 6);
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
            PopID();
        }
        EndTable();
    }
}
void Basic(GalleryState &s) {
    Heading(s, "Actions / Selection");
    if (s.focusApply) {
        SetKeyboardFocusHere();
        s.focusApply = false;
    }
    if (ActionButton("Apply", ActionVariant::Primary, {}, {&s.theme, &s.animation}))
        ++s.clicks;
    Record(s, "apply");
    s.applyFocused = IsItemFocused();
    SameLine();
    ActionButton("Secondary", ActionVariant::Secondary);
    SameLine();
    ActionButton("Ghost", ActionVariant::Ghost);
    SameLine();
    ActionButton("Delete", ActionVariant::Destructive, {}, {&s.theme});
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
    Text("Applied: %d", s.clicks);
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
    Selectable("Selected row has an underline", &s.selected);
    Record(s, "selectable");
    static const char *items[] = {"Low", "Medium", "High"};
    Combo("Quality", &s.combo, items, 3);
    Record(s, "combo");
    if (BeginListBox("List", {0, 110})) {
        for (int i = 0; i < 3; ++i) {
            PushID(i);
            if (Selectable(items[i], s.combo == i))
                s.combo = i;
            PopID();
        }
        EndListBox();
    }
    SeparatorText("Text and links");
    TextUnformatted("Precision Layers / 表示設定");
    TextColored(s.theme.colors.success, "Ready");
    TextDisabled("Unavailable");
    TextWrapped("A neutral palette separates the canvas, panels, inputs and overlays. Standard Dear ImGui "
                "behavior is retained.");
    LabelText("Version", "%s", IMKIT_VERSION);
    BulletText("Keyboard focus and selection are different states.");
    TextLink("Local link action");
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
void Numeric(GalleryState &s) {
    Heading(s, "Numeric / Units");
    SetNextItemWidth(420);
    DragFloat("Value", &s.scalar, .01f, 0, 1);
    Record(s, "drag");
    SetNextItemWidth(420);
    SliderFloat("Level", &s.scalar, 0, 1);
    Record(s, "slider");
    DragFloatRange2("Range", &s.lower, &s.upper, 1, 0, 100);
    Record(s, "range");
    InputScalar("64-bit integer", ImGuiDataType_S64, &s.integer64);
    Record(s, "int64");
    InputDouble("Precision", &s.precise);
    InputFloat("Float", &s.scalar);
    InputInt("Count", &s.integers[0]);
    DragFloat2("Drag 2", s.vector);
    DragFloat3("Drag 3", s.vector);
    DragFloat4("Drag 4", s.vector);
    SliderInt4("Int 4", s.integers, 0, 100);
    InputInt3("Input 3", s.integers);
    SetNextItemWidth(300);
    InputScalarWithUnit("Distance", ImGuiDataType_Double, &s.precise, "mm");
    SetNextItemWidth(400);
    InputVector3WithUnit("Position", s.vector, "mm");
    SetNextItemWidth(300);
    DragFloatWithUnit("Angle", &s.vector[0], "deg", .1f, -180, 180);
    VSliderFloat("##vertical", {56, 90}, &s.scalar, 0, 1, "%.2f");
    SameLine();
    ProgressBar(s.scalar, {340, 28}, "Level");
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
    Heading(s, "Input / Color / Image / Plot");
    InputText("Name / 名前", s.name, sizeof(s.name), ImGuiInputTextFlags_CallbackEdit, TextCallback, &s);
    Record(s, "text");
    InputText("Resizable", s.growing.data(), s.growing.size(), ImGuiInputTextFlags_CallbackResize,
              ResizeCallback, &s);
    Record(s, "resize-text");
    InputTextWithHint("Required", "Enter a name", s.validation, sizeof(s.validation));
    Record(s, "validation");
    ValidationMessage("A name is required", s.validation[0] == 0, &s.theme);
    InputTextMultiline("Notes", s.notes, sizeof(s.notes), {460, 90});
    Record(s, "multiline");
    ColorEdit4("Tint", &s.color.x);
    Record(s, "color");
    SetNextItemWidth(180);
    ColorPicker4("Picker", &s.color.x, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs);
    Record(s, "picker");
    SameLine();
    BeginGroup();
    Image(s.texture, {180, 100});
    ImageWithBg(s.texture, {180, 48}, {0, 0}, {1, 1}, s.theme.colors.input, s.color);
    s.imageActivated = ImageButton("Texture action", s.texture, {40, 40}) || s.imageActivated;
    Record(s, "image-button");
    EndGroup();
    const float values[] = {.1f, .4f, .8f, .5f, .3f, .9f, .65f};
    PlotLines("Signal", values, 7, 0, nullptr, 0, 1, {420, 70});
    PlotHistogram("Histogram", values, 7, 0, nullptr, 0, 1, {420, 70});
}
void Hierarchy(GalleryState &s) {
    Heading(s, "Hierarchy / Tabs / Tables");
    bool open = TreeNode("Settings");
    Record(s, "tree");
    s.treeOpen = open;
    if (open) {
        TreeNodeEx("Display", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        TreePop();
    }
    if (CollapsingHeader("Quality options", ImGuiTreeNodeFlags_DefaultOpen))
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
    if (BeginTable("Items", 3,
                   ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable |
                       ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_ScrollY,
                   {620, 280})) {
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
    Text("Selected %d / inline actions %d", s.selectedRow + 1, s.inlineActions);
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
}
void Overlay(GalleryState &s) {
    Heading(s, "Overlay / Layout / Docking");
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
    if (BeginChild("Scroll", {500, 140}, ImGuiChildFlags_Borders)) {
        for (int i = 0; i < 12; ++i)
            Text("Scrollable content %02d", i);
    }
    EndChild();
    SeparatorText("Docking surface (host owns layout)");
    DockSpace(GetID("CatalogDock"), {620, 180}, ImGuiDockNodeFlags_None);
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
void Composites(GalleryState &s) {
    Heading(s, "Composite controls / 日本語");
    TextUnformatted("表示設定 — ひらがな・カタカナ・漢字・半角ｶﾅ。");
    InputText("名前 / Name", s.name, sizeof(s.name));
    Toggle("表示を有効にする / Enabled", &s.toggle, {&s.theme, &s.animation});
    static const char *labels[] = {"Low", "Medium", "High"};
    Segmented("segments", &s.segment, labels);
    Record(s, "segments");
    static const bool disabled[] = {false, true, false};
    SearchableCombo("Search quality", &s.combo, labels, s.search, sizeof(s.search), disabled);
    Record(s, "search-combo");
    StatusBadge("Ready / 準備完了", StatusKind::Success, &s.theme);
    SameLine();
    StatusBadge("Warning / 注意", StatusKind::Warning, &s.theme);
    SameLine();
    StatusBadge("Error / エラー", StatusKind::Error, &s.theme);
    if (s.notice &&
        NotificationCard({"notice", "Settings saved / 設定を保存しました", StatusKind::Success, 0}, GetTime(),
                         &s.theme))
        s.notice = false;
    InputScalarWithUnit("位置", ImGuiDataType_Double, &s.precise, "mm");
    ActionButton("適用 / Apply", ActionVariant::Primary, {}, {&s.theme, &s.animation});
    SameLine();
    BeginDisabled();
    ActionButton("削除できません", ActionVariant::Destructive, {}, {&s.theme});
    EndDisabled();
    TextWrapped("Values and notifications belong to the host. Font files are optional catalog assets. No "
                "operating-system font discovery or hidden context is used.");
}
struct PageInfo {
    int page;
    const char *group;
    const char *title;
    const char *keywords;
};
constexpr PageInfo Pages[] = {
    {-1,"Start","Home","start overview quick integration design"},
    {0,"Components","Actions & selection","button checkbox radio selectable action"},
    {1,"Components","Numeric & units","slider drag range scalar vector unit"},
    {2,"Components","Input, color & media","text input color image plot validation"},
    {5,"Components","Composite controls","switch segment search badge notification toolbar japanese"},
    {3,"Patterns","Hierarchy & data","tree table tabs sort drag drop"},
    {4,"Patterns","Overlay & layout","popup modal tooltip docking settings"},
    {10,"Themes & Icons","Theme presets","theme palette color accent light dark contrast"},
    {6,"Themes & Icons","Icon catalog","icon glyph atlas camera projector"},
    {7,"Editor Examples","Editor Core","ruler graph property asset animation"},
    {8,"Editor Examples","Video Editor","timeline video audio transition fade"},
    {9,"Editor Examples","3D Editor","viewport scene outliner gizmo cg"},
};
bool ContainsInsensitive(std::string_view text, std::string_view query) {
    if (query.empty()) return true;
    auto same=[](char a,char b) {
        return std::tolower(static_cast<unsigned char>(a))==std::tolower(static_cast<unsigned char>(b));
    };
    return std::search(text.begin(),text.end(),query.begin(),query.end(),same)!=text.end();
}
bool PageMatches(const PageInfo &page,const char *query) {
    return ContainsInsensitive(page.title,query)||ContainsInsensitive(page.group,query)||
           ContainsInsensitive(page.keywords,query);
}
void ApplyPreset(GalleryState &s,int index) {
    const auto presets=ThemePresets();
    index=std::clamp(index,0,static_cast<int>(presets.size())-1);
    const auto fonts=s.fonts;
    const bool motion=s.theme.motion.enabled;
    s.presetIndex=index;
    s.theme=MakeTheme(presets[index].preset);
    s.theme.fonts=fonts;
    s.theme.motion.enabled=motion;
    s.dark=s.theme.scheme==ColorScheme::Dark;
}
const char *PageCode(int page) {
    switch(page) {
    case 0:return "if (imkit::ActionButton(\"Apply\", imkit::ActionVariant::Primary, {}, {&theme})) {\n    SaveSettings();\n}";
    case 1:return "imkit::DragFloatWithUnit(\"Exposure\", &exposure, \"EV\", 0.01f, -8.0f, 8.0f);";
    case 2:return "imkit::InputTextWithHint(\"Name\", \"Display name\", name, sizeof(name));\nimkit::ValidationMessage(\"Required\", name[0] == 0, &theme);";
    case 3:return "if (imkit::BeginTable(\"Items\", 3, ImGuiTableFlags_RowBg)) {\n    // Host-owned rows and selection.\n    imkit::EndTable();\n}";
    case 4:return "if (imkit::BeginPopup(\"Actions\")) {\n    imkit::OverlayDecoration(theme);\n    imkit::EndPopup();\n}";
    case 5:return "imkit::Toggle(\"Enabled\", &enabled, {&theme, &animation});\nimkit::StatusBadge(\"Ready\", imkit::StatusKind::Success, &theme);";
    case 6:return "imkit::IconButton(icons, imkit::IconId::Settings, \"Settings\");";
    case 10:return "auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);\nimkit::SetAccent(theme, ImVec4(0.35f, 0.78f, 0.84f, 1.0f));\nimkit::ApplyTheme(theme);";
    default:return "// Editor data, selection and undo stay in the host.\n// ImKit renders non-owning views and emits typed edit requests.";
    }
}
void CodePanel(GalleryState &s,int page) {
    SeparatorText("Use it / コード例");
    BeginChild("Code sample",{0,88*s.scale},ImGuiChildFlags_Borders);
    PushStyleColor(ImGuiCol_Text,s.theme.colors.muted);
    TextUnformatted(PageCode(page));
    PopStyleColor();
    EndChild();
    if(Button("Copy code##page")) {
        SetClipboardText(PageCode(page));
        ++s.copyClicks;
    }
    Record(s,"copy-code");
    SameLine();TextDisabled("Host-owned state · native Dear ImGui behavior");
}
void PageIntro(GalleryState &s,int page) {
    const auto found=std::find_if(std::begin(Pages),std::end(Pages),[&](const auto &entry){return entry.page==page;});
    if(found!=std::end(Pages)) {
        PushFont(s.fonts.emphasis,26*s.scale);TextUnformatted(found->title);PopFont();
    }
    const char *purpose="Live public API specimen. Interact with every state, then copy the minimal host-side pattern.";
    if(page>=7&&page<=9) purpose="Advanced workspace example. Data, selection, undo and processing remain host-owned.";
    if(page==10) purpose="Choose a complete semantic preset, then customize the accent or individual palette roles.";
    TextWrapped("%s",purpose);
    if(page!=10) CodePanel(s,page);
    Spacing();Separator();Spacing();
}
void Home(GalleryState &s) {
    PushFont(s.fonts.emphasis,36*s.scale);
    TextUnformatted("Build focused tools, not another UI framework.");
    PopFont();
    TextColored(s.theme.colors.accent,"Precision Layers for Dear ImGui");
    TextWrapped("A compact C++20 extension library for modern native controls, semantic themes and reusable editor surfaces. Your application keeps its renderer, context, data and frame lifecycle.");
    Spacing();
    if(ActionButton("Explore components",ActionVariant::Primary,{160*s.scale,0},{&s.theme,&s.animation})) s.page=0;
    Record(s,"home-components");SameLine();
    if(Button("Browse 12 themes")) s.page=10;
    Record(s,"home-themes");SameLine();
    if(Button("Open editor examples")) s.page=8;
    Record(s,"home-editors");
    SeparatorText("30-second start");
    CodePanel(s,10);
    SeparatorText("Designed for real native tools");
    if(BeginTable("Home cards",3,ImGuiTableFlags_SizingStretchSame)) {
        const char *titles[]={"Native by default","Explicit ownership","From controls to editors"};
        const char *bodies[]={"Public Dear ImGui behavior, IDs, focus and input stay intact.",
                              "No hidden context, renderer, worker, font loader or persistence.",
                              "Use small components alone or compose data-heavy production surfaces."};
        for(int i=0;i<3;++i) {TableNextColumn();BeginChild(titles[i],{0,112*s.scale},ImGuiChildFlags_Borders);
            TextColored(s.theme.colors.accent,"%s",titles[i]);TextWrapped("%s",bodies[i]);EndChild();}
        EndTable();
    }
    SeparatorText("Theme preview");
    const auto presets=ThemePresets();
    for(int i=0;i<static_cast<int>(presets.size());++i) {
        if(i) SameLine();
        const auto preview=MakeTheme(presets[i].preset);
        PushID(i);if(ColorButton(presets[i].displayName.data(),preview.colors.accent,0,{28*s.scale,28*s.scale})) ApplyPreset(s,i);
        if(IsItemHovered()) SetTooltip("%s",presets[i].displayName.data());PopID();
    }
}
void Themes(GalleryState &s) {
    Heading(s,"Theme presets / 配色プリセット");
    TextWrapped("Every preset initializes component and editor semantic colors. Store the preset ID or a Theme copy in the host; ImKit never writes settings.");
    RadioButton("All",&s.presetFilter,0);SameLine();RadioButton("Light",&s.presetFilter,1);SameLine();RadioButton("Dark",&s.presetFilter,2);
    const auto presets=ThemePresets();
    if(BeginTable("Preset grid",3,ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_BordersInnerV)) {
        for(int i=0;i<static_cast<int>(presets.size());++i) {
            const auto &info=presets[i];
            if(s.presetFilter==1&&info.scheme!=ColorScheme::Light) continue;
            if(s.presetFilter==2&&info.scheme!=ColorScheme::Dark) continue;
            const auto preview=MakeTheme(info.preset);
            TableNextColumn();PushID(i);BeginGroup();
            ColorButton("canvas",preview.colors.canvas,ImGuiColorEditFlags_NoTooltip,{28*s.scale,28*s.scale});SameLine(0,3);
            ColorButton("surface",preview.colors.surface,ImGuiColorEditFlags_NoTooltip,{28*s.scale,28*s.scale});SameLine(0,3);
            ColorButton("accent",preview.colors.accent,ImGuiColorEditFlags_NoTooltip,{28*s.scale,28*s.scale});SameLine();
            if(Selectable(info.displayName.data(),s.presetIndex==i,0,{0,32*s.scale})) ApplyPreset(s,i);
            if(info.preset==ThemePreset::Ocean) Record(s,"preset-ocean");
            TextDisabled("%s · %s",info.id.data(),info.scheme==ColorScheme::Dark?"Dark":"Light");
            EndGroup();PopID();
        }
        EndTable();
    }
    SeparatorText("Customize the selected preset");
    ImVec4 accent=s.theme.colors.accent;
    if(ColorEdit4("Accent",&accent.x)) SetAccent(s.theme,accent);
    SameLine();if(Button("Reset preset")) ApplyPreset(s,s.presetIndex);
    SameLine();Checkbox("Edit every palette role",&s.palette);
    CodePanel(s,10);
}
void Navigation(GalleryState &s,bool compact) {
    if(compact) {
        SetNextItemWidth(260*s.scale);
        const auto current=std::find_if(std::begin(Pages),std::end(Pages),[&](auto &p){return p.page==s.page;});
        const char *preview=current==std::end(Pages)?"Home":current->title;
        if(BeginCombo("##page",preview)) {
            for(const auto &page:Pages) if(Selectable(page.title,s.page==page.page)) s.page=page.page;
            EndCombo();
        }
        Record(s,"compact-navigation");
        return;
    }
    BeginChild("Navigation",{232*s.scale,0},ImGuiChildFlags_Borders);
    const char *group=nullptr;
    for(const auto &page:Pages) {
        if(!PageMatches(page,s.gallerySearch)) continue;
        if(!group||std::strcmp(group,page.group)!=0) {group=page.group;SeparatorText(group);}
        if(Selectable(page.title,s.page==page.page)) s.page=page.page;
        if(page.page==0) Record(s,"nav-components");
        if(page.page==10) Record(s,"nav-themes");
    }
    EndChild();
}
} // namespace
void Show(GalleryState &s) {
    s.probes.clear();
    s.theme.fonts = s.fonts;
    s.animation.Prune(GetFrameCount());
    ThemeScope scope(s.theme, s.scale);
    SetNextWindowPos({0, 0});
    SetNextWindowSize(ImGui::GetIO().DisplaySize);
    Begin("Precision Layers catalog",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings);
    PushFont(s.fonts.emphasis,22*s.scale);TextUnformatted("ImKit · Precision Layers");PopFont();
    SameLine();TextDisabled("%s",IMKIT_VERSION);
    SameLine(0,24*s.scale);SetNextItemWidth(300*s.scale);
    InputTextWithHint("##gallery-search","Search components, APIs and use cases",s.gallerySearch,sizeof(s.gallerySearch));
    Record(s,"gallery-search");SameLine();
    if(Button("Appearance")) OpenPopup("gallery-appearance");
    if(BeginPopup("gallery-appearance")) {
        const auto presets=ThemePresets();
        if(BeginCombo("Preset",presets[s.presetIndex].displayName.data())) {
            for(int i=0;i<static_cast<int>(presets.size());++i)
                if(Selectable(presets[i].displayName.data(),i==s.presetIndex)) ApplyPreset(s,i);
            EndCombo();
        }
        Checkbox("Animation",&s.theme.motion.enabled);SetNextItemWidth(160);SliderFloat("Scale",&s.scale,1,1.5f,"%.2f");
        Checkbox("日本語",&s.editors.japanese);EndPopup();
    }
    Separator();
    const bool compact=GetContentRegionAvail().x<920*s.scale;
    if(compact) {Navigation(s,true);Separator();}
    else {Navigation(s,false);SameLine();}
    BeginChild("Content",{0,0},ImGuiChildFlags_Borders,s.page==4?ImGuiWindowFlags_MenuBar:0);
    PushItemWidth(420 * s.scale);
    if(s.page==-1) Home(s); else PageIntro(s,s.page);
    switch (s.page) {
    case -1: break;
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
    case 10:
        Themes(s);
        break;
    }
    PopItemWidth();
    EndChild();
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
} // namespace imkit::gallery
