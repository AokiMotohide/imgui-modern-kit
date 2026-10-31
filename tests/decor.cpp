#include <imkit/imkit.h>
#include <cmath>
#include <cstdio>
#include <utility>

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

imkit::SpriteSheet MakeSheet() {
    imkit::SpriteSheet sheet;
    sheet.sheetPixels = {400, 200};
    sheet.cellPixels = {100, 100};
    sheet.columns = 4;
    sheet.frames = 6; // Two rows; the second holds two frames.
    sheet.fps = 10.f;
    sheet.restFrame = 2;
    return sheet;
}

void TestSpriteMath() {
    auto sheet = MakeSheet();
    // Loop wraps; the frame is always inside the sheet.
    Check(imkit::SpriteFrameAt(sheet, 0.0, true) == 0, "loop starts at frame zero");
    Check(imkit::SpriteFrameAt(sheet, 0.35, true) == 3, "loop advances by fps");
    Check(imkit::SpriteFrameAt(sheet, 0.6, true) == 0 && imkit::SpriteFrameAt(sheet, 0.75, true) == 1, "loop wraps");
    Check(imkit::SpriteFrameAt(sheet, -5.0, true) == 0, "negative time clamps to the start");
    Check(imkit::SpriteFrameAt(sheet, 1e30, true) == 0, "huge time stays inside the sheet");
    // Reduced motion and bad inputs show the rest frame.
    Check(imkit::SpriteFrameAt(sheet, 0.35, false) == 2, "motion off shows the rest frame");
    Check(imkit::SpriteFrameAt(sheet, std::nan(""), true) == 2, "non-finite time shows the rest frame");
    sheet.fps = 0.f;
    Check(imkit::SpriteFrameAt(sheet, 0.35, true) == 2, "zero fps shows the rest frame");
    sheet.fps = 10.f;
    sheet.restFrame = 99;
    Check(imkit::SpriteFrameAt(sheet, 0.35, false) == 5, "rest frame is clamped");
    sheet.restFrame = 2;
    // Once returns to the rest frame; HoldLast stays on the last one.
    sheet.playback = imkit::SpritePlayback::Once;
    Check(imkit::SpriteFrameAt(sheet, 0.55, true) == 5 && imkit::SpriteFrameAt(sheet, 0.6, true) == 2, "once ends on the rest frame");
    sheet.playback = imkit::SpritePlayback::HoldLast;
    Check(imkit::SpriteFrameAt(sheet, 0.6, true) == 5 && imkit::SpriteFrameAt(sheet, 1e30, true) == 5, "hold-last stays on the last frame");
    sheet.frames = 0;
    Check(imkit::SpriteFrameAt(sheet, 1.0, true) == 0, "no frames is frame zero");
    sheet.frames = 1;
    Check(imkit::SpriteFrameAt(sheet, 1.0, true) == 0, "one frame never changes");

    sheet = MakeSheet();
    ImVec2 uv0, uv1;
    Check(imkit::SpriteFrameUV(sheet, 0, uv0, uv1) && Near(uv0.x, 0) && Near(uv0.y, 0) && Near(uv1.x, .25f) && Near(uv1.y, .5f), "frame 0 uv");
    Check(imkit::SpriteFrameUV(sheet, 5, uv0, uv1) && Near(uv0.x, .25f) && Near(uv0.y, .5f) && Near(uv1.x, .5f) && Near(uv1.y, 1.f), "frame 5 is row 1, column 1");
    Check(!imkit::SpriteFrameUV(sheet, 6, uv0, uv1) && !imkit::SpriteFrameUV(sheet, -1, uv0, uv1), "frames outside the sheet are refused");
    auto atlas = sheet;
    atlas.uvMin = {.5f, .5f};
    atlas.uvMax = {1.f, 1.f};
    Check(imkit::SpriteFrameUV(atlas, 1, uv0, uv1) && Near(uv0.x, .5f + .5f * .25f) && Near(uv1.x, .5f + .5f * .5f) && Near(uv1.y, .5f + .5f * .5f), "sheet inside an atlas");
    auto broken = sheet;
    broken.columns = 5; // Five columns of 100px do not fit a 400px sheet.
    Check(!imkit::SpriteFrameUV(broken, 4, uv0, uv1), "a cell outside the sheet is refused");
    broken = sheet;
    broken.cellPixels = {0, 100};
    Check(!imkit::SpriteFrameUV(broken, 0, uv0, uv1), "empty cell is refused");
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

struct ThumbProbe {
    int calls = 0;
    imkit::ProgressThumbInfo last;
};
void ThumbFn(const imkit::ProgressThumbInfo &info, void *user) {
    auto &probe = *static_cast<ThumbProbe *>(user);
    ++probe.calls;
    probe.last = info;
}

void TestSpriteWidget() {
    auto &io = ImGui::GetIO();
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    auto sheet = MakeSheet();
    sheet.texture = io.Fonts->TexRef;
    imkit::SpriteState state;
    const auto draw = [&](double now, bool animate) {
        int frame = -1;
        const int vertices = Vertices([&] { frame = imkit::Sprite("sprite", sheet, state, now, {64, 64}, animate); });
        return std::pair<int, int>{frame, vertices};
    };
    draw(0.0, true); // The first frame of a window may skip its items.
    state.Restart();
    const auto first = draw(10.0, true);
    Check(first.first == 0 && first.second > 0, "the first call starts at frame zero and draws");
    Check(draw(10.35, true).first == 3, "later calls follow the host clock");
    Check(draw(10.35, false).first == sheet.restFrame, "reduced motion shows the rest frame");
    state.Restart();
    Check(draw(20.0, true).first == 0, "restart begins again at frame zero");
    auto unusable = sheet;
    unusable.cellPixels = {0, 0};
    int drawn = -1;
    const int vertices = Vertices([&] { imkit::Sprite("bad", unusable, state, 0.0, {64, 64}); drawn = 1; });
    Check(drawn == 1 && vertices == 0, "an unusable sheet reserves space but draws nothing");

    // The thumb hook of a progress track.
    auto theme = imkit::MakeTheme(imkit::ThemePreset::PrecisionLight);
    imkit::ApplyTheme(theme, 1.f);
    const imkit::ComponentOptions options{&theme};
    ThumbProbe probe;
    imkit::ProgressTrackView view;
    view.fraction = .5f;
    view.thumb = &ThumbFn;
    view.thumbUser = &probe;
    const auto track = [&] { imkit::ProgressTrack("thumb", view, options); };
    Vertices(track);
    probe.calls = 0;
    Vertices(track);
    Check(probe.calls == 1, "a determinate track calls the thumb once per frame");
    Check(probe.last.draw != nullptr && probe.last.trackMax.x > probe.last.trackMin.x &&
              probe.last.head.x > probe.last.trackMin.x && probe.last.head.x <= probe.last.trackMax.x + 1e-3f &&
              probe.last.head.y > probe.last.trackMin.y && probe.last.head.y < probe.last.trackMax.y,
          "the head lies on the track");
    Check(probe.last.fraction >= 0.f && probe.last.fraction <= 1.f, "the shown fraction is a value");
    view.fraction = -1.f;
    probe.calls = 0;
    Vertices(track);
    Check(probe.calls == 1 && probe.last.fraction < 0.f, "a running indeterminate track calls the thumb with a negative fraction");
    view.status = imkit::ProgressStatus::Queued;
    probe.calls = 0;
    Vertices(track);
    Check(probe.calls == 0, "a queued indeterminate track draws no segment and no thumb");
    view.status = imkit::ProgressStatus::Running;
    view.fraction = .5f;
    view.thumb = nullptr;
    float without = 0, with = 0;
    Vertices([&] {
        const float top = ImGui::GetCursorScreenPos().y;
        track();
        without = ImGui::GetCursorScreenPos().y - top;
    });
    view.thumb = &ThumbFn;
    view.thumbHeight = 30.f;
    Vertices([&] {
        const float top = ImGui::GetCursorScreenPos().y;
        track();
        with = ImGui::GetCursorScreenPos().y - top;
    });
    Check(with > without + 29.f, "thumbHeight reserves layout height above the track");
}
} // namespace

int main() {
    TestNineSlice();
    TestSpriteMath();
    auto *context = ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = {1280, 720};
    ImGui::GetIO().DeltaTime = 1.f / 60.f;
    ImGui::GetIO().IniFilename = nullptr;
    TestSkin();
    TestSpriteWidget();
    ImGui::DestroyContext(context);
    if (failures == 0)
        std::puts("nine-slice geometry, skin hooks, sprite playback and progress thumbs validated");
    return failures == 0 ? 0 : 1;
}
