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
    auto theme=imkit::MakeTheme(imkit::ColorScheme::Dark,imkit::ContrastMode::HighContrast,imkit::Density::Touch);
    if(!imkit::ValidateContrast(theme)) return 2;
    imkit::ApplyTheme(theme);
    imkit::accessibility::SemanticNode nodes[8];
    imkit::accessibility::AccessibilityFrame semantics(nodes);
    semantics.Begin(1);

    ImGui::NewFrame();
    ImGui::Begin("Consumer");
    imkit::Button("Button");
    imkit::ComponentOptions options; options.theme=&theme; options.accessibility=&semantics;
    imkit::ActionButton("Accessible action",imkit::ActionVariant::Primary,{},options);
    ImGui::End();
    ImGui::Render();
    if(!semantics.Tree().Validate() || semantics.Tree().nodes.size()!=1) return 3;
    ImGui::DestroyContext();
    return 0;
}
