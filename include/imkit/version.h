#pragma once
#include <imgui.h>
#define IMKIT_VERSION "2.0.0"
#if IMGUI_VERSION_NUM != 19291 && !(defined(IMKIT_WINDOW_FRAME_LEGACY_IMGUI) && IMGUI_VERSION_NUM == 18814)
#error "ImKit 2.0 requires Dear ImGui 1.92.9b. Use the pinned docking version and rebuild from source."
#endif
#if !defined(IMGUI_HAS_DOCK) && !defined(IMKIT_WINDOW_FRAME_LEGACY_IMGUI)
#error "ImKit 2.0 requires Dear ImGui docking."
#endif
