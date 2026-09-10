#pragma once
#include <imkit/version.h>
#include <cstdint>
#include <span>
#include <string_view>

namespace imkit::accessibility {
using StableId = std::uint64_t;
enum class SemanticRole { Group, Button, Toggle, Radio, ComboBox, TextField, Dialog, Toolbar,
    TabList, Tab, Tree, TreeItem, Grid, Row, Cell, Status, Alert, Progress, Menu, MenuItem, Text };
enum class SemanticAction : unsigned { None=0, Press=1, Toggle=2, Increment=4, Decrement=8,
    Focus=16, Expand=32, Collapse=64, SetValue=128, Select=256, Dismiss=512 };
constexpr SemanticAction operator|(SemanticAction a, SemanticAction b) {
    return static_cast<SemanticAction>(static_cast<unsigned>(a)|static_cast<unsigned>(b));
}
constexpr bool Supports(SemanticAction set, SemanticAction action) {
    return action != SemanticAction::None &&
        (static_cast<unsigned>(set)&static_cast<unsigned>(action))==static_cast<unsigned>(action);
}
struct SemanticState {
    bool disabled=false, focused=false, selected=false, checked=false, mixed=false;
    bool expanded=false, expandable=false, readOnly=false, required=false, invalid=false, busy=false;
};
// All text and children are borrowed until the end of the host's frame publication.
struct SemanticNode {
    StableId id=0, parent=0, labelledBy=0;
    SemanticRole role=SemanticRole::Group;
    std::string_view name{}, description{}, value{};
    SemanticState state{};
    ImVec2 minimum{}, maximum{}; // Desktop screen coordinates, like ImGui item rectangles.
    std::span<const StableId> children{};
    SemanticAction actions=SemanticAction::None;
};
struct AccessibilityTree {
    std::span<const SemanticNode> nodes{};
    std::uint64_t generation=0;
    bool overflow=false;
    const SemanticNode* Find(StableId id) const;
    bool Validate() const;
};
struct AccessibilitySink {
    void* user=nullptr;
    void (*publish)(void*, const AccessibilityTree&)=nullptr;
};
struct ActionRequest { StableId id=0; SemanticAction action=SemanticAction::None; std::string_view value{}; };
// Host serializes access, including requests from a native accessibility adapter.
class ActionQueue {
public:
    explicit ActionQueue(std::span<ActionRequest> storage): storage_(storage) {}
    bool Push(ActionRequest request);
    bool Take(StableId id, SemanticAction action, ActionRequest* result=nullptr);
    void Clear() { count_=0; }
    std::size_t Size() const { return count_; }
private:
    std::span<ActionRequest> storage_;
    std::size_t count_=0;
};
// No current-frame singleton. Pass this explicitly to component options.
class AccessibilityFrame {
public:
    explicit AccessibilityFrame(std::span<SemanticNode> storage, ActionQueue* actions=nullptr)
        : storage_(storage), actions_(actions) {}
    void Begin(std::uint64_t generation);
    bool Add(const SemanticNode& node);
    AccessibilityTree Tree() const;
    void Publish(AccessibilitySink sink) const;
    bool Take(StableId id, SemanticAction action);
private:
    std::span<SemanticNode> storage_;
    ActionQueue* actions_;
    std::size_t count_=0;
    std::uint64_t generation_=0;
    bool overflow_=false;
};
// Attach to the immediately preceding public ImGui item. Never submits another item.
bool AnnotateLastItem(AccessibilityFrame& frame, SemanticNode node);
} // namespace imkit::accessibility
