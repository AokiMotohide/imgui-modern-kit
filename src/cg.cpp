#include <imkit/cg.h>
#include "transaction_support.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace imkit::cg {
namespace {
Vec3 Add(Vec3 a, Vec3 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3 Mul(Vec3 a, double f) {
    return {a.x * f, a.y * f, a.z * f};
}
double Dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3 RotateVector(Vec3 value, Vec3 rotation) {
    const double angle=std::sqrt(Dot(rotation,rotation));
    if (angle<1e-12) return value;
    const Vec3 axis=Mul(rotation,1/angle);
    const Vec3 cross{axis.y*value.z-axis.z*value.y,axis.z*value.x-axis.x*value.z,axis.x*value.y-axis.y*value.x};
    return Add(Add(Mul(value,std::cos(angle)),Mul(cross,std::sin(angle))),Mul(axis,Dot(axis,value)*(1-std::cos(angle))));
}
Vec3 EulerFromBasis(const Basis &basis) {
    const double y=std::asin(std::clamp(-basis.x.z,-1.,1.));
    if (std::abs(std::cos(y))>1e-8)
        return {std::atan2(basis.y.z,basis.z.z),y,std::atan2(basis.x.y,basis.x.x)};
    return {std::atan2(-basis.z.y,basis.y.y),y,0};
}
double Quantize(double v, double snap) {
    return snap > 0 ? std::round(v / snap) * snap : v;
}
editor::Value Value(Vec3 v) {
    return {0, 0, 0, 0, v.x, v.y, v.z};
}
ImVec2 UVScreen(editor::Point p, const editor::CanvasState &s, ImVec2 origin) {
    auto v = editor::ToScreen(p, s, {origin.x, origin.y});
    return {static_cast<float>(v.x), static_cast<float>(v.y)};
}
void Emit(editor::EventBuffer &out, StableId id, std::uint64_t revision, editor::EditKind kind,
          editor::Value original = {}, editor::Value proposed = {}) {
    out.Push({id, revision, editor::Phase::Commit, kind, original, proposed, editor::CurrentModifiers()});
}
} // namespace
ProjectionResult Project(Vec3 world, const Camera &c, ImVec2 origin, ImVec2 size) {
    Vec3 right{std::cos(c.yaw), 0, -std::sin(c.yaw)},
        up{-std::sin(c.yaw) * std::sin(c.pitch), std::cos(c.pitch), -std::cos(c.yaw) * std::sin(c.pitch)};
    Vec3 forward{std::sin(c.yaw) * std::cos(c.pitch), std::sin(c.pitch), std::cos(c.yaw) * std::cos(c.pitch)};
    Vec3 relative = Add(world, Mul(c.target, -1));
    double z = c.distance - Dot(relative, forward);
    if (size.x <= 0 || size.y <= 0 || z <= .001)
        return {{}, z, false};
    double scale = c.projection == Projection::Orthographic ? size.y / (std::max)(.001, c.orthographicHeight)
                                                            : size.y / (2 * z * std::tan(c.verticalFov * .5));
    return {{origin.x + size.x * .5f + static_cast<float>(Dot(relative, right) * scale),
             origin.y + size.y * .5f - static_cast<float>(Dot(relative, up) * scale)},
            z,
            true};
}
void NavigateCamera(Camera &camera, editor::Point orbit, editor::Point pan, double wheel,
                    double viewportHeight) {
    if (camera.projection == Projection::Camera || viewportHeight <= 0)
        return;
    camera.yaw += orbit.x * .01;
    camera.pitch = std::clamp(camera.pitch + orbit.y * .01, -1.5707963267948966, 1.5707963267948966);
    const auto basis = OrientationBasis(Orientation::View, {}, camera);
    const double height = camera.projection == Projection::Orthographic ? camera.orthographicHeight :
                          2 * camera.distance * std::tan(camera.verticalFov * .5);
    camera.target = Add(camera.target, Add(Mul(basis.x,-pan.x*height/viewportHeight),
                                           Mul(basis.y,pan.y*height/viewportHeight)));
    double &extent = camera.projection == Projection::Orthographic ? camera.orthographicHeight : camera.distance;
    extent = std::clamp(extent * std::pow(.85,wheel), .001, 10000.);
}
void AlignCamera(Camera &camera, Axis axis, bool negative) {
    constexpr double pi=3.14159265358979323846;
    if (axis!=Axis::X && axis!=Axis::Y && axis!=Axis::Z) return;
    camera.projection=Projection::Orthographic;
    camera.yaw=axis==Axis::X ? (negative ? -pi/2 : pi/2) : axis==Axis::Z && negative ? pi : 0;
    camera.pitch=axis==Axis::Y ? (negative ? -pi/2 : pi/2) : 0;
}
Transform TransformDelta(const Transform &original, TransformTool tool, Axis axis, Vec3 delta,
                         const Basis &basis, double snap, bool fine) {
    if (fine)
        delta = Mul(delta, .1);
    delta = {Quantize(delta.x, snap), Quantize(delta.y, snap), Quantize(delta.z, snap)};
    if (axis == Axis::X)
        delta.y = delta.z = 0;
    if (axis == Axis::Y)
        delta.x = delta.z = 0;
    if (axis == Axis::Z)
        delta.x = delta.y = 0;
    if (axis == Axis::XY)
        delta.z = 0;
    if (axis == Axis::YZ)
        delta.x = 0;
    if (axis == Axis::ZX)
        delta.y = 0;
    Transform result = original;
    Vec3 oriented = Add(Add(Mul(basis.x, delta.x), Mul(basis.y, delta.y)), Mul(basis.z, delta.z));
    if (tool == TransformTool::Translate || tool == TransformTool::Unified)
        result.translation = Add(original.translation, oriented);
    if (tool == TransformTool::Rotate) {
        const auto local=OrientationBasis(Orientation::Local,original,{});
        result.rotation=EulerFromBasis({RotateVector(local.x,oriented),RotateVector(local.y,oriented),RotateVector(local.z,oriented)});
    }
    if (tool == TransformTool::Scale)
        result.scale = {original.scale.x * (1 + delta.x), original.scale.y * (1 + delta.y),
                        original.scale.z * (1 + delta.z)};
    return result;
}
Transform TransformAroundPivot(const Transform &original, TransformTool tool, Axis axis, Vec3 delta,
                               const Basis &basis, Vec3 pivot, double snap, bool fine) {
    auto result=TransformDelta(original,tool,axis,delta,basis,snap,fine);
    const Vec3 offset=Add(original.translation,Mul(pivot,-1));
    if (tool==TransformTool::Rotate) {
        const auto rotation=TransformDelta({},TransformTool::Translate,axis,delta,basis,snap,fine).translation;
        result.translation=Add(pivot,RotateVector(offset,rotation));
    } else if (tool==TransformTool::Scale) {
        const auto scale=TransformDelta({},TransformTool::Scale,axis,delta,{},snap,fine).scale;
        const Vec3 scaled=Add(Add(Mul(basis.x,Dot(offset,basis.x)*scale.x),
                                 Mul(basis.y,Dot(offset,basis.y)*scale.y)),Mul(basis.z,Dot(offset,basis.z)*scale.z));
        result.translation=Add(pivot,scaled);
    }
    return result;
}
Basis OrientationBasis(Orientation orientation, const Transform &object, const Camera &camera,
                       const Basis &parent, const Basis &custom) {
    if (orientation == Orientation::Parent)
        return parent;
    if (orientation == Orientation::Custom)
        return custom;
    if (orientation == Orientation::View)
        return {{std::cos(camera.yaw), 0, -std::sin(camera.yaw)},
                {-std::sin(camera.yaw) * std::sin(camera.pitch), std::cos(camera.pitch),
                 -std::cos(camera.yaw) * std::sin(camera.pitch)},
                {std::sin(camera.yaw) * std::cos(camera.pitch), std::sin(camera.pitch),
                 std::cos(camera.yaw) * std::cos(camera.pitch)}};
    if (orientation == Orientation::World)
        return {};
    auto rotate = [&](Vec3 p) {
        auto r = [](double &a, double &b, double angle) {
            double c = std::cos(angle), s = std::sin(angle), v = a * c - b * s;
            b = a * s + b * c;
            a = v;
        };
        r(p.y, p.z, object.rotation.x);
        r(p.z, p.x, object.rotation.y);
        r(p.x, p.y, object.rotation.z);
        return p;
    };
    return {rotate({1, 0, 0}), rotate({0, 1, 0}), rotate({0, 0, 1})};
}
ViewportView BeginViewport(const char *id, ViewportState &s, ImTextureRef texture, ImVec2 size,
                           const Theme &theme) {
    ImGui::PushID(id);
    ImGui::BeginChild("viewport", size, ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    const char *tools[] = {"Select", "Translate", "Rotate", "Scale", "Unified"};
    for (int i = 0; i < 5; ++i) {
        if (i)
            ImGui::SameLine();
        if (ImGui::Selectable(tools[i], static_cast<int>(s.tool) == i, 0, {ImGui::CalcTextSize(tools[i]).x+ImGui::GetStyle().FramePadding.x*2, ImGui::GetFrameHeight()}))
            s.tool = static_cast<TransformTool>(i);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize()*9);
    const char *projections[]={"Perspective","Orthographic","Camera"};
    if (ImGui::BeginCombo("##projection",projections[static_cast<int>(s.camera.projection)])) {
        for (int i=0;i<3;++i) {
            ImGui::BeginDisabled(i==2 && !s.cameraView);
            if (ImGui::Selectable(projections[i],static_cast<int>(s.camera.projection)==i))
                s.camera.projection=static_cast<Projection>(i);
            ImGui::EndDisabled();
        }
        ImGui::EndCombo();
    }
    if (s.camera.projection==Projection::Camera && s.cameraView) {
        s.camera=*s.cameraView;
        s.camera.projection=Projection::Camera;
    }
    ImGui::Checkbox("Grid", &s.grid);
    ImGui::SameLine();
    ImGui::Checkbox("Gizmo", &s.gizmo);
    ImGui::SameLine();
    ImGui::Checkbox("Snap", &s.snap);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize()*8);
    int orientation = static_cast<int>(s.orientation);
    if (ImGui::Combo("##orientation", &orientation, "World\0Local\0View\0Parent\0Custom\0"))
        s.orientation = static_cast<Orientation>(orientation);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize()*9);
    int pivot = static_cast<int>(s.pivot);
    if (ImGui::Combo("##pivot", &pivot, "Individual\0Median\0Bounds\0Cursor\0"))
        s.pivot = static_cast<Pivot>(pivot);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize()*7);
    int shading=s.shading==Shading::Wireframe ? 0 : 1;
    if (ImGui::Combo("##shading",&shading,"Wireframe\0Solid\0"))
        s.shading=shading==0 ? Shading::Wireframe : Shading::Solid;
    if (s.tool==TransformTool::Select) {
        if (s.icons) {
            for (int i=0;i<2;++i) {
                if (i) ImGui::SameLine();
                const bool active=s.lassoSelection==(i==1);
                if (active) ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                if (IconLabelButton(i ? "lasso" : "box",*s.icons,i ? IconId::LassoSelect : IconId::BoxSelect,
                                    i ? "Lasso select" : "Box select",{16*ImGui::GetFontSize()/14})) s.lassoSelection=i==1;
                if (active) ImGui::PopStyleColor();
            }
        } else ImGui::Checkbox("Lasso selection",&s.lassoSelection);
    }
    ViewportView v{ImGui::GetCursorScreenPos(), ImGui::GetContentRegionAvail(), ImGui::IsWindowHovered()};
    auto *d = ImGui::GetWindowDrawList();
    d->PushClipRect(v.min, {v.min.x + v.size.x, v.min.y + v.size.y}, true);
    if (texture.GetTexID())
        d->AddImage(texture, v.min, {v.min.x + v.size.x, v.min.y + v.size.y}, {0, 1}, {1, 0});
    else
        d->AddRectFilled(v.min, {v.min.x + v.size.x, v.min.y + v.size.y},
                         ImGui::GetColorU32(theme.editor.canvas));
    auto line = [&](Vec3 a, Vec3 b, ImU32 color) {
        auto pa = Project(a, s.camera, v.min, v.size), pb = Project(b, s.camera, v.min, v.size);
        if (pa.visible && pb.visible)
            d->AddLine(pa.screen, pb.screen, color);
    };
    if (s.grid)
        for (int i = -10; i <= 10; ++i) {
            line({static_cast<double>(i), 0, -10}, {static_cast<double>(i), 0, 10},
                 ImGui::GetColorU32(theme.editor.grid));
            line({-10, 0, static_cast<double>(i)}, {10, 0, static_cast<double>(i)},
                 ImGui::GetColorU32(theme.editor.grid));
        }
    if (s.axes) {
        line({0, 0, 0}, {3, 0, 0}, ImGui::GetColorU32(theme.editor.axisX));
        line({0, 0, 0}, {0, 3, 0}, ImGui::GetColorU32(theme.editor.axisY));
        line({0, 0, 0}, {0, 0, 3}, ImGui::GetColorU32(theme.editor.axisZ));
    }
    bool navigationHovered=false;
    if (s.navigationGizmo && v.size.x>=110 && v.size.y>=110) {
        ImVec2 corner{v.min.x+v.size.x-104,v.min.y+8},center{corner.x+48,corner.y+48};
        ImGui::SetCursorScreenPos(corner);
        ImGui::InvisibleButton("navigation",{96,96});
        navigationHovered=ImGui::IsItemHovered();
        auto basis=OrientationBasis(Orientation::View,{},s.camera);
        Vec3 axes[]={{1,0,0},{0,1,0},{0,0,1}};
        const ImU32 colors[]={ImGui::GetColorU32(theme.editor.axisX),ImGui::GetColorU32(theme.editor.axisY),ImGui::GetColorU32(theme.editor.axisZ)};
        int hit=-1; double closest=12,front=-2;
        std::array<int,6> order{0,1,2,3,4,5};
        auto depthOf=[&](int i){return Dot(Mul(axes[i/2],i%2 ? -1 : 1),basis.z);};
        std::sort(order.begin(),order.end(),[&](int a,int b){return depthOf(a)<depthOf(b);});
        for (int i:order) {
            auto axis=Mul(axes[i/2],i%2 ? -1 : 1);
            ImVec2 p{center.x+static_cast<float>(Dot(axis,basis.x)*32),
                     center.y-static_cast<float>(Dot(axis,basis.y)*32)};
            d->AddLine(center,p,colors[i/2],1.5f);
            d->AddCircleFilled(p,8,ImGui::GetColorU32(ImGuiCol_WindowBg));
            d->AddCircle(p,8,colors[i/2],16,2);
            const char *names[]={"X","-X","Y","-Y","Z","-Z"};
            auto textSize=ImGui::CalcTextSize(names[i]);
            d->AddText({p.x-textSize.x*.5f,p.y-textSize.y*.5f},colors[i/2],names[i]);
            auto mouse=ImGui::GetIO().MousePos;
            double distance=std::hypot(mouse.x-p.x,mouse.y-p.y),depth=Dot(axis,basis.z);
            if (distance<closest || (std::abs(distance-closest)<.01 && depth>front)) {
                closest=distance;front=depth;hit=i;
            }
        }
        if (navigationHovered && hit>=0 && ImGui::IsMouseClicked(0))
            AlignCamera(s.camera,static_cast<Axis>(hit/2+1),hit%2!=0);
        if (navigationHovered) ImGui::SetTooltip("Align view: click an axis");
        ImGui::SetCursorScreenPos(v.min);
    }
    if (v.hovered && !navigationHovered && ImGui::GetIO().MousePos.y >= v.min.y) {
        const auto &io = ImGui::GetIO();
        editor::Point orbit{},pan{};
        if (ImGui::IsMouseDragging(2)) {
            if (io.KeyShift) pan={io.MouseDelta.x,io.MouseDelta.y};
            else orbit={io.MouseDelta.x,io.MouseDelta.y};
        }
        NavigateCamera(s.camera,orbit,pan,io.MouseWheel,v.size.y);
    }
    v.hovered &= !navigationHovered;
    return v;
}
void ViewportObjects(const ViewportView &v, std::span<const ObjectView> objects, ViewportState &s,
                     editor::Selection &selection, std::uint64_t revision, editor::EventBuffer &out,
                     const Theme &theme) {
    auto *d = ImGui::GetWindowDrawList();
    StableId hit = 0;
    double depth = 1e30;
    auto mouse = ImGui::GetIO().MousePos;
    for (const auto &o : objects)
        if (o.visible) {
            auto p = Project(o.transform.translation, s.camera, v.min, v.size);
            if (!p.visible)
                continue;
            bool selected = selection.Contains(o.id);
            float r = selected ? 9.f : 5.f;
            d->AddCircle(p.screen, r, ImGui::GetColorU32(selected ? theme.colors.warning : theme.colors.text),
                         0, selected ? 2.f : 1.f);
            d->AddText({p.screen.x + 12, p.screen.y}, ImGui::GetColorU32(theme.colors.muted), o.label);
            if (o.selectable && !o.locked && std::hypot(mouse.x - p.screen.x, mouse.y - p.screen.y) < 12 &&
                p.depth < depth) {
                hit = o.id;
                depth = p.depth;
            }
        }
    if (v.hovered && hit && ImGui::IsMouseClicked(0) && !s.drag.active) {
        if (selection.Set(hit, ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyCtrl)) Emit(out,hit,revision,editor::EditKind::Select);
        else out.overflow=true;
    }
    if (s.selectionCanvas.selecting && s.selectionRevision!=revision) s.selectionCanvas.selecting=false;
    if (s.tool!=TransformTool::Select || s.drag.active) {s.selectionCanvas.selecting=false;return;}
    const bool inside=v.hovered && mouse.x>=v.min.x && mouse.x<=v.min.x+v.size.x && mouse.y>=v.min.y && mouse.y<=v.min.y+v.size.y;
    if (s.selectionCanvas.selecting || (!hit && inside && ImGui::IsMouseClicked(0))) {
        std::size_t count=0;
        for (const auto &object:objects) if (object.visible && object.selectable && !object.locked) {
            const auto projected=Project(object.transform.translation,s.camera,v.min,v.size);
            if (!projected.visible) continue;
            if (count==s.selectionPoints.size()) {out.overflow=true;s.selectionCanvas.selecting=false;return;}
            s.selectionPoints[count++]={object.id,{projected.screen.x-v.min.x,projected.screen.y-v.min.y},false};
        }
        auto points=s.selectionPoints.first(count);
        const editor::CanvasView canvas{v.min,{v.min.x+v.size.x,v.min.y+v.size.y},{{0,0},{v.size.x,v.size.y}},inside};
        s.selectionCanvas.origin={};s.selectionCanvas.scale={1,1};
        const bool wasSelecting=s.selectionCanvas.selecting;
        editor::CanvasSelection(canvas,s.selectionCanvas,{&points,revision,[](void *user,editor::Rect) {
            return std::span<const editor::SelectablePoint>(*static_cast<std::span<editor::SelectablePoint>*>(user));
        }},selection,out,theme,s.lassoSelection);
        if (!wasSelecting && s.selectionCanvas.selecting) s.selectionRevision=revision;
    }

}
void TransformGizmo(const ViewportView &v, const ObjectView &object, ViewportState &s, std::uint64_t revision,
                    editor::EventBuffer &out, const Theme &theme) {
    constexpr double Pi=3.14159265358979323846;
    auto finish=[&](bool cancel) {
        std::size_t count=1+(s.pivotDrag.active ? 1 : 0);
        for (std::size_t i=0;i<s.companionCount;++i) {
            auto &member=s.companions[i];count+=1+(member.position.active ? 1 : 0);
            member.transform.draft.phase=member.position.draft.phase=cancel ? editor::Phase::Cancel : editor::Phase::Commit;
        }
        s.drag.draft.phase=cancel ? editor::Phase::Cancel : editor::Phase::Commit;
        if (s.pivotDrag.active) s.pivotDrag.draft.phase=s.drag.draft.phase;
        if (out.storage.size()-out.count<count) {out.overflow=true;return;}
        if (cancel) {s.drag.Cancel(out);if (s.pivotDrag.active) s.pivotDrag.Cancel(out);}
        else {s.drag.Commit(revision,out);if (s.pivotDrag.active) s.pivotDrag.Commit(revision,out);}
        for (std::size_t i=0;i<s.companionCount;++i) {
            auto &member=s.companions[i];
            if (cancel) {member.transform.Cancel(out);if (member.position.active) member.position.Cancel(out);}
            else {member.transform.Commit(revision,out);if (member.position.active) member.position.Commit(revision,out);}
        }
        s.companionCount=0;
    };
    bool selectionChanged=false;
    if (s.drag.active) for (std::size_t i=0;i<s.companionCount;++i) {
        const auto target=s.companions[i].transform.draft.target;
        auto found=std::find_if(s.selectedObjects.begin(),s.selectedObjects.end(),[&](const auto &o){return o.id==target;});
        selectionChanged |= found==s.selectedObjects.end() || found->locked;
    }
    if (s.drag.active && (selectionChanged || revision!=s.drag.draft.revision || object.id!=s.drag.draft.target || object.locked ||
                         !s.gizmo || s.tool==TransformTool::Select || ImGui::IsKeyPressed(ImGuiKey_Escape))) {
        finish(true);return;
    }
    if (s.drag.active && (s.drag.draft.phase==editor::Phase::Commit || s.drag.draft.phase==editor::Phase::Cancel)) {
        finish(s.drag.draft.phase==editor::Phase::Cancel);return;
    }
    if (!s.gizmo || s.tool == TransformTool::Select || object.locked)
        return;
    auto pivot = s.pivot == Pivot::Individual ? object.transform.translation : s.pivotPosition;
    auto center = Project(pivot, s.camera, v.min, v.size);
    if (!center.visible)
        return;
    auto basis = OrientationBasis(s.orientation, object.transform, s.camera, s.parentBasis, s.customBasis);
    auto *d = ImGui::GetWindowDrawList();
    const Vec3 axes[] = {basis.x, basis.y, basis.z};
    const ImU32 colors[] = {ImGui::GetColorU32(theme.editor.axisX), ImGui::GetColorU32(theme.editor.axisY),
                            ImGui::GetColorU32(theme.editor.axisZ)};
    ImVec2 projected[3]{};
    Axis hit = Axis::None;
    const bool unified=s.tool==TransformTool::Unified;
    TransformTool hitTool=unified ? TransformTool::Translate : s.tool;
    auto mouse = ImGui::GetIO().MousePos;
    for (int i = 0; i < 3; ++i) {
        auto end = Project(Add(pivot, axes[i]), s.camera, v.min, v.size);
        if (!end.visible)
            continue;
        double dx = end.screen.x - center.screen.x, dy = end.screen.y - center.screen.y,
               length = std::hypot(dx, dy);
        if (length < 1)
            continue;
        projected[i] = {static_cast<float>(dx), static_cast<float>(dy)};
        if (s.tool == TransformTool::Rotate) continue;
        ImVec2 tip{center.screen.x + static_cast<float>(dx / length * 70),
                   center.screen.y + static_cast<float>(dy / length * 70)};
        d->AddLine(center.screen, tip, colors[i], 3);
        if (s.tool == TransformTool::Scale)
            d->AddRectFilled({tip.x-6,tip.y-6},{tip.x+6,tip.y+6},colors[i]);
        else d->AddCircleFilled(tip, 6, colors[i]);
        const char *label = i == 0 ? "X" : i == 1 ? "Y" : "Z";
        d->AddText({tip.x + 8, tip.y}, colors[i], label);
        if (std::hypot(mouse.x - tip.x, mouse.y - tip.y) < 12)
            {hit = static_cast<Axis>(i + 1);hitTool=unified ? TransformTool::Translate : s.tool;}
        if (unified) {
            const ImVec2 scaleTip{float(center.screen.x+dx/length*110),float(center.screen.y+dy/length*110)};
            d->AddRectFilled({scaleTip.x-5,scaleTip.y-5},{scaleTip.x+5,scaleTip.y+5},colors[i]);
            if (std::hypot(mouse.x-scaleTip.x,mouse.y-scaleTip.y)<9)
                {hit=static_cast<Axis>(i+1);hitTool=TransformTool::Scale;}
        }
    }
    if (s.tool == TransformTool::Rotate || unified) {
        const float screenRadius=unified ? 130.f : 88.f;
        const float ringRadius=unified ? 88.f : 70.f;
        d->AddCircle(center.screen,screenRadius,ImGui::GetColorU32(theme.editor.gizmo),64,
                     s.drag.active && s.activeAxis==Axis::Screen ? 4.f : 2.f);
        if (std::abs(std::hypot(mouse.x-center.screen.x,mouse.y-center.screen.y)-screenRadius)<7)
            {hit=Axis::Screen;hitTool=TransformTool::Rotate;}
        double nearest = 8;
        for (int axis=0; axis<3; ++axis) {
            const auto u=projected[(axis+1)%3], w=projected[(axis+2)%3];
            const double extent=std::max(std::hypot(u.x,u.y),std::hypot(w.x,w.y));
            if (extent<1 || std::abs(u.x*w.y-u.y*w.x)<1e-6) continue;
            ImVec2 ring[64];
            for (int k=0;k<64;++k) {
                const double angle=k*2*Pi/64;
                ring[k]={float(center.screen.x+ringRadius/extent*(u.x*std::cos(angle)+w.x*std::sin(angle))),
                         float(center.screen.y+ringRadius/extent*(u.y*std::cos(angle)+w.y*std::sin(angle)))};
            }
            d->AddPolyline(ring,64,colors[axis],ImDrawFlags_Closed,
                           s.drag.active && s.activeAxis==static_cast<Axis>(axis+1) ? 4.f : 2.f);
            for (int k=0;k<64;++k) {
                const auto x=ring[k], y=ring[(k+1)%64];
                const double dx=y.x-x.x,dy=y.y-x.y,length=dx*dx+dy*dy;
                const double f=length>0 ? std::clamp(((mouse.x-x.x)*dx+(mouse.y-x.y)*dy)/length,0.,1.) : 0;
                const double distance=std::hypot(mouse.x-x.x-f*dx,mouse.y-x.y-f*dy);
                if (distance<nearest) {nearest=distance;hit=static_cast<Axis>(axis+1);hitTool=TransformTool::Rotate;}
            }
        }
    }
    if (s.tool != TransformTool::Rotate) {
        for (int plane = 0; plane < 3; ++plane) {
            const int a = plane, b = (plane + 1) % 3;
            const auto u = projected[a], w = projected[b];
            const double lu = std::hypot(u.x, u.y), lw = std::hypot(w.x, w.y);
            if (lu < 1 || lw < 1) continue;
            const ImVec2 x{float(u.x / lu), float(u.y / lu)}, y{float(w.x / lw), float(w.y / lw)};
            const double determinant = x.x * y.y - x.y * y.x;
            if (std::abs(determinant) < .15) continue;
            ImVec2 corners[4];
            const float distances[4][2] = {{18,18},{34,18},{34,34},{18,34}};
            for (int k = 0; k < 4; ++k)
                corners[k] = {center.screen.x + x.x * distances[k][0] + y.x * distances[k][1],
                              center.screen.y + x.y * distances[k][0] + y.y * distances[k][1]};
            const Axis axis = static_cast<Axis>(static_cast<int>(Axis::XY) + plane);
            auto fill = ImGui::ColorConvertU32ToFloat4(colors[plane]);
            fill.w *= 65.f / 255.f;
            d->AddConvexPolyFilled(corners, 4, ImGui::ColorConvertFloat4ToU32(fill));
            d->AddPolyline(corners, 4, colors[plane], ImDrawFlags_Closed,
                           s.drag.active && s.activeAxis == axis ? 3.f : 1.f);
            const double mx = mouse.x - center.screen.x, my = mouse.y - center.screen.y;
            const double first = (mx * y.y - my * y.x) / determinant;
            const double second = (x.x * my - x.y * mx) / determinant;
            if (first >= 18 && first <= 34 && second >= 18 && second <= 34) {hit = axis;hitTool=unified ? TransformTool::Translate : s.tool;}
        }
        const ImVec2 low{center.screen.x - 7, center.screen.y - 7}, high{center.screen.x + 7, center.screen.y + 7};
        d->AddRect(low, high, ImGui::GetColorU32(theme.editor.gizmo), 0, 0,
                   s.drag.active && s.activeAxis == Axis::Screen ? 3.f : 2.f);
        if (mouse.x >= low.x && mouse.x <= high.x && mouse.y >= low.y && mouse.y <= high.y)
            {hit = Axis::Screen;hitTool=unified ? TransformTool::Translate : s.tool;}
    }
    TransformTool operation=s.drag.active ?
        (s.drag.draft.kind==editor::EditKind::Rotate ? TransformTool::Rotate :
         s.drag.draft.kind==editor::EditKind::Scale ? TransformTool::Scale : TransformTool::Translate) : hitTool;
    if (v.hovered && hit != Axis::None && ImGui::IsMouseClicked(0) && !s.drag.active) {
        const bool needsPosition=s.pivot!=Pivot::Individual && operation!=TransformTool::Translate &&
            (object.transform.translation.x!=pivot.x || object.transform.translation.y!=pivot.y || object.transform.translation.z!=pivot.z);
        std::size_t members=0,required=needsPosition ? 2u : 1u;
        const bool memberPosition=s.pivot!=Pivot::Individual && operation!=TransformTool::Translate;
        for (std::size_t i=0;i<s.selectedObjects.size();++i) {
            const auto &member=s.selectedObjects[i];
            if (member.locked) return;
            for (std::size_t j=0;j<i;++j) if (s.selectedObjects[j].id==member.id) {out.overflow=true;return;}
            if (member.id==object.id) continue;
            ++members;required+=memberPosition ? 2u : 1u;
        }
        if (members>s.companions.size() || out.storage.size()-out.count<required) {out.overflow=true;return;}
        s.companionCount=0;
        s.activeAxis = hit;
        s.mouseStart = {mouse.x, mouse.y};
        s.original = object.transform;
        s.rotationMouse = s.mouseStart;
        s.rotationAngle = 0;
        auto kind = operation == TransformTool::Rotate  ? editor::EditKind::Rotate
                    : operation == TransformTool::Scale ? editor::EditKind::Scale
                                                     : editor::EditKind::Translate;
        auto value = Value(operation == TransformTool::Rotate  ? object.transform.rotation
                           : operation == TransformTool::Scale ? object.transform.scale
                                                            : object.transform.translation);
        s.drag.Begin(object.id, revision, kind, value, editor::CurrentModifiers(), out);
        if (needsPosition) s.pivotDrag.Begin(object.id,revision,editor::EditKind::Translate,
                                            Value(object.transform.translation),editor::CurrentModifiers(),out);
        for (const auto &member:s.selectedObjects) if (member.id!=object.id) {
            auto &companion=s.companions[s.companionCount++];companion.original=member.transform;
            companion.transform.Begin(member.id,revision,kind,
                Value(operation==TransformTool::Rotate ? member.transform.rotation :
                      operation==TransformTool::Scale ? member.transform.scale : member.transform.translation),editor::CurrentModifiers(),out);
            if (memberPosition) companion.position.Begin(member.id,revision,editor::EditKind::Translate,
                Value(member.transform.translation),editor::CurrentModifiers(),out);
        }
    }
    if (s.drag.active) {
        double delta = ((mouse.x - s.mouseStart.x) - (mouse.y - s.mouseStart.y)) * .01;
        Vec3 dv{delta, delta, delta};
        if (operation != TransformTool::Rotate) {
            const double mx = mouse.x - s.mouseStart.x, my = mouse.y - s.mouseStart.y;
            if (s.activeAxis>=Axis::X && s.activeAxis<=Axis::Z) {
                const auto axis=projected[static_cast<int>(s.activeAxis)-1];
                const double length=axis.x*axis.x+axis.y*axis.y;
                const double amount=length>1e-6 ? (mx*axis.x+my*axis.y)/length : 0;
                dv={amount,amount,amount};
            } else if (s.activeAxis == Axis::Screen && operation == TransformTool::Scale) {
                dv={delta,delta,delta};
            } else if (s.activeAxis == Axis::Screen) {
                basis = OrientationBasis(Orientation::View, s.original, s.camera, s.parentBasis, s.customBasis);
                const auto px = Project(Add(pivot, basis.x), s.camera, v.min, v.size);
                const auto py = Project(Add(pivot, basis.y), s.camera, v.min, v.size);
                const double xx = px.screen.x - center.screen.x, xy = px.screen.y - center.screen.y;
                const double yx = py.screen.x - center.screen.x, yy = py.screen.y - center.screen.y;
                const double det = xx * yy - xy * yx;
                dv = std::abs(det) > 1e-6 ? Vec3{(mx * yy - my * yx) / det, (xx * my - xy * mx) / det, 0} : Vec3{};
            } else if (s.activeAxis >= Axis::XY && s.activeAxis <= Axis::ZX) {
                const int a = static_cast<int>(s.activeAxis) - static_cast<int>(Axis::XY), b = (a + 1) % 3;
                const auto x = projected[a], y = projected[b];
                const double det = x.x * y.y - x.y * y.x;
                double values[3]{};
                if (std::abs(det) > 1e-6) {
                    values[a] = (mx * y.y - my * y.x) / det;
                    values[b] = (x.x * my - x.y * mx) / det;
                }
                dv = {values[0], values[1], values[2]};
            }
        }
        if (operation == TransformTool::Rotate && s.activeAxis>=Axis::X && s.activeAxis<=Axis::Z) {
            const int axis=static_cast<int>(s.activeAxis)-1;
            const auto u=projected[(axis+1)%3], w=projected[(axis+2)%3];
            const double det=u.x*w.y-u.y*w.x;
            auto angle=[&](double x,double y) {
                x-=center.screen.x; y-=center.screen.y;
                return std::atan2((u.x*y-u.y*x)/det,(x*w.y-y*w.x)/det);
            };
            if (std::abs(det)>1e-6)
                s.rotationAngle += std::remainder(angle(mouse.x,mouse.y)-
                    angle(s.rotationMouse.x,s.rotationMouse.y),2*Pi);
            const double rotation=s.rotationAngle;
            dv={axis==0 ? rotation : 0,axis==1 ? rotation : 0,axis==2 ? rotation : 0};
        }
        if (operation == TransformTool::Rotate && s.activeAxis == Axis::Screen) {
            basis=OrientationBasis(Orientation::View,s.original,s.camera,s.parentBasis,s.customBasis);
            const double start=std::atan2(-(s.rotationMouse.y-center.screen.y),s.rotationMouse.x-center.screen.x);
            const double end=std::atan2(-(mouse.y-center.screen.y),mouse.x-center.screen.x);
            s.rotationAngle += std::remainder(end-start,2*Pi);
            dv={0,0,s.rotationAngle};
        }
        if (operation == TransformTool::Rotate) s.rotationMouse={mouse.x,mouse.y};
        auto result = TransformAroundPivot(s.original, operation, s.activeAxis, dv, basis, pivot, s.snap ? .1 : 0,
                                     ImGui::GetIO().KeyShift);
        auto value = Value(operation == TransformTool::Rotate  ? result.rotation
                           : operation == TransformTool::Scale ? result.scale
                                                            : result.translation);
        if (ImGui::IsMouseDown(0)) {
            const auto position=Value(result.translation);
            const bool mainChanged=!(value==s.drag.draft.proposed);
            const bool positionChanged=s.pivotDrag.active && !(position==s.pivotDrag.draft.proposed);
            std::size_t count=(mainChanged ? 1u : 0u)+(positionChanged ? 1u : 0u);
            for (std::size_t i=0;i<s.companionCount;++i) count+=1+(s.companions[i].position.active ? 1u : 0u);
            if (out.storage.size()-out.count<count) out.overflow=true;
            else {
                if (mainChanged) s.drag.Update(revision,value,out);
                if (positionChanged) s.pivotDrag.Update(revision,position,out);
                for (std::size_t i=0;i<s.companionCount;++i) {
                    auto &member=s.companions[i];
                    const auto memberPivot=s.pivot==Pivot::Individual ? member.original.translation : pivot;
                    const auto memberBasis=s.pivot==Pivot::Individual && s.activeAxis!=Axis::Screen ?
                        OrientationBasis(s.orientation,member.original,s.camera,s.parentBasis,s.customBasis) : basis;
                    const auto transformed=TransformAroundPivot(member.original,operation,s.activeAxis,dv,memberBasis,
                                                               memberPivot,s.snap ? .1 : 0,ImGui::GetIO().KeyShift);
                    const auto memberValue=Value(operation==TransformTool::Rotate ? transformed.rotation :
                        operation==TransformTool::Scale ? transformed.scale : transformed.translation);
                    if (!(memberValue==member.transform.draft.proposed)) member.transform.Update(revision,memberValue,out);
                    const auto memberLocation=Value(transformed.translation);
                    if (member.position.active && !(memberLocation==member.position.draft.proposed))
                        member.position.Update(revision,memberLocation,out);
                }
            }
        }
        if (ImGui::IsMouseReleased(0)) finish(false);
    }
}
void EndViewport() {
    ImGui::GetWindowDrawList()->PopClipRect();
    ImGui::EndChild();
    ImGui::PopID();
}
void Outliner(const char *id, const SceneProvider &p, OutlinerState &s, editor::Selection &selection,
              editor::EventBuffer &out) {
    detail::ResumeTerminal(s.renameTransaction,p.revision,out);
    if (!s.renameTransaction.active) s.renaming=0;
    ImGui::PushID(id);
    ImGui::InputText("Search", s.search, sizeof(s.search));
    if (ImGui::BeginTable("tree", 5,
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        for (const char *label : {"Visible", "Selectable", "Renderable", "Locked"})
            ImGui::TableSetupColumn(label, ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
        ImGuiListClipper clipper;
        clipper.Begin(p.visibleCount, ImGui::GetFrameHeightWithSpacing());
        while (clipper.Step()) {
            auto rows = p.query ? p.query(p.user, clipper.DisplayStart,
                                          clipper.DisplayEnd - clipper.DisplayStart, s.search)
                                : std::span<const ObjectView>{};
            for (const auto &row : rows) {
                ImGui::PushID(reinterpret_cast<void *>(static_cast<std::uintptr_t>(row.id)));
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Indent(row.depth * 14.f);
                if (row.hasChildren) {
                    if (ImGui::SmallButton(row.expanded ? "v" : ">"))
                        Emit(out, row.id, p.revision, editor::EditKind::Toggle,
                             Value({4, row.expanded ? 1. : 0., 0}), Value({4, row.expanded ? 0. : 1., 0}));
                    ImGui::SameLine();
                }
                auto beginRename=[&] {
                    if (row.locked || s.renameTransaction.active) return;
                    editor::Event event{row.id,p.revision,editor::Phase::Begin,editor::EditKind::Rename};
                    if (std::snprintf(event.originalText.data(),event.originalText.size(),"%s",row.label)>=static_cast<int>(event.originalText.size())) {
                        out.overflow=true;return;
                    }
                    event.proposedText=event.originalText;
                    if (!out.Push(event)) return;
                    s.renameTransaction.draft=event;s.renameTransaction.active=true;
                    std::snprintf(s.rename,sizeof(s.rename),"%s",row.label);s.renaming=row.id;s.renameFocus=true;
                };
                if (s.renaming==row.id) {
                    if (row.locked) s.renameTransaction.Cancel(out);
                    else if (s.renameTransaction.draft.phase==editor::Phase::Commit || s.renameTransaction.draft.phase==editor::Phase::Cancel)
                        ImGui::TextUnformatted(s.rename);
                    else {
                        if (s.renameFocus) {ImGui::SetKeyboardFocusHere();s.renameFocus=false;}
                        ImGui::SetNextItemWidth(-1);
                        const bool accept=ImGui::InputText("##rename",s.rename,sizeof(s.rename),
                            ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_AutoSelectAll);
                        auto &draft=s.renameTransaction.draft;
                        if (std::string_view(draft.proposedText.data())!=s.rename) {
                            auto event=draft;event.phase=editor::Phase::Update;
                            std::snprintf(event.proposedText.data(),event.proposedText.size(),"%s",s.rename);
                            if (out.Push(event)) draft=event;
                        }
                        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) s.renameTransaction.Cancel(out);
                        else if (accept) {
                            std::snprintf(draft.proposedText.data(),draft.proposedText.size(),"%s",s.rename);
                            s.renameTransaction.Commit(p.revision,out);
                        }
                        if (!s.renameTransaction.active) s.renaming=0;
                    }
                } else {
                    if (ImGui::Selectable(row.label, selection.Contains(row.id)) && row.selectable && !row.locked) {
                        if (selection.Set(row.id, ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyCtrl))
                            Emit(out,row.id,p.revision,editor::EditKind::Select);
                        else out.overflow=true;
                    }
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) beginRename();
                    if (ImGui::BeginPopupContextItem("actions")) {
                        ImGui::BeginDisabled(row.locked || s.renameTransaction.active);
                        if (ImGui::MenuItem("Rename")) beginRename();
                        if (ImGui::MenuItem("Duplicate")) Emit(out,row.id,p.revision,editor::EditKind::Duplicate);
                        if (row.geometry) {
                            if (ImGui::MenuItem("Linked duplicate")) Emit(out,row.id,p.revision,editor::EditKind::Duplicate,{},editor::Value{0,0,1});
                            if (selection.active && selection.active!=row.id && ImGui::MenuItem("Link geometry to active"))
                                Emit(out,row.id,p.revision,editor::EditKind::LinkGeometry,editor::Value{0,0,0,row.geometry},editor::Value{0,0,0,selection.active});
                            if (ImGui::MenuItem("Make geometry single user"))
                                Emit(out,row.id,p.revision,editor::EditKind::LinkGeometry,editor::Value{0,0,0,row.geometry},{});
                        }
                        if (ImGui::MenuItem("Move up")) Emit(out,row.id,p.revision,editor::EditKind::Reorder,{},editor::Value{0,0,-1,row.parent});
                        if (ImGui::MenuItem("Move down")) Emit(out,row.id,p.revision,editor::EditKind::Reorder,{},editor::Value{0,0,1,row.parent});
                        if (row.parent && ImGui::MenuItem("Move to root")) Emit(out,row.id,p.revision,editor::EditKind::Reparent,editor::Value{0,0,0,row.parent},{});
                        if (row.hasChildren) {
                            if (ImGui::MenuItem("Expand hierarchy")) Emit(out,row.id,p.revision,editor::EditKind::Toggle,{},Value({5,1,0}));
                            if (ImGui::MenuItem("Collapse hierarchy")) Emit(out,row.id,p.revision,editor::EditKind::Toggle,{},Value({5,0,0}));
                        }
                        ImGui::EndDisabled();ImGui::EndPopup();
                    }
                }
                if (!row.locked && !s.renameTransaction.active && ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("IMKIT_OBJECT", &row.id, sizeof(row.id));
                    ImGui::TextUnformatted(row.label);
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (auto *payload = ImGui::AcceptDragDropPayload("IMKIT_OBJECT")) {
                        auto source = *static_cast<const StableId *>(payload->Data);
                        if (source != row.id && !row.locked)
                            Emit(out, source, p.revision, editor::EditKind::Reparent, {},
                                 editor::Value{0, 0, 0, row.id});
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::Unindent(row.depth * 14.f);
                const bool values[] = {row.visible, row.selectable, row.renderable, row.locked};
                const char *labels[] = {"V", "S", "R", "L"};
                for (int f = 0; f < 4; ++f) {
                    ImGui::TableNextColumn();
                    ImGui::PushID(f);
                    bool draft = values[f];
                    if (ImGui::Checkbox("##restriction", &draft))
                        Emit(out, row.id, p.revision, editor::EditKind::Toggle,
                             Value({static_cast<double>(f), values[f] ? 1. : 0., 0}),
                             Value({static_cast<double>(f), draft ? 1. : 0., 0}));
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", labels[f]);
                    ImGui::PopID();
                }
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
}
void ComponentStack(const char *id,std::span<const ComponentView> components,
                    std::uint64_t revision,editor::EventBuffer &out,const ComponentStackOptions &options) {
    ImGui::PushID(id);
    if (options.owner && !options.availableTypes.empty()) {
        ImGui::BeginDisabled(options.locked);
        if (ImGui::Button("Add component")) ImGui::OpenPopup("add");
        if (ImGui::BeginPopup("add")) {
            for (const auto &type:options.availableTypes) {
                ImGui::PushID(reinterpret_cast<void*>(static_cast<std::uintptr_t>(type.id)));
                if (ImGui::MenuItem(type.label)) Emit(out,options.owner,revision,editor::EditKind::ComponentAdd,{},editor::Value{0,0,0,type.id});
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }
        ImGui::EndDisabled();
    }
    if (ImGui::BeginTable("components",3,ImGuiTableFlags_RowBg|ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Enabled",ImGuiTableColumnFlags_WidthFixed,ImGui::GetFrameHeight());
        ImGui::TableSetupColumn("Component",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Actions",ImGuiTableColumnFlags_WidthFixed,ImGui::GetFrameHeight());
        for (const auto &component:components) {
            ImGui::PushID(reinterpret_cast<void*>(static_cast<std::uintptr_t>(component.id)));
            ImGui::TableNextRow();ImGui::TableNextColumn();
            bool enabled=component.enabled;ImGui::BeginDisabled(component.locked || options.locked);
            if (ImGui::Checkbox("##enabled",&enabled))
                Emit(out,component.id,revision,editor::EditKind::Toggle,Value({0,component.enabled?1.:0.,0}),Value({0,enabled?1.:0.,0}));
            ImGui::EndDisabled();ImGui::TableNextColumn();
            if (ImGui::Selectable(component.label,component.expanded))
                Emit(out,component.id,revision,editor::EditKind::Toggle,Value({1,component.expanded?1.:0.,0}),Value({1,component.expanded?0.:1.,0}));
            if (component.expanded) ImGui::TextWrapped("%s",component.description);
            ImGui::TableNextColumn();
            if (ImGui::Button("...")) ImGui::OpenPopup("actions");
            if (ImGui::BeginPopup("actions")) {
                ImGui::BeginDisabled(options.locked);
                if (ImGui::MenuItem(component.locked?"Unlock":"Lock"))
                    Emit(out,component.id,revision,editor::EditKind::Toggle,Value({2,component.locked?1.:0.,0}),Value({2,component.locked?0.:1.,0}));
                ImGui::EndDisabled();
                ImGui::BeginDisabled(component.locked || options.locked);
                if (ImGui::MenuItem("Move up")) Emit(out,component.id,revision,editor::EditKind::Reorder,{},editor::Value{0,0,-1,component.owner});
                if (ImGui::MenuItem("Move down")) Emit(out,component.id,revision,editor::EditKind::Reorder,{},editor::Value{0,0,1,component.owner});
                if (ImGui::MenuItem("Remove")) Emit(out,component.id,revision,editor::EditKind::Remove);
                ImGui::EndDisabled();ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
}

editor::Point TransformUV(editor::Point p, editor::Point pivot, editor::Point translation, double angle,
                          editor::Point scale, double snap) {
    double x = (p.x - pivot.x) * scale.x, y = (p.y - pivot.y) * scale.y, c = std::cos(angle),
           s = std::sin(angle);
    return {Quantize(pivot.x + x * c - y * s + translation.x, snap),
            Quantize(pivot.y + x * s + y * c + translation.y, snap)};
}
void UVEditor(const char *id, const UVProvider &p, ImTextureRef texture, UVState &s,
              editor::Selection &selection, editor::EventBuffer &out, const Theme &theme, ImVec2 size) {
    auto view = editor::BeginCanvas(id, s.canvas, size, theme);
    s.view = view;
    if (!s.drag.active && editor::CommandPressed(editor::Command::SelectAll,s.bindings,
        ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))) {
        if (!p.all) out.overflow=true;
        else {
            auto ids=p.all(p.user,s.selection);
            if (ids.size()>selection.storage.size() || ids.size()>out.storage.size()-out.count) out.overflow=true;
            else {
                selection.Clear();
                for (auto selected:ids) {selection.Set(selected,true);Emit(out,selected,p.revision,editor::EditKind::Select);}
            }
        }
    }
    auto *d = ImGui::GetWindowDrawList();
    if (s.checker) for (int y=0;y<8;++y) for (int x=0;x<8;++x) {
        const auto a=UVScreen({x/8.,y/8.},s.canvas,view.min),b=UVScreen({(x+1)/8.,(y+1)/8.},s.canvas,view.min);
        d->AddRectFilled(a,b,ImGui::GetColorU32((x+y)%2?theme.colors.surface:theme.colors.border));
    }
    if (s.showTexture && texture.GetTexID())
        d->AddImage(texture, UVScreen({0, 0}, s.canvas, view.min), UVScreen({1, 1}, s.canvas, view.min));
    if (s.grid) editor::DrawGrid(view, s.canvas, {.1, .1}, theme);
    auto vertices = p.vertices ? p.vertices(p.user, view.visible) : std::span<const UVVertex>{};
    auto edges = p.edges ? p.edges(p.user, view.visible) : std::span<const UVEdge>{};
    auto preview=[&](StableId vertexId,editor::Point original) {
        if (!vertexId || !s.drag.active || p.revision!=s.drag.draft.revision ||
            s.drag.draft.phase==editor::Phase::Cancel || ImGui::IsKeyPressed(ImGuiKey_Escape)) return original;
        const editor::Transaction *transaction=s.drag.draft.target==vertexId?&s.drag:nullptr;
        for (const auto &drag:s.companionDrags.first(s.companionCount))
            if (drag.draft.target==vertexId) {transaction=&drag;break;}
        if (!transaction) return original;
        return editor::Point{transaction->draft.proposed.x,transaction->draft.proposed.y};
    };
    for (const auto &e : edges)
        d->AddLine(UVScreen(preview(e.aVertex,e.a), s.canvas, view.min), UVScreen(preview(e.bVertex,e.b), s.canvas, view.min),
                   ImGui::GetColorU32(e.seam       ? theme.colors.destructive
                                      : (e.selected || (s.selection==UVSelection::Edge && selection.Contains(e.id))) ? theme.colors.warning
                                                   : theme.colors.text),
                   e.seam ? 2.f : 1.f);
    StableId hit = 0;
    editor::Point original{};
    auto mouse = ImGui::GetIO().MousePos;
    for (const auto &vertex : vertices) {
        auto pos = UVScreen(preview(vertex.id,vertex.uv), s.canvas, view.min);
        d->AddCircleFilled(pos, selection.Contains(vertex.id) ? 5.f : 3.f,
                           ImGui::GetColorU32(vertex.pinned                   ? theme.colors.destructive
                                              : selection.Contains(vertex.id) ? theme.colors.warning
                                                                              : theme.colors.text));
        if (selection.active==vertex.id)
            d->AddCircle(pos,8,ImGui::GetColorU32(theme.colors.text),0,2);
        if (vertex.pinned) {
            const auto color=ImGui::GetColorU32(theme.colors.text);
            d->AddLine({pos.x-4,pos.y-6},{pos.x+4,pos.y-6},color,2);
            d->AddLine({pos.x,pos.y-6},{pos.x,pos.y-11},color,2);
        }
        if (s.selection==UVSelection::Vertex && view.hovered && std::hypot(mouse.x - pos.x, mouse.y - pos.y) < 8) {
            d->AddCircle(pos,10,ImGui::GetColorU32(theme.colors.text));
            if (s.coordinates==UVCoordinates::Pixel)
                ImGui::SetTooltip("Pixel (%.2f, %.2f)%s",vertex.uv.x*s.imageSize.x,vertex.uv.y*s.imageSize.y,
                    vertex.pinned?" - Pinned":"");
            else if (s.coordinates==UVCoordinates::Tiles) {
                const double u=std::floor(vertex.uv.x),v=std::floor(vertex.uv.y);
                if (u>=0 && u<10 && v>=0)
                    ImGui::SetTooltip("UDIM %.0f - UV (%.3f, %.3f)",1001+u+10*v,vertex.uv.x-u,vertex.uv.y-v);
                else ImGui::SetTooltip("Tile (%.0f, %.0f) - outside UDIM columns",u,v);
            } else ImGui::SetTooltip("UV (%.3f, %.3f)%s",vertex.uv.x,vertex.uv.y,vertex.pinned?" - Pinned":"");
            hit = vertex.id;
            original = vertex.uv;
        }
    }
    if (s.selection==UVSelection::Edge && view.hovered) {
        double best=8;
        for (const auto &edge:edges) {
            auto a=UVScreen(preview(edge.aVertex,edge.a),s.canvas,view.min);
            auto b=UVScreen(preview(edge.bVertex,edge.b),s.canvas,view.min);
            double dx=b.x-a.x,dy=b.y-a.y,length=dx*dx+dy*dy;
            double t=length>0?std::clamp(((mouse.x-a.x)*dx+(mouse.y-a.y)*dy)/length,0.,1.):0;
            double distance=std::hypot(mouse.x-a.x-t*dx,mouse.y-a.y-t*dy);
            if (distance<best) {best=distance;hit=edge.id;}
        }
    }
    if (p.faces) for (const auto &face:p.faces(p.user,view.visible)) {
        if (face.vertices.size()<3) continue;
        const auto id=s.selection==UVSelection::Island?face.island:face.id;
        bool inside=false;
        const bool selected=(s.selection==UVSelection::Face || s.selection==UVSelection::Island) && selection.Contains(id);
        for (std::size_t i=0,j=face.vertices.size()-1;i<face.vertices.size();j=i++) {
            auto a=UVScreen(preview(face.vertices[j].id,face.vertices[j].uv),s.canvas,view.min);
            auto b=UVScreen(preview(face.vertices[i].id,face.vertices[i].uv),s.canvas,view.min);
            if ((a.y>mouse.y)!=(b.y>mouse.y) && mouse.x<(b.x-a.x)*(mouse.y-a.y)/(b.y-a.y)+a.x) inside=!inside;
            if (selected || face.overlap)
                d->AddLine(a,b,ImGui::GetColorU32(face.overlap?theme.colors.destructive:theme.colors.warning),selected?3.f:2.f);
        }
        if (inside && view.hovered && (s.selection==UVSelection::Face || s.selection==UVSelection::Island)) hit=id;
        if (face.overlap) {
            auto position=UVScreen(preview(face.vertices.front().id,face.vertices.front().uv),s.canvas,view.min);
            d->AddText(position,ImGui::GetColorU32(theme.colors.destructive),"Overlap");
        }
    }
    auto capacity=[&] {
        if (out.storage.size()-out.count>=1+s.companionCount) return true;
        out.overflow=true;return false;
    };
    auto finish=[&](bool cancel) {
        cancel |= s.drag.draft.phase==editor::Phase::Cancel || p.revision!=s.drag.draft.revision;
        s.drag.draft.phase=cancel?editor::Phase::Cancel:editor::Phase::Commit;
        for (auto &drag:s.companionDrags.first(s.companionCount)) drag.draft.phase=s.drag.draft.phase;
        if (!capacity()) return;
        if (cancel) s.drag.Cancel(out);else s.drag.Commit(p.revision,out);
        for (auto &drag:s.companionDrags.first(s.companionCount))
            if (cancel) drag.Cancel(out);else drag.Commit(p.revision,out);
        s.companionCount=0;
    };
    if (s.drag.active && (s.drag.draft.revision!=p.revision || ImGui::IsKeyPressed(ImGuiKey_Escape) ||
        s.drag.draft.phase==editor::Phase::Cancel || s.drag.draft.phase==editor::Phase::Commit))
        finish(ImGui::IsKeyPressed(ImGuiKey_Escape));
    if (view.hovered && hit && ImGui::IsMouseClicked(0) && !s.drag.active) {
        if ((!selection.Contains(hit) || ImGui::GetIO().KeyCtrl) &&
            !selection.Set(hit,ImGui::GetIO().KeyCtrl,ImGui::GetIO().KeyCtrl)) out.overflow=true;
        if (!selection.Contains(hit)) {editor::EndCanvas();return;}
        auto members=p.selected?p.selected(p.user,selection.storage.first(selection.count),s.selection):std::span<const UVVertex>{};
        if ((s.selection==UVSelection::Vertex ? selection.count>1 && members.size()!=selection.count : members.empty()) || members.size()>s.companionDrags.size()+1 ||
            out.storage.size()-out.count<(std::max)(std::size_t{1},members.size())) {
            out.overflow=true;editor::EndCanvas();return;
        }
        StableId primary=hit;
        if (s.selection!=UVSelection::Vertex) {primary=members.front().id;original=members.front().uv;}
        s.mouseStart = {mouse.x, mouse.y};
        const auto kind=s.tool==TransformTool::Rotate?editor::EditKind::Rotate:
            s.tool==TransformTool::Scale?editor::EditKind::Scale:editor::EditKind::Translate;
        s.drag.Begin(primary, p.revision, kind, Value({original.x, original.y, 0}),
                     editor::CurrentModifiers(), out);
        s.companionCount=0;
        for (const auto &vertex:members) if (vertex.id!=primary)
            s.companionDrags[s.companionCount++].Begin(vertex.id,p.revision,kind,Value({vertex.uv.x,vertex.uv.y,0}),editor::CurrentModifiers(),out);
    }
    if (s.drag.active && s.drag.draft.phase!=editor::Phase::Cancel && s.drag.draft.phase!=editor::Phase::Commit && capacity()) {
        editor::Point delta{(mouse.x - s.mouseStart.x) / s.canvas.scale.x,
                            (mouse.y - s.mouseStart.y) / s.canvas.scale.y};
        auto update=[&](editor::Transaction &drag) {
        auto start = drag.draft.original;
        auto transformed = TransformUV(
            {start.x, start.y}, s.pivot, s.tool == TransformTool::Translate ? delta : editor::Point{},
            s.tool == TransformTool::Rotate ? delta.x : 0,
            s.tool == TransformTool::Scale ? editor::Point{1 + delta.x, 1 + delta.y} : editor::Point{1, 1},
            s.snap);
        auto value = Value({transformed.x, transformed.y, 0});
        if (ImGui::IsMouseDown(0) && !(value == drag.draft.proposed)) drag.Update(p.revision,value,out);
        };
        update(s.drag);
        for (auto &drag:s.companionDrags.first(s.companionCount)) update(drag);
        if (!ImGui::IsMouseDown(0)) finish(false);
    }
    if (p.selectionQuery && !s.drag.active && (!hit || s.canvas.selecting)) {
        struct Query {const UVProvider &provider;UVSelection mode;} query{p,s.selection};
        editor::CanvasSelection(view,s.canvas,{&query,p.revision,
            [](void *u,editor::Rect bounds) {
                auto &q=*static_cast<Query *>(u);return q.provider.selectionQuery(q.provider.user,bounds,q.mode);
            }},selection,out,theme,s.lassoSelect);
    }
    ImGui::PushID(id);
    if (view.hovered && ImGui::IsMouseReleased(1) && !s.drag.active) ImGui::OpenPopup("UV selection options");
    if (ImGui::BeginPopup("UV selection options")) {
        int selectionMode=static_cast<int>(s.selection);
        if (ImGui::Combo("Selection",&selectionMode,"Vertex\0Edge\0Face\0Island\0")) {
            s.selection=static_cast<UVSelection>(selectionMode);selection.Clear();
        }
        ImGui::Checkbox("Checker",&s.checker);
        ImGui::Checkbox("Texture",&s.showTexture);
        ImGui::Checkbox("Grid",&s.grid);
        ImGui::Checkbox("Lasso selection",&s.lassoSelect);
        int coordinates=static_cast<int>(s.coordinates);
        if (ImGui::Combo("Coordinates",&coordinates,"Normalized\0Pixel\0UDIM\0"))
            s.coordinates=static_cast<UVCoordinates>(coordinates);
        int tool=s.tool==TransformTool::Rotate?1:s.tool==TransformTool::Scale?2:0;
        if (ImGui::Combo("Transform",&tool,"Move\0Rotate\0Scale\0"))
            s.tool=tool==1?TransformTool::Rotate:tool==2?TransformTool::Scale:TransformTool::Translate;
        double pivot[2]={s.pivot.x,s.pivot.y};
        if (ImGui::DragScalarN("Pivot",ImGuiDataType_Double,pivot,2,.01f)) s.pivot={pivot[0],pivot[1]};
        const double minimum=0,maximum=1;
        ImGui::DragScalar("Snap step",ImGuiDataType_Double,&s.snap,.001f,&minimum,&maximum,"%.3f",ImGuiSliderFlags_AlwaysClamp);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    editor::EndCanvas();
}
void AnimationStrips(const char *id, std::span<const StripView> strips, std::uint64_t revision,
                     editor::CanvasState &canvas, editor::Transaction &drag, editor::EventBuffer &out,
                     const Theme &theme, ImVec2 size) {
    detail::ResumeTerminal(drag, revision, out);
    auto view = editor::BeginCanvas(id, canvas, size, theme);
    auto *d = ImGui::GetWindowDrawList();
    int row = 0;
    if (drag.active && (drag.draft.revision != revision || ImGui::IsKeyPressed(ImGuiKey_Escape)))
        drag.Cancel(out);
    for (const auto &strip : strips) {
        float y = view.min.y + row++ * 30;
        auto range=strip.range;
        double repeat=strip.repeat,blend=strip.blend,scale=strip.scale;
        if (drag.active && drag.draft.target==strip.id && drag.draft.phase!=editor::Phase::Cancel) {
            if (drag.draft.kind==editor::EditKind::Move || drag.draft.kind==editor::EditKind::TrimStart ||
                drag.draft.kind==editor::EditKind::TrimEnd) range={drag.draft.proposed.first,drag.draft.proposed.last};
            if (drag.draft.kind==editor::EditKind::StripSettings) {
                repeat=drag.draft.proposed.y;blend=drag.draft.proposed.z;scale=drag.draft.proposed.x;
            }
        }
        float x = view.min.x +
                  static_cast<float>((editor::Seconds(range.first) - canvas.origin.x) * canvas.scale.x);
        float w = static_cast<float>(editor::Seconds(range.last - range.first) * canvas.scale.x);
        const auto fill=strip.muted?theme.colors.muted:theme.colors.accent;
        const auto textColor=ImGui::GetColorU32(fill.x*.2126f+fill.y*.7152f+fill.z*.0722f>.5f ?
            ImVec4{.06f,.06f,.08f,1}:ImVec4{.96f,.96f,.98f,1});
        d->AddRectFilled({x, y}, {x + w, y + 26},ImGui::GetColorU32(fill),4);
        d->PushClipRect({x,y},{x+(std::max)(1.f,w),y+26},true);
        if (repeat>1 && std::isfinite(repeat)) {
            const int divisions=static_cast<int>((std::min)(repeat,128.));
            for (int i=1;i<=divisions;++i) {
                float boundary=x+static_cast<float>(w*i/repeat);
                d->AddLine({boundary,y+2},{boundary,y+24},ImGui::GetColorU32(theme.colors.border));
            }
        }
        d->AddLine({x+2,y+24},{x+2+(std::max)(0.f,w-4)*static_cast<float>(std::clamp(blend,0.,1.)),y+24},
                   ImGui::GetColorU32(theme.colors.text),2);
        char description[256];
        std::snprintf(description,sizeof(description),"%s  x%.2f / %.2f repeats%s%s",strip.label,scale,repeat,
            strip.muted?" [Muted]":"",strip.locked?" [Locked]":"");
        d->AddText({x + 4, y + 4}, textColor, description);
        d->PopClipRect();
        ImGui::SetCursorScreenPos({x, y});
        ImGui::PushID(reinterpret_cast<void *>(static_cast<std::uintptr_t>(strip.id)));
        ImGui::BeginDisabled(strip.locked);
        ImGui::InvisibleButton("strip", {(std::max)(1.f, w), 26});
        if (ImGui::IsItemActivated()) {
            const float mx=ImGui::GetIO().MousePos.x;
            auto kind=mx<x+6 ? editor::EditKind::TrimStart : mx>x+w-6 ? editor::EditKind::TrimEnd : editor::EditKind::Move;
            drag.Begin(strip.id, revision, kind, {strip.range.first, strip.range.last},editor::CurrentModifiers(), out);
        }
        if (ImGui::IsItemHovered() && (ImGui::GetIO().MousePos.x<x+6 || ImGui::GetIO().MousePos.x>x+w-6))
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (ImGui::IsItemActive() && drag.active) {
            auto value = drag.draft.proposed;
            auto delta = editor::FromSeconds(ImGui::GetIO().MouseDelta.x / canvas.scale.x);
            if (drag.draft.kind==editor::EditKind::TrimStart) value.first=(std::min)(value.first+delta,value.last-1);
            else if (drag.draft.kind==editor::EditKind::TrimEnd) value.last=(std::max)(value.last+delta,value.first+1);
            else {value.first+=delta;value.last+=delta;}
            if (delta)
                drag.Update(revision, value, out);
        }
        if (ImGui::IsItemDeactivated() && drag.active)
            drag.Commit(revision, out);
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::IsMouseReleased(1))
            ImGui::OpenPopup("strip settings");
        if (ImGui::BeginPopup("strip settings")) {
            const editor::Value original{strip.range.first,strip.range.last,
                (strip.muted?1:0)|(strip.locked?2:0),strip.channel,strip.scale,strip.repeat,strip.blend};
            auto value=drag.active && drag.draft.target==strip.id &&
                drag.draft.kind==editor::EditKind::StripSettings ? drag.draft.proposed : original;
            ImGui::BeginDisabled(strip.locked);
            auto setting=[&](const char *label,double &field,double low,double high) {
                bool changed=ImGui::DragScalar(label,ImGuiDataType_Double,&field,.01f,&low,&high,"%.3f",ImGuiSliderFlags_AlwaysClamp);
                if (ImGui::IsItemActivated())
                    drag.Begin(strip.id,revision,editor::EditKind::StripSettings,original,editor::CurrentModifiers(),out);
                if (changed && drag.active) drag.Update(revision,value,out);
                if (ImGui::IsItemDeactivated() && drag.active) drag.Commit(revision,out);
            };
            setting("Scale",value.x,.001,1000);setting("Repeat",value.y,.001,1000);setting("Blend",value.z,0,1);
            const auto index=static_cast<std::size_t>(&strip-strips.data());
            for (int direction:{-1,1}) {
                const auto neighbor=static_cast<std::ptrdiff_t>(index)+direction;
                const bool available=neighbor>=0 && neighbor<static_cast<std::ptrdiff_t>(strips.size()) &&
                    !strips[neighbor].locked;
                if (ImGui::MenuItem(direction<0?"Move up":"Move down",nullptr,false,available))
                    Emit(out,strip.id,revision,editor::EditKind::Reorder,{},
                         {0,0,direction,strips[neighbor].id});
            }
            bool muted=strip.muted;
            if (ImGui::Checkbox("Mute",&muted)) {
                value=original;value.offset=muted?value.offset|1:value.offset&~editor::Tick{1};
                Emit(out,strip.id,revision,editor::EditKind::StripSettings,original,value);
            }
            ImGui::EndDisabled();
            bool locked=strip.locked;
            if (ImGui::Checkbox("Lock",&locked)) {
                value=original;value.offset=locked?value.offset|2:value.offset&~editor::Tick{2};
                Emit(out,strip.id,revision,editor::EditKind::StripSettings,original,value);
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    editor::EndCanvas();
}
void DopeSheet(const char *id, const editor::CurveProvider &p, editor::CurveState &s,
               editor::Selection &selection, editor::EventBuffer &out, const Theme &theme, ImVec2 size) {
    auto capacity=[&](std::size_t count) {
        if (out.storage.size()-out.count>=count) return true;
        out.overflow=true;return false;
    };
    auto finish=[&](bool cancel) {
        cancel |= s.drag.draft.phase==editor::Phase::Cancel || s.drag.draft.revision!=p.revision;
        s.drag.draft.phase=cancel?editor::Phase::Cancel:editor::Phase::Commit;
        for (auto &drag:s.companionDrags.first(s.companionCount)) drag.draft.phase=s.drag.draft.phase;
        if (!capacity(1+s.companionCount)) return;
        if (cancel) s.drag.Cancel(out);else s.drag.Commit(p.revision,out);
        for (auto &drag:s.companionDrags.first(s.companionCount))
            if (cancel) drag.Cancel(out);else drag.Commit(p.revision,out);
        s.companionCount=0;
    };
    if (s.drag.active && (s.drag.draft.phase==editor::Phase::Cancel ||
        s.drag.draft.phase==editor::Phase::Commit || s.drag.draft.revision!=p.revision ||
        ImGui::IsKeyPressed(ImGuiKey_Escape))) finish(ImGui::IsKeyPressed(ImGuiKey_Escape));
    s.canvas.wheelZoomY = false;
    auto view = editor::BeginCanvas(id, s.canvas, size, theme);
    s.view=view;
    auto *d = ImGui::GetWindowDrawList();
    auto keys =
        p.query
            ? p.query(
                  p.user,
                  {{editor::FromSeconds(view.visible.min.x), editor::FromSeconds(view.visible.max.x)}, 0, 0})
            : std::span<const editor::Keyframe>{};
    if (!s.drag.active && selection.count && editor::CommandPressed(editor::Command::Delete,s.bindings,
        ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))) {
        auto targets=p.selected ? p.selected(p.user,selection.storage.first(selection.count))
                                : std::span<const editor::Keyframe>{};
        if (!p.selected && selection.count==1) {
            auto key=std::find_if(keys.begin(),keys.end(),[&](const auto &k){return selection.Contains(k.id);});
            if (key!=keys.end()) targets={&*key,1};
        }
        bool available=targets.size()==selection.count;
        if (!available) out.overflow=true;
        for (const auto &key:targets) available &= !key.locked;
        if (available && capacity(targets.size())) for (const auto &key:targets)
            Emit(out,key.id,p.revision,editor::EditKind::Remove,{key.tick,0,0,0,key.value});
    }
    StableId channel = 0, hit = 0;
    int row = -1;
    editor::Value original{};
    auto mouse = ImGui::GetIO().MousePos;
    if (s.drag.active && s.drag.draft.phase!=editor::Phase::Cancel &&
        s.drag.draft.phase!=editor::Phase::Commit && capacity(1+s.companionCount)) {
        const double dx=(mouse.x-s.mouseStart.x)/s.canvas.scale.x;
        auto delta=editor::FromSeconds(dx);
        if (s.snapToFrame && !s.scaling)
            delta=editor::FrameToTick(editor::TickToFrame(s.drag.draft.original.first+delta,s.rate),s.rate)-
                s.drag.draft.original.first;
        auto update=[&](editor::Transaction &drag) {
            auto value=drag.draft.original;
            if (s.scaling) {
                const double factor=std::exp2(std::clamp(dx*s.canvas.scale.x/100.,-16.,16.));
                value.first=s.scalePivot+static_cast<editor::Tick>(std::llround(
                    static_cast<long double>(value.first-s.scalePivot)*factor));
                if (s.snapToFrame) value.first=editor::FrameToTick(editor::TickToFrame(value.first,s.rate),s.rate);
            } else value.first+=delta;
            if (ImGui::IsMouseDown(0) && !(value==drag.draft.proposed)) drag.Update(p.revision,value,out);
        };
        update(s.drag);
        for (auto &drag:s.companionDrags.first(s.companionCount)) update(drag);
    }
    for (const auto &key : keys) {
        if (key.channel != channel || row < 0) {
            channel = key.channel;
            ++row;
            float y = view.min.y + row * 30;
            d->AddLine({view.min.x, y}, {view.max.x, y}, ImGui::GetColorU32(theme.colors.border));
        }
        float y = view.min.y + row * 30 + 15;
        if (y > view.max.y)
            break;
        auto tick=key.tick;
        if (s.drag.active && s.drag.draft.phase!=editor::Phase::Cancel) {
            if (s.drag.draft.target==key.id) tick=s.drag.draft.proposed.first;
            for (const auto &drag:s.companionDrags.first(s.companionCount))
                if (drag.draft.target==key.id) {tick=drag.draft.proposed.first;break;}
        }
        float x = view.min.x +
                  static_cast<float>((editor::Seconds(tick) - s.canvas.origin.x) * s.canvas.scale.x);
        auto color =
            ImGui::GetColorU32(selection.Contains(key.id) ? theme.colors.warning : theme.colors.text);
        d->AddQuadFilled({x, y - 5}, {x + 5, y}, {x, y + 5}, {x - 5, y}, color);
        if (!key.locked && std::hypot(mouse.x - x, mouse.y - y) < 8) {
            hit = key.id;
            original = {key.tick, 0, 0, 0, key.value};
        }
    }
    ImGui::PushID(id);
    if (view.hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !s.drag.active)
        ImGui::OpenPopup("dope-options");
    if (ImGui::BeginPopup("dope-options")) {
        ImGui::Checkbox("Scale key timing",&s.scaleTime);
        ImGui::Checkbox("Snap to frame",&s.snapToFrame);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    if (view.hovered && hit && ImGui::IsMouseClicked(0) && !s.drag.active) {
        if (!selection.Contains(hit) || ImGui::GetIO().KeyCtrl) {
            if (!selection.Set(hit,ImGui::GetIO().KeyCtrl,ImGui::GetIO().KeyCtrl)) out.overflow=true;
            if (!selection.Contains(hit)) {editor::EndCanvas();return;}
        }
        auto selected=p.selected ? p.selected(p.user,selection.storage.first(selection.count))
                                 : std::span<const editor::Keyframe>{};
        bool allowed=selection.count<=1 || selected.size()==selection.count;
        if (!allowed || selected.size()>s.companionDrags.size()+1) {out.overflow=true;allowed=false;}
        for (const auto &key:selected) if (key.locked) allowed=false;
        if (allowed && capacity((std::max)(std::size_t{1},selected.size()))) {
            s.mouseStart={mouse.x,mouse.y};s.companionCount=0;
            s.scaling=s.scaleTime && !ImGui::GetIO().KeyAlt;s.scalePivot=original.first;
            for (const auto &key:selected) s.scalePivot=(std::min)(s.scalePivot,key.tick);
            auto kind=ImGui::GetIO().KeyAlt ? editor::EditKind::Duplicate :
                s.scaling ? editor::EditKind::KeyScale : editor::EditKind::Keyframe;
            s.drag.Begin(hit,p.revision,kind,original,editor::CurrentModifiers(),out);
            for (const auto &key:selected) if (key.id!=hit)
                s.companionDrags[s.companionCount++].Begin(key.id,p.revision,kind,
                    {key.tick,0,0,0,key.value},editor::CurrentModifiers(),out);
        }
    }
    if (s.drag.active && !ImGui::IsMouseDown(0)) finish(false);
    editor::EndCanvas();
}
} // namespace imkit::cg
