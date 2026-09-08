#pragma once

#include <cstddef>

#include <imkit/version.h>

namespace imkit {

bool Button(const char *label, const ImVec2 &size = ImVec2(0, 0));

bool Checkbox(const char *label, bool *value);

bool SliderFloat(const char *label, float *value, float minimum, float maximum, const char *format = "%.3f",
                 ImGuiSliderFlags flags = 0);

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
