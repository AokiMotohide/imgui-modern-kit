#pragma once
#include <imgui.h>
#define IMKIT_VERSION "0.2.0"
#if IMGUI_VERSION_NUM != 19291
#error "ImKit 0.2 requires Dear ImGui 1.92.9b. Use the pinned docking version and rebuild from source."
#endif
#ifndef IMGUI_HAS_DOCK
#error "ImKit 0.2 requires Dear ImGui docking."
#endif
