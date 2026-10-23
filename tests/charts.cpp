#include <imkit/imkit.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>

namespace {
int failures = 0;
void Check(bool ok, const char* label) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", label);
        ++failures;
    }
}
}

int main() {
    using namespace imkit;
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {640, 480};
    io.DeltaTime = 1.f / 60;
    io.Fonts->AddFontDefault();
    unsigned char* pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    auto theme = MakeTheme(ThemePreset::PrecisionLight);
    ApplyTheme(theme);
    std::array<accessibility::SemanticNode, 32> nodes{};
    accessibility::AccessibilityFrame semantics{nodes};
    AnimationState animation;
    ComponentOptions options{&theme, &animation, &semantics};

    // Colours: slot 0 follows the theme accent; slots wrap and are opaque.
    Check(ChartColor(&theme, 0).x == theme.semantic.accent.x, "slot zero is the accent");
    Check(ChartColor(&theme, 1).w == 1.f, "palette colours are opaque");
    Check(ChartColor(&theme, 8).x == ChartColor(&theme, 0).x, "palette wraps after eight slots");

    const std::array<ChartDatum, 4> segments{{{"A", 3}, {"B", -1}, {"C", std::numeric_limits<float>::quiet_NaN()}, {"D", 1}}};
    const std::array<float, 5> series{1, 4, 2, std::numeric_limits<float>::infinity(), 3};
    const std::array<GaugeZone, 2> zones{{{.5f, FeedbackKind::Success}, {1.f, FeedbackKind::Error}}};
    int donut = 0, bars = 0;
    auto frame = [&]() {
        semantics.Begin(ImGui::GetFrameCount() + 1);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Host");
        donut = DonutChart("donut", {segments, "4", "items"}, {}, options);
        DonutChart("empty", {}, {64, 0, false}, options);
        DonutChart("total", {segments, "", "", 10.f}, {}, options);
        bars = BarChart("bars", {segments}, {}, options);
        BarChart("horizontal", {segments}, {{300, 0}, Orientation::Horizontal}, options);
        BarChart("none", {}, {}, options);
        Gauge("gauge", {.7f, "70%", "Load", zones}, {}, options);
        Gauge("gauge-unavailable", {std::numeric_limits<float>::quiet_NaN(), "", "Unknown"}, {}, options);
        Sparkline("spark", {series, "Latency", "3 ms"}, {}, options);
        Sparkline("short", {std::span<const float>(series).first(1), "", ""}, {{80, 0}}, options);
        ImGui::End();
        ImGui::Render();
    };
    for (int i = 0; i < 3; ++i)
        frame();
    Check(donut == -1 && bars == -1, "no click without input");
    Check(semantics.Tree().Validate(), "chart semantic tree is valid");
    bool sawGauge = false, sawUnavailable = false;
    for (const auto& node : semantics.Tree().nodes) {
        if (node.role == accessibility::SemanticRole::Progress && node.name == "Load")
            sawGauge = node.value == "70%" && !node.state.invalid;
        if (node.role == accessibility::SemanticRole::Progress && node.name == "Unknown")
            sawUnavailable = node.state.invalid && node.value == "\xE2\x80\x94";
    }
    Check(sawGauge && sawUnavailable, "gauge semantics and unavailable state");

    // Reduced motion draws without relying on animation state.
    theme.motion.reducedMotion = true;
    frame();
    ImGui::DestroyContext();
    std::printf("Chart failures: %d\n", failures);
    return failures ? 1 : 0;
}
