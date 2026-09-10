#include <imkit/editor_canvas.h>
#include <algorithm>
#include <cmath>

namespace imkit::editor {
namespace {
bool Positive(double n) {return std::isfinite(n)&&n>0;}
float FiniteExtent(float n,float fallback) {return std::isfinite(n)&&n>0?n:fallback;}
ImVec2 ScreenPoint(Point p,const CanvasState& s,const CanvasView& v) {auto q=ToScreen(p,s,{v.min.x,v.min.y});return {static_cast<float>(q.x),static_cast<float>(q.y)};}
void Push(StableId id) {ImGui::PushID(static_cast<int>(id>>32));ImGui::PushID(static_cast<int>(id));}
void Pop() {ImGui::PopID();ImGui::PopID();}
const char* Safe(const char* s) {return s?s:"";}
}
bool ImageGeometryValid(Point pixels,Point viewport) {return Positive(pixels.x)&&Positive(pixels.y)&&Positive(viewport.x)&&Positive(viewport.y);}
void FitImage(CanvasState& s,Point pixels,Point viewport,ImageScaleMode mode) {
    if(!ImageGeometryValid(pixels,viewport)) {s.origin={};s.scale={1,1};return;}
    double scale=1;
    if(mode==ImageScaleMode::Fit) scale=std::min(viewport.x/pixels.x,viewport.y/pixels.y);
    else if(mode==ImageScaleMode::Fill) scale=std::max(viewport.x/pixels.x,viewport.y/pixels.y);
    else if(mode==ImageScaleMode::Manual) return;
    s.scale={scale,scale};s.origin={(pixels.x-viewport.x/scale)*.5,(pixels.y-viewport.y/scale)*.5};
}
void ClampImage(CanvasState& s,Point pixels,Point viewport) {
    if(!ImageGeometryValid(pixels,viewport)) return;
    if(!Positive(s.scale.x)||!Positive(s.scale.y)) s.scale={1,1};
    auto clamp=[](double origin,double extent,double visible) {return visible>=extent?(extent-visible)*.5:std::clamp(std::isfinite(origin)?origin:0.,0.,extent-visible);};
    s.origin={clamp(s.origin.x,pixels.x,viewport.x/s.scale.x),clamp(s.origin.y,pixels.y,viewport.y/s.scale.y)};
}
Point PixelToNormalized(Point p,Point pixels) {return Positive(pixels.x)&&Positive(pixels.y)?Point{p.x/pixels.x,p.y/pixels.y}:Point{};}
Point NormalizedToPixel(Point p,Point pixels) {return Positive(pixels.x)&&Positive(pixels.y)?Point{p.x*pixels.x,p.y*pixels.y}:Point{};}
CanvasView BeginImageViewport(const char* id,const ImageView& image,ImageViewportState& s,ImVec2 size,const Theme& t,ImageViewportOptions options,ComponentOptions components) {
    if(!Positive(s.canvas.scale.x)||!Positive(s.canvas.scale.y)) s.canvas.scale={1,1};
    const auto savedOrigin=s.canvas.origin,savedScale=s.canvas.scale;
    const bool wheel=s.canvas.wheelZoom;
    // Image zoom is uniform even when Shift is held; the general Canvas remains unchanged.
    s.canvas.wheelZoom=false;
    auto view=BeginCanvas(id,s.canvas,size,t);s.canvas.wheelZoom=wheel;
    Point extent{std::max(0.f,view.max.x-view.min.x),std::max(0.f,view.max.y-view.min.y)};
    const bool disabled=options.disabled || (ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
    if(disabled) {s.canvas.origin=savedOrigin;s.canvas.scale=savedScale;view.hovered=false;}
    bool geometryChanged=extent.x!=s.previousSize.x||extent.y!=s.previousSize.y||image.pixels.x!=s.previousPixels.x||image.pixels.y!=s.previousPixels.y;
    if(s.reset || (geometryChanged && s.mode!=ImageScaleMode::Manual)) {FitImage(s.canvas,image.pixels,extent,s.mode);s.reset=false;}
    auto& io=ImGui::GetIO();
    if(!disabled && view.hovered && wheel && io.MouseWheel && !ImGui::IsAnyItemActive()) {
        double minimum=Positive(options.minimumZoom)?options.minimumZoom:.01;
        double maximum=Positive(options.maximumZoom)?std::max(minimum,options.maximumZoom):256;
        double target=std::clamp(s.canvas.scale.x*std::pow(1.15,io.MouseWheel),minimum,maximum);
        double factor=target/s.canvas.scale.x;
        ZoomAt(s.canvas,{io.MousePos.x-view.min.x,io.MousePos.y-view.min.y},{factor,factor});s.mode=ImageScaleMode::Manual;
    }
    if(!disabled && view.hovered && (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)||io.MouseWheelH)) s.mode=ImageScaleMode::Manual;
    if(options.clamp) ClampImage(s.canvas,image.pixels,extent);
    s.previousSize=extent;s.previousPixels=image.pixels;
    view.visible=VisibleRange(s.canvas,extent);
    auto* draw=ImGui::GetWindowDrawList();
    draw->AddRectFilled(view.min,view.max,ImGui::GetColorU32(t.semantic.canvas));
    if(options.checkerboard && extent.x>0 && extent.y>0) {
        // Bounded by the visible screen area, independent of source resolution.
        float step=std::max(t.spacing.steps[4],std::max(static_cast<float>(extent.x),static_cast<float>(extent.y))/128);
        for(float y=view.min.y;y<view.max.y;y+=step) for(float x=view.min.x;x<view.max.x;x+=step) {
            int parity=static_cast<int>((x-view.min.x)/step+.5f)+static_cast<int>((y-view.min.y)/step+.5f);
            if(parity%2) draw->AddRectFilled({x,y},{std::min(x+step,view.max.x),std::min(y+step,view.max.y)},ImGui::GetColorU32(t.semantic.surface));
        }
    }
    if(ImageGeometryValid(image.pixels,extent) && image.texture.GetTexID()!=ImTextureID{})
        draw->AddImage(image.texture,ScreenPoint({},s.canvas,view),ScreenPoint(image.pixels,s.canvas,view),image.uv0,image.uv1);
    if(components.accessibility) {
        accessibility::SemanticNode node;node.id=ImGui::GetID("image-viewport");node.parent=components.parent;node.role=accessibility::SemanticRole::Group;
        node.name=components.locale?components.locale->Text("image_viewport","Image viewport"):"Image viewport";
        node.minimum=view.min;node.maximum=view.max;node.state.disabled=disabled;components.accessibility->Add(node);
    }
    return view;
}
void EndImageViewport() {EndCanvas();}
void ZoomToolbar(const char* id,ImageViewportState& s,ComponentOptions o) {
    auto text=[&](const char* key,const char* fallback){return o.locale?o.locale->Text(key,fallback):fallback;};
    const imkit::Command commands[]={{1,text("fit","Fit")},{2,text("fill","Fill")},{3,text("actual_size","1:1")},
        {4,text("zoom_out","Zoom out")},{5,text("zoom_in","Zoom in")},{6,text("pan_left","Pan left")},
        {7,text("pan_right","Pan right")},{8,text("pan_up","Pan up")},{9,text("pan_down","Pan down")}};
    auto action=ResponsiveToolbar(id,s.toolbar,commands,ToolbarOptions{},o);if(!action)return;
    int i=static_cast<int>(action)-1;
    if(i<3) {s.mode=static_cast<ImageScaleMode>(i);s.reset=true;}
    else if(i<5) {double target=std::clamp(s.canvas.scale.x*(i==3?1/1.2:1.2),.01,256.);double factor=target/s.canvas.scale.x;ZoomAt(s.canvas,{s.previousSize.x*.5,s.previousSize.y*.5},{factor,factor});s.mode=ImageScaleMode::Manual;}
    else {double delta=ImGui::GetFontSize()*2/s.canvas.scale.x;if(i==5)s.canvas.origin.x-=delta;if(i==6)s.canvas.origin.x+=delta;if(i==7)s.canvas.origin.y-=delta;if(i==8)s.canvas.origin.y+=delta;s.mode=ImageScaleMode::Manual;}
}
void DrawOverlay(const CanvasView& v,const CanvasState& s,const OverlayView& o,const Theme& t) {
    if(o.points.empty()) return;
    auto* d=ImGui::GetWindowDrawList();d->PushClipRect(v.min,v.max,true);
    auto color=ImGui::GetColorU32(o.selected?t.colors.accent:o.hovered?t.colors.warning:t.colors.text);
    auto p=ScreenPoint(o.points.front(),s,v);float stroke=o.selected?t.stroke.focus:t.stroke.border;
    switch(o.shape) {
    case OverlayShape::Point:d->AddCircleFilled(p,t.spacing.steps[1],color);if(o.selected||o.hovered)d->AddCircle(p,t.spacing.steps[2],color,0,stroke);break;
    case OverlayShape::Polyline:
        for(std::size_t i=1;i<o.points.size();++i)d->AddLine(ScreenPoint(o.points[i-1],s,v),ScreenPoint(o.points[i],s,v),color,stroke);
        if(o.closed && o.points.size()>2)d->AddLine(ScreenPoint(o.points.back(),s,v),p,color,stroke);break;
    case OverlayShape::Rectangle:if(o.points.size()>1){auto q=ScreenPoint(o.points[1],s,v);d->AddRect({std::min(p.x,q.x),std::min(p.y,q.y)},{std::max(p.x,q.x),std::max(p.y,q.y)},color,0,0,stroke);}break;
    case OverlayShape::Circle:if(Positive(o.radius))d->AddEllipse(p,{static_cast<float>(o.radius*s.scale.x),static_cast<float>(o.radius*s.scale.y)},color,0,0,stroke);break;
    case OverlayShape::Label:d->AddText(p,color,Safe(o.label));break;
    }d->PopClipRect();
}
bool TileEventBuffer::Push(TileEvent e) {if(count>=storage.size()){overflow=true;return false;}storage[count++]=e;return true;}
void PreviewTile(const char* id,const PreviewTileView& tile,bool selected,ImVec2 size,TileEventBuffer& events,ComponentOptions o) {
    ImGui::PushID(id);Push(tile.id);
    const bool visible=BeginCard("tile",size,o);ImGui::BeginDisabled(tile.disabled);
    const bool effectiveDisabled=(ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
    if(visible) {
        if(ImGui::Selectable("##select",selected,ImGuiSelectableFlags_AllowDoubleClick,{0,ImGui::GetFrameHeight()})) {
            events.Push({ImGui::IsMouseDoubleClicked(0)?TileAction::Open:TileAction::Select,tile.id});
        }
        auto titleMin=ImGui::GetItemRectMin(),titleMax=ImGui::GetItemRectMax();
        auto* draw=ImGui::GetWindowDrawList();draw->PushClipRect(titleMin,titleMax,true);
        draw->AddText({titleMin.x,titleMin.y+(titleMax.y-titleMin.y-ImGui::GetFontSize())*.5f},ImGui::GetColorU32(ImGuiCol_Text),Safe(tile.title));draw->PopClipRect();
        if(ImGui::IsItemHovered()||ImGui::IsItemFocused())ImGui::SetTooltip("%s",Safe(tile.title));
        if(ImGui::IsItemFocused())ImGui::SetNavCursorVisible(true);
        if(o.accessibility) {
            accessibility::SemanticNode n;n.id=ImGui::GetItemID();n.parent=o.parent;n.name=Safe(tile.title);n.role=accessibility::SemanticRole::Button;n.state.selected=selected;n.actions=accessibility::SemanticAction::Select|accessibility::SemanticAction::Focus;
            accessibility::AnnotateLastItem(*o.accessibility,n);
            if(o.accessibility->Take(n.id,accessibility::SemanticAction::Select)&&!effectiveDisabled)events.Push({TileAction::Select,tile.id});
            if(o.accessibility->Take(n.id,accessibility::SemanticAction::Focus)&&!effectiveDisabled)ImGui::SetKeyboardFocusHere(-1);
        }
        if(ImGui::IsItemHovered() && ImGui::IsMouseReleased(1))events.Push({TileAction::Context,tile.id});
        if(ImGui::BeginPopupContextItem("context")) {for(auto& action:tile.actions) {Push(action.id);if(ImGui::MenuItem(action.label,action.shortcut,false,!action.disabled))events.Push({TileAction::Command,tile.id,action.id});Pop();}ImGui::EndPopup();}
        if(*Safe(tile.detail))ImGui::TextWrapped("%s",tile.detail);
        ToolbarState toolbar;if(auto action=ResponsiveToolbar("actions",toolbar,tile.actions,ToolbarOptions{},o))events.Push({TileAction::Command,tile.id,action});
        auto avail=ImGui::GetContentRegionAvail();avail.x=std::max(1.f,avail.x);avail.y=std::max(1.f,avail.y);
        if(tile.status==PreviewState::Ready && tile.image.texture.GetTexID()!=ImTextureID{} && ImageGeometryValid(tile.image.pixels,{avail.x,avail.y})) {
            double scale=std::min(avail.x/tile.image.pixels.x,avail.y/tile.image.pixels.y);
            ImVec2 imageSize{static_cast<float>(scale*tile.image.pixels.x),static_cast<float>(scale*tile.image.pixels.y)};
            auto p=ImGui::GetCursorScreenPos();ImGui::SetCursorScreenPos({p.x+(avail.x-imageSize.x)*.5f,p.y+(avail.y-imageSize.y)*.5f});
            ImGui::Image(tile.image.texture,imageSize,tile.image.uv0,tile.image.uv1);
        } else {
            const char* names[]={"Empty","Loading","Empty","Offline","Error"};
            const char* key[]={"empty","loading","empty","offline","error"};
            auto index=std::clamp(static_cast<int>(tile.status),0,4);
            StateView state;state.heading=o.locale?o.locale->Text(key[index],names[index]):names[index];
            state.kind=tile.status==PreviewState::Error?FeedbackKind::Error:FeedbackKind::Info;
            EmptyState("placeholder",state,o);if(tile.status==PreviewState::Loading)Skeleton("skeleton",{avail.x,std::max(1.f,avail.y-ImGui::GetFrameHeight())},o);
        }
    }ImGui::EndDisabled();EndCard();Pop();ImGui::PopID();
}
void ResizableTileStrip(const char* id,std::span<const PreviewTileView> tiles,std::span<TileSize> sizes,StableId selected,TileStripState& state,TileEventBuffer& events,TileStripOptions layout,ComponentOptions o) {
    ImGui::PushID(id);const bool horizontal=layout.orientation==Orientation::Horizontal;
    const float minimum=FiniteExtent(layout.minimum,ImGui::GetFrameHeight()*5);
    auto find=[&](StableId target)->TileSize*{for(auto& size:sizes)if(size.id==target)return &size;return nullptr;};
    if(!ImGui::IsMouseDown(0))state.blockUntilRelease=false;
    if(state.resizing) {
        auto first=find(state.resizing),second=find(state.neighbor);
        auto present=[&](StableId target){return std::any_of(tiles.begin(),tiles.end(),[&](const auto& t){return t.id==target;});};
        bool cancel=state.cancelPending||ImGui::IsKeyPressed(ImGuiKey_Escape)||!first||!second||!present(state.resizing)||!present(state.neighbor);
        if(cancel||!ImGui::IsMouseDown(0)) {
            if(cancel) {state.cancelPending=true;state.blockUntilRelease=ImGui::IsMouseDown(0);if(first)first->extent=state.originalFirst;if(second)second->extent=state.originalSecond;}
            if(events.Push({cancel?TileAction::ResizeCancel:TileAction::ResizeCommit,state.resizing,0,first?first->extent:0,second?second->extent:0})){state.resizing=state.neighbor=0;state.cancelPending=false;}
        }
    }
    if(ImGui::BeginChild("strip",layout.size,ImGuiChildFlags_None,horizontal?ImGuiWindowFlags_HorizontalScrollbar:0)) {
        for(std::size_t i=0;i<tiles.size();++i) {
            if(horizontal && i)ImGui::SameLine();
            auto* first=find(tiles[i].id);float extent=first?std::max(minimum,FiniteExtent(first->extent,minimum)):minimum;
            if(first)first->extent=extent;
            PreviewTile("preview",tiles[i],selected==tiles[i].id,horizontal?ImVec2(extent,0):ImVec2(0,extent),events,o);
            if(i+1==tiles.size())continue;auto* second=find(tiles[i+1].id);if(!first||!second)continue;
            if(horizontal)ImGui::SameLine();
            second->extent=std::max(minimum,FiniteExtent(second->extent,minimum));float total=first->extent+second->extent;
            const float before=first->extent;
            Push(tiles[i].id);ImGui::BeginDisabled(state.blockUntilRelease);
            bool changed=Splitter("resize",first->extent,total,horizontal,minimum);
            if(o.accessibility) {
                accessibility::SemanticNode n;n.id=ImGui::GetItemID();n.parent=o.parent;n.name=o.locale?o.locale->Text("resize_tile","Resize preview"):"Resize preview";n.role=accessibility::SemanticRole::Button;
                n.actions=accessibility::SemanticAction::Increment|accessibility::SemanticAction::Decrement|accessibility::SemanticAction::Focus;
                accessibility::AnnotateLastItem(*o.accessibility,n);
                bool plus=o.accessibility->Take(n.id,accessibility::SemanticAction::Increment),minus=o.accessibility->Take(n.id,accessibility::SemanticAction::Decrement);
                bool disabled=(ImGui::GetItemFlags()&ImGuiItemFlags_Disabled)!=0;
                if(o.accessibility->Take(n.id,accessibility::SemanticAction::Focus)&&!disabled)ImGui::SetKeyboardFocusHere(-1);
                if((plus||minus)&&!disabled){first->extent=std::clamp(first->extent+(plus?1:-1)*ImGui::GetFontSize(),minimum,total-minimum);changed=true;}
            }
            auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRectFilled(a,b,ImGui::GetColorU32(ImGui::IsItemHovered()||ImGui::IsItemFocused()?ImGuiCol_CheckMark:ImGuiCol_Border));
            if(ImGui::IsItemActivated() && !state.resizing) {
                if(events.Push({TileAction::ResizeBegin,tiles[i].id,0,before,second->extent})) {state.resizing=tiles[i].id;state.neighbor=tiles[i+1].id;state.originalFirst=before;state.originalSecond=second->extent;}
            }
            if(changed) {
                second->extent=total-first->extent;
                if(!ImGui::IsMouseDown(0) && !state.resizing) {
                    if(events.storage.size()-std::min(events.count,events.storage.size())>=3) {
                        events.Push({TileAction::ResizeBegin,tiles[i].id,0,before,total-before});
                        events.Push({TileAction::ResizeUpdate,tiles[i].id,0,first->extent,second->extent});
                        events.Push({TileAction::ResizeCommit,tiles[i].id,0,first->extent,second->extent});
                    } else {events.overflow=true;first->extent=before;second->extent=total-before;}
                } else if(!state.resizing || !events.Push({TileAction::ResizeUpdate,tiles[i].id,0,first->extent,second->extent})){first->extent=before;second->extent=total-before;}
            }
            ImGui::EndDisabled();Pop();
        }
    }ImGui::EndChild();ImGui::PopID();
}
}
