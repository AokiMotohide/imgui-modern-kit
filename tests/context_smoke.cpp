#include <cmath>
#include <cstdio>
#include <cstring>

#include <imgui.h>
#include <imkit/imkit.h>

namespace {

bool RunFrame(ImGuiContext *context, const char *windowName) {
    ImGui::SetCurrentContext(context);
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(640.0F, 480.0F);
    io.DeltaTime = 1.0F / 60.0F;

    bool checked = false;
    float value = 0.5F;
    char text[32] = "Sample text";
    const bool selected = false;

    auto theme = imkit::MakePrecisionTheme(imkit::ColorScheme::Dark);
    imkit::ApplyTheme(theme, 1.5f);
    const float height = ImGui::GetStyle().FramePadding.y;
    imkit::ApplyTheme(theme, 1.5f);
    if (ImGui::GetStyle().FramePadding.y != height)
        return false;
    imkit::ApplyTheme(theme);
    ImGui::NewFrame();
    const auto original = ImGui::GetStyle();
    const auto originalFont = ImGui::GetFont();
    {
        imkit::ThemeScope outer(imkit::MakePrecisionTheme(imkit::ColorScheme::Light));
        const auto light = ImGui::GetStyle();
        {
            imkit::ThemeScope inner(theme, 1.5f);
        }
        if (ImGui::GetStyle().Colors[ImGuiCol_WindowBg].x != light.Colors[ImGuiCol_WindowBg].x)
            return false;
    }
    if (ImGui::GetStyle().FramePadding.y != original.FramePadding.y ||
        ImGui::GetStyle().Colors[ImGuiCol_WindowBg].x != original.Colors[ImGuiCol_WindowBg].x)
        return false;
    if (ImGui::GetFont() != originalFont)
        return false;
    ImGui::Begin(windowName);
    ImGui::BeginDisabled();
    const auto disabledAlpha = ImGui::GetStyle().Alpha;
    {
        imkit::ThemeScope disabledScope(theme);
        if (ImGui::GetStyle().Alpha != disabledAlpha)
            return false;
        if (imkit::Toggle("Disabled switch", &checked))
            return false;
    }
    ImGui::EndDisabled();
    const bool buttonPressed = imkit::Button("Button");
    const bool checkboxChanged = imkit::Checkbox("Checkbox", &checked);
    const bool sliderChanged = imkit::SliderFloat("SliderFloat", &value, 0.0F, 1.0F);
    const bool textChanged = imkit::InputText("InputText", text, sizeof(text));
    const bool selectablePressed = imkit::Selectable("Selectable", selected);
    imkit::ProgressBar(value);
    ImGui::End();
    ImGui::Render();

    return !buttonPressed && !checkboxChanged && !sliderChanged && !textChanged && !selectablePressed &&
           !checked && std::fabs(value - 0.5F) < 0.0001F && std::strcmp(text, "Sample text") == 0;
}

void PrepareDefaultFont() {
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    unsigned char *pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
}

} // namespace

int main() {
    IMGUI_CHECKVERSION();

    ImGuiContext *first = ImGui::CreateContext();
    PrepareDefaultFont();
    ImGuiContext *second = ImGui::CreateContext();
    ImGui::SetCurrentContext(second);
    PrepareDefaultFont();

    ImGui::GetStyle().FramePadding = {19, 23};
    const bool firstPassed = RunFrame(first, "First context");
    ImGui::SetCurrentContext(second);
    const bool independent = ImGui::GetStyle().FramePadding.y == 23;
    const bool secondPassed = RunFrame(second, "Second context");

    imkit::AnimationState animation;
    animation.Reset(42);
    bool motion = animation.Update(1, 0, .01f, .06f, 0) == 0;
    float midpoint = animation.Update(1, 1, .03f, .06f, 1);
    motion &= midpoint > 0 && midpoint < 1;
    motion &= animation.Update(1, 1, .06f, .06f, 2) == 1;
    motion &= animation.Update(1, 0, .001f, .06f, 3, false) == 0;
    animation.Prune(500);
    motion &= animation.Update(1, .25f, 0, .06f, 501) == .25f;
    animation.Reset(43);
    motion &= animation.Generation() == 43 && animation.Update(1, .75f, 0, .06f, 0) == .75f;

    ImGui::DestroyContext(second);
    ImGui::DestroyContext(first);

    if (!firstPassed || !secondPassed || !independent || !motion) {
        std::fprintf(stderr, "ImKit changed state without input or failed context switching.\n");
        return 1;
    }

    return 0;
}
