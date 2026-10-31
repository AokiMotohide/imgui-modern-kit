#include "look_pages.h"
#include "gallery.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

namespace imkit::gallery {
namespace {
std::string Color(ImVec4 c) {
    const auto byte = [](float v) { return static_cast<unsigned>(std::lround(std::clamp(v, 0.f, 1.f) * 255.f)); };
    char buffer[16];
    if (byte(c.w) == 255)
        std::snprintf(buffer, sizeof buffer, "#%02x%02x%02x", byte(c.x), byte(c.y), byte(c.z));
    else
        std::snprintf(buffer, sizeof buffer, "#%02x%02x%02x%02x", byte(c.x), byte(c.y), byte(c.z), byte(c.w));
    return buffer;
}
ImVec4 MixColors(ImVec4 a, ImVec4 b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}
// A titled card of the Gallery grid. BeginSection must always be paired with EndSection, visible or not.
struct Section {
    bool visible;
    Section(GalleryState& state, const char* title, const char* hint = nullptr) : visible(BeginSection(state, title, hint)) {}
    ~Section() { EndSection(visible); }
    Section(const Section&) = delete;
    Section& operator=(const Section&) = delete;
    explicit operator bool() const { return visible; }
};
constexpr CornerStyle kCorners[] = {CornerStyle::Sharp, CornerStyle::Soft, CornerStyle::Round, CornerStyle::Pill};
constexpr const char* kCornerNames[] = {"sharp", "soft", "round", "pill"};
constexpr const char* kDensityNames[] = {"compact", "comfortable", "touch"};

// Where the sample worlds live: examples/gallery/worlds, found by walking up from the executable and the working folder.
std::filesystem::path WorldsDirectory() {
    std::vector<std::filesystem::path> starts{std::filesystem::current_path()};
#ifdef _WIN32
    wchar_t module[MAX_PATH]{};
    if (GetModuleFileNameW(nullptr, module, MAX_PATH) > 0)
        starts.push_back(std::filesystem::path(module).parent_path());
#endif
    for (auto folder : starts)
        for (int depth = 0; depth < 8 && !folder.empty(); ++depth, folder = folder.parent_path()) {
            const auto candidate = folder / "examples" / "gallery" / "worlds";
            if (std::filesystem::is_directory(candidate))
                return candidate;
            if (folder == folder.parent_path())
                break;
        }
    return {};
}
} // namespace

void LookPages::FindWorlds() {
    worldsSearched = true;
    worlds.clear();
    const auto directory = WorldsDirectory();
    if (directory.empty())
        return;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
        if (entry.is_regular_file() && entry.path().extension() == ".theme")
            worlds.push_back(entry.path());
    std::sort(worlds.begin(), worlds.end());
}

void LookPages::SyncFromTheme(const Theme& theme) {
    const auto& c = theme.semantic;
    brand.canvas = c.canvas;
    brand.surface = c.surface;
    brand.text = c.text;
    brand.accent = c.accent;
    brand.border = c.border;
    deriveBorder = false;
    corner = 4;
    for (int i = 0; i < 4; ++i) {
        Theme probe;
        SetCornerStyle(probe, kCorners[i]);
        if (probe.radius.subtle == theme.radius.subtle && probe.radius.control == theme.radius.control &&
            probe.radius.overlay == theme.radius.overlay)
            corner = i;
    }
    border = theme.stroke.border;
    hard = theme.elevation.hard;
    shadowOffset[0] = theme.elevation.offset.x;
    shadowOffset[1] = theme.elevation.offset.y;
    if (c.shadow.w > 0)
        shadow = c.shadow;
    density = static_cast<int>(theme.density);
}

// The compact theme text for the current controls: the same text a theme file would hold.
std::string LookPages::Compose() const {
    std::string t;
    const auto add = [&t](const char* key, ImVec4 value) { t += std::string(key) + " = " + Color(value) + "\n"; };
    add("brand.canvas", brand.canvas);
    add("brand.surface", brand.surface);
    add("brand.text", brand.text);
    add("brand.accent", brand.accent);
    if (!deriveBorder)
        add("brand.border", brand.border);
    char line[96];
    if (corner < 4)
        t += std::string("corner = ") + kCornerNames[corner] + "\n";
    std::snprintf(line, sizeof line, "stroke.border = %g\n", border);
    t += line;
    t += hard ? "elevation.hard = true\n" : "elevation.hard = false\n";
    if (hard) {
        std::snprintf(line, sizeof line, "elevation.offset = %g %g\n", shadowOffset[0], shadowOffset[1]);
        t += line;
        add("semantic.shadow", shadow);
    }
    t += std::string("density = ") + kDensityNames[std::clamp(density, 0, 2)] + "\n";
    return t;
}

void LookPages::Apply(GalleryState& s) {
    text = Compose();
    warnings = ParseTheme(text, s.theme).warnings;
    s.dark = s.theme.scheme == ColorScheme::Dark;
    s.design.density = density;
}

bool LookPages::LoadFile(GalleryState& s, const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        fileMessage = "Cannot read " + path.string();
        return false;
    }
    std::stringstream content;
    content << in.rdbuf();
    // A clean base, so what the file does not name is not left over from earlier experiments.
    Theme base = MakeTheme(ThemePreset::PrecisionLight);
    base.fonts = s.theme.fonts;
    base.skin = s.theme.skin;
    const auto result = ParseTheme(content.str(), base);
    s.theme = base;
    s.dark = s.theme.scheme == ColorScheme::Dark;
    s.design.density = static_cast<int>(s.theme.density);
    warnings = result.warnings;
    fileMessage = path.filename().string() + ": " + std::to_string(result.applied) + " applied, " + std::to_string(result.ignored) + " skipped";
    std::snprintf(file, sizeof file, "%s", path.string().c_str());
    std::error_code error;
    fileTime = std::filesystem::last_write_time(path, error);
    SyncFromTheme(s.theme);
    text = Compose();
    synced = true;
    return true;
}

