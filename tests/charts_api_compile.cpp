#include <imkit/imkit.h>
ImVec4 (*volatile color)(const imkit::Theme*, std::size_t) = &imkit::ChartColor;
int (*volatile donut)(const char*, const imkit::DonutChartView&, imkit::DonutChartOptions, imkit::ComponentOptions) = &imkit::DonutChart;
int (*volatile bar)(const char*, const imkit::BarChartView&, imkit::BarChartOptions, imkit::ComponentOptions) = &imkit::BarChart;
void (*volatile gauge)(const char*, const imkit::GaugeView&, imkit::GaugeOptions, imkit::ComponentOptions) = &imkit::Gauge;
void (*volatile spark)(const char*, const imkit::SparklineView&, imkit::SparklineOptions, imkit::ComponentOptions) = &imkit::Sparkline;
int main() {
    imkit::ChartDatum datum;
    imkit::GaugeZone zone;
    return datum.value != 0 || zone.upTo != 1.f;
}
