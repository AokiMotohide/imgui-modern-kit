#pragma once
// Hosts that link Dear ImGui via a DLL without propagating IMGUI_API=__declspec(dllimport)
// still resolve ImGui::GetCurrentContext() as a function, while the GImGui data symbol cannot
// be imported. Route GImGui through GetCurrentContext() before including imgui_internal.h.
#include <imgui.h>
#ifndef GImGui
#define GImGui (ImGui::GetCurrentContext())
#endif
#include <imgui_internal.h>
