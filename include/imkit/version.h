#pragma once
#include <imgui.h>
#define IMKIT_VERSION "3.0.0"
#define IMKIT_IMGUI_COMMIT "367b2c24f399988ddafc0bb4628da0106bcc09be"
#if IMGUI_VERSION_NUM != 19297
#error "ImKit 3.0 requires the pinned Dear ImGui 1.93.0 WIP docking revision."
#endif
#if !defined(IMGUI_HAS_DOCK) || !defined(IMGUI_HAS_VIEWPORT) || !defined(IMGUI_HAS_TEXTURES)
#error "ImKit 3.0 requires Dear ImGui docking, multi-viewport and dynamic texture support."
#endif
