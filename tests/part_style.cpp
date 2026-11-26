#include <imkit/part_style.h>
#include <imkit/theme.h>
#include <imkit/widgets.h>
#include <imkit/components.h>
#include <imkit/native.h>
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

bool Near(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) <= eps;
}

void TestParsingAndNames() {
    const ImVec4 c = imkit::Hex("#3d7eff");
    Check(Near(c.x, 0x3d / 255.f) && Near(c.y, 0x7e / 255.f) && Near(c.z, 1.f) && Near(c.w, 1.f),
          "Hex parses 6-digit hex");
    const ImVec4 ca = imkit::Hex("11223380");
    Check(Near(ca.x, 0x11 / 255.f) && Near(ca.w, 0x80 / 255.f), "Hex parses 8-digit hex without #");
    const ImVec4 bad = imkit::Hex("not-a-color");
    Check(Near(bad.x, 1.f) && Near(bad.y, 0.f) && Near(bad.z, 1.f) && Near(bad.w, 1.f),
          "Invalid Hex returns opaque magenta");

    Check(imkit::PartFromName("button") == imkit::Part::Button, "PartFromName button");
    Check(imkit::PartFromName("small-button") == imkit::Part::SmallButton, "PartFromName small-button");
    Check(!imkit::PartFromName("unknown-part").has_value(), "PartFromName rejects unknown");
    Check(imkit::PartVarFromName("thumb.size") == imkit::PartVar::ThumbSize, "PartVarFromName dotted");
    Check(imkit::PartVarFromName("thumb-size") == imkit::PartVar::ThumbSize, "PartVarFromName dashed");
    Check(imkit::PartStateFromName("hover") == imkit::PartState::Hover, "PartStateFromName hover");
    Check(imkit::PartColorFromName("mark") == imkit::PartColor::Mark, "PartColorFromName mark");
}

void TestLayerPrecedence() {
    auto theme = imkit::MakeTheme(imkit::ThemePreset::Graphite);
    theme.partOverrides.SetVar(imkit::Part::Button, imkit::PartVar::Height, 38.f);
    theme.partOverrides.SetColor(imkit::Part::Button, imkit::PartState::Hover, imkit::PartColor::Fill,
                                 imkit::Hex("#224466"));
    imkit::ApplyTheme(theme, 1.f);

    imkit::PartLayer layer = imkit::PartLayer::Default;
    Check(Near(imkit::ResolvePartVar(imkit::Part::Button, imkit::PartVar::Height, nullptr, &layer), 38.f) &&
              layer == imkit::PartLayer::Theme,
          "L1 Theme override wins over Default");

    imkit::ButtonStyle named;
    named.height = 42.f;
    named.colors.hover.fill = imkit::Hex("#336699");
    imkit::RegisterStyle("card-action", named);
    Check(imkit::IsStyleRegistered("card-action"), "RegisterStyle registers style");

    {
        imkit::StyleScope scope("card-action");
        Check(Near(imkit::ResolvePartVar(imkit::Part::Button, imkit::PartVar::Height, nullptr, &layer), 42.f) &&
                  layer == imkit::PartLayer::NamedStyle,
              "L2 StyleScope wins over Theme");

        imkit::PushPartVar(imkit::Part::Button, imkit::PartVar::Height, 46.f);
        Check(Near(imkit::ResolvePartVar(imkit::Part::Button, imkit::PartVar::Height, nullptr, &layer), 46.f) &&
                  layer == imkit::PartLayer::Pushed,
              "PushPartVar wins over StyleScope");

        imkit::ButtonStyle call;
        call.height = 50.f;
        const imkit::PartValues callVals = imkit::ToPartValues(call);
        Check(Near(imkit::ResolvePartVar(imkit::Part::Button, imkit::PartVar::Height, &callVals, &layer), 50.f) &&
                  layer == imkit::PartLayer::Call,
              "Call style wins over PushPartVar");
        imkit::PopPartVar();
    }

    Check(Near(imkit::ResolvePartVar(imkit::Part::Button, imkit::PartVar::Height, nullptr, &layer), 38.f) &&
              layer == imkit::PartLayer::Theme,
          "StyleScope pops back to Theme");
    imkit::UnregisterStyle("card-action");
    Check(!imkit::IsStyleRegistered("card-action"), "UnregisterStyle removes style");
}

