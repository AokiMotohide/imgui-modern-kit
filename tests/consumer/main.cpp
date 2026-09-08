#include <imkit/imkit.h>

int main() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(320.0F, 240.0F);
    io.DeltaTime = 1.0F / 60.0F;
    io.Fonts->AddFontDefault();
    unsigned char *pixels = nullptr;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    imkit::ApplyTheme(imkit::MakePrecisionTheme());

    ImGui::NewFrame();
    ImGui::Begin("Consumer");
    imkit::Button("Button");
    ImGui::End();
    ImGui::Render();
    ImGui::DestroyContext();
    return 0;
}
