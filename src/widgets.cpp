#include <imkit/widgets.h>
#include <cstdarg>

namespace imkit {
namespace {
void SelectionMark(bool selected) {
    if (!selected)
        return;
    auto a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddLine({a.x, b.y - 1}, {b.x, b.y - 1},
                                        ImGui::GetColorU32(ImGuiCol_CheckMark), 2);
}
} // namespace

bool Button(const char *label, const ImVec2 &size) {
    return ImGui::Button(label, size);
}

bool Checkbox(const char *label, bool *value) {
    return ImGui::Checkbox(label, value);
}

bool SliderFloat(const char *label, float *value, float minimum, float maximum, const char *format,
                 ImGuiSliderFlags flags) {
    return ImGui::SliderFloat(label, value, minimum, maximum, format, flags);
}

bool InputText(const char *label, char *buffer, std::size_t bufferSize, ImGuiInputTextFlags flags,
               ImGuiInputTextCallback callback, void *userData) {
    return ImGui::InputText(label, buffer, bufferSize, flags, callback, userData);
}

bool Selectable(const char *label, bool selected, ImGuiSelectableFlags flags, const ImVec2 &size) {
    bool pressed = ImGui::Selectable(label, selected, flags, size);
    SelectionMark(selected);
    return pressed;
}

bool Selectable(const char *label, bool *selected, ImGuiSelectableFlags flags, const ImVec2 &size) {
    bool pressed = ImGui::Selectable(label, selected, flags, size);
    SelectionMark(*selected);
    return pressed;
}
bool BeginTabItem(const char *label, bool *open, ImGuiTabItemFlags flags) {
    bool active = ImGui::BeginTabItem(label, open, flags);
    SelectionMark(active);
    return active;
}
bool TreeNodeEx(const char *label, ImGuiTreeNodeFlags flags) {
    bool open = ImGui::TreeNodeEx(label, flags);
    SelectionMark((flags & ImGuiTreeNodeFlags_Selected) != 0);
    return open;
}
bool TreeNodeExV(const char *id, ImGuiTreeNodeFlags flags, const char *format, va_list args) {
    bool open = ImGui::TreeNodeExV(id, flags, format, args);
    SelectionMark((flags & ImGuiTreeNodeFlags_Selected) != 0);
    return open;
}
bool TreeNodeExV(const void *id, ImGuiTreeNodeFlags flags, const char *format, va_list args) {
    bool open = ImGui::TreeNodeExV(id, flags, format, args);
    SelectionMark((flags & ImGuiTreeNodeFlags_Selected) != 0);
    return open;
}
bool TreeNodeEx(const char *id, ImGuiTreeNodeFlags flags, const char *format, ...) {
    va_list args;
    va_start(args, format);
    bool open = imkit::TreeNodeExV(id, flags, format, args);
    va_end(args);
    return open;
}
bool TreeNodeEx(const void *id, ImGuiTreeNodeFlags flags, const char *format, ...) {
    va_list args;
    va_start(args, format);
    bool open = imkit::TreeNodeExV(id, flags, format, args);
    va_end(args);
    return open;
}

void ProgressBar(float fraction, const ImVec2 &size, const char *overlay) {
    ImGui::ProgressBar(fraction, size, overlay);
}

} // namespace imkit
