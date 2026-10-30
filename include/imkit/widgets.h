#pragma once

#include <cstddef>

#include <imkit/version.h>

namespace imkit {

bool Button(const char *label, const ImVec2 &size = ImVec2(0, 0));

bool Checkbox(const char *label, bool *value);
// Compact check mark and radio: the square follows the font, not the 30 px control height.
bool RadioButton(const char *label, bool active);
bool RadioButton(const char *label, int *value, int buttonValue);

bool SliderFloat(const char *label, float *value, float minimum, float maximum, const char *format = "%.3f",
                 ImGuiSliderFlags flags = 0);
// Fill-style sliders: the value is shown as a tinted bar inside the frame instead of a handle that
// overlaps the value text. Input behaviour is ImGui's own; Logarithmic sliders keep the native grab.
bool SliderInt(const char *label, int *value, int minimum, int maximum, const char *format = "%d",
               ImGuiSliderFlags flags = 0);
bool VSliderFloat(const char *label, const ImVec2 &size, float *value, float minimum, float maximum,
                  const char *format = "%.3f", ImGuiSliderFlags flags = 0);

bool SliderFloat2(const char *label, float v[2], float minimum, float maximum, const char *format = "%.3f",
                  ImGuiSliderFlags flags = 0);
bool SliderFloat3(const char *label, float v[3], float minimum, float maximum, const char *format = "%.3f",
                  ImGuiSliderFlags flags = 0);
bool SliderFloat4(const char *label, float v[4], float minimum, float maximum, const char *format = "%.3f",
                  ImGuiSliderFlags flags = 0);
bool SliderInt2(const char *label, int v[2], int minimum, int maximum, const char *format = "%d", ImGuiSliderFlags flags = 0);
bool SliderInt3(const char *label, int v[3], int minimum, int maximum, const char *format = "%d", ImGuiSliderFlags flags = 0);
bool SliderInt4(const char *label, int v[4], int minimum, int maximum, const char *format = "%d", ImGuiSliderFlags flags = 0);

// Combo with a chevron instead of the stock arrow box; the preview is shortened with an ellipsis.
// Flags that change the frame width (NoArrowButton, NoPreview, WidthFitPreview) use the stock drawing.
bool BeginCombo(const char *label, const char *preview, ImGuiComboFlags flags = 0);
void EndCombo();
bool Combo(const char *label, int *current, const char *const items[], int count, int heightInItems = -1);
bool Combo(const char *label, int *current, const char *zeroSeparated, int heightInItems = -1);
bool Combo(const char *label, int *current, const char *(*getter)(void *user, int index), void *user, int count,
           int heightInItems = -1);

bool InputText(const char *label, char *buffer, std::size_t bufferSize, ImGuiInputTextFlags flags = 0,
               ImGuiInputTextCallback callback = nullptr, void *userData = nullptr);

bool Selectable(const char *label, bool selected = false, ImGuiSelectableFlags flags = 0,
                const ImVec2 &size = ImVec2(0, 0));

void ProgressBar(float fraction, const ImVec2 &size = ImVec2(-1, 0), const char *overlay = nullptr);

bool Selectable(const char *label, bool *selected, ImGuiSelectableFlags flags = 0,
                const ImVec2 &size = ImVec2(0, 0));
bool BeginTabItem(const char *label, bool *open = nullptr, ImGuiTabItemFlags flags = 0);
bool TreeNodeEx(const char *label, ImGuiTreeNodeFlags flags = 0);
bool TreeNodeEx(const char *id, ImGuiTreeNodeFlags flags, const char *format, ...);
bool TreeNodeEx(const void *id, ImGuiTreeNodeFlags flags, const char *format, ...);
bool TreeNodeExV(const char *id, ImGuiTreeNodeFlags flags, const char *format, va_list args);
bool TreeNodeExV(const void *id, ImGuiTreeNodeFlags flags, const char *format, va_list args);

} // namespace imkit
