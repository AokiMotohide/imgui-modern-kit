#include <imkit/widgets.h>
#include <imkit/patterns.h>
#include <algorithm>
#include <cstdarg>

namespace imkit {
namespace {
// Selected rows get an accent bar on the leading edge; tabs get an inset pill underneath.
void SelectionMark(bool selected, bool underline = false) {
    if (!selected)
        return;
    const auto a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    const float height = b.y - a.y;
    auto *draw = ImGui::GetWindowDrawList();
    const ImU32 color = ImGui::GetColorU32(ImGuiCol_CheckMark);
    if (underline) {
        const float inset = std::min(10.f, (b.x - a.x) * .2f);
        draw->AddRectFilled({a.x + inset, b.y - 3}, {b.x - inset, b.y - 1}, color, 1.f);
    } else {
        // Drawn in the padding to the left of the row so it never overlaps the label.
        draw->AddRectFilled({a.x - 6, a.y + height * .22f}, {a.x - 3, b.y - height * .22f}, color, 1.5f);
    }
}
// Shrinks the frame height for check marks only, leaving inputs and buttons at the control height.
struct CompactFrame {
    CompactFrame() {
        const auto &style = ImGui::GetStyle();
        const float pad = std::max(1.f, ImGui::GetFontSize() * .225f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {style.FramePadding.x, pad});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, std::min(style.FrameRounding, 6.f));
    }
    ~CompactFrame() { ImGui::PopStyleVar(2); }
    CompactFrame(const CompactFrame &) = delete;
    CompactFrame &operator=(const CompactFrame &) = delete;
};
} // namespace

bool Button(const char *label, const ImVec2 &size) {
    return ImGui::Button(label, size);
}

bool Checkbox(const char *label, bool *value) {
    CompactFrame compact;
    return ImGui::Checkbox(label, value);
}

bool RadioButton(const char *label, bool active) {
    CompactFrame compact;
    return ImGui::RadioButton(label, active);
}

bool RadioButton(const char *label, int *value, int buttonValue) {
    CompactFrame compact;
    return ImGui::RadioButton(label, value, buttonValue);
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
    SelectionMark(active, true);
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
    ProgressTrackView view;
    view.fraction = fraction;
    view.label = overlay ? overlay : "";
    const bool fixedWidth = size.x > 0;
    if (fixedWidth) {
        ImGui::BeginChild("##imkit-progress", {size.x, 0}, ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
    }
    ProgressTrack("##imkit-progress-track", view);
    if (fixedWidth)
        ImGui::EndChild();
}

} // namespace imkit
