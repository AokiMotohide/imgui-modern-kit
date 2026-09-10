#include <imkit/accessibility.h>
#include <algorithm>

namespace imkit::accessibility {
const SemanticNode* AccessibilityTree::Find(StableId id) const {
    for (const auto& node:nodes) if(node.id==id) return &node;
    return nullptr;
}
bool AccessibilityTree::Validate() const {
    if(overflow) return false;
    for(std::size_t i=0;i<nodes.size();++i) {
        const auto& n=nodes[i];
        if(!n.id || n.parent==n.id || (n.parent && !Find(n.parent)) ||
           (n.labelledBy && !Find(n.labelledBy))) return false;
        for(std::size_t j=0;j<i;++j) if(nodes[j].id==n.id) return false;
        auto parent=n.parent;
        for(std::size_t depth=0;parent;++depth) {
            if(depth>=nodes.size()) return false;
            auto p=Find(parent); if(!p) return false; parent=p->parent;
        }
        for(auto child:n.children) { auto p=Find(child); if(!p || p->parent!=n.id) return false; }
    }
    return true;
}
bool ActionQueue::Push(ActionRequest request) {
    if(!request.id || request.action==SemanticAction::None || count_==storage_.size()) return false;
    storage_[count_++]=request; return true;
}
bool ActionQueue::Take(StableId id, SemanticAction action, ActionRequest* result) {
    for(std::size_t i=0;i<count_;++i) if(storage_[i].id==id && storage_[i].action==action) {
        if(result) *result=storage_[i];
        std::move(storage_.begin()+i+1,storage_.begin()+count_,storage_.begin()+i);
        --count_; return true;
    }
    return false;
}
void AccessibilityFrame::Begin(std::uint64_t generation) { count_=0; overflow_=false; generation_=generation; }
bool AccessibilityFrame::Add(const SemanticNode& node) {
    if(count_==storage_.size()) { overflow_=true; return false; }
    storage_[count_++]=node; return true;
}
AccessibilityTree AccessibilityFrame::Tree() const { return {storage_.first(count_),generation_,overflow_}; }
void AccessibilityFrame::Publish(AccessibilitySink sink) const { if(sink.publish) sink.publish(sink.user,Tree()); }
bool AccessibilityFrame::Take(StableId id, SemanticAction action) { return actions_ && actions_->Take(id,action); }
bool AnnotateLastItem(AccessibilityFrame& frame, SemanticNode node) {
    if(!node.id) node.id=ImGui::GetItemID();
    node.minimum=ImGui::GetItemRectMin(); node.maximum=ImGui::GetItemRectMax();
    node.state.focused=ImGui::IsItemFocused();
    node.state.disabled=node.state.disabled || (ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
    if(node.state.disabled) node.actions=SemanticAction::None;
    return frame.Add(node);
}
}
