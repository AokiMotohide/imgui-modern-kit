#include <imkit/imkit.h>
#include <cmath>
#include <cstdio>

namespace {
int failures = 0;
void Check(bool ok, const char *text) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", text);
        ++failures;
    }
}
bool Near(float a, float b) {
    return std::fabs(a - b) < 1e-4f;
}
float Area(const imkit::NineSliceQuad *quads, int count) {
    float total = 0;
    for (int i = 0; i < count; ++i)
        total += (quads[i].max.x - quads[i].min.x) * (quads[i].max.y - quads[i].min.y);
    return total;
}

void TestNineSlice() {
    imkit::NineSliceQuad quads[9];
    const imkit::NineSliceBorder border{8, 8, 8, 8};
    int count = imkit::NineSliceQuads({10, 20}, {110, 80}, {32, 32}, border, 1.f, {0, 0}, {1, 1}, quads);
    Check(count == 9, "nine quads for a roomy destination");
    Check(Near(Area(quads, count), 100.f * 60.f), "quads tile the destination exactly");
    // Row-major: the first quad is the top-left corner, the fifth is the centre.
    Check(Near(quads[0].min.x, 10) && Near(quads[0].max.x, 18) && Near(quads[0].max.y, 28), "corner keeps its pixel size");
    Check(Near(quads[0].uv1.x, .25f) && Near(quads[0].uv1.y, .25f), "corner samples the border of the picture");
    Check(Near(quads[4].min.x, 18) && Near(quads[4].max.x, 102) && Near(quads[4].uv0.x, .25f) && Near(quads[4].uv1.x, .75f),
          "centre stretches and samples the middle");

    count = imkit::NineSliceQuads({0, 0}, {100, 60}, {32, 32}, border, 2.f, {0, 0}, {1, 1}, quads);
    Check(count == 9 && Near(quads[0].max.x, 16) && Near(quads[0].max.y, 16), "scale grows the corners");

    // Smaller than both corners: the borders shrink together, nothing overlaps or leaves the rectangle.
    count = imkit::NineSliceQuads({0, 0}, {10, 10}, {32, 32}, border, 1.f, {0, 0}, {1, 1}, quads);
    Check(count == 4 && Near(Area(quads, count), 100.f), "tiny destination shrinks the corners");
    for (int i = 0; i < count; ++i)
        Check(quads[i].min.x >= 0 && quads[i].max.x <= 10 && quads[i].min.y >= 0 && quads[i].max.y <= 10, "quads stay inside");

    // A picture inside an atlas: UVs stay within the selected region.
    count = imkit::NineSliceQuads({0, 0}, {100, 100}, {32, 32}, border, 1.f, {.5f, .5f}, {1.f, 1.f}, quads);
    Check(count == 9 && Near(quads[0].uv0.x, .5f) && Near(quads[0].uv1.x, .625f) && Near(quads[8].uv1.x, 1.f), "atlas region");

    // No borders: one quad covering everything.
    count = imkit::NineSliceQuads({0, 0}, {50, 40}, {32, 32}, {}, 1.f, {0, 0}, {1, 1}, quads);
    Check(count == 1 && Near(Area(quads, count), 2000.f) && Near(quads[0].uv1.x, 1.f), "zero border is a plain image");

    // Borders larger than the picture are reduced, never extrapolated.
    count = imkit::NineSliceQuads({0, 0}, {100, 100}, {32, 32}, {40, 40, 40, 40}, 1.f, {0, 0}, {1, 1}, quads);
    Check(count > 0 && quads[0].uv1.x <= .5f + 1e-4f, "oversized border");

    Check(imkit::NineSliceQuads({0, 0}, {0, 10}, {32, 32}, border, 1.f, {0, 0}, {1, 1}, quads) == 0, "empty destination");
    Check(imkit::NineSliceQuads({0, 0}, {10, 10}, {0, 32}, border, 1.f, {0, 0}, {1, 1}, quads) == 0, "empty texture");
    Check(imkit::NineSliceQuads({0, 0}, {10, 10}, {32, 32}, border, 0.f, {0, 0}, {1, 1}, quads) == 0, "non-positive scale");
}

