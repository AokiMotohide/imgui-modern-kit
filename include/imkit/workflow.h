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
struct StepGroup { StableId id=0; const char* label=""; std::size_t first=0, count=0; };
StableId GroupedStepNavigator(const char* id, std::span<const StepGroup> groups,
    std::span<const StepItem> items, StableId current, ComponentOptions options={});
struct IconToolbarItem {
    StableId id=0;
    IconId icon=IconId::Count;
    const char* label="";
    const char* description="";
    bool selected=false, mixed=false, disabled=false;
};
// Wrapping is based on actual available width. The host owns textures and state.
StableId IconToolbar(const char* id, const IconAtlas& atlas,
    std::span<const IconToolbarItem> items, ComponentOptions options={});
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
