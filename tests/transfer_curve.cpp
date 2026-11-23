#include <imkit/imkit.h>
#include <imkit/transfer_curve.h>
#include <array>
#include <cmath>
#include <cstdio>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

namespace {
int failures = 0;

void Check(bool ok, const char *text) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", text);
        ++failures;
    }
}

void TestClampMonotone() {
    std::array<imkit::TransferCurveNode, 3> nodes{{
        {.pos = {0.f, 0.6f}, .in = {-0.1f, 0.9f}, .out = {0.6f, 0.8f}},
        {.pos = {0.5f, 0.4f}, .in = {0.2f, 0.2f}, .out = {0.9f, 0.7f}},
        {.pos = {1.f, 0.3f}, .in = {0.7f, 0.9f}, .out = {1.2f, 0.1f}},
    }};
    const bool changed = imkit::TransferCurveClampMonotone(nodes);
    Check(changed, "TransferCurveClampMonotone reports adjustments");
    Check(nodes[1].pos.y >= nodes[0].pos.y && nodes[2].pos.y >= nodes[1].pos.y,
          "knots become non-decreasing");
    Check(nodes[0].out.x <= nodes[1].pos.x && nodes[1].in.x >= nodes[0].pos.x,
          "handles clamp inside segment x bounds");
    Check(nodes[0].out.y >= nodes[0].pos.y && nodes[0].out.y <= nodes[1].pos.y,
          "handles clamp inside segment y bounds");
    Check(!imkit::TransferCurveClampMonotone(nodes), "second clamp is idempotent");
}

void TestEditorSmoke() {
    const auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);
    imkit::ApplyTheme(theme, 1.f);

    std::array<imkit::TransferCurveNode, 2> crossNodes{{
        {.pos = {0.f, 0.f}, .in = {0.f, 0.f}, .out = {0.25f, 0.1f}},
        {.pos = {1.f, 1.f}, .in = {0.75f, 0.9f}, .out = {1.f, 1.f}},
    }};
    std::array<imkit::TransferCurveNode, 2> longNodes{{
        {.pos = {0.f, 0.f}, .in = {0.f, 0.f}, .out = {0.3f, 0.3f}},
        {.pos = {1.f, 1.f}, .in = {0.7f, 0.7f}, .out = {1.f, 1.f}},
    }};
    std::array<imkit::TransferCurveSeries, 1> crossSeries{{{.nodes = crossNodes, .color = 0xFF66CCFFu}}};
    std::array<imkit::TransferCurveSeries, 1> longSeries{{{.nodes = longNodes, .color = 0xFFFFAA44u, .dashed = true}}};

    imkit::TransferCurveState state;
    imkit::TransferCurveSection section = imkit::TransferCurveSection::Cross;
    imkit::TransferCurveOptions opts;
    opts.size = {260.f, 180.f};
    opts.monotone = true;

    ImGui::NewFrame();
    ImGui::SetNextWindowPos({10, 10});
    ImGui::SetNextWindowSize({640, 480});
    if (imkit::Begin("TransferCurveSmoke")) {
        imkit::TransferCurveInteraction interaction{};
        const bool curveEdited = imkit::TransferCurveEditor("curve", crossSeries, state, opts, &interaction, {&theme});
        Check(!curveEdited && !interaction.changed, "idle TransferCurveEditor reports no edit");
        bool sectionChanged = false;
        const bool secEdited = imkit::TransferCurveSectionEditor(
            "section-curve", section, crossSeries, longSeries, state, opts, &interaction, &sectionChanged, {&theme});
        Check(!secEdited && !sectionChanged, "idle TransferCurveSectionEditor reports no edit");
    }
    imkit::End();
    ImGui::EndFrame();
}
} // namespace

int main() {
#ifdef _MSC_VER
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    auto *context = ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.DisplaySize = {1280, 720};
    io.DeltaTime = 1.f / 60.f;
    io.IniFilename = nullptr;
    unsigned char *pixels = nullptr;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    TestClampMonotone();
    TestEditorSmoke();

    ImGui::DestroyContext(context);
    if (failures == 0)
        std::puts("transfer_curve monotone clamp and editor smoke validated");
    return failures == 0 ? 0 : 1;
}
