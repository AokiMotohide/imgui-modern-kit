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
    if (tool == TransformTool::Rotate)
        result.rotation = Add(original.rotation, oriented);
    if (tool == TransformTool::Scale)
        result.scale = {original.scale.x * (1 + delta.x), original.scale.y * (1 + delta.y),
                        original.scale.z * (1 + delta.z)};
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
    ViewportView v{ImGui::GetCursorScreenPos(), ImGui::GetContentRegionAvail(), ImGui::IsWindowHovered()};
    auto *d = ImGui::GetWindowDrawList();
    d->PushClipRect(v.min, {v.min.x + v.size.x, v.min.y + v.size.y}, true);
    if (texture.GetTexID())
        d->AddImage(texture, v.min, {v.min.x + v.size.x, v.min.y + v.size.y}, {0, 1}, {1, 0});
    else
        d->AddRectFilled(v.min, {v.min.x + v.size.x, v.min.y + v.size.y},
                         ImGui::GetColorU32(theme.colors.canvas));
    auto line = [&](Vec3 a, Vec3 b, ImU32 color) {
        auto pa = Project(a, s.camera, v.min, v.size), pb = Project(b, s.camera, v.min, v.size);
        if (pa.visible && pb.visible)
            d->AddLine(pa.screen, pb.screen, color);
    };
    if (s.grid)
        for (int i = -10; i <= 10; ++i) {
            line({static_cast<double>(i), 0, -10}, {static_cast<double>(i), 0, 10},
                 ImGui::GetColorU32(theme.colors.border));
            line({-10, 0, static_cast<double>(i)}, {10, 0, static_cast<double>(i)},
                 ImGui::GetColorU32(theme.colors.border));
        }
    if (s.axes) {
        line({0, 0, 0}, {3, 0, 0}, IM_COL32(220, 85, 85, 255));
        line({0, 0, 0}, {0, 3, 0}, IM_COL32(90, 195, 100, 255));
        line({0, 0, 0}, {0, 0, 3}, IM_COL32(95, 145, 235, 255));
    }
    bool navigationHovered=false;
    if (s.navigationGizmo && v.size.x>=110 && v.size.y>=110) {
        ImVec2 corner{v.min.x+v.size.x-104,v.min.y+8},center{corner.x+48,corner.y+48};
        ImGui::SetCursorScreenPos(corner);
        ImGui::InvisibleButton("navigation",{96,96});
        navigationHovered=ImGui::IsItemHovered();
        auto basis=OrientationBasis(Orientation::View,{},s.camera);
        Vec3 axes[]={{1,0,0},{0,1,0},{0,0,1}};
        const ImU32 colors[]={IM_COL32(220,85,85,255),IM_COL32(90,195,100,255),IM_COL32(95,145,235,255)};
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
        selection.Set(hit, ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyCtrl);
        Emit(out, hit, revision, editor::EditKind::Select);
    }
}
void TransformGizmo(const ViewportView &v, const ObjectView &object, ViewportState &s, std::uint64_t revision,
                    editor::EventBuffer &out, const Theme &) {
    detail::ResumeTerminal(s.drag, revision, out);
    if (s.drag.active && (revision != s.drag.draft.revision || ImGui::IsKeyPressed(ImGuiKey_Escape)))
        s.drag.Cancel(out);
    if (!s.gizmo || s.tool == TransformTool::Select || object.locked)
        return;
    auto pivot = s.pivot == Pivot::Individual ? object.transform.translation : s.pivotPosition;
    auto center = Project(pivot, s.camera, v.min, v.size);
    if (!center.visible)
        return;
    auto basis = OrientationBasis(s.orientation, object.transform, s.camera, s.parentBasis, s.customBasis);
    auto *d = ImGui::GetWindowDrawList();
    const Vec3 axes[] = {basis.x, basis.y, basis.z};
    const ImU32 colors[] = {IM_COL32(230, 85, 85, 255), IM_COL32(85, 220, 110, 255),
                            IM_COL32(85, 145, 240, 255)};
    Axis hit = Axis::None;
    auto mouse = ImGui::GetIO().MousePos;
    for (int i = 0; i < 3; ++i) {
        auto end = Project(Add(pivot, axes[i]), s.camera, v.min, v.size);
        if (!end.visible)
            continue;
        double dx = end.screen.x - center.screen.x, dy = end.screen.y - center.screen.y,
               length = std::hypot(dx, dy);
        if (length < 1)
            continue;
        ImVec2 tip{center.screen.x + static_cast<float>(dx / length * 70),
                   center.screen.y + static_cast<float>(dy / length * 70)};
        d->AddLine(center.screen, tip, colors[i], 3);
        d->AddCircleFilled(tip, 6, colors[i]);
        const char *label = i == 0 ? "X" : i == 1 ? "Y" : "Z";
        d->AddText({tip.x + 8, tip.y}, colors[i], label);
        if (std::hypot(mouse.x - tip.x, mouse.y - tip.y) < 12)
            hit = static_cast<Axis>(i + 1);
    }
    if (v.hovered && hit != Axis::None && ImGui::IsMouseClicked(0) && !s.drag.active) {
        s.activeAxis = hit;
        s.mouseStart = {mouse.x, mouse.y};
        s.original = object.transform;
        auto kind = s.tool == TransformTool::Rotate  ? editor::EditKind::Rotate
                    : s.tool == TransformTool::Scale ? editor::EditKind::Scale
                                                     : editor::EditKind::Translate;
        auto value = Value(s.tool == TransformTool::Rotate  ? object.transform.rotation
                           : s.tool == TransformTool::Scale ? object.transform.scale
                                                            : object.transform.translation);
        s.drag.Begin(object.id, revision, kind, value, editor::CurrentModifiers(), out);
    }
    if (s.drag.active) {
        double delta = ((mouse.x - s.mouseStart.x) - (mouse.y - s.mouseStart.y)) * .01;
        Vec3 dv{delta, delta, delta};
        auto result = TransformDelta(s.original, s.tool, s.activeAxis, dv, basis, s.snap ? .1 : 0,
                                     ImGui::GetIO().KeyShift);
        auto value = Value(s.tool == TransformTool::Rotate  ? result.rotation
                           : s.tool == TransformTool::Scale ? result.scale
                                                            : result.translation);
        if (ImGui::IsMouseDown(0) && !(value == s.drag.draft.proposed))
            s.drag.Update(revision, value, out);
        if (ImGui::IsMouseReleased(0))
            s.drag.Commit(revision, out);
    }
}
void EndViewport() {
    ImGui::GetWindowDrawList()->PopClipRect();
    ImGui::EndChild();
    ImGui::PopID();
}
void Outliner(const char *id, const SceneProvider &p, OutlinerState &s, editor::Selection &selection,
              editor::EventBuffer &out) {
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
                if (ImGui::Selectable(row.label, selection.Contains(row.id)) && row.selectable &&
                    !row.locked) {
                    selection.Set(row.id, ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyCtrl);
                    Emit(out, row.id, p.revision, editor::EditKind::Select);
                }
                if (ImGui::BeginDragDropSource()) {
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
    if (texture.GetTexID())
        d->AddImage(texture, UVScreen({0, 0}, s.canvas, view.min), UVScreen({1, 1}, s.canvas, view.min));
    editor::DrawGrid(view, s.canvas, {.1, .1}, theme);
    auto vertices = p.vertices ? p.vertices(p.user, view.visible) : std::span<const UVVertex>{};
    auto edges = p.edges ? p.edges(p.user, view.visible) : std::span<const UVEdge>{};
    for (const auto &e : edges)
        d->AddLine(UVScreen(e.a, s.canvas, view.min), UVScreen(e.b, s.canvas, view.min),
                   ImGui::GetColorU32(e.seam       ? theme.colors.destructive
                                      : e.selected ? theme.colors.warning
                                                   : theme.colors.text),
                   e.seam ? 2.f : 1.f);
    StableId hit = 0;
    editor::Point original{};
    auto mouse = ImGui::GetIO().MousePos;
    for (const auto &vertex : vertices) {
        auto pos = UVScreen(vertex.uv, s.canvas, view.min);
        d->AddCircleFilled(pos, selection.Contains(vertex.id) ? 5.f : 3.f,
                           ImGui::GetColorU32(vertex.pinned                   ? theme.colors.destructive
                                              : selection.Contains(vertex.id) ? theme.colors.warning
                                                                              : theme.colors.text));
        if (std::hypot(mouse.x - pos.x, mouse.y - pos.y) < 8) {
            hit = vertex.id;
            original = vertex.uv;
        }
    }
    detail::ResumeTerminal(s.drag, p.revision, out);
    if (s.drag.active && (s.drag.draft.revision != p.revision || ImGui::IsKeyPressed(ImGuiKey_Escape)))
        s.drag.Cancel(out);
    if (view.hovered && hit && ImGui::IsMouseClicked(0) && !s.drag.active) {
        selection.Set(hit, ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyCtrl);
        s.mouseStart = {mouse.x, mouse.y};
        s.drag.Begin(hit, p.revision, editor::EditKind::Translate, Value({original.x, original.y, 0}),
                     editor::CurrentModifiers(), out);
    }
    if (s.drag.active) {
        editor::Point delta{(mouse.x - s.mouseStart.x) / s.canvas.scale.x,
                            (mouse.y - s.mouseStart.y) / s.canvas.scale.y};
        auto start = s.drag.draft.original;
        auto transformed = TransformUV(
            {start.x, start.y}, s.pivot, s.tool == TransformTool::Translate ? delta : editor::Point{},
            s.tool == TransformTool::Rotate ? delta.x : 0,
            s.tool == TransformTool::Scale ? editor::Point{1 + delta.x, 1 + delta.y} : editor::Point{1, 1},
            s.snap);
        auto value = Value({transformed.x, transformed.y, 0});
        if (ImGui::IsMouseDown(0) && !(value == s.drag.draft.proposed))
            s.drag.Update(p.revision, value, out);
        if (ImGui::IsMouseReleased(0))
            s.drag.Commit(p.revision, out);
    }
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
