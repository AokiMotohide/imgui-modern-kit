#pragma once

#include <imkit/workflow.h>

namespace imkit {

struct AppBarView {
    const char* product="";
    const char* document="";
    const char* context="";
    const char* status="";
    bool dirty=false;
    FeedbackKind statusKind=FeedbackKind::Info;
    std::span<const Command> actions{};
};

struct WorkspaceHeaderView {
    const char* eyebrow="";
    const char* title="";
    const char* description="";
    const char* status="";
    FeedbackKind statusKind=FeedbackKind::Info;
    std::span<const Command> actions{};
};

struct BottomActionBarView {
    const char* status="";
    FeedbackKind statusKind=FeedbackKind::Info;
    std::span<const Command> actions{};
};

struct DiagnosticsDrawerState {
    bool open=false;
    bool focusPending=false;
};

struct ThemePickerState {
    char search[64]{};
    int focused=0;
};

// All actions are requests. The host owns document, status and command state.
StableId AppBar(const char* id, const AppBarView& view, ToolbarState& state,
                ComponentOptions options={});
StableId WorkspaceHeader(const char* id, const WorkspaceHeaderView& view,
                         ToolbarState& state, ComponentOptions options={});
bool InspectorSection(const char* id, const char* label, const char* description,
                      bool& open, ComponentOptions options={});
bool AdvancedSection(const char* id, const char* label, bool& open,
                     ComponentOptions options={});
StableId BottomActionBar(const char* id, const BottomActionBarView& view,
                         ToolbarState& state, ComponentOptions options={});

// Pair EndDiagnosticsDrawer only when BeginDiagnosticsDrawer returns true.
bool BeginDiagnosticsDrawer(const char* id, const char* title,
                            DiagnosticsDrawerState& state, ImVec2 size={},
                            ComponentOptions options={});
void EndDiagnosticsDrawer();

// Theme selection is host-owned and expressed as a stable ThemePreset value.
bool ThemePicker(const char* id, ThemePreset* selected, ThemePickerState& state,
                 ComponentOptions options={});

} // namespace imkit