void LookPages::Poll(GalleryState& s) {
    if (!watch || !file[0])
        return;
    const double now = ImGui::GetTime();
    if (now - lastPoll < .5)
        return;
    lastPoll = now;
    std::error_code error;
    const auto time = std::filesystem::last_write_time(file, error);
    if (!error && time != fileTime)
        LoadFile(s, file);
}

namespace {
// A strip of components so a change is visible without leaving the page. Every component receives the theme.
void Preview(GalleryState& s) {
    const ComponentOptions options{&s.theme};
    ActionButton("Primary", ActionVariant::Primary, {}, options);
    ImGui::SameLine();
    ActionButton("Secondary", ActionVariant::Secondary, {}, options);
    ImGui::SameLine();
    ActionButton("Ghost", ActionVariant::Ghost, {}, options);
    ImGui::SameLine();
    ActionButton("Destructive", ActionVariant::Destructive, {}, options);
    ImGui::Spacing();
    StatusBadge("Ready", StatusKind::Success, options);
    ImGui::SameLine();
    StatusBadge("Check", StatusKind::Warning, options);
    ImGui::SameLine();
    StatusBadge("Failed", StatusKind::Error, options);
    ImGui::SameLine();
    StatusBadge("Idle", StatusKind::Neutral, options);
    ImGui::Spacing();
    static bool toggle = true;
    static int segment = 1;
    static float value = .4f;
    static char name[64] = "Display 1";
    Toggle("Enabled", &toggle, options);
    const char* labels[] = {"Day", "Week", "Month"};
    Segmented("range", &segment, labels, options);
    ImGui::SetNextItemWidth(220.f);
    ImGui::SliderFloat("Level", &value, 0.f, 1.f);
    ImGui::SetNextItemWidth(220.f);
    ImGui::InputText("Name", name, sizeof name);
    static const Notification note{"preview-note", "Saved to the project.", StatusKind::Success, 0};
    NotificationCard(note, 0, options);
    if (ImGui::BeginChild("preview-card", {0, ImGui::GetFrameHeight() * 2.2f}, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar)) {
        ImGui::TextUnformatted("A bordered panel");
        ImGui::TextDisabled("Secondary text on the surface");
    }
    ImGui::EndChild();
}
} // namespace

