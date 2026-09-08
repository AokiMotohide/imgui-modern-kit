#include <cmath>
#include <cstdio>
#include <cstring>

#include <imgui.h>
#include <imkit/imkit.h>

namespace {

bool RunFrame(ImGuiContext* context, const char* windowName) {
    ImGui::SetCurrentContext(context);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(640.0F, 480.0F);
    io.DeltaTime = 1.0F / 60.0F;

    bool checked = false;
    float value = 0.5F;
    char text[32] = "Sample text";
    const bool selected = false;

    ImGui::NewFrame();
    ImGui::Begin(windowName);
    const bool buttonPressed = imkit::Button("Button");
    const bool checkboxChanged = imkit::Checkbox("Checkbox", &checked);
    const bool sliderChanged = imkit::SliderFloat("SliderFloat", &value, 0.0F, 1.0F);
    const bool textChanged = imkit::InputText("InputText", text, sizeof(text));
    const bool selectablePressed = imkit::Selectable("Selectable", selected);
    imkit::ProgressBar(value);
    ImGui::End();
    ImGui::Render();

    return !buttonPressed && !checkboxChanged && !sliderChanged && !textChanged
        && !selectablePressed && !checked && std::fabs(value - 0.5F) < 0.0001F
        && std::strcmp(text, "Sample text") == 0;
}

void PrepareDefaultFont() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
}

}  // namespace

int main() {
    IMGUI_CHECKVERSION();

    ImGuiContext* first = ImGui::CreateContext();
    PrepareDefaultFont();
    ImGuiContext* second = ImGui::CreateContext();
    ImGui::SetCurrentContext(second);
    PrepareDefaultFont();

    const bool firstPassed = RunFrame(first, "First context");
    const bool secondPassed = RunFrame(second, "Second context");

    ImGui::DestroyContext(second);
    ImGui::DestroyContext(first);

    if (!firstPassed || !secondPassed) {
        std::fprintf(stderr, "ImKit changed state without input or failed context switching.\n");
        return 1;
    }

    return 0;
}
