#include <imkit/widgets.h>

namespace imkit {

bool Button(const char* label, const ImVec2& size) {
    return ImGui::Button(label, size);
}

bool Checkbox(const char* label, bool* value) {
    return ImGui::Checkbox(label, value);
}

bool SliderFloat(
    const char* label,
    float* value,
    float minimum,
    float maximum,
    const char* format,
    ImGuiSliderFlags flags) {
    return ImGui::SliderFloat(label, value, minimum, maximum, format, flags);
}

bool InputText(
    const char* label,
    char* buffer,
    std::size_t bufferSize,
    ImGuiInputTextFlags flags,
    ImGuiInputTextCallback callback,
    void* userData) {
    return ImGui::InputText(label, buffer, bufferSize, flags, callback, userData);
}

bool Selectable(
    const char* label,
    bool selected,
    ImGuiSelectableFlags flags,
    const ImVec2& size) {
    return ImGui::Selectable(label, selected, flags, size);
}

void ProgressBar(float fraction, const ImVec2& size, const char* overlay) {
    ImGui::ProgressBar(fraction, size, overlay);
}

}  // namespace imkit

