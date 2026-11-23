#include "editor_workspaces_internal.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace imkit::gallery {
using namespace detail;

cg::Vec3 EditorWorkspaces::SelectionPivot(cg::Pivot mode) const {
    if (mode==cg::Pivot::Cursor) return cursorPivot;
    cg::Vec3 sum{},low{},high{};std::size_t count=0;bool haveBounds=false;
    const auto include=[&](cg::Vec3 point) {
        if (!haveBounds) {low=high=point;haveBounds=true;}
        else {
            low={std::min(low.x,point.x),std::min(low.y,point.y),std::min(low.z,point.z)};
            high={std::max(high.x,point.x),std::max(high.y,point.y),std::max(high.z,point.z)};
        }
    };
    for (const auto &object:objects) if (objectSelection.Contains(object.id)) {
        const auto p=object.transform.translation;sum.x+=p.x;sum.y+=p.y;sum.z+=p.z;++count;
        if (mode!=cg::Pivot::Bounds) continue;
        const auto geometry=geometries.find(object.geometry);
        if (!object.geometry || geometry==geometries.end() || geometry->second.vertices.empty()) {include(p);continue;}
        const auto basis=cg::LinearBasis(object.transform);
        for (const auto &vertex:geometry->second.vertices) {
            const auto x=vertex.position[0],y=vertex.position[1],z=vertex.position[2];
            include({p.x+basis.x.x*x+basis.y.x*y+basis.z.x*z,
                     p.y+basis.x.y*x+basis.y.y*y+basis.z.y*z,
                     p.z+basis.x.z*x+basis.y.z*y+basis.z.z*z});
        }
    }
    if (!count) return {};
    return mode==cg::Pivot::Bounds ? cg::Vec3{(low.x+high.x)/2,(low.y+high.y)/2,(low.z+high.z)/2} :
        cg::Vec3{sum.x/count,sum.y/count,sum.z/count};
}
void EditorWorkspaces::RebuildOutlinerRows() {
    outlinerRows.clear();outlinerRows.reserve(objects.size());
    orderedObjects.assign(objects.begin(),objects.end());
    auto &ordered=orderedObjects;
    std::sort(ordered.begin(),ordered.end(),[&](const auto &a,const auto &b){
        return std::find(objectOrder.begin(),objectOrder.end(),a.id)<std::find(objectOrder.begin(),objectOrder.end(),b.id);
    });
    for (const auto &component:components) {
        const auto owner=std::find_if(objects.begin(),objects.end(),[&](const auto &o){return o.id==component.view.owner;});
        if (owner==objects.end()) continue;
        cg::ObjectView row;row.id=component.view.id;row.parent=component.view.owner;row.label=component.view.label;
        row.kind=component.wireOverride ? cg::ObjectKind::Modifier : cg::ObjectKind::Component;
        row.visible=component.view.enabled;row.locked=component.view.locked || owner->locked;row.expanded=component.view.expanded;
        row.transform=owner->transform;ordered.push_back(row);
    }
    ImGuiTextFilter filter(outliner.search);
    auto matches=[&](auto &&self,const cg::ObjectView &object,std::size_t depth)->bool {
        if (depth>ordered.size()) return false;
        const bool stateMatches=outliner.filter==cg::OutlinerFilter::All ||
            (outliner.filter==cg::OutlinerFilter::Visible && object.visible) ||
            (outliner.filter==cg::OutlinerFilter::Hidden && !object.visible) ||
            (outliner.filter==cg::OutlinerFilter::Locked && object.locked) ||
            (outliner.filter==cg::OutlinerFilter::Selected && objectSelection.Contains(object.id));
        if (stateMatches && (outliner.kindFilter<0 || outliner.kindFilter==static_cast<int>(object.kind)) && filter.PassFilter(object.label)) return true;
        for (const auto &child:ordered) if (child.parent==object.id && self(self,child,depth+1)) return true;
        return false;
    };
    auto append=[&](auto &&self,const cg::ObjectView &object,int depth)->void {
        if (depth>=static_cast<int>(ordered.size()) || !matches(matches,object,0)) return;
        auto row=object;row.depth=depth;
        row.hasChildren=std::any_of(ordered.begin(),ordered.end(),[&](const auto &child){return child.parent==object.id;});
        outlinerRows.push_back(row);
        if (object.expanded || filter.IsActive() || outliner.filter!=cg::OutlinerFilter::All || outliner.kindFilter>=0)
            for (const auto &child:ordered) if (child.parent==object.id) self(self,child,depth+1);
    };
    for (const auto &object:ordered)
        if (!object.parent || std::none_of(ordered.begin(),ordered.end(),[&](const auto &parent){return parent.id==object.parent;}))
            append(append,object,0);
}
void CGWorkspace(EditorWorkspaces &s, const Theme &theme, ImTextureRef texture) {
    CompactEditorScope density;
    s.viewport.labels={};
    if (s.japanese) {
        auto &l=s.viewport.labels;
        l.tools={"選択","移動","回転","拡縮","統合"};
        l.projections={"透視投影","平行投影","カメラ"};
        l.orientations={"ワールド","ローカル","ビュー","親","カスタム"};
        l.pivots={"各原点","中心","境界中心","カーソル"};
        l.shading={"ワイヤー","ソリッド"};
        l.grid="グリッド";l.gizmo="ギズモ";l.snap="スナップ";
        l.vertexNormals="頂点法線";l.faceNormals="面法線";
        l.boxSelect="矩形選択";l.lassoSelect="投げ縄選択";
        l.alignView="軸をクリックして視点を整列";l.orbit="中ボタンドラッグで回転、Shift併用で平行移動";
        l.overlays="表示";l.axes="軸";l.origins="原点";
        l.cameraFrame="カメラ枠";l.safeFrame="安全枠";l.renderRegion="レンダー領域";
        l.passepartout="枠外を暗くする";l.measurement="寸法";l.selectionOutline="選択輪郭";
    }

    s.curve.icons=s.icons;
    s.uvState.icons=s.icons;
    s.Initialize();
    Options(s);
    float width = ImGui::GetContentRegionAvail().x, side = (s.narrow ? 180 : s.cgSide)*ImGui::GetFontSize()/14,
          top = ImGui::GetContentRegionAvail().y * s.cgTopRatio;
    const bool compact=width<1000*ImGui::GetFontSize()/14;
    if(compact) {imkit::Checkbox("Inspector",&s.showInspector);top=ImGui::GetContentRegionAvail().y*s.cgTopRatio;}
    const bool inspectorVisible=!compact || s.showInspector;
    const float workspaceHeight=ImGui::GetContentRegionAvail().y;
    side=std::min(side,width*.28f);
    const char *aspectNames[]={"4:3","16:9","1:1","9:16"};
    ImGui::SetNextItemWidth(100);imkit::Combo(s.japanese?"Viewport比率":"Viewport aspect",&s.cgPreviewAspect,aspectNames,4);
    const float aspectValues[]={4.f/3,16.f/9,1.f,9.f/16};
    const float maximumViewWidth=width-(inspectorVisible?side+10:0);
    const float estimatedCanvasHeight=std::max(100.f,top-2*ImGui::GetFrameHeightWithSpacing()-ImGui::GetStyle().WindowPadding.y*2);
    const float viewStackWidth=std::min(maximumViewWidth,std::max(180.f,estimatedCanvasHeight*aspectValues[s.cgPreviewAspect]+ImGui::GetStyle().WindowPadding.x*2));
    ImGui::BeginChild("View stack", {viewStackWidth, top}, ImGuiChildFlags_Borders);
    s.viewport.icons=s.icons;
    if(s.icons) {Icon(*s.icons,IconId::Monitor,{ImGui::GetFontSize()});ImGui::SameLine();}
    s.previewHelpers=true;
    imkit::Checkbox("OpenGL preview", &s.useGL);
    if (s.viewport.pivot==cg::Pivot::Cursor) {
        double cursor[]={s.cursorPivot.x,s.cursorPivot.y,s.cursorPivot.z};
        ImGui::BeginDisabled(s.viewport.drag.active);
        ImGui::SetNextItemWidth(ImGui::GetFontSize()*20);
        if (ImGui::DragScalarN("Cursor pivot",ImGuiDataType_Double,cursor,3,.01f))
            s.cursorPivot={cursor[0],cursor[1],cursor[2]};
        ImGui::EndDisabled();
    }
    s.SyncSceneCamera();
    auto view = cg::BeginViewport(
        "Scene", s.viewport,
        s.useGL ? ImTextureRef(static_cast<ImTextureID>(s.previewRenderer.Texture())) : ImTextureRef{},
        {0, 0}, theme);
    s.viewportOrigin = view.min;
    s.viewportSize = view.size;
    s.viewportSelectionPoints.resize(s.objects.size());
    s.viewport.selectionPoints=s.viewportSelectionPoints;
    s.viewport.selectionCanvas.selectionPath=s.viewportSelectionPath;
    if (s.viewport.measurement) {
        s.viewport.measurementStart={};s.viewport.measurementEnd={};
        int endpoints=0;
        for (const auto &object:s.objects) if (s.objectSelection.Contains(object.id)) {
            if (endpoints++==0) s.viewport.measurementEnd=object.transform.translation;
            else {s.viewport.measurementStart=object.transform.translation;break;}
        }
    }

    if (!s.viewport.drag.active) s.viewport.pivotPosition=s.SelectionPivot(s.viewport.pivot);
    s.gizmoSelection.resize(s.objects.size());
    if (!s.viewport.drag.active) s.gizmoCompanions.resize(s.objects.size()-1);
    std::size_t selectedCount=0;
    for (const auto &object:s.objects) if (s.objectSelection.Contains(object.id)) s.gizmoSelection[selectedCount++]=object;
    s.viewport.selectedObjects={s.gizmoSelection.data(),selectedCount};
    s.viewport.companions=s.gizmoCompanions;
    bool hasActiveObject=false;
    for (const auto &o : s.objects)
        if (o.id == s.objectSelection.active) {
            hasActiveObject=true;
            cg::TransformGizmo(view, o, s.viewport, s.revision, s.events, theme);
        }
    if (!hasActiveObject && s.viewport.drag.active)
        cg::TransformGizmo(view, {}, s.viewport, s.revision, s.events, theme);
    // Apply terminal proposals before drawing overlays so the release frame uses the committed pose.
    if (!s.viewport.drag.active && s.events.count) s.ApplyEvents();
    if (!s.useGL) {
        preview::DrawListPreview(*ImGui::GetWindowDrawList(),s.BuildSceneMeshes(),s.viewport.camera,
                                 view.min,view.size,s.scratch);
    }
    if (s.viewport.selectionOutline) for (const auto &mesh:s.BuildSceneMeshes())
        if (s.objectSelection.Contains(mesh.id)) {
            if (s.outlineScratch.size()<mesh.indices.size()) s.outlineScratch.resize(mesh.indices.size());
            preview::DrawMeshOutline(*ImGui::GetWindowDrawList(),mesh,s.viewport.camera,view.min,view.size,
                s.outlineScratch,ImGui::GetColorU32(theme.colors.accent));
        }
    if (s.viewport.normals || s.viewport.faceNormals)
        preview::DrawMeshNormals(*ImGui::GetWindowDrawList(),s.BuildSceneMeshes(),s.viewport.camera,view.min,view.size,
            {s.viewport.normals,s.viewport.faceNormals,.2,ImGui::GetColorU32(theme.colors.accent)});
    cg::ViewportObjects(view,s.objects,s.viewport,s.objectSelection,s.revision,s.events,theme);
    cg::EndViewport();
    ImGui::EndChild();
    if(inspectorVisible) {
    float leftWidth=width-side-10;
    PaneDivider("scene-divider",leftWidth,top,width-140);
    s.cgSide=(width-leftWidth-10)/(ImGui::GetFontSize()/14);
    ImGui::BeginChild("Scene properties", {0, top}, ImGuiChildFlags_Borders);
    ImGui::BeginChild("Outliner", {0, top * .42f});
    s.outliner.icons=s.icons;s.outliner.labels={};
    if (s.japanese) {
        auto &l=s.outliner.labels;l.search="検索";l.name="名前";
        l.filters={"すべて","表示中","非表示","ロック中","選択中"};
        l.kinds={"全種類","オブジェクト","コレクション","メッシュ","カメラ","ライト","コンポーネント","モディファイア"};
        l.restrictions={"表示","選択可能","レンダー対象","ロック"};
        l.rename="名前変更";l.duplicate="複製";l.linkedDuplicate="リンク複製";
        l.linkGeometry="アクティブ対象の形状を共有";l.singleUser="形状を独立化";
        l.moveUp="上へ";l.moveDown="下へ";l.moveRoot="ルートへ";
        l.expand="階層を展開";l.collapse="階層を折り畳む";
    }
    s.RebuildOutlinerRows();
    cg::SceneProvider scene{&s,s.revision,static_cast<int>(s.outlinerRows.size()),
        [](void *user,int first,int count,std::string_view) {
            auto &rows=static_cast<EditorWorkspaces*>(user)->outlinerRows;
            const auto begin=std::min(rows.size(),static_cast<std::size_t>(std::max(0,first)));
            return std::span<const cg::ObjectView>(rows).subspan(begin,std::min(rows.size()-begin,static_cast<std::size_t>(std::max(0,count))));
        }};
    cg::Outliner("Hierarchy", scene, s.outliner, s.objectSelection, s.events);
    ImGui::EndChild();
    ImGui::SeparatorText("Inspector");
    auto inspectorOwner=s.objectSelection.active;
    const auto selectedComponent=std::find_if(s.components.begin(),s.components.end(),[&](const auto &c){return c.view.id==inspectorOwner;});
    if (selectedComponent!=s.components.end()) inspectorOwner=selectedComponent->view.owner;
    auto object = std::find_if(s.objects.begin(), s.objects.end(),
                               [&](const auto &o) { return o.id == inspectorOwner; });
    if (object != s.objects.end() && ImGui::BeginTabBar("Inspector pages")) {
        if (ImGui::BeginTabItem("Transform")) {
            ImGui::SetNextItemWidth(-1);
            if(imkit::BeginCombo("##curve-channel",s.japanese ? "アニメーション編集チャンネル" : "Animation channel")) {
                const char *names[]={"Position X","Position Y","Position Z","Rotation X","Rotation Y","Rotation Z","Scale X","Scale Y","Scale Z"};
                for(int i=0;i<9;++i) if(ImGui::Selectable(names[i],s.propertyCurveChannel==ObjectPropertyId(s,object->id,i))) {
                    s.propertyCurveChannel=ObjectPropertyId(s,object->id,i);s.curve.activeChannel=s.propertyCurveChannel;
                    s.curve.fitRequested=true;s.keySelection.Clear();s.animationPage=0;
                }
                if(ImGui::Selectable(s.japanese?"全体":"All channels",s.propertyCurveChannel==0)) s.propertyCurveChannel=0;
                imkit::EndCombo();
            }
            const auto displayTransform=cg::PreviewTransform(*object,s.viewport);
            editor::PropertyView rows[] = {
                {ObjectPropertyId(s, object->id, 0), "Position X", "Transform", displayTransform.translation.x, 0},
                {ObjectPropertyId(s, object->id, 1), "Position Y", "Transform", displayTransform.translation.y, 0},
                {ObjectPropertyId(s, object->id, 2), "Position Z", "Transform", displayTransform.translation.z, 0},
                {ObjectPropertyId(s, object->id, 3), "Rotation X (rad)", "Rotation", displayTransform.rotation.x, 0},
                {ObjectPropertyId(s, object->id, 4), "Rotation Y (rad)", "Rotation", displayTransform.rotation.y, 0},
                {ObjectPropertyId(s, object->id, 5), "Rotation Z (rad)", "Rotation", displayTransform.rotation.z, 0},
                {ObjectPropertyId(s, object->id, 6), "Scale X", "Scale", displayTransform.scale.x, 1},
                {ObjectPropertyId(s, object->id, 7), "Scale Y", "Scale", displayTransform.scale.y, 1},
                {ObjectPropertyId(s, object->id, 8), "Scale Z", "Scale", displayTransform.scale.z, 1}};
            for (auto &row:rows)
                row.flags=static_cast<editor::PropertyFlags>(s.propertyFlags[row.id]|
                    (row.value!=row.defaultValue?2u:0u)|(object->locked?16u:0u));
            for (int component=0;component<9;++component) {
                unsigned flags=static_cast<unsigned>(rows[component].flags);
                for (auto &target:s.objects) if (s.objectSelection.Contains(target.id)) {
                    auto targetPreview=cg::PreviewTransform(target,s.viewport);
                    if (TransformComponent(targetPreview,component)!=rows[component].value) flags|=1u;
                    if (target.locked || (s.propertyFlags[ObjectPropertyId(s,target.id,component)]&16u)) flags|=16u;
                }
                rows[component].flags=static_cast<editor::PropertyFlags>(flags);
            }
            s.objectProperties.icons=s.icons;
            s.objectProperties.time=s.timeline.time.playhead;
            for (auto &row:rows) {
                auto channel=s.propertyKeys.find(row.id);
                if (channel==s.propertyKeys.end() || channel->second.empty()) continue;
                unsigned flags=static_cast<unsigned>(row.flags)|32u;
                if (std::any_of(channel->second.begin(),channel->second.end(),[&](const auto &key){return key.tick==s.timeline.time.playhead;})) flags|=64u;
                row.flags=static_cast<editor::PropertyFlags>(flags);
            }
            PropertyRows visibleProperties{std::span(rows).first(FilterProperties(rows,s.objectProperties.search))};
            editor::PropertyProvider properties{&visibleProperties,s.revision,static_cast<int>(visibleProperties.rows.size()),PropertyRows::Query};
            PropertyLanguage(s.objectProperties,s.japanese);
            editor::PropertyGrid("object", properties, s.objectProperties, s.events);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Components")) {
            s.inspectorComponents.clear();
            for (const auto &component:s.components) if (component.view.owner==object->id) {auto view=component.view;view.icon=component.wireOverride?IconId::Modifier:IconId::Layers;s.inspectorComponents.push_back(view);}
            const cg::ComponentTypeView componentTypes[]={{7801,"Mesh renderer",IconId::LayerAdd},{7802,"Wireframe override",IconId::ModifierAdd}};
            cg::ComponentStackOptions options{object->id,object->geometry ? std::span<const cg::ComponentTypeView>(componentTypes) : std::span<const cg::ComponentTypeView>{},object->locked};
            options.icons=s.icons;
            if (s.japanese) options.labels={"コンポーネント追加","有効","コンポーネント","操作","ロック","ロック解除","上へ","下へ","削除"};
            cg::ComponentStack("Components",s.inspectorComponents,s.revision,s.events,options);
            if(ImGui::CollapsingHeader(s.japanese?"カスタム配列":"Custom property array")) {
                s.arrayProperties.time=s.timeline.time.playhead;
                s.arrayProperties.icons=s.icons;PropertyLanguage(s.arrayProperties,s.japanese);
                editor::PropertyProvider arrayProvider{&s,s.revision,3,[](void *u,int first,int count,std::string_view search) {
                    auto &s=*static_cast<EditorWorkspaces*>(u);std::size_t size=0;ImGuiTextFilter filter(search.data());
                    for(const auto &p:s.customProperties) if(filter.PassFilter(p.label)) {
                        auto value=p;const auto channel=s.propertyKeys.find(p.id);
                        if(channel!=s.propertyKeys.end() && !channel->second.empty()) {
                            value.flags=static_cast<editor::PropertyFlags>(static_cast<unsigned>(value.flags)|32u);
                            if(std::any_of(channel->second.begin(),channel->second.end(),[&](const auto &k){return k.tick==s.timeline.time.playhead;}))
                                value.flags=static_cast<editor::PropertyFlags>(static_cast<unsigned>(value.flags)|64u);
                        }
                        s.filteredCustomProperties[size++]=value;
                    }
                    const auto begin=std::min(size,static_cast<std::size_t>(std::max(0,first)));
                    return std::span<const editor::PropertyView>(s.filteredCustomProperties).subspan(begin,std::min(size-begin,static_cast<std::size_t>(std::max(0,count))));
                },[](void *u,editor::StableId id,int direction)->const editor::PropertyView* {
                    auto &s=*static_cast<EditorWorkspaces*>(u);
                    for(int i=0;i<3;++i) if(s.customProperties[i].id==id && i+direction>=0 && i+direction<3) return &s.customProperties[i+direction];
                    return nullptr;
                }};
                editor::PropertyGrid("custom-array",arrayProvider,s.arrayProperties,s.events);
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();
    }
    Divider("cg-divider",s.cgTopRatio,workspaceHeight);
    if(width>1000*ImGui::GetFontSize()/14) {
    ImGui::BeginChild("Assets", {side, 0}, ImGuiChildFlags_Borders);
    const char *assetPath[]={"All assets","Media"};
    editor::AssetBrowser("assets", Assets(s), s.assetState, s.selection, s.events, std::span(assetPath).first(s.assetPathDepth));
    ImGui::EndChild();
    ImGui::SameLine();
    }
    ImGui::BeginChild("Animation UV", {0, 0}, ImGuiChildFlags_Borders);
    if (s.icons) {
        Icon(*s.icons,IconId::AnimationTimeline,{16*ImGui::GetFontSize()/14});
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",s.japanese ? "アニメーション時間軸" : "Animation timeline");
        ImGui::SameLine();
    }
    TransportLanguage(s.timeline.time,s.japanese);
    editor::Transport(s.timeline.time, std::span(s.bindings).first(s.bindingCount), s.icons);
    if (ImGui::BeginTabBar("Animation editors")) {
        if(ImGui::BeginTabItem("Timeline")) {
            s.curve.time=s.timeline.time.playhead;
            editor::TimeRuler("animation-time",s.timeline.time,s.curve.canvas,std::span(s.markers).first(s.markerCount),s.revision,s.events,theme);
            const auto a=ImGui::GetCursorScreenPos();const auto size=ImGui::GetContentRegionAvail();
            ImGui::InvisibleButton("animation-keys",{std::max(1.f,size.x),std::max(1.f,size.y)});
            auto *draw=ImGui::GetWindowDrawList();
            const auto keys=s.propertyCurveChannel ? std::span<const editor::Keyframe>(s.propertyKeys[s.propertyCurveChannel]) : std::span<const editor::Keyframe>(s.keys);
            for(const auto &key:keys) {
                const float x=a.x+float((editor::Seconds(key.tick)-s.curve.canvas.origin.x)*s.curve.canvas.scale.x);
                if(x<a.x || x>a.x+size.x) continue;
                const float y=a.y+std::min(12.f,size.y*.5f);
                draw->AddQuadFilled({x,y-5},{x+5,y},{x,y+5},{x-5,y},ImGui::GetColorU32(s.keySelection.Contains(key.id)?theme.colors.accent:theme.colors.muted));
                if(ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && std::abs(ImGui::GetIO().MousePos.x-x)<7) {
                    s.keySelection.Set(key.id,ImGui::GetIO().KeyCtrl);s.timeline.time.playhead=key.tick;
                }
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Graph Editor",nullptr,s.animationPage==0?ImGuiTabItemFlags_SetSelected:0)) {
            if (s.icons) {Icon(*s.icons,IconId::CurveEditor,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
            ImGui::TextUnformatted(s.japanese ? "カーブエディター" : "Curve Editor");
            editor::CurveEditor("graph", Curves(s), s.curve, s.keySelection, s.events, theme, {0, 0});
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Dope Sheet",nullptr,s.animationPage==1?ImGuiTabItemFlags_SetSelected:0)) {
            if (s.icons) {Icon(*s.icons,IconId::DopeSheet,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
            ImGui::TextUnformatted(s.japanese ? "ドープシート" : "Dope Sheet");
            cg::DopeSheet("dope", Curves(s), s.curve, s.keySelection, s.events, theme, {0, 0});
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Animation strips",nullptr,s.animationPage==3?ImGuiTabItemFlags_SetSelected:0)) {
            if (s.icons) {Icon(*s.icons,IconId::AnimationStrip,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
            ImGui::TextUnformatted(s.japanese ? "アニメーションストリップ" : "Animation strips");
            cg::StripOptions options{s.icons};
            options.selection=&s.stripSelection;options.time=&s.timeline.time;options.snap=s.timeline.snapping;
            if (s.japanese) {
                auto &labels=options.labels;
                labels.scale="時間倍率";labels.repeat="繰り返し";labels.blend="混合率";labels.mute="ミュート";labels.lock="ロック";
                labels.moveUp="上へ移動";labels.moveDown="下へ移動";labels.repeats="回";
                labels.muted=" [ミュート]";labels.locked=" [ロック]";
            }
            cg::AnimationStrips("strips",s.animationStrips,s.revision,s.stripCanvas,s.stripDrag,s.events,theme,{0,0},options);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("UV / Image",nullptr,s.animationPage==2?ImGuiTabItemFlags_SetSelected:0)) {
            if (s.icons) {Icon(*s.icons,IconId::UVEditor,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
            ImGui::TextUnformatted(s.japanese ? "UVエディター" : "UV Editor");
            for (int i = 0; i < 4; ++i)
                s.edges[i] = {static_cast<editor::StableId>(i + 1), s.uv[i].uv, s.uv[(i + 1) % 4].uv,
                    false,false,s.uv[i].id,s.uv[(i+1)%4].id};
            cg::UVProvider p{&s, s.revision,
                             [](void *u, editor::Rect) {
                                 return std::span<const cg::UVVertex>(static_cast<EditorWorkspaces *>(u)->uv);
                             },
                             [](void *u, editor::Rect) {
                                 return std::span<const cg::UVEdge>(
                                     static_cast<EditorWorkspaces *>(u)->edges);
                             }};
            s.uvFaces[0]={910001,920001,s.uv,false};
            p.faces=[](void *u,editor::Rect) {return std::span<const cg::UVFace>(static_cast<EditorWorkspaces *>(u)->uvFaces);};
            p.all=[](void *,cg::UVSelection mode)->std::span<const editor::StableId> {
                static constexpr std::array<editor::StableId,4> ids{900001,900002,900003,900004};
                static constexpr std::array<editor::StableId,4> edgeIds{1,2,3,4};
                static constexpr std::array<editor::StableId,1> faceIds{910001},islandIds{920001};
                if (mode==cg::UVSelection::Face) return faceIds;
                if (mode==cg::UVSelection::Island) return islandIds;
                if (mode==cg::UVSelection::Edge) return edgeIds;
                return mode==cg::UVSelection::Vertex ? std::span<const editor::StableId>(ids) : std::span<const editor::StableId>{};
            };
            p.selectionQuery=[](void *u,editor::Rect,cg::UVSelection mode)->std::span<const editor::SelectablePoint> {
                auto &host=*static_cast<EditorWorkspaces *>(u);
                if (mode==cg::UVSelection::Face || mode==cg::UVSelection::Island) {
                    editor::Point center{};
                    for (const auto &v:host.uv) {center.x+=v.uv.x/4;center.y+=v.uv.y/4;}
                    host.uvSelectionPoints[0]={mode==cg::UVSelection::Face?910001u:920001u,center,false};
                    return std::span<const editor::SelectablePoint>(host.uvSelectionPoints).first(1);
                }
                for (std::size_t i=0;i<host.uv.size();++i) {
                    const auto &edge=host.edges[i];
                    host.uvSelectionPoints[i]=mode==cg::UVSelection::Vertex ?
                        editor::SelectablePoint{host.uv[i].id,host.uv[i].uv,false} :
                        editor::SelectablePoint{edge.id,{(edge.a.x+edge.b.x)*.5,(edge.a.y+edge.b.y)*.5},false};
                }
                return host.uvSelectionPoints;
            };
            p.selected=[](void *u,std::span<const editor::StableId> ids,cg::UVSelection mode)->std::span<const cg::UVVertex> {
                auto &host=*static_cast<EditorWorkspaces *>(u);std::size_t count=0;
                for (const auto &vertex:host.uv) {
                    bool selected=mode==cg::UVSelection::Vertex && std::find(ids.begin(),ids.end(),vertex.id)!=ids.end();
                    if (mode==cg::UVSelection::Edge) for (const auto &edge:host.edges)
                        if ((edge.aVertex==vertex.id || edge.bVertex==vertex.id) &&
                            std::find(ids.begin(),ids.end(),edge.id)!=ids.end()) selected=true;
                    if (mode==cg::UVSelection::Face || mode==cg::UVSelection::Island) {
                        const editor::StableId id=mode==cg::UVSelection::Face?910001:920001;
                        selected=std::find(ids.begin(),ids.end(),id)!=ids.end();
                    }
                    if (selected) host.selectedUVVertices[count++]=vertex;
                }
                return std::span<const cg::UVVertex>(host.selectedUVVertices).first(count);
            };
            s.uvState.companionDrags=s.uvCompanions;
            s.uvState.imageSize={2,2};
            s.uvState.canvas.selectionPath=s.uvSelectionPath;
            s.uvState.bindings=std::span(s.bindings).first(s.bindingCount);
            s.uvState.labels={};
            if (s.japanese) {
                auto &labels=s.uvState.labels;
                labels.selection="選択単位";labels.modes={"頂点","辺","面","島"};
                labels.checker="市松模様";labels.texture="テクスチャ";labels.grid="グリッド";
                labels.box="矩形選択";labels.lasso="投げ縄選択";labels.coordinates="座標";
                labels.coordinateModes={"正規化","ピクセル","UDIM"};labels.transform="変換";
                labels.tools={"移動","回転","拡大縮小"};labels.pivot="ピボット";labels.snap="スナップ間隔";
                labels.seams="シーム";labels.overlap="重なり";labels.tiles="UDIMタイル";labels.pinned="固定";labels.clearSelection="選択を解除";
            }
            cg::UVEditor("uv", p, texture, s.uvState, s.uvSelection, s.events, theme, {0, 0});
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();
    s.ApplyEvents();
}
void CoreWorkspace(EditorWorkspaces &s, const Theme &theme) {
    s.curve.icons=s.icons;
    s.uvState.icons=s.icons;
    s.Initialize();
    TransportLanguage(s.timeline.time,s.japanese);
    editor::Transport(s.timeline.time, std::span(s.bindings).first(s.bindingCount), s.icons);
    editor::TimeRuler("ruler", s.timeline.time, s.curve.canvas, {}, s.revision, s.events, theme);
    editor::CurveEditor("core curve", Curves(s), s.curve, s.keySelection, s.events, theme, {0, 400});
    editor::StatusBar("Host-owned state / revision checked events", s.keySelection);
    s.ApplyEvents();
}
} // namespace imkit::gallery