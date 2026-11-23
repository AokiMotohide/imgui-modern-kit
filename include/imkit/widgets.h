#pragma once

#include <cstddef>

#include <imkit/part_style.h>
#include <imkit/version.h>

namespace imkit {
struct Theme;

bool Button(const char *label, const ImVec2 &size = ImVec2(0, 0));
bool Button(const char *label, const ImVec2 &size, const ButtonStyle &style);
bool SmallButton(const char *label);
bool SmallButton(const char *label, const ButtonStyle &style);

bool Checkbox(const char *label, bool *value);
bool Checkbox(const char *label, bool *value, const CheckboxStyle &style);
// Compact check mark and radio: the square follows the font, not the 30 px control height.
bool RadioButton(const char *label, bool active);
bool RadioButton(const char *label, bool active, const RadioStyle &style);
bool RadioButton(const char *label, int *value, int buttonValue);
bool RadioButton(const char *label, int *value, int buttonValue, const RadioStyle &style);

bool SliderFloat(const char *label, float *value, float minimum, float maximum, const char *format = "%.3f",
                 ImGuiSliderFlags flags = 0);
bool SliderFloat(const char *label, float *value, float minimum, float maximum, const char *format,
                 ImGuiSliderFlags flags, const SliderStyle &style);
// Fill-style sliders: the value is shown as a tinted bar inside the frame instead of a handle that
// overlaps the value text. Input behaviour is ImGui's own; Logarithmic sliders keep the native grab.
bool SliderInt(const char *label, int *value, int minimum, int maximum, const char *format = "%d",
               ImGuiSliderFlags flags = 0);
bool SliderInt(const char *label, int *value, int minimum, int maximum, const char *format, ImGuiSliderFlags flags,
               const SliderStyle &style);
bool VSliderFloat(const char *label, const ImVec2 &size, float *value, float minimum, float maximum,
                  const char *format = "%.3f", ImGuiSliderFlags flags = 0);
bool VSliderFloat(const char *label, const ImVec2 &size, float *value, float minimum, float maximum,
                  const char *format, ImGuiSliderFlags flags, const SliderStyle &style);

bool SliderFloat2(const char *label, float v[2], float minimum, float maximum, const char *format = "%.3f",
                  ImGuiSliderFlags flags = 0);
bool SliderFloat2(const char *label, float v[2], float minimum, float maximum, const char *format,
                  ImGuiSliderFlags flags, const SliderStyle &style);
bool SliderFloat3(const char *label, float v[3], float minimum, float maximum, const char *format = "%.3f",
                  ImGuiSliderFlags flags = 0);
bool SliderFloat3(const char *label, float v[3], float minimum, float maximum, const char *format,
                  ImGuiSliderFlags flags, const SliderStyle &style);
bool SliderFloat4(const char *label, float v[4], float minimum, float maximum, const char *format = "%.3f",
                  ImGuiSliderFlags flags = 0);
bool SliderFloat4(const char *label, float v[4], float minimum, float maximum, const char *format,
                  ImGuiSliderFlags flags, const SliderStyle &style);
bool SliderInt2(const char *label, int v[2], int minimum, int maximum, const char *format = "%d",
                ImGuiSliderFlags flags = 0);
bool SliderInt2(const char *label, int v[2], int minimum, int maximum, const char *format, ImGuiSliderFlags flags,
                const SliderStyle &style);
bool SliderInt3(const char *label, int v[3], int minimum, int maximum, const char *format = "%d",
                ImGuiSliderFlags flags = 0);
bool SliderInt3(const char *label, int v[3], int minimum, int maximum, const char *format, ImGuiSliderFlags flags,
                const SliderStyle &style);
bool SliderInt4(const char *label, int v[4], int minimum, int maximum, const char *format = "%d",
                ImGuiSliderFlags flags = 0);
bool SliderInt4(const char *label, int v[4], int minimum, int maximum, const char *format, ImGuiSliderFlags flags,
                const SliderStyle &style);

bool DragFloat(const char *label, float *value, float speed = 1.0f, float minimum = 0.0f, float maximum = 0.0f,
               const char *format = "%.3f", ImGuiSliderFlags flags = 0);
bool DragFloat(const char *label, float *value, float speed, float minimum, float maximum, const char *format,
               ImGuiSliderFlags flags, const DragStyle &style);
bool DragInt(const char *label, int *value, float speed = 1.0f, int minimum = 0, int maximum = 0,
             const char *format = "%d", ImGuiSliderFlags flags = 0);
bool DragInt(const char *label, int *value, float speed, int minimum, int maximum, const char *format,
             ImGuiSliderFlags flags, const DragStyle &style);

// Combo with a chevron instead of the stock arrow box; the preview is shortened with an ellipsis.
// Flags that change the frame width (NoArrowButton, NoPreview, WidthFitPreview) use the stock drawing.
bool BeginCombo(const char *label, const char *preview, ImGuiComboFlags flags = 0);
bool BeginCombo(const char *label, const char *preview, ImGuiComboFlags flags, const ComboStyle &style);
void EndCombo();
bool Combo(const char *label, int *current, const char *const items[], int count, int heightInItems = -1);
bool Combo(const char *label, int *current, const char *const items[], int count, int heightInItems,
           const ComboStyle &style);
bool Combo(const char *label, int *current, const char *zeroSeparated, int heightInItems = -1);
bool Combo(const char *label, int *current, const char *zeroSeparated, int heightInItems, const ComboStyle &style);
bool Combo(const char *label, int *current, const char *(*getter)(void *user, int index), void *user, int count,
           int heightInItems = -1);
bool Combo(const char *label, int *current, const char *(*getter)(void *user, int index), void *user, int count,
           int heightInItems, const ComboStyle &style);

bool InputText(const char *label, char *buffer, std::size_t bufferSize, ImGuiInputTextFlags flags = 0,
               ImGuiInputTextCallback callback = nullptr, void *userData = nullptr);
bool InputText(const char *label, char *buffer, std::size_t bufferSize, ImGuiInputTextFlags flags,
               ImGuiInputTextCallback callback, void *userData, const InputStyle &style);
bool InputFloat(const char *label, float *value, float step = 0.0f, float stepFast = 0.0f,
                const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
bool InputFloat(const char *label, float *value, float step, float stepFast, const char *format,
                ImGuiInputTextFlags flags, const InputStyle &style);
bool InputInt(const char *label, int *value, int step = 1, int stepFast = 100, ImGuiInputTextFlags flags = 0);
bool InputInt(const char *label, int *value, int step, int stepFast, ImGuiInputTextFlags flags,
              const InputStyle &style);

bool Selectable(const char *label, bool selected = false, ImGuiSelectableFlags flags = 0,
                const ImVec2 &size = ImVec2(0, 0));
bool Selectable(const char *label, bool selected, ImGuiSelectableFlags flags, const ImVec2 &size,
                const SelectableStyle &style);
bool Selectable(const char *label, bool *selected, ImGuiSelectableFlags flags = 0,
                const ImVec2 &size = ImVec2(0, 0));
bool Selectable(const char *label, bool *selected, ImGuiSelectableFlags flags, const ImVec2 &size,
                const SelectableStyle &style);

bool CollapsingHeader(const char *label, ImGuiTreeNodeFlags flags = 0);
bool CollapsingHeader(const char *label, ImGuiTreeNodeFlags flags, const HeaderStyle &style);
bool CollapsingHeader(const char *label, bool *visible, ImGuiTreeNodeFlags flags = 0);
bool CollapsingHeader(const char *label, bool *visible, ImGuiTreeNodeFlags flags, const HeaderStyle &style);

bool ColorEdit3(const char *label, float col[3], ImGuiColorEditFlags flags = 0);
bool ColorEdit3(const char *label, float col[3], ImGuiColorEditFlags flags, const ColorEditStyle &style);
bool ColorEdit4(const char *label, float col[4], ImGuiColorEditFlags flags = 0);
bool ColorEdit4(const char *label, float col[4], ImGuiColorEditFlags flags, const ColorEditStyle &style);

void ProgressBar(float fraction, const ImVec2 &size = ImVec2(-1, 0), const char *overlay = nullptr);
void ProgressBar(float fraction, const ImVec2 &size, const char *overlay, const ProgressStyle &style);

bool BeginTabItem(const char *label, bool *open = nullptr, ImGuiTabItemFlags flags = 0);
bool BeginTabItem(const char *label, bool *open, ImGuiTabItemFlags flags, const TabStyle &style);
bool TreeNodeEx(const char *label, ImGuiTreeNodeFlags flags = 0);
bool TreeNodeEx(const char *label, ImGuiTreeNodeFlags flags, const TreeStyle &style);
bool TreeNodeEx(const char *id, ImGuiTreeNodeFlags flags, const char *format, ...);
bool TreeNodeEx(const void *id, ImGuiTreeNodeFlags flags, const char *format, ...);
bool TreeNodeExV(const char *id, ImGuiTreeNodeFlags flags, const char *format, va_list args);
bool TreeNodeExV(const void *id, ImGuiTreeNodeFlags flags, const char *format, va_list args);

void SetItemTooltip(const char *format, ...) IM_FMTARGS(1);
void SetItemTooltipV(const char *format, va_list args) IM_FMTLIST(1);
void SetItemTooltip(const PartStyle &style, const char *format, ...) IM_FMTARGS(2);

// Interactive style inspector and editor for the four customisation layers.
void ShowStyleEditor(Theme *theme = nullptr, bool *open = nullptr);

} // namespace imkit