struct Probe {
    int calls = 0;
    bool handle = true;
    imkit::SkinPart part = imkit::SkinPart::Button;
    ImVec2 min{0, 0}, max{0, 0};
    float rounding = 0;
    const imkit::Theme *theme = nullptr;
};
bool ProbeFrame(const imkit::SkinFrame &frame, void *user) {
    auto &probe = *static_cast<Probe *>(user);
    ++probe.calls;
    probe.part = frame.part;
    probe.min = frame.min;
    probe.max = frame.max;
    probe.rounding = frame.rounding;
    probe.theme = frame.theme;
    return probe.handle;
}

// Runs one frame, draws a single component and returns how many vertices the window produced.
template <class Draw> int Vertices(Draw draw) {
    ImGui::NewFrame();
    ImGui::SetNextWindowPos({10, 10});
    ImGui::SetNextWindowSize({400, 200});
    ImGui::Begin("skin");
    const int before = ImGui::GetWindowDrawList()->VtxBuffer.Size;
    draw();
    const int after = ImGui::GetWindowDrawList()->VtxBuffer.Size;
    ImGui::End();
    ImGui::EndFrame();
    return after - before;
}

void TestSkin() {
    auto &io = ImGui::GetIO();
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    auto theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionLight);
    imkit::ApplyTheme(theme, 1.f);
    Probe probe;
    imkit::Skin skin{&ProbeFrame, &probe};
    // Spelled out: `{&theme}` alone would also match the deprecated Theme* overloads of StatusBadge and NotificationCard.
    const imkit::ComponentOptions options{&theme};
    const auto button = [&] { imkit::ActionButton("Go", imkit::ActionVariant::Primary, {}, options); };

    Vertices(button); // The first frame of a window may skip its items.
    // Without a skin the stock ImGui path is used; this is the reference for what the default frame looks like.
    const int stock = Vertices(button);
    Check(probe.calls == 0, "no skin, no call");

    theme.skin = &skin;
    probe.handle = false;
    const int declined = Vertices(button);
    Check(probe.calls == 1 && probe.part == imkit::SkinPart::Button && probe.theme == &theme, "skin is offered the button");
    Check(declined == stock, "a skin that declines leaves the default look unchanged");
    Check(probe.rounding == ImGui::GetStyle().FrameRounding && probe.max.x > probe.min.x && probe.max.y > probe.min.y, "frame geometry");

    probe.handle = true;
    const int replaced = Vertices(button);
    Check(probe.calls == 2 && replaced < stock, "a skin that paints replaces the default frame");

    // Badges and cards go through the same hook with their own part.
    Vertices([&] { imkit::StatusBadge("OK", imkit::StatusKind::Success, options); });
    Check(probe.part == imkit::SkinPart::Badge, "badge part");
    imkit::Notification note{"n", "Saved", imkit::StatusKind::Success, 0};
    Vertices([&] { imkit::NotificationCard(note, 0, options); });
    Check(probe.part == imkit::SkinPart::Card, "card part");

    // Components that do not receive the theme keep the default look, whatever the theme holds.
    const int calls = probe.calls;
    Vertices([] { imkit::ActionButton("Go", imkit::ActionVariant::Primary); });
    Check(probe.calls == calls, "no theme, no skin");

    // A hard shadow without a skin takes the same painted path and adds the shadow behind the button.
    theme.skin = nullptr;
    theme.elevation.hard = true;
    theme.elevation.offset = {0.f, 3.f};
    const int shadowed = Vertices(button);
    Check(shadowed > stock, "hard shadow draws behind the button");
    theme.elevation.hard = false;
    Check(Vertices(button) == stock, "soft elevation leaves buttons alone");
}
} // namespace

int main() {
    TestNineSlice();
    auto *context = ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = {1280, 720};
    ImGui::GetIO().DeltaTime = 1.f / 60.f;
    ImGui::GetIO().IniFilename = nullptr;
    TestSkin();
    ImGui::DestroyContext(context);
    if (failures == 0)
        std::puts("nine-slice geometry and skin hooks validated");
    return failures == 0 ? 0 : 1;
}
