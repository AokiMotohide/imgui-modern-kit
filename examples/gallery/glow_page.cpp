#include "glow_page.h"
#include "gallery.h"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <cstdio>
#include <string>
namespace imkit::gallery {
namespace {
constexpr ImVec4 kClay{.851f, .467f, .341f, 1.f};

// Every field of a GlowFrameStyle, grouped as light, motion and ripples. `prefix` keeps the probe names unique.
bool StyleEditor(GalleryState& host, const char* prefix, GlowFrameStyle& s, ComponentOptions o) {
    bool changed = false;
    ImGui::PushID(prefix);
    char probe[64];
    auto record = [&](const char* name) {
        std::snprintf(probe, sizeof(probe), "%s-%s", prefix, name);
        Record(host, probe);
    };
    static const char* const kMotion[] = {"Steady", "Breathe", "Orbit"};
    static const char* const kPlacement[] = {"Inside", "Outside", "Centered"};
    int motion = static_cast<int>(s.motion), placement = static_cast<int>(s.placement);
    ImGui::TextDisabled("Motion");
    if (Segmented("motion", &motion, kMotion, o)) {
        s.motion = static_cast<GlowMotion>(motion);
        changed = true;
    }
    record("motion");
    ImGui::TextDisabled("Placement");
    if (Segmented("placement", &placement, kPlacement, o)) {
        s.placement = static_cast<GlowPlacement>(placement);
        changed = true;
    }
    ImGui::SeparatorText("Light");
    changed |= ImGui::ColorEdit4("Color", &s.color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    changed |= ImGui::SliderFloat("Intensity", &s.intensity, 0.f, 1.f, "%.2f");
    record("intensity");
    changed |= ImGui::SliderFloat("Spread", &s.spread, 0.f, 120.f, "%.0f px");
    record("spread");
    changed |= ImGui::SliderFloat("Falloff", &s.falloff, .5f, 5.f, "%.1f");
    changed |= ImGui::SliderFloat("Line", &s.lineThickness, 0.f, 8.f, "%.1f px");
    changed |= ImGui::SliderFloat("Line opacity", &s.lineIntensity, 0.f, 1.f, "%.2f");
    changed |= ImGui::SliderFloat("Corner radius", &s.rounding, 0.f, 40.f, "%.0f px");
    ImGui::SeparatorText("Motion");
    ImGui::BeginDisabled(s.motion == GlowMotion::Steady);
    changed |= ImGui::SliderFloat("Period", &s.period, .2f, 8.f, "%.1f s");
    record("period");
    changed |= ImGui::SliderFloat(s.motion == GlowMotion::Orbit ? "Track level" : "Low point", &s.minimumIntensity, 0.f, 1.f, "%.2f");
    ImGui::EndDisabled();
    ImGui::BeginDisabled(s.motion != GlowMotion::Orbit);
    changed |= ImGui::SliderInt("Heads", &s.orbitHeads, 1, 8);
    changed |= ImGui::SliderFloat("Tail", &s.orbitLength, .02f, 1.f, "%.2f of the edge");
    ImGui::EndDisabled();
    changed |= ImGui::SliderFloat("Fade in/out", &s.fadeSeconds, 0.f, 1.5f, "%.2f s");
    ImGui::SeparatorText("Ripples");
    changed |= ImGui::SliderFloat("Life", &s.rippleSeconds, .1f, 3.f, "%.2f s");
    changed |= ImGui::SliderFloat("Distance", &s.rippleDistance, 0.f, 300.f, "%.0f px");
    record("distance");
    changed |= ImGui::SliderFloat("Interval", &s.rippleInterval, 0.f, 5.f, s.rippleInterval > 0.f ? "every %.1f s" : "on Pulse only");
    record("interval");
    ImGui::PopID();
    return changed;
}

// A stand-in application: toolbar, sidebar, a chart and an inspector, in the theme's colours.
void MockApplication(ImDrawList* draw, ImVec2 a, ImVec2 b, const Theme& theme, float scale) {
    const auto& c = theme.semantic;
    const float r = theme.radius.overlay;
    auto col = [](ImVec4 v, float alpha = 1.f) { v.w *= alpha; return ImGui::ColorConvertFloat4ToU32(v); };
    draw->AddRectFilled(a, b, col(c.canvas), r);
    const float bar = 28.f * scale, pad = 10.f * scale;
    draw->AddRectFilled(a, {b.x, a.y + bar}, col(c.surface), r, ImDrawFlags_RoundCornersTop);
    for (int i = 0; i < 4; ++i)
        draw->AddRectFilled({a.x + pad + i * 58.f * scale, a.y + 9.f * scale}, {a.x + pad + i * 58.f * scale + 42.f * scale, a.y + 19.f * scale},
                            col(c.textSecondary, .35f), 3.f * scale);
    const float side = std::min(150.f * scale, (b.x - a.x) * .24f), inspector = std::min(170.f * scale, (b.x - a.x) * .26f);
    draw->AddRectFilled({a.x + pad, a.y + bar + pad}, {a.x + pad + side, b.y - pad}, col(c.surface), r * .5f);
    draw->AddRectFilled({b.x - pad - inspector, a.y + bar + pad}, {b.x - pad, b.y - pad}, col(c.surface), r * .5f);
    for (int i = 0; i < 7; ++i) {
        const float y = a.y + bar + pad * 2 + i * 22.f * scale;
        if (y + 10.f * scale > b.y - pad)
            break;
        draw->AddRectFilled({a.x + pad * 2, y}, {a.x + pad * 2 + side * (.75f - i * .06f), y + 9.f * scale}, col(c.textSecondary, .28f), 3.f * scale);
        draw->AddRectFilled({b.x - pad - inspector + pad, y}, {b.x - pad * 2, y + 11.f * scale}, col(c.textSecondary, .2f), 3.f * scale);
    }
    const ImVec2 chartA{a.x + pad * 2 + side, a.y + bar + pad}, chartB{b.x - pad * 2 - inspector, b.y - pad};
    draw->AddRectFilled(chartA, chartB, col(c.surfaceRaised), r * .5f);
    ImVec2 points[48];
    for (int i = 0; i < 48; ++i) {
        const float t = i / 47.f;
        points[i] = {chartA.x + pad + (chartB.x - chartA.x - pad * 2) * t,
                     (chartA.y + chartB.y) * .5f + std::sin(t * 9.f) * (chartB.y - chartA.y) * .22f * (1.f - t * .4f)};
    }
    draw->AddPolyline(points, 48, col(c.textSecondary, .55f), 0, 2.f * scale);
}

struct SessionStep {
    double at;
    ExternalControlPhase phase;
    const char* activity;
};
// A scripted remote session for the demo: connect, operate, lock input while writing, finish, then repeat.
constexpr SessionStep kSession[] = {
    {0.0, ExternalControlPhase::Connected, "Waiting for commands"},
    {1.4, ExternalControlPhase::Active, "Selecting fixture 12"},
    {2.6, ExternalControlPhase::Active, "Moving the camera to preset 3"},
    {3.8, ExternalControlPhase::Active, "Setting intensity to 80%"},
    {5.0, ExternalControlPhase::InputLocked, "Writing the calibration"},
    {8.0, ExternalControlPhase::Active, "Verifying the result"},
    {9.4, ExternalControlPhase::Active, "Saving the scene"},
    {10.8, ExternalControlPhase::Off, ""},
};
constexpr double kSessionLength = 12.5;
} // namespace

void GlowPage::ApplyPreset(int index, const Theme& theme) {
    preset = index;
    style = {};
    style.fadeSeconds = theme.motion.overlaySeconds;
    switch (index) {
    case 0: // Breathe: the computer-use look.
        style.color = kClay;
        break;
    case 1: // Orbit: work in progress.
        style.color = theme.semantic.accent;
        style.motion = GlowMotion::Orbit;
        style.period = 3.2f;
        style.minimumIntensity = .25f;
        style.spread = 24.f;
        style.lineThickness = 2.f;
        style.orbitLength = .22f;
        break;
    case 2: // Ripple: a heartbeat for every command.
        style.color = theme.semantic.accent;
        style.motion = GlowMotion::Steady;
        style.intensity = .55f;
        style.spread = 18.f;
        style.rippleInterval = 1.6f;
        style.rippleSeconds = 1.1f;
        style.rippleDistance = 120.f;
        break;
    case 3: // Quiet line: present but calm.
        style.color = theme.semantic.accent;
        style.motion = GlowMotion::Steady;
        style.intensity = .9f;
        style.spread = 8.f;
        style.lineThickness = 1.5f;
        break;
    case 4: // Recording: a red pulse around a panel.
        style.color = theme.semantic.error;
        style.period = 1.2f;
        style.minimumIntensity = .2f;
        style.placement = GlowPlacement::Outside;
        style.rounding = 12.f;
        style.spread = 22.f;
        break;
    default: // Live: two heads circling on the edge.
        style.color = theme.semantic.success;
        style.motion = GlowMotion::Orbit;
        style.orbitHeads = 2;
        style.placement = GlowPlacement::Centered;
        style.period = 4.f;
        style.spread = 26.f;
        style.rounding = 12.f;
        break;
    }
    preview.Reset();
}

void GlowPage::Show(GalleryState& host) {
    const ComponentOptions o{&host.theme, &host.animation};
    const float scale = host.scale;
    const double now = ImGui::GetTime();
    const bool themeMotion = host.theme.motion.enabled && !host.theme.motion.reducedMotion;
    if (!initialized) {
        ApplyPreset(0, host.theme);
        remote = MakeExternalControlStyle(host.theme);
        initialized = true;
    }
    ImGui::TextWrapped("DrawGlowFrame lights the edge of any rectangle. ExternalControlIndicator uses it around the whole window to "
                       "show that an automation API, a remote operator or an agent is driving the application.");
    ImGui::Spacing();
    if (!BeginGrid(host, "glow-grid", 2))
        return;

    // ---- Glow frame -----------------------------------------------------------------------------------------
    const bool frameVisible = BeginSection(host, "Glow frame", "Pick a look, then change any value. The frame redraws as you drag.");
    if (frameVisible) {
        static const char* const kPresets[] = {"Breathe", "Orbit", "Ripple", "Quiet line", "Recording", "Live"};
        for (int i = 0; i < 6; ++i) {
            if (i)
                ImGui::SameLine();
            char label[48];
            std::snprintf(label, sizeof(label), "%s##glow-preset", kPresets[i]);
            if (ActionButton(label, preset == i ? ActionVariant::Primary : ActionVariant::Secondary, {}, o))
                ApplyPreset(i, host.theme);
            char probe[32];
            std::snprintf(probe, sizeof(probe), "glow-preset-%d", i);
            Record(host, probe);
        }
        ImGui::Spacing();
        const float width = ImGui::GetContentRegionAvail().x, height = 300.f * scale;
        const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + width, a.y + height};
        ImGui::Dummy({width, height});
        Record(host, "glow-preview");
        auto* draw = ImGui::GetWindowDrawList();
        draw->PushClipRect(a, b, true);
        // Outside placement needs room around the frame; Inside fills the area.
        const float margin = style.placement == GlowPlacement::Inside ? 0.f : 44.f * scale;
        const ImVec2 frameA{a.x + margin, a.y + margin}, frameB{b.x - margin, b.y - margin};
        MockApplication(draw, frameA, frameB, host.theme, scale);
        DrawGlowFrame(draw, frameA, frameB, style, preview, now, active, animate && themeMotion);
        draw->PopClipRect();
        ImGui::Spacing();
        ImGui::Checkbox("Active", &active);
        Record(host, "glow-active");
        ImGui::SameLine();
        if (ActionButton("Pulse", ActionVariant::Secondary, {}, o))
            preview.Pulse();
        Record(host, "glow-pulse");
        ImGui::SameLine();
        ImGui::Checkbox("Animate", &animate);
        if (!themeMotion) {
            ImGui::SameLine();
            ImGui::TextDisabled("(the theme asks for reduced motion)");
        }
        ImGui::Spacing();
        StyleEditor(host, "glow", style, o);
        ImGui::Spacing();
        // The current values as code, ready to paste.
        static const char* const kMotionNames[] = {"Steady", "Breathe", "Orbit"};
        static const char* const kPlacementNames[] = {"Inside", "Outside", "Centered"};
        char code[1024];
        std::snprintf(code, sizeof(code),
                      "imkit::GlowFrameStyle glow;\n"
                      "glow.color = {%.3ff, %.3ff, %.3ff, %.2ff};\n"
                      "glow.motion = imkit::GlowMotion::%s;\n"
                      "glow.placement = imkit::GlowPlacement::%s;\n"
                      "glow.intensity = %.2ff; glow.minimumIntensity = %.2ff; glow.period = %.1ff;\n"
                      "glow.spread = %.0ff; glow.falloff = %.1ff; glow.lineThickness = %.1ff; glow.rounding = %.0ff;\n"
                      "glow.orbitHeads = %d; glow.orbitLength = %.2ff;\n"
                      "glow.rippleSeconds = %.2ff; glow.rippleDistance = %.0ff; glow.rippleInterval = %.1ff;\n"
                      "imkit::DrawGlowFrame(draw, min, max, glow, state, ImGui::GetTime(), active);",
                      style.color.x, style.color.y, style.color.z, style.color.w, kMotionNames[static_cast<int>(style.motion)],
                      kPlacementNames[static_cast<int>(style.placement)], style.intensity, style.minimumIntensity, style.period,
                      style.spread, style.falloff, style.lineThickness, style.rounding, style.orbitHeads, style.orbitLength,
                      style.rippleSeconds, style.rippleDistance, style.rippleInterval);
        if (ImGui::TreeNode("C++ for this look")) {
            ImGui::PushFont(nullptr, host.theme.metrics.bodySize * .92f);
            ImGui::TextUnformatted(code);
            ImGui::PopFont();
            if (ActionButton("Copy", ActionVariant::Secondary, {}, o))
                ImGui::SetClipboardText(code);
            ImGui::TreePop();
        }
    }
    EndSection(frameVisible);

