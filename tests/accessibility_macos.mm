#include <imkit/accessibility_macos.h>
#import <Cocoa/Cocoa.h>
#include <array>

int main() {
    @autoreleasepool {
        NSView *view=[[NSView alloc] initWithFrame:NSMakeRect(0,0,320,200)];
        bool dispatched=false;
        imkit::accessibility::NativeActionSink sink{&dispatched,[](void *user,auto,auto,std::string_view){*static_cast<bool*>(user)=true;return true;}};
        imkit::accessibility::MacOSAccessibilityAdapter adapter;
        if(!adapter.Attach((__bridge void*)view,sink)) return 1;
        std::array<imkit::accessibility::SemanticNode,1> nodes{};
        nodes[0].id=1;nodes[0].role=imkit::accessibility::SemanticRole::Button;nodes[0].name="Apply";
        nodes[0].actions=imkit::accessibility::SemanticAction::Press;nodes[0].maximum={100,40};
        imkit::accessibility::AccessibilityTree tree{nodes,1,false};
        if(!adapter.Publish(tree) || view.accessibilityChildren.count!=1) return 2;
        if(![view.accessibilityChildren.firstObject accessibilityPerformPress] || !dispatched) return 3;
        adapter.Detach(); return 0;
    }
}
