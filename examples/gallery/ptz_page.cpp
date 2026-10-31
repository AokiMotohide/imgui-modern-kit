#include "ptz_page.h"
#include "gallery.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <map>
#include <string>

namespace imkit::gallery {
namespace {
constexpr float PanLimit = 170.f, TiltMin = -35.f, TiltMax = 90.f, ZoomMax = 20.f;

const char* Japanese(void* user, std::string_view key, const char* fallback) {
    if (!*static_cast<bool*>(user))
        return fallback;
    struct Entry { std::string_view key; const char* text; };
    static constexpr Entry table[] = {
        {"ptz.up", "上"}, {"ptz.upRight", "右上"}, {"ptz.right", "右"}, {"ptz.downRight", "右下"},
        {"ptz.down", "下"}, {"ptz.downLeft", "左下"}, {"ptz.left", "左"}, {"ptz.upLeft", "左上"},
        {"ptz.home", "ホーム"}, {"ptz.zoomIn", "ズームイン"}, {"ptz.zoomOut", "ズームアウト"},
        {"ptz.speed", "速度"}, {"ptz.presets", "プリセット"}, {"ptz.pad", "パン・チルト操作パッド"},
        {"ptz.connected", "接続済み"}, {"ptz.offline", "オフライン"},
        {"ptz.presetHint", "クリックで呼び出し。長押しまたは Shift+クリックで保存"},
        {"ptz.presetEmptyHint", "空きスロット。クリックで現在位置を保存"},
        {"ptz.presetRecallHint", "クリックで呼び出し"}, {"ptz.panPad", "パン操作パッド"}, {"ptz.tiltPad", "チルト操作パッド"},
        {"ptz.unsupported", "操作できる機能がありません"},
    };
    for (const auto& e : table)
        if (e.key == key)
            return e.text;
    return fallback;
}

float Approach(float value, float target, float step) {
    return std::abs(target - value) <= step ? target : value + (target > value ? step : -step);
}

// Fake camera feed: angle ticks and three objects positioned in pan/tilt space, so pan, tilt and zoom are all
// visible. Field of view is 60 degrees divided by the zoom.
void DrawCamera(ImDrawList* draw, ImVec2 a, ImVec2 b, const PtzPage::Pose& pose, bool live, float scale, const Theme& theme) {
    const float w = b.x - a.x, h = b.y - a.y;
    const float fovX = 62.f / pose.zoom, fovY = fovX * h / w;
    auto sx = [&](float pan) { return a.x + w * .5f + (pan - pose.pan) / fovX * w; };
    auto sy = [&](float tilt) { return a.y + h * .5f - (tilt - pose.tilt) / fovY * h; };
    draw->PushClipRect(a, b, true);
    const float horizon = std::clamp(sy(0.f), a.y, b.y);
    draw->AddRectFilledMultiColor(a, {b.x, horizon}, IM_COL32(26, 38, 64, 255), IM_COL32(26, 38, 64, 255), IM_COL32(58, 78, 112, 255), IM_COL32(58, 78, 112, 255));
    draw->AddRectFilledMultiColor({a.x, horizon}, b, IM_COL32(40, 42, 50, 255), IM_COL32(40, 42, 50, 255), IM_COL32(20, 21, 26, 255), IM_COL32(20, 21, 26, 255));
    draw->AddLine({a.x, sy(0.f)}, {b.x, sy(0.f)}, IM_COL32(255, 255, 255, 46), 1.f);
    const ImU32 grid = IM_COL32(255, 255, 255, 22), label = IM_COL32(255, 255, 255, 96);
    const float stepPan = fovX > 36 ? 15.f : fovX > 12 ? 5.f : 1.f;
    for (float p = std::floor((pose.pan - fovX * .5f) / stepPan) * stepPan; p <= pose.pan + fovX * .5f; p += stepPan) {
        draw->AddLine({sx(p), a.y}, {sx(p), b.y}, grid, 1.f);
        char text[16];
        std::snprintf(text, sizeof(text), "%.0f", p);
        draw->AddText({sx(p) + 3 * scale, a.y + 6 * scale}, label, text);
    }
    const float stepTilt = fovY > 24 ? 10.f : fovY > 8 ? 5.f : 1.f;
    for (float t = std::floor((pose.tilt - fovY * .5f) / stepTilt) * stepTilt; t <= pose.tilt + fovY * .5f; t += stepTilt)
        draw->AddLine({a.x, sy(t)}, {b.x, sy(t)}, grid, 1.f);
    struct Thing { float pan, tilt, size; ImU32 color; };
    const Thing things[] = {{-40, 4, 9, IM_COL32(236, 130, 90, 255)}, {-12, 10, 6, IM_COL32(120, 200, 150, 255)},
                            {18, 2, 12, IM_COL32(140, 160, 240, 255)}, {58, 12, 7, IM_COL32(230, 200, 100, 255)}};
    for (const Thing& t : things) {
        const float r = t.size / fovX * w * .5f;
        draw->AddRectFilled({sx(t.pan) - r, sy(t.tilt) - r}, {sx(t.pan) + r, sy(t.tilt) + r}, t.color, r * .22f);
        draw->AddRectFilled({sx(t.pan) - r, sy(t.tilt) - r}, {sx(t.pan) + r, sy(t.tilt) - r * .1f}, IM_COL32(255, 255, 255, 40), r * .22f, ImDrawFlags_RoundCornersTop);
    }
    // Overlay: crosshair, corner brackets and readout.
    const ImVec2 c{a.x + w * .5f, a.y + h * .5f};
    const ImU32 hud = IM_COL32(255, 255, 255, 190);
    draw->AddLine({c.x - 10 * scale, c.y}, {c.x + 10 * scale, c.y}, hud, 1.f);
    draw->AddLine({c.x, c.y - 10 * scale}, {c.x, c.y + 10 * scale}, hud, 1.f);
    const float m = 14 * scale, l = 16 * scale;
    for (int i = 0; i < 4; ++i) {
        const float x = i % 2 ? b.x - m : a.x + m, y = i / 2 ? b.y - m : a.y + m, dx = i % 2 ? -l : l, dy = i / 2 ? -l : l;
        draw->AddLine({x, y}, {x + dx, y}, hud, 1.5f);
        draw->AddLine({x, y}, {x, y + dy}, hud, 1.5f);
    }
    char readout[64];
    std::snprintf(readout, sizeof(readout), "P %+.1f\xC2\xB0   T %+.1f\xC2\xB0   %.1f\xC3\x97", pose.pan, pose.tilt, pose.zoom);
    const ImVec2 size = ImGui::CalcTextSize(readout);
    draw->AddText({a.x + w * .5f - size.x * .5f, b.y - m - size.y - 2 * scale}, hud, readout);
    draw->AddCircleFilled({a.x + m + 4 * scale, a.y + m + 12 * scale}, 4 * scale, live ? ImGui::GetColorU32(theme.semantic.error) : IM_COL32(150, 150, 150, 255));
    draw->AddText({a.x + m + 14 * scale, a.y + m + 4 * scale}, hud, live ? "LIVE" : "OFFLINE");
    draw->PopClipRect();
}
std::string ProbeName(std::string_view name) {
    std::string out = "ptz-";
    for (char c : name)
        out += c == ' ' ? '-' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}
} // namespace

void PtzPage::Show(GalleryState& host) {
    const float scale = host.scale;
    if (!initialized) {
        stored[0] = {-60, 8, 1.f};
        stored[1] = {0, 0, 1.f};
        stored[2] = {48, 14, 3.2f};
        initialized = true;
    }
    LocaleContext locale;
    locale.lookup = Japanese;
    locale.user = &japanese;
    locale.languageTag = japanese ? "ja" : "en";
    std::array<accessibility::SemanticNode, 256> nodes{};
    accessibility::AccessibilityFrame semantics{nodes};
    semantics.Begin(ImGui::GetFrameCount());
    ComponentOptions o{&host.theme, &host.animation, &semantics, 0, &locale};

    imkit::Checkbox("日本語", &japanese);
    ImGui::SameLine();
    imkit::Checkbox("Connected", &connected);

    for (std::size_t i = 0; i < presets.size(); ++i) {
        presets[i].stored = storedSlot[i];
        presets[i].name = "";
    }
    const char* names[] = {"Wide", "Centre", "Stage right"};
    for (std::size_t i = 0; i < 3; ++i)
        presets[i].name = names[i];
    int active = -1;
    for (std::size_t i = 0; i < presets.size(); ++i)
        if (storedSlot[i] && std::abs(stored[i].pan - pose.pan) < .25f && std::abs(stored[i].tilt - pose.tilt) < .25f &&
            std::abs(stored[i].zoom - pose.zoom) < .05f)
            active = static_cast<int>(i);

    if (BeginGrid(host, "ptz-grid", 2)) {
        const bool controller = BeginSection(host, "Controller", "Hold an arrow, drag the pad, hold + or -.");
        if (controller) {
            PtzView view;
            view.label = "Camera 1";
            view.position = {pose.pan, pose.tilt, pose.zoom, connected};
            view.zoomFraction = std::log(pose.zoom) / std::log(ZoomMax);
            view.presets = presets;
            view.activePreset = active;
            view.connected = connected;
            const PtzRequest r = PtzControl("main", view, state, {}, o);
            Record(host, "ptz-main");
            last = r;
            const float dt = ImGui::GetIO().DeltaTime;
            if (r.moving) {
                moveToTarget = false;
                pose.pan += r.velocity.x * 60.f * dt / std::sqrt(pose.zoom);
                pose.tilt += r.velocity.y * 40.f * dt / std::sqrt(pose.zoom);
                pose.zoom *= std::exp(r.zoom * .9f * dt);
            }
            if (r.stop)
                ++stopCount;
            if (r.home) {
                target = {0, 0, 1};
                moveToTarget = true;
                ++homeCount;
            }
            if (r.recallPreset >= 0) {
                target = stored[static_cast<std::size_t>(r.recallPreset)];
                moveToTarget = true;
                ++recallCount;
            }
            if (r.storePreset >= 0) {
                stored[static_cast<std::size_t>(r.storePreset)] = pose;
                storedSlot[static_cast<std::size_t>(r.storePreset)] = true;
                ++storeCount;
            }
            if (moveToTarget) {
                pose.pan = Approach(pose.pan, target.pan, 120.f * dt);
                pose.tilt = Approach(pose.tilt, target.tilt, 80.f * dt);
                pose.zoom = Approach(pose.zoom, target.zoom, std::max(.02f, std::abs(target.zoom - pose.zoom) * 6.f * dt + .02f));
                if (pose.pan == target.pan && pose.tilt == target.tilt && pose.zoom == target.zoom)
                    moveToTarget = false;
            }
            pose.pan = std::clamp(pose.pan, -PanLimit, PanLimit);
            pose.tilt = std::clamp(pose.tilt, TiltMin, TiltMax);
            pose.zoom = std::clamp(pose.zoom, 1.f, ZoomMax);
            ImGui::TextDisabled("moves: %s   stops: %d   recalls: %d   stores: %d", r.moving ? "active" : "idle", stopCount, recallCount, storeCount);
        }
        EndSection(controller);

        const bool preview = BeginSection(host, "Camera preview", "Simulated feed driven by the requests above.");
        if (preview) {
            const float width = ImGui::GetContentRegionAvail().x, height = width * 9.f / 16.f;
            const ImVec2 a = ImGui::GetCursorScreenPos();
            ImGui::Dummy({width, height});
            auto* draw = ImGui::GetWindowDrawList();
            DrawCamera(draw, a, {a.x + width, a.y + height}, pose, connected, scale, host.theme);
            draw->AddRect(a, {a.x + width, a.y + height}, ImGui::GetColorU32(ImGuiCol_Border), 0.f, 0, 1.f);
        }
        EndSection(preview);

        const bool variants = BeginSection(host, "Compact and offline", "Toolbar form, and the disabled state.");
        if (variants) {
            PtzView compactView;
            compactView.connected = connected;
            PtzOptions compact;
            compact.compact = true;
            const PtzRequest r = PtzControl("compact", compactView, compactState, compact, o);
            if (r.moving) {
                moveToTarget = false;
                pose.pan += r.velocity.x * 60.f * ImGui::GetIO().DeltaTime / std::sqrt(pose.zoom);
                pose.tilt += r.velocity.y * 40.f * ImGui::GetIO().DeltaTime / std::sqrt(pose.zoom);
                pose.zoom *= std::exp(r.zoom * .9f * ImGui::GetIO().DeltaTime);
            }
            ImGui::Spacing();
            PtzView offline;
            offline.label = "Camera 2";
            offline.connected = false;
            offline.presets = presets;
            (void)PtzControl("offline", offline, offlineState, {}, o);
        }
        EndSection(variants);

        // Each camera offers a different set of controls; only those are drawn.
        const bool capable = BeginSection(host, "Capabilities", "Only the controls a camera offers are drawn.");
        if (capable) {
            struct Variant {
                const char* label;
                PtzCapabilities capabilities;
            };
            static const Variant variantsList[] = {
                {"Pan / tilt head", {.zoom = false}},
                {"Zoom lens only", {.pan = false, .tilt = false}},
                {"Pan only, recall-only presets", {.tilt = false, .zoom = false, .home = false, .presetStore = false}},
                {"Nothing to control", {.pan = false, .tilt = false, .zoom = false, .presets = false}},
            };
            if (ImGui::BeginTable("ptz-capabilities", 2, ImGuiTableFlags_SizingStretchSame)) {
                for (std::size_t i = 0; i < std::size(variantsList); ++i) {
                    ImGui::TableNextColumn();
                    PtzView v;
                    v.label = variantsList[i].label;
                    v.position = {pose.pan, pose.tilt, pose.zoom, connected};
                    v.presets = presets;
                    v.connected = connected;
                    v.capabilities = variantsList[i].capabilities;
                    PtzOptions layout;
                    layout.speed = false;
                    layout.presets = i == 2; // Only the recall-only head shows its preset slots.
                    ImGui::PushID(static_cast<int>(i));
                    (void)PtzControl("capability", v, capabilityStates[i], layout, o);
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
        EndSection(capable);
        EndGrid();
    }

    // The first control wins when the compact variant repeats a button name.
    std::map<std::string, Probe> frame;
    for (const auto& n : semantics.Tree().nodes)
        if (n.role == accessibility::SemanticRole::Button && n.parent == 0 && !n.name.empty())
            frame.emplace(ProbeName(n.name), Probe{n.minimum, n.maximum});
    for (const auto& [name, probe] : frame)
        host.probes[name] = probe;
}
} // namespace imkit::gallery