    // ---- External control indicator ----------------------------------------------------------------------
    ExternalControlView view;
    const bool remoteVisible = BeginSection(host, "External control indicator", "Frames this window and names who is in control. Stop is a request the host handles.");
    if (remoteVisible) {
        static const char* const kPhases[] = {"Off", "Connected", "Active", "Input locked"};
        ImGui::BeginDisabled(simulate);
        Segmented("phase", &phase, kPhases, o);
        ImGui::EndDisabled();
        Record(host, "glow-phase");
        ImGui::Spacing();
        if (ImGui::Checkbox("Simulate a remote session", &simulate)) {
            simulateStart = now;
            simulateStep = -1;
        }
        Record(host, "glow-simulate");
        ImGui::SameLine();
        if (ActionButton("Send command", ActionVariant::Secondary, {}, o))
            remoteState.Pulse();
        Record(host, "glow-command");
        ImGui::TextDisabled("Connect, operate with a ripple per command, lock input while writing, then finish.");
        ImGui::Spacing();
        ImGui::InputText("Client", client, sizeof(client));
        ImGui::InputText("Activity", activity, sizeof(activity));
        ImGui::Checkbox("Show the pill", &remote.showPill);
        static const char* const kPill[] = {"Top left", "Top center", "Top right", "Bottom left", "Bottom center", "Bottom right"};
        int pill = static_cast<int>(remote.pill);
        if (ImGui::Combo("Pill position", &pill, kPill, 6))
            remote.pill = static_cast<OverlayAnchor>(pill);
        ImGui::SliderFloat("Pill margin", &remote.pillMargin, 0.f, 80.f, "%.0f px");
        ImGui::SliderFloat("Locked wash", &remote.lockedTint, 0.f, .3f, "%.2f");
        ImGui::Checkbox("Dot follows the frame", &remote.pulseDot);
        static const char* const kFill[] = {"Solid", "Surface"};
        int fill = static_cast<int>(remote.pillFill);
        if (ImGui::Combo("Pill fill", &fill, kFill, 2))
            remote.pillFill = static_cast<ExternalControlPillFill>(fill);
        ImGui::SliderFloat("Pill outline", &remote.pillOutline, 0.f, 4.f, "%.1f px");
        ImGui::SliderFloat("Pill halo", &remote.pillGlow, 0.f, 1.f, "%.2f");
        ImGui::Checkbox("Pill shadow", &remote.pillShadow);
        ImGui::SliderFloat("Hold a higher phase", &remote.holdSeconds, 0.f, 3.f, "%.1f s");
        ImGui::SliderFloat("Merge pulses within", &remote.pulseInterval, 0.f, 2.f, "%.2f s");
        ImGui::SliderFloat("Phase blend", &remote.transitionSeconds, 0.f, 1.5f, "%.2f s");
        ImGui::Text("Stop pressed %d time%s.", stops, stops == 1 ? "" : "s");
        ImGui::Spacing();
        if (ImGui::TreeNodeEx("Frame look per phase", ImGuiTreeNodeFlags_DefaultOpen)) {
            static const char* const kEdit[] = {"Connected", "Active", "Input locked"};
            int edit = editPhase - 1;
            if (Segmented("edit-phase", &edit, kEdit, o))
                editPhase = edit + 1;
            GlowFrameStyle& edited = editPhase == 1 ? remote.connected : editPhase == 3 ? remote.locked : remote.active;
            StyleEditor(host, "remote", edited, o);
            if (ActionButton("Reset to the theme's look", ActionVariant::Secondary, {}, o)) {
                const auto pill = remote.pill;
                const bool showPill = remote.showPill;
                remote = MakeExternalControlStyle(host.theme);
                remote.pill = pill;
                remote.showPill = showPill;
            }
            ImGui::TreePop();
        }
    }
    EndSection(remoteVisible);
    EndGrid();

    // The scripted session sets the phase and the activity; a new step is a new command, so it ripples.
    if (simulate) {
        const double t = std::fmod(now - simulateStart, kSessionLength);
        int step = 0;
        for (int i = 0; i < static_cast<int>(std::size(kSession)); ++i)
            if (t >= kSession[i].at)
                step = i;
        if (step != simulateStep) {
            simulateStep = step;
            phase = static_cast<int>(kSession[step].phase);
            if (*kSession[step].activity)
                std::snprintf(activity, sizeof(activity), "%s", kSession[step].activity);
            if (kSession[step].phase == ExternalControlPhase::Active || kSession[step].phase == ExternalControlPhase::InputLocked)
                remoteState.Pulse();
        }
    }
    view.phase = static_cast<ExternalControlPhase>(phase);
    view.client = client;
    view.activity = activity;
    view.title = "External control";
    const auto request = ExternalControlIndicator("gallery-remote", view, remoteState, now, remote, ImGui::GetWindowViewport(), o);
    if (request.stopRequested) {
        ++stops;
        phase = 0;
        simulate = false;
    }
}
}
