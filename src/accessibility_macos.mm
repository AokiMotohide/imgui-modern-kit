#include <imkit/accessibility_macos.h>
#import <Cocoa/Cocoa.h>
#include <unordered_map>

using namespace imkit::accessibility;

@interface IMKitAccessibilityElement : NSAccessibilityElement
@property(nonatomic) StableId stableId;
@property(nonatomic) SemanticAction semanticActions;
@property(nonatomic) NativeActionSink actionSink;
@property(nonatomic,strong) id publishedValue;
@property(nonatomic) BOOL publishedSelected;
@property(nonatomic) BOOL publishedExpanded;
@property(nonatomic) BOOL publishedFocused;
@end

@implementation IMKitAccessibilityElement
- (BOOL)dispatch:(SemanticAction)action value:(NSString*)value {
    NativeActionSink sink=self.actionSink;
    if(!sink.dispatch || !Supports(self.semanticActions,action)) return NO;
    const char *utf8=value ? value.UTF8String : "";
    return sink.dispatch(sink.user,self.stableId,action,std::string_view(utf8 ? utf8 : ""));
}
- (BOOL)accessibilityPerformPress { return [self dispatch:(Supports(self.semanticActions,SemanticAction::Toggle)?SemanticAction::Toggle:SemanticAction::Press) value:nil]; }
- (BOOL)accessibilityPerformIncrement { return [self dispatch:SemanticAction::Increment value:nil]; }
- (BOOL)accessibilityPerformDecrement { return [self dispatch:SemanticAction::Decrement value:nil]; }
- (id)accessibilityValue { return self.publishedValue; }
- (BOOL)isAccessibilitySelected { return self.publishedSelected; }
- (BOOL)isAccessibilityExpanded { return self.publishedExpanded; }
- (BOOL)isAccessibilityFocused { return self.publishedFocused; }
- (void)setAccessibilityFocused:(BOOL)focused { if(focused) [self dispatch:SemanticAction::Focus value:nil]; }
- (void)setAccessibilityValue:(id)value { [self dispatch:SemanticAction::SetValue value:[value description]]; }
- (void)setAccessibilitySelected:(BOOL)selected { if(selected) [self dispatch:SemanticAction::Select value:nil]; }
- (void)setAccessibilityExpanded:(BOOL)expanded { [self dispatch:(expanded?SemanticAction::Expand:SemanticAction::Collapse) value:nil]; }
@end

namespace imkit::accessibility {
namespace {
NSString *Text(std::string_view value) {
    return [[NSString alloc] initWithBytes:value.data() length:value.size() encoding:NSUTF8StringEncoding] ?: @"";
}
NSString *Role(SemanticRole role) {
    switch(role) {
    case SemanticRole::Button:return NSAccessibilityButtonRole;
    case SemanticRole::Toggle:return NSAccessibilityCheckBoxRole;
    case SemanticRole::Radio:return NSAccessibilityRadioButtonRole;
    case SemanticRole::ComboBox:return NSAccessibilityComboBoxRole;
    case SemanticRole::TextField:return NSAccessibilityTextFieldRole;
    case SemanticRole::Dialog:return NSAccessibilityDialogRole;
    case SemanticRole::Toolbar:return NSAccessibilityToolbarRole;
    case SemanticRole::TabList:return NSAccessibilityTabGroupRole;
    case SemanticRole::Tab:return NSAccessibilityRadioButtonRole;
    case SemanticRole::Tree:return NSAccessibilityOutlineRole;
    case SemanticRole::TreeItem:return NSAccessibilityRowRole;
    case SemanticRole::Grid:return NSAccessibilityTableRole;
    case SemanticRole::Row:return NSAccessibilityRowRole;
    case SemanticRole::Cell:return NSAccessibilityCellRole;
    case SemanticRole::Status:return NSAccessibilityStaticTextRole;
    case SemanticRole::Progress:return NSAccessibilityProgressIndicatorRole;
    case SemanticRole::Menu:return NSAccessibilityMenuRole;
    case SemanticRole::MenuItem:return NSAccessibilityMenuItemRole;
    case SemanticRole::Text:return NSAccessibilityStaticTextRole;
    default:return NSAccessibilityGroupRole;
    }
}
struct Bridge {
    __weak NSView *view=nil;
    NSArray *originalChildren=nil;
    NativeActionSink actions{};
    NSArray<IMKitAccessibilityElement*> *elements=@[];
};
}

bool MacOSAccessibilityAdapter::Attach(void *nsView,NativeActionSink actions) {
    if(bridge_ || !nsView) return false;
    NSView *view=(__bridge NSView*)nsView;
    auto *bridge=new Bridge; bridge->view=view; bridge->originalChildren=view.accessibilityChildren; bridge->actions=actions;
    bridge_=bridge; return true;
}

bool MacOSAccessibilityAdapter::Publish(const AccessibilityTree &tree) {
    if(!bridge_ || !tree.Validate()) return false;
    auto *bridge=static_cast<Bridge*>(bridge_); NSView *view=bridge->view; if(!view) return false;
    NSMutableArray<IMKitAccessibilityElement*> *elements=[NSMutableArray arrayWithCapacity:tree.nodes.size()];
    std::unordered_map<StableId,IMKitAccessibilityElement*> byId;
    for(const auto &node:tree.nodes) {
        auto *element=[IMKitAccessibilityElement new];
        element.accessibilityRole=Role(node.role);
        element.accessibilityFrame=NSMakeRect(node.minimum.x,node.minimum.y,node.maximum.x-node.minimum.x,node.maximum.y-node.minimum.y);
        element.accessibilityLabel=Text(node.name);
        element.stableId=node.id; element.semanticActions=node.actions; element.actionSink=bridge->actions;
        element.accessibilityHelp=Text(node.description); element.accessibilityEnabled=!node.state.disabled;
        element.publishedValue=Text(node.value); element.publishedSelected=node.state.selected;
        element.publishedExpanded=node.state.expanded; element.publishedFocused=node.state.focused;
        [elements addObject:element]; byId[node.id]=element;
    }
    NSMutableArray *roots=[NSMutableArray array];
    for(std::size_t i=0;i<tree.nodes.size();++i) {
        const auto &node=tree.nodes[i]; auto *element=elements[i];
        if(node.parent==0 || !byId.contains(node.parent)) { element.accessibilityParent=view; [roots addObject:element]; continue; }
        auto *parent=byId[node.parent]; element.accessibilityParent=parent;
        NSMutableArray *children=[(parent.accessibilityChildren ?: @[]) mutableCopy]; [children addObject:element]; parent.accessibilityChildren=children;
    }
    bridge->elements=[elements copy]; view.accessibilityChildren=roots;
    NSAccessibilityPostNotification(view,NSAccessibilityLayoutChangedNotification); return true;
}

void MacOSAccessibilityAdapter::Detach() {
    if(!bridge_) return; auto *bridge=static_cast<Bridge*>(bridge_);
    if(NSView *view=bridge->view) { view.accessibilityChildren=bridge->originalChildren; NSAccessibilityPostNotification(view,NSAccessibilityLayoutChangedNotification); }
    delete bridge; bridge_=nullptr;
}
} // namespace imkit::accessibility
