#include "charts_page.h"
#include "gallery.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace imkit::gallery {
void ChartsPage::Show(GalleryState& host) {
    auto& theme = host.theme;
    ComponentOptions o{&theme, &host.animation};
    if (!initialized) {
        for (std::size_t i = 0; i < series.size(); ++i)
            series[i] = 40.f + 18.f * std::sin(static_cast<float>(i) * .55f) + 9.f * std::sin(static_cast<float>(i) * 1.7f);
        initialized = true;
    }
    // Animate the sample data so the transitions (value tween, sheen, hover) are visible.
    if (livePlay) {
        clock += ImGui::GetIO().DeltaTime;
        progress = .5f + .45f * std::sin(clock * .6f);
        gauge = .55f + .35f * std::sin(clock * .45f + 1.f);
        if (static_cast<int>(clock * 2.f) != static_cast<int>((clock - ImGui::GetIO().DeltaTime) * 2.f)) {
            std::rotate(series.begin(), series.begin() + 1, series.end());
            series.back() = std::clamp(series[series.size() - 2] + 12.f * std::sin(clock * 3.7f), 12.f, 88.f);
        }
    }
    imkit::Checkbox("Animate sample data", &livePlay);
    ImGui::SameLine();
    const char* periods[] = {"Day", "Week", "Month"};
    Segmented("period", &period, periods, o);
    ImGui::Spacing();

    if (BeginGrid(host, "charts-grid", 3)) {
        const std::array<ChartDatum, 5> usage{{{"Render", 42}, {"Cache", 24}, {"Decode", 14}, {"Audio", 9}, {"Other", 6}}};
        char centre[24];
        std::snprintf(centre, sizeof(centre), "%.0f GB", 95.f);
        { const bool visible = BeginSection(host, "Storage", "Share by category. Hover a segment, click to select.");
        if (visible) {
            const int hit = DonutChart("storage", {usage, centre, "Used"}, {}, o);
            if (hit >= 0)
                donutSelected = hit;
            ImGui::TextDisabled("Selected: %s", donutSelected >= 0 ? usage[static_cast<std::size_t>(donutSelected)].label : "none");
        }
        EndSection(visible); }
        std::array<ChartDatum, 7> weekly{{{"Mon", 32}, {"Tue", 48}, {"Wed", 41}, {"Thu", 66}, {"Fri", 58}, {"Sat", 24}, {"Sun", 19}}};
        if (period == 0)
            weekly = {{{"6", 12}, {"9", 28}, {"12", 54}, {"15", 63}, {"18", 47}, {"21", 22}, {"24", 8}}};
        if (period == 2)
            weekly = {{{"W1", 210}, {"W2", 244}, {"W3", 198}, {"W4", 276}, {"W5", 232}, {"W6", 150}, {"W7", 181}}};
        { const bool visible = BeginSection(host, "Exports", "Bars with value labels and a hairline grid.");
        if (visible) {
            const int hit = BarChart("exports", {weekly}, {{0, 0}, Orientation::Vertical, 0, true, true, "%.0f"}, o);
            if (hit >= 0)
                barSelected = hit;
            ImGui::TextDisabled("Selected: %s", barSelected >= 0 ? weekly[static_cast<std::size_t>(barSelected)].label : "none");
        }
        EndSection(visible); }
        const std::array<GaugeZone, 3> zones{{{.6f, FeedbackKind::Success}, {.85f, FeedbackKind::Warning}, {1.f, FeedbackKind::Error}}};
        char gaugeValue[16];
        std::snprintf(gaugeValue, sizeof(gaugeValue), "%.0f%%", gauge * 100.f);
        { const bool visible = BeginSection(host, "GPU load", "Zones colour the ring: calm, busy, saturated.");
        if (visible) {
            Gauge("gpu", {gauge, gaugeValue, "Utilisation", zones}, {}, o);
            imkit::SliderFloat("##gauge", &gauge, 0.f, 1.f, "%.2f");
        }
        EndSection(visible); }
        { const bool visible = BeginSection(host, "Trends", "Sparklines read at a glance; hover for a value.");
        if (visible) {
            char latest[24];
            std::snprintf(latest, sizeof(latest), "%.0f ms", series.back());
            Sparkline("latency", {series, "Latency", latest}, {}, o);
            ImGui::Spacing();
            std::array<float, 24> inverted{};
            for (std::size_t i = 0; i < series.size(); ++i)
                inverted[i] = 100.f - series[i];
            std::snprintf(latest, sizeof(latest), "%.0f%%", inverted.back());
            Sparkline("quality", {inverted, "Quality", latest}, {{0, 0}, true, FeedbackKind::Success}, o);
        }
        EndSection(visible); }
        { const bool visible = BeginSection(host, "Progress", "One shared track for every progress surface.");
        if (visible) {
            ProgressTrackView running;
            running.fraction = progress;
            running.label = "Exporting";
            ProgressTrack("running", running, o);
            ImGui::Spacing();
            ProgressTrackView paused;
            paused.fraction = .38f;
            paused.label = "Uploading";
            paused.status = ProgressStatus::Paused;
            ProgressTrack("paused", paused, o);
            ImGui::Spacing();
            ProgressTrackView done;
            done.fraction = 1.f;
            done.label = "Verified";
            done.status = ProgressStatus::Succeeded;
            ProgressTrack("done", done, o);
            ImGui::Spacing();
            ProgressTrackView failed;
            failed.fraction = .21f;
            failed.label = "Sync";
            failed.status = ProgressStatus::Failed;
            ProgressTrack("failed", failed, o);
            ImGui::Spacing();
            ProgressTrackView waiting;
            waiting.label = "Waiting for host";
            ProgressTrack("indeterminate", waiting, o);
        }
        EndSection(visible); }
        { const bool visible = BeginSection(host, "Rings", "Circular progress shares the donut's geometry.");
        if (visible) {
            char value[16];
            std::snprintf(value, sizeof(value), "%.0f%%", progress * 100.f);
            CircularProgress("ring-main", CircularProgressView{progress, value, "Coverage"}, {88, 0}, o);
            ImGui::SameLine();
            CircularProgress("ring-ok", CircularProgressView{1.f, "10/10", "PJ1", FeedbackKind::Success}, {60, 0}, o);
            ImGui::SameLine();
            CircularProgress("ring-warn", CircularProgressView{.7f, "7/10", "PJ2", FeedbackKind::Warning}, {60, 0}, o);
            ImGui::SameLine();
            CircularProgress("ring-none", CircularProgressView{-1.f, "", "PJ3"}, {60, 0}, o);
            ImGui::Spacing();
            Spinner("spinner", o);
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Spinner");
            ImGui::SameLine();
            Skeleton("skeleton", {ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight()}, o);
        }
        EndSection(visible); }
        EndGrid();
    }
}
}
