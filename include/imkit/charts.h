#pragma once
#include <imkit/workflow.h>
#include <cstddef>
#include <span>

namespace imkit {
// Categorical chart colours: slot 0 is the theme accent, the others follow a
// colour-blind-safe sequence tuned for the theme's light or dark scheme. A null
// theme falls back to the current ImGui style. Indices wrap.
ImVec4 ChartColor(const Theme* theme, std::size_t index);

struct ChartDatum {
    const char* label="";
    float value=0;
    // Alpha <= 0 selects ChartColor for the datum's index.
    ImVec4 color{};
};

struct DonutChartView {
    std::span<const ChartDatum> segments{};
    // Optional centre text. Both are borrowed for the current frame.
    const char* value="";
    const char* caption="";
    // Zero uses the sum of the segments. A larger total leaves the remainder as an empty track.
    float total=0;
};
struct DonutChartOptions {
    // Zero uses six frame heights. Thickness zero derives from the diameter.
    float diameter=0;
    float thickness=0;
    // Draws one row per segment (swatch, label, share) beside, or below when narrow.
    bool legend=true;
};
// Draw-only. Returns the segment index clicked this frame, or -1. Hovering a
// segment enlarges it and shows its value. Negative and non-finite values are ignored.
int DonutChart(const char* id, const DonutChartView& view, DonutChartOptions layout={}, ComponentOptions options={});

struct SparklineView {
    std::span<const float> values{};
    const char* label="";
    // Host-formatted latest value or delta, shown on the right of the header row.
    const char* valueText="";
};
struct SparklineOptions {
    // Width zero fills the available width; height zero uses 1.6 frame heights.
    ImVec2 size{};
    bool area=true;
    FeedbackKind kind=FeedbackKind::Info;
};
// Draw-only trend line with a highlighted last point and a hover read-out.
void Sparkline(const char* id, const SparklineView& view, SparklineOptions layout={}, ComponentOptions options={});

struct BarChartView {
    std::span<const ChartDatum> bars{};
};
struct BarChartOptions {
    // Zero width fills the available width; zero height uses eight frame heights.
    ImVec2 size{};
    Orientation orientation=Orientation::Vertical;
    // Zero derives the axis maximum from the data.
    float maximum=0;
    bool grid=true;
    bool values=true;
    // printf format for value labels and the tooltip.
    const char* format="%.0f";
};
// Draw-only. Returns the bar index clicked this frame, or -1.
int BarChart(const char* id, const BarChartView& view, BarChartOptions layout={}, ComponentOptions options={});

struct GaugeZone {
    // Zone covers fractions up to and including this value.
    float upTo=1.f;
    FeedbackKind kind=FeedbackKind::Info;
};
struct GaugeView {
    // Negative or non-finite values render as unavailable. Finite values are clamped to [0, 1].
    float fraction=-1;
    const char* value="";
    const char* label="";
    // Ascending zones choose the fill colour; empty uses the kind below.
    std::span<const GaugeZone> zones{};
    FeedbackKind kind=FeedbackKind::Info;
};
struct GaugeOptions {
    // Zero uses five frame heights.
    float diameter=0;
    float thickness=0;
};
// Draw-only open-ring gauge (240 degrees) with a centred value.
void Gauge(const char* id, const GaugeView& view, GaugeOptions layout={}, ComponentOptions options={});
} // namespace imkit
