#pragma once
#include <imkit/patterns.h>
#include <imkit/icons.h>

namespace imkit {
enum class Orientation { Horizontal, Vertical };
enum class FeedbackKind { Info, Success, Warning, Error };
struct StepItem {
    StableId id=0;
    const char* label="";
    const char* description="";
    bool completed=false, available=true, disabled=false;
    FeedbackKind status=FeedbackKind::Info;
    IconId icon=IconId::Count;
};
struct StepNavigatorState { StableId focused=0; bool focusPending=false; };
struct StepNavigatorOptions {
    Orientation orientation=Orientation::Horizontal;
    ImVec2 size{};
    float itemExtent=0;
    const IconAtlas* icons=nullptr;
    bool numbers=true;
};
// Selection is a request; current and completion are never mutated.
StableId StepNavigator(const char* id, std::span<const StepItem> items, StableId current,
                      StepNavigatorState& state, StepNavigatorOptions layout={}, ComponentOptions options={});
StableId NavigationRail(const char* id, std::span<const StepItem> items, StableId current,
                        StepNavigatorState& state, ImVec2 size={}, ComponentOptions options={});
// Groups reference contiguous item ranges; selecting the current group retains current.
struct StepGroup {
    StableId id=0;
    const char* label="";
    std::size_t first=0, count=0;
    // Alpha <= 0 uses the active theme accent.
    ImVec4 accent{};
};
StableId GroupedStepNavigator(const char* id, std::span<const StepGroup> groups,
    std::span<const StepItem> items, StableId current, ComponentOptions options={});
struct IconToolbarItem {
    StableId id=0;
    IconId icon=IconId::Count;
    const char* label="";
    const char* description="";
    bool selected=false, mixed=false, disabled=false;
};
struct IconToolbarOptions {
    bool showLabels=false;
    bool wrap=true;
};
// Wrapping is based on actual available width. The host owns textures and state.
StableId IconToolbar(const char* id, const IconAtlas& atlas,
    std::span<const IconToolbarItem> items, ComponentOptions options={});
StableId IconToolbar(const char* id, const IconAtlas& atlas,
    std::span<const IconToolbarItem> items, IconToolbarOptions layout,
    ComponentOptions options={});
struct WorkspaceTab {
    StableId id=0;
    const char* label="";
    const char* description="";
    IconId icon=IconId::Count;
    bool disabled=false;
};
// Returns a selection request. The host owns the selected tab and its content.
StableId WorkspaceTabs(const char* id, std::span<const WorkspaceTab> tabs,
                       StableId selected, const IconAtlas* icons=nullptr,
                       ComponentOptions options={});

struct HierarchyRowView {
    StableId id=0;
    const char* label="";
    const char* detail="";
    IconId icon=IconId::Count;
    int depth=0;
    bool selected=false, visible=true, locked=false, disabled=false;
    const char* visibleLabel="Visible";
    const char* hiddenLabel="Hidden";
    const char* lockedLabel="Locked";
    const char* unlockedLabel="Unlocked";
    const char* moreLabel="More actions";
};
enum class HierarchyRowAction { None, Select, ToggleVisibility, ToggleLock, More };
// Header open state is host-owned; a row only returns a requested action.
bool HierarchyGroupHeader(const char* id, const char* label, int count,
                          bool* open, const IconAtlas* icons=nullptr,
                          IconId icon=IconId::Count, ComponentOptions options={});
HierarchyRowAction HierarchyRow(const char* id, const HierarchyRowView& row,
                               const IconAtlas* icons=nullptr,
                               ComponentOptions options={});

// Always pair EndInspectorCard with BeginInspectorCard, including a false return.
bool BeginInspectorCard(const char* id, const char* title, const char* description="",
                        const IconAtlas* icons=nullptr, IconId icon=IconId::Count,
                        ComponentOptions options={});
void EndInspectorCard();
// Returns a toggle request; current remains owned by the host.
bool SettingToggleRow(const char* id, const char* label, const char* description,
                      bool current, bool disabled=false, const char* disabledReason="",
                      ComponentOptions options={});
bool FilterChip(const char* id, const char* label, bool selected, ComponentOptions options={});

struct FeedbackView {
    StableId id=0;
    const char* title="";
    const char* description="";
    FeedbackKind kind=FeedbackKind::Info;
    double expiresAt=0;
    int priority=0;
    bool dismissible=true;
};
struct RequestBuffer {
    std::span<StableId> storage{};
    std::size_t count=0;
    bool overflow=false;
    bool Push(StableId id);
};
// Last occurrence of a duplicate ID wins. Scratch contains sorted visible indices.
std::size_t SelectNotifications(std::span<const FeedbackView> items, double now,
                                std::span<std::size_t> scratch, std::size_t maximum);
bool NotificationCard(const FeedbackView& item, double now, ComponentOptions options={});
struct ToastOptions { std::size_t maximum=4; float width=0; };
void ToastRegion(const char* id, std::span<const FeedbackView> items, double now,
                 std::span<std::size_t> scratch, RequestBuffer& dismiss,
                 ToastOptions layout={}, ComponentOptions options={});
bool InlineAlert(const char* id, const FeedbackView& item, ComponentOptions options={});
bool PersistentBanner(const char* id, const FeedbackView& item, ComponentOptions options={});

struct StateView {
    const char* heading="";
    const char* description="";
    const char* action="";
    FeedbackKind kind=FeedbackKind::Info;
    IconId icon=IconId::Count;
    const IconAtlas* icons=nullptr;
    bool disabled=false;
};
bool EmptyState(const char* id, const StateView& view, ComponentOptions options={});
bool UnavailableState(const char* id, const StateView& view, ComponentOptions options={});
bool RetryState(const char* id, const StateView& view, ComponentOptions options={});
enum class ProgressPresentation { Inline, Overlay, Modal };
struct ProgressView {
    float fraction=-1;
    const char* stage="";
    const char* description="";
    bool cancellable=false;
};
struct CircularProgressView {
    // Negative or non-finite values render as unavailable. Finite values are clamped to [0, 1].
    float fraction=-1;
    const char* value="";
    const char* label="";
    FeedbackKind kind=FeedbackKind::Info;
};
struct CircularProgressOptions {
    // Zero uses three frame heights. Stroke width zero derives from the resolved diameter.
    float diameter=0;
    float strokeWidth=0;
};
// Draw-only progress. Text is borrowed for the current frame; the host owns values and state.
void CircularProgress(const char* id, const CircularProgressView& view,
                      CircularProgressOptions layout={}, ComponentOptions options={});
// Modal open state is explicit. Overlay occupies/clips to the requested child region.
bool Progress(const char* id, const ProgressView& view, ProgressPresentation presentation,
              DialogState& state, ImVec2 size={}, ComponentOptions options={});
// Always pair EndCard, including a false return (same contract as BeginChild).
bool BeginCard(const char* id, ImVec2 size={}, ComponentOptions options={});
void EndCard();
bool SectionHeader(const char* id, const char* label, bool& open, ComponentOptions options={});
StableId MultiSelectionBar(const char* id, std::size_t count, std::span<const Command> actions,
                           ToolbarState& state, ComponentOptions options={});
void HelpCallout(const char* id, const StateView& view, ComponentOptions options={});
StableId ValidationSummary(const char* id, std::span<const StepItem> issues, ComponentOptions options={});
struct ToolbarOptions { bool overflow=true; bool iconOnly=false; const IconAtlas* icons=nullptr; std::span<const IconId> iconsByCommand{}; };
StableId ResponsiveToolbar(const char* id, ToolbarState& state, std::span<const Command> commands,
                           ToolbarOptions layout, ComponentOptions options={});
} // namespace imkit