void TestWidgetsAndPrimitivesSmoke() {
    auto DrawPass = [](imkit::ThemePreset preset) {
        const auto theme = imkit::MakeTheme(preset);
        imkit::ApplyTheme(theme, 1.f);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({10, 10});
        ImGui::SetNextWindowSize({640, 480});
        if (imkit::Begin("PartStyleSmoke")) {
            static bool flag = true;
            static int radio = 0;
            static float sliderVal = 0.5f;
            static int dragVal = 12;
            static char textBuf[64] = "ImKit 4.2";
            static float col[4] = {0.2f, 0.5f, 0.9f, 1.f};
            static int comboIdx = 0;
            const char *items[] = {"One", "Two", "Three"};

            imkit::Button("Btn");
            imkit::SmallButton("Small");
            imkit::Checkbox("Chk", &flag);
            imkit::Toggle("Tog", &flag);
            imkit::RadioButton("Rad", &radio, 0);
            imkit::SliderFloat("Sld", &sliderVal, 0.f, 1.f);
            imkit::DragInt("Drg", &dragVal, 1.f, 0, 100);
            imkit::InputText("Inp", textBuf, sizeof(textBuf));
            imkit::Combo("Cmb", &comboIdx, items, 3);
            imkit::Selectable("Sel", flag);
            imkit::ColorEdit4("Col", col);
            imkit::ProgressBar(0.65f, {-1.f, 0.f}, "65%");
            if (imkit::CollapsingHeader("Hdr")) {
                if (imkit::TreeNode("Tr"))
                    imkit::TreePop();
            }
            if (imkit::BeginTabBar("##tabs")) {
                if (imkit::BeginTabItem("TabA"))
                    imkit::EndTabItem();
                imkit::EndTabBar();
            }
            imkit::SeparatorText("Sep");
            auto *draw = ImGui::GetWindowDrawList();
            imkit::DrawSmoothRect(draw, {10, 10}, {90, 40}, IM_COL32(60, 130, 240, 255), 8.f);
            imkit::DrawSmoothRectBorder(draw, {10, 10}, {90, 40}, IM_COL32(255, 255, 255, 200), 8.f, 1.5f);
            imkit::DrawDashedRect(draw, {14, 14}, {86, 36}, 6.f, IM_COL32(200, 200, 200, 180), 1.f, 4.f);
            imkit::DrawFocusRing(draw, {10, 10}, {90, 40}, 8.f, 1.f);
            imkit::DrawChevronRotated(draw, {50, 25}, 8.f, IM_COL32(255, 255, 255, 255), 0.5f);
            imkit::DrawCheckMark(draw, {50, 25}, 10.f, IM_COL32(255, 255, 255, 255), 1.8f, 0.7f);
            const auto res = imkit::Interact("custom-hit", {80, 28}, imkit::InteractFlags_Layout);
            Check(res.id != 0, "Interact allocates an item ID");
            const float anim = imkit::AnimatePart(res.id, 1.f);
            Check(anim >= 0.f && anim <= 1.f, "AnimatePart stays in 0..1");
            if (imkit::BeginSectionCard("sec", "Section", "Description", imkit::ComponentOptions{&theme})) {
                imkit::LabeledSliderFloat("Gamma", &sliderVal, 0.5f, 3.0f, "0.5 - 3.0");
                imkit::ChoiceCardOptions cardOpts;
                cardOpts.subtitle = "Direct capture";
                cardOpts.trailingText = "1.80";
                cardOpts.dotColor = {0.3f, 0.7f, 1.0f, 1.0f};
                cardOpts.selected = true;
                cardOpts.components = imkit::ComponentOptions{&theme};
                imkit::ChoiceCard("pj0", "PJ0", cardOpts);
                const imkit::ActionRowItem rowItems[] = {{"Back"}, {"Next", imkit::ActionVariant::Primary}};
                imkit::ActionButtonRow("row", rowItems, imkit::ComponentOptions{&theme});

                // Nested SectionCard + BeginTable safety, typography helpers, ControlSize, and new form widgets.
                imkit::SectionHeader("Header", "Subtitle");
                imkit::TextTitle("Title");
                imkit::TextHeading("Heading");
                imkit::TextLabel("Label");
                imkit::TextCaption("Caption");
                imkit::KeyValueRow("Resolution", "1920 x 1080", true);
                imkit::StatusPill("pill", "OK", imkit::StatusKind::Success);
                imkit::SettingToggleRow("Enable", &flag, "Hint text");
                imkit::LabeledCombo("Choice", &comboIdx, items, "Combo hint");

                imkit::ComponentOptions smOpts{&theme};
                smOpts.size = imkit::ControlSize::Sm;
                imkit::ActionButton("SmBtn", imkit::ActionVariant::Secondary, {}, smOpts);
                const float smH = ImGui::GetItemRectSize().y;

                imkit::ComponentOptions mdOpts{&theme};
                mdOpts.size = imkit::ControlSize::Md;
                imkit::ActionButton("MdBtn", imkit::ActionVariant::Secondary, {}, mdOpts);
                const float mdH = ImGui::GetItemRectSize().y;

                imkit::ComponentOptions lgOpts{&theme};
                lgOpts.size = imkit::ControlSize::Lg;
                imkit::ActionButton("LgBtn", imkit::ActionVariant::Primary, {}, lgOpts);
                const float lgH = ImGui::GetItemRectSize().y;
                Check(smH < mdH && mdH < lgH, "ControlSize Sm < Md < Lg");

                if (imkit::BeginSectionCard("nested-sec", "Nested", nullptr, imkit::ComponentOptions{&theme})) {
                    if (ImGui::BeginTable("##nested-tbl", 2)) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        imkit::TextLabel("A");
                        ImGui::TableSetColumnIndex(1);
                        imkit::TextCaption("B");
                        ImGui::EndTable();
                    }
                    imkit::EndSectionCard();
                }
                imkit::EndSectionCard();
            }
        }
        imkit::End();
        auto mutableTheme = theme;
        bool editorOpen = true;
        imkit::ShowStyleEditor(&mutableTheme, &editorOpen);
        ImGui::EndFrame();
    };

    DrawPass(imkit::ThemePreset::Graphite);
    DrawPass(imkit::ThemePreset::Classic);
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

    TestParsingAndNames();
    TestLayerPrecedence();
    TestWidgetsAndPrimitivesSmoke();

    ImGui::DestroyContext(context);
    if (failures == 0)
        std::puts("part_style parsing, layer precedence, 22-part modern/classic smoke, L4 primitives validated");
    return failures == 0 ? 0 : 1;
}