void LookPages::Studio(GalleryState& s) {
    using namespace ImGui;
    if (!worldsSearched)
        FindWorlds();
    if (!synced) {
        SyncFromTheme(s.theme);
        text = Compose();
        synced = true;
    }
    TextWrapped("Edit the colours and shape below: the whole Gallery follows, and the text at the bottom is what a theme file "
                "would hold. A theme file is applied with ParseTheme; ImKit itself reads no files.");
    if (!BeginGrid(s, "look-studio", 2))
        return;
    if (const Section section{s, "Start from", "A preset, or a sample world file from examples/gallery/worlds."}) {
        const auto presets = ThemePresets();
        SetNextItemWidth(220.f);
        const bool presetOpen = BeginCombo("Preset", presets[static_cast<std::size_t>(preset)].displayName.data());
        Record(s, "look-preset");
        if (presetOpen) {
            for (int i = 0; i < static_cast<int>(presets.size()); ++i)
                if (Selectable(presets[static_cast<std::size_t>(i)].displayName.data(), preset == i)) {
                    preset = i;
                    const FontSet fonts = s.theme.fonts;
                    s.theme = MakeTheme(presets[static_cast<std::size_t>(i)].preset);
                    s.theme.fonts = fonts;
                    s.dark = s.theme.scheme == ColorScheme::Dark;
                    s.design.density = static_cast<int>(s.theme.density);
                    SyncFromTheme(s.theme);
                    text = Compose();
                }
            EndCombo();
        }
        if (worlds.empty())
            TextDisabled("No worlds folder found next to this build.");
        for (const auto& world : worlds) {
            if (Button(world.stem().string().c_str()))
                LoadFile(s, world);
            Record(s, ("look-world-" + world.stem().string()).c_str());
            SameLine();
        }
        NewLine();
        TextDisabled("Theme file");
        SetNextItemWidth(-FLT_MIN);
        InputText("##theme-file", file, sizeof file);
        if (Button("Load") && file[0])
            LoadFile(s, file);
        SameLine();
        Checkbox("Watch", &watch);
        if (IsItemHovered())
            SetTooltip("Reload when the file changes on disk: edit it in any editor and watch this window.");
        if (!fileMessage.empty())
            TextDisabled("%s", fileMessage.c_str());
    }

    bool changed = false;
    if (const Section section{s, "Colours", "Four colours make a complete theme; the rest is derived."}) {
        changed |= ColorEdit3("Canvas", &brand.canvas.x, ImGuiColorEditFlags_NoInputs);
        SameLine();
        changed |= ColorEdit3("Surface", &brand.surface.x, ImGuiColorEditFlags_NoInputs);
        SameLine();
        changed |= ColorEdit3("Text", &brand.text.x, ImGuiColorEditFlags_NoInputs);
        SameLine();
        changed |= ColorEdit3("Accent", &brand.accent.x, ImGuiColorEditFlags_NoInputs);
        changed |= Checkbox("Derive the border colour", &deriveBorder);
        if (!deriveBorder) {
            SameLine();
            changed |= ColorEdit3("Border", &brand.border.x, ImGuiColorEditFlags_NoInputs);
        }
    }
    if (const Section section{s, "Shape, outline and shadow"}) {
        static const char* names[] = {"Sharp", "Soft", "Round", "Pill", "Custom"};
        SetNextItemWidth(160.f);
        changed |= Combo("Corners", &corner, names, corner == 4 ? 5 : 4);
        Record(s, "look-corners");
        SetNextItemWidth(220.f);
        changed |= SliderFloat("Outline", &border, 0.f, 6.f, "%.1f px", ImGuiSliderFlags_AlwaysClamp);
        if (IsItemHovered())
            SetTooltip("%.0f px or more is an outline: full-strength colour on windows, frames and every button.", OutlineMinimumWidth);
        changed |= Checkbox("Hard shadow", &hard);
        Record(s, "look-hard-shadow");
        if (hard) {
            SameLine();
            SetNextItemWidth(220.f);
            changed |= SliderFloat2("Offset", shadowOffset, -8.f, 8.f, "%.1f");
            SameLine();
            changed |= ColorEdit4("Shadow", &shadow.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        }
        static const char* densities[] = {"Compact", "Comfortable", "Touch"};
        SetNextItemWidth(160.f);
        changed |= Combo("Density", &density, densities, 3);
    }
    if (changed) {
        // The corner style combo has a fifth entry only while the radii are custom; picking one makes it a style.
        Apply(s);
    }

    if (const Section section{s, "Preview", "These components receive the theme, so the hard shadow shows."}) {
        Preview(s);
    }
    if (const Section section{s, "Theme text", "Save it as a .theme file, or paste it into ParseTheme."}) {
        if (text.empty())
            text = Compose();
        if (ValidateContrast(s.theme))
            TextDisabled("Contrast: text, border and focus meet the minimum on every surface.");
        else
            TextColored(s.theme.semantic.warning, "Contrast warning: some text or border colours fall below the minimum. The theme still applies.");
        if (!warnings.empty())
            TextColored(s.theme.semantic.warning, "%s", warnings.c_str());
        InputTextMultiline("##theme-text", text.data(), text.size() + 1, {-FLT_MIN, GetTextLineHeight() * 11.f}, ImGuiInputTextFlags_ReadOnly);
        if (Button("Copy"))
            SetClipboardText(text.c_str());
        SameLine();
        if (Button("Copy every token")) {
            const std::string full = SerializeTheme(s.theme);
            SetClipboardText(full.c_str());
        }
    }
    EndGrid();
}

namespace {
// A procedural skin: thick outline, a glossy highlight, state-dependent fill. The library draws the hard shadow of the theme
// behind the frame before this runs, and the label, focus ring and input stay with ImKit.
bool StickerSkin(const SkinFrame& f, void*) {
    if (!f.theme)
        return false;
    const auto& c = f.theme->semantic;
    ImVec4 fill;
    if (f.part == SkinPart::Button) {
        if (f.variant == ActionVariant::Ghost || f.variant == ActionVariant::SubtleDestructive)
            return false; // Those variants keep their default look.
        fill = f.variant == ActionVariant::Primary ? c.accent : f.variant == ActionVariant::Destructive ? c.error : c.surfaceRaised;
    } else if (f.part == SkinPart::Badge) {
        fill = f.status == StatusKind::Success ? c.success : f.status == StatusKind::Warning ? c.warning
             : f.status == StatusKind::Error   ? c.error : c.surfaceRaised;
        fill = MixColors(c.surfaceRaised, fill, .35f);
    } else {
        return false;
    }
    if (f.hovered)
        fill = MixColors(fill, ImVec4(1, 1, 1, 1), .14f * f.hover + .06f);
    if (f.active)
        fill = MixColors(fill, ImVec4(0, 0, 0, 1), .16f);
    const float scale = ImGui::GetStyle().FontScaleMain, line = 2.5f * scale;
    f.draw->AddRectFilled(f.min, f.max, ImGui::GetColorU32(fill), f.rounding);
    const float inset = line + 1.5f * scale, height = (f.max.y - f.min.y) * .42f;
    f.draw->AddRectFilled({f.min.x + inset, f.min.y + inset}, {f.max.x - inset, f.min.y + inset + height},
                          ImGui::GetColorU32(ImVec4(1, 1, 1, .22f)), std::max(0.f, f.rounding - inset), ImDrawFlags_RoundCornersTop);
    f.draw->AddRect(f.min, f.max, ImGui::GetColorU32(c.border), f.rounding, ImDrawFlags_None, line);
    return true;
}
const char* const kPatternNames[] = {"Dots", "Stripes", "Rays"};
} // namespace

void LookPages::Skins(GalleryState& s) {
    using namespace ImGui;
    static Skin skin{&StickerSkin, nullptr};
    TextWrapped("A Skin replaces the frame and background of ImKit components; the label, icon, focus ring, accessibility and input "
                "stay with the library. It reaches components that receive the theme.");
    if (!BeginGrid(s, "look-skins", 1))
        return;
    if (const Section section{s, "Sprite sheet and progress thumb", "Sprite plays a sheet of frames; ProgressTrackView::thumb draws on the leading edge of a fill."}) {
        if (s.spriteSheet.GetTexID() == ImTextureID{}) {
            TextDisabled("This build has no sample sheet.");
        } else {
            SpriteSheet sheet;
            sheet.texture = s.spriteSheet;
            sheet.sheetPixels = {256.f, 192.f};
            sheet.cellPixels = {64.f, 64.f};
            sheet.columns = 4;
            sheet.frames = 12;
            sheet.fps = spriteFps;
            sheet.playback = static_cast<SpritePlayback>(spritePlayback);
            sheet.restFrame = 0;
            static const char* const kPlayback[] = {"Loop", "Once", "HoldLast"};
            SetNextItemWidth(140.f);
            Combo("Playback", &spritePlayback, kPlayback, 3);
            SameLine();
            SetNextItemWidth(160.f);
            SliderFloat("fps", &spriteFps, 2.f, 30.f, "%.0f");
            SameLine();
            if (Button("Restart"))
                sprite.Restart();
            SameLine();
            Checkbox("Animate (off = rest frame)", &spriteAnimate);
            const bool animate = spriteAnimate && s.theme.motion.enabled && !s.theme.motion.reducedMotion;
            const double now = GetTime();
            const ImVec2 sizes[] = {{32.f, 32.f}, {64.f, 64.f}, {128.f, 128.f}};
            for (const ImVec2 size : sizes) {
                Sprite(size.x < 40.f ? "sprite-s" : size.x < 100.f ? "sprite-m" : "sprite-l", sheet, sprite, now, size, animate);
                SameLine(0, 24.f);
            }
            NewLine();
            Checkbox("Advance the bar", &progressAuto);
            if (progressAuto)
                progressValue = static_cast<float>(std::fmod(now * .12, 1.0));
            else
                SliderFloat("Value", &progressValue, 0.f, 1.f, "%.2f");
            struct Rider {
                const SpriteSheet* sheet;
                double now;
            } rider{&sheet, now};
            const float height = 44.f;
            ProgressTrackView view;
            view.fraction = progressValue;
            view.label = "Determinate: the picture rides the fill";
            view.thumb = [](const ProgressThumbInfo& info, void* user) {
                const auto* r = static_cast<const Rider*>(user);
                const float size = 44.f;
                const ImVec2 minimum{info.head.x - size * .5f, info.trackMin.y - size + 6.f};
                DrawSprite(info.draw, *r->sheet, SpriteFrameAt(*r->sheet, r->now, info.animate), minimum,
                           {minimum.x + size, minimum.y + size});
            };
            view.thumbUser = &rider;
            view.thumbHeight = height;
            ProgressTrack("sprite-progress", view, {&s.theme});
            ProgressTrackView idle = view;
            idle.fraction = -1.f;
            idle.label = "Indeterminate: the picture follows the moving segment";
            ProgressTrack("sprite-idle", idle, {&s.theme});
        }
    }
    if (const Section section{s, "The same components, three looks"}) {
        Theme plain = s.theme;
        plain.skin = nullptr;
        plain.elevation.hard = false;
        Theme shadowed = plain;
        shadowed.elevation.hard = true;
        if (shadowed.elevation.offset.x == 0.f && shadowed.elevation.offset.y == 0.f)
            shadowed.elevation.offset = {0.f, 3.f};
        Theme skinned = shadowed;
        skinned.skin = &skin;
        struct Column { const char* title; const Theme* theme; };
        const Column columns[] = {{"Default", &plain}, {"Hard shadow", &shadowed}, {"Hard shadow + skin", &skinned}};
        for (const auto& column : columns) {
            BeginGroup();
            TextDisabled("%s", column.title);
            const ComponentOptions options{column.theme};
            PushID(column.title);
            ActionButton("Primary", ActionVariant::Primary, {}, options);
            ActionButton("Secondary", ActionVariant::Secondary, {}, options);
            ActionButton("Destructive", ActionVariant::Destructive, {}, options);
            ActionButton("Ghost", ActionVariant::Ghost, {}, options);
            StatusBadge("Ready", StatusKind::Success, options);
            SameLine();
            StatusBadge("Check", StatusKind::Warning, options);
            PopID();
            EndGroup();
            if (column.theme != &skinned)
                SameLine(0, 36.f);
        }
    }
    if (const Section section{s, "Backdrop patterns", "DrawBackdropPattern: Dots, Stripes and Rays, clipped to a rectangle."}) {
        static int pattern = 0;
        static ImVec4 color{.45f, .3f, .15f, .35f};
        SetNextItemWidth(160.f);
        Combo("Pattern", &pattern, kPatternNames, 3);
        SameLine();
        ColorEdit4("Colour", &color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        SetNextItemWidth(180.f);
        SliderFloat("Spacing", &backdrop.spacing, 4.f, 40.f, "%.0f");
        SameLine();
        SetNextItemWidth(180.f);
        SliderFloat("Size", &backdrop.size, 1.f, 16.f, "%.0f");
        SetNextItemWidth(180.f);
        SliderAngle("Angle", &backdrop.angle, 0.f, 180.f);
        SameLine();
        SetNextItemWidth(180.f);
        SliderInt("Rays", &backdrop.rays, 4, 64);
        backdrop.pattern = static_cast<BackdropPattern>(pattern);
        backdrop.color = ColorConvertFloat4ToU32(color);
        const ImVec2 origin = GetCursorScreenPos();
        const float width = std::min(GetContentRegionAvail().x, 560.f), height = 180.f;
        auto* draw = GetWindowDrawList();
        draw->AddRectFilled(origin, {origin.x + width, origin.y + height}, GetColorU32(s.theme.semantic.surfaceRaised), s.theme.radius.overlay);
        DrawBackdropPattern(draw, origin, {origin.x + width, origin.y + height}, backdrop);
        draw->AddRect(origin, {origin.x + width, origin.y + height}, GetColorU32(s.theme.semantic.border), s.theme.radius.overlay, ImDrawFlags_None,
                      std::max(1.f, s.theme.stroke.border));
        Dummy({width, height});
    }
    if (const Section section{s, "Nine-slice picture", "DrawNineSlice keeps the corners of a picture at their size and stretches the rest."}) {
        const ImTextureRef picture = s.sampleImages[5];
        if (picture.GetTexID() == ImTextureID{}) {
            TextDisabled("This build has no sample picture.");
        } else {
            SetNextItemWidth(220.f);
            SliderFloat("Border (picture pixels)", &nineSliceBorder, 0.f, 48.f, "%.0f");
            const NineSliceBorder border{nineSliceBorder, nineSliceBorder, nineSliceBorder, nineSliceBorder};
            const ImVec2 sizes[] = {{70.f, 50.f}, {180.f, 60.f}, {300.f, 110.f}};
            auto* draw = GetWindowDrawList();
            for (const ImVec2 size : sizes) {
                const ImVec2 origin = GetCursorScreenPos();
                DrawNineSlice(draw, picture, origin, {origin.x + size.x, origin.y + size.y}, {96.f, 96.f}, border);
                Dummy(size);
                SameLine(0, 20.f);
            }
            NewLine();
        }
    }
    EndGrid();
}
} // namespace imkit::gallery
