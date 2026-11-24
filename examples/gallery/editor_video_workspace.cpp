#include "editor_workspaces_internal.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace imkit::gallery {
using namespace detail;

void VideoWorkspace(EditorWorkspaces &s, const Theme &theme, ImTextureRef texture) {
    CompactEditorScope density;
    s.previewHelpers=false;
    s.curve.icons=s.icons;
    s.uvState.icons=s.icons;
    s.Initialize();
    Options(s);s.SyncClipProperties();
    float available = ImGui::GetContentRegionAvail().x;
    float side = (s.narrow ? 180 : s.videoSide) * ImGui::GetFontSize()/14;
    const bool compact=available<1000*ImGui::GetFontSize()/14;
    if(compact) {imkit::Checkbox(s.japanese?"素材":"Media",&s.showMedia);ImGui::SameLine();imkit::Checkbox("Inspector",&s.showInspector);}
    const bool mediaVisible=!compact || s.showMedia,inspectorVisible=!compact || s.showInspector;
    const float workspaceHeight=ImGui::GetContentRegionAvail().y;
    const float ratioScale=ImGui::GetIO().DisplaySize.y<900 ? .72f : 1.f;
    float displayedRatio=s.videoTopRatio*ratioScale;
    float top = std::max(150.f,workspaceHeight*displayedRatio);
    side=std::min(side,available*.22f);
    if(mediaVisible) {
    ImGui::BeginChild("Media bin", {side, top}, ImGuiChildFlags_Borders);
    ImGui::SeparatorText(s.japanese ? "素材" : "Media Bin");
    const char *assetPath[]={"All assets","Media"};
    const auto previousAsset=s.assetSelection.active;
    editor::AssetBrowser("media", Assets(s), s.assetState, s.assetSelection, s.events, std::span(assetPath).first(s.assetPathDepth));
    if (previousAsset!=s.assetSelection.active) s.monitorMode=1;
    ImGui::EndChild();
    PaneDivider("media-divider",side,top,available*.42f);
    s.videoSide=side/(ImGui::GetFontSize()/14);
    }
    ImGui::BeginChild("Monitors", {(std::max)(100.f, available - (mediaVisible?side:0) - (inspectorVisible?side:0) - 20), top},
                      ImGuiChildFlags_Borders);
    float width = ImGui::GetContentRegionAvail().x;
    const char *modesEN[]={"Program","Source","Compare","Preview contract"};
    const char *modesJP[]={"プログラム","ソース","比較","Preview契約"};
    ImGui::SetNextItemWidth(std::min(width,160.f));
    imkit::Combo("##monitors",&s.monitorMode,s.japanese?modesJP:modesEN,4);
    float monitorHeight = std::max(40.f,top-2*ImGui::GetFrameHeightWithSpacing()-ImGui::GetFontSize()-32-(s.monitorMode==1 ? ImGui::GetFrameHeightWithSpacing():0));
    video::MonitorLabels monitorLabels;
    if (s.japanese) {
        monitorLabels.safeArea="セーフエリア";monitorLabels.guides="ガイド";monitorLabels.timecode="タイムコード";
        monitorLabels.bounds="変形枠";monitorLabels.anchor="アンカーポイント";monitorLabels.metadata="付加情報";
        monitorLabels.presets={"非表示","クリップ","詳細"};
    }
    if(s.monitorMode==3) {
        const char *sources[]={"Synthetic source A","Synthetic source B","Offline input"};
        const editor::StableId sourceIds[]={41001,41003,41009};
        ImGui::SetNextItemWidth(std::min(180.f,width*.3f));
        SearchableCombo("Source",&s.monitorContractSource,sources,s.monitorContractSearch,sizeof(s.monitorContractSearch));
        s.monitorContractSource=std::clamp(s.monitorContractSource,0,2);
        ImGui::SameLine();
        const char *aspects[]={"4:3","16:9","1:1","9:16"};
        ImGui::SetNextItemWidth(90);imkit::Combo("Aspect",&s.monitorContractAspect,aspects,4);
        s.motionAspectMin=ImGui::GetItemRectMin();s.motionAspectMax=ImGui::GetItemRectMax();
        ImGui::SameLine();
        const char *statuses[]={"Ready","Loading","Empty","Offline","Error"};
        ImGui::SetNextItemWidth(110);imkit::Combo("State",&s.monitorContractStatus,statuses,5);
        s.motionStateMin=ImGui::GetItemRectMin();s.motionStateMax=ImGui::GetItemRectMax();
        ImGui::TextDisabled("Stable source ID: %llu",static_cast<unsigned long long>(sourceIds[s.monitorContractSource]));
        const float aspectValues[]={4.f/3,16.f/9,1.f,9.f/16};
        const char *placementNames[]={"Fit","Fill","Stretch"};
        const auto placements=std::array{editor::ImagePlacementMode::Fit,editor::ImagePlacementMode::Fill,editor::ImagePlacementMode::Stretch};
        const float gap=ImGui::GetStyle().ItemSpacing.x,cellWidth=std::max(50.f,(width-gap*2)/3);
        const float previewArea=std::max(50.f,monitorHeight-2*ImGui::GetFrameHeightWithSpacing());
        for(int i=0;i<3;++i) {
            ImGui::PushID(i);ImGui::BeginGroup();ImGui::TextUnformatted(placementNames[i]);
            const float previewWidth=std::min(cellWidth,previewArea*aspectValues[s.monitorContractAspect]);
            const float previewHeight=previewWidth/aspectValues[s.monitorContractAspect];
            const float offset=std::max(0.f,(cellWidth-previewWidth)*.5f);ImGui::SetCursorPosX(ImGui::GetCursorPosX()+offset);
            video::MonitorView contract{{texture,{1920,1080}}};
            contract.placement=placements[i];contract.status=static_cast<editor::PreviewState>(s.monitorContractStatus);
            contract.stateView={statuses[s.monitorContractStatus],"Host-owned preview state",
                                s.monitorContractStatus==4?"Retry":"",s.monitorContractStatus==4?FeedbackKind::Error:FeedbackKind::Info,
                                IconId::Monitor,s.icons};
            auto display=s.sourceMonitorOptions;display.label=placementNames[i];
            if(video::Monitor("contract",contract,{previewWidth,previewHeight},s.timeline.time,display,theme))
                s.editMessage="Preview action request";
            ImGui::EndGroup();ImGui::PopID();if(i<2) ImGui::SameLine(0,gap);
        }
    } else {
    if (s.monitorMode!=0) {
    ImGui::BeginGroup();
    if (s.icons) {Icon(*s.icons,IconId::SourceMonitor,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
    ImGui::TextUnformatted(s.japanese ? "ソース" : "Source");
    s.sourceMonitorOptions.label=s.japanese ? "ソース / Studio" : "Source / Studio";
    video::Monitor("Source",texture,{s.monitorMode==2?width*.48f:width,monitorHeight},s.timeline.time,s.sourceMonitorOptions,theme);
    double sourceIn=editor::Seconds(s.sourceTime.inOut.first),sourceOut=editor::Seconds(s.sourceTime.inOut.last);
    ImGui::SetNextItemWidth(70);
    if(ImGui::DragScalar("In",ImGuiDataType_Double,&sourceIn,.05f)) s.sourceTime.inOut.first=editor::FromSeconds(std::clamp(sourceIn,0.,60.));
    ImGui::SameLine();ImGui::SetNextItemWidth(70);
    if(ImGui::DragScalar("Out",ImGuiDataType_Double,&sourceOut,.05f)) s.sourceTime.inOut.last=editor::FromSeconds(std::clamp(sourceOut,0.,60.));
    if (ImGui::BeginPopupContextItem("source display")) {
        video::MonitorControls("source",s.sourceMonitorOptions,s.icons,monitorLabels);ImGui::EndPopup();
    }
    ImGui::EndGroup();
    if(s.monitorMode==2) ImGui::SameLine();
    }
    if (s.monitorMode!=1) {
    ImGui::BeginGroup();
    if (s.icons) {Icon(*s.icons,IconId::ProgramMonitor,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
    ImGui::TextUnformatted(s.japanese ? "プログラム" : "Program");
    if (s.monitorRevision!=s.revision || s.monitorClipId!=s.selection.active) {
        s.monitorRevision=s.revision;s.monitorClipId=s.selection.active;
        auto clip=std::find_if(s.clips.begin(),s.clips.end(),[&](const auto &c){return c.id==s.monitorClipId;});
        s.monitorClipIndex=static_cast<std::size_t>(clip-s.clips.begin());
    }
    s.programMonitorOptions.metadataPreset=s.monitorMetadata;
    auto program=s.programMonitorOptions;
    program.label=s.japanese ? "プログラム" : "Program";program.flipY=true;
    program.metadataPreset=s.monitorMetadata;
    const double extent=.3*std::max(0.,s.clipPropertyValues[1]),center=.5+s.clipPropertyValues[2];
    program.transformBounds={{center-extent,.5-extent},{center+extent,.5+extent}};
    program.anchor={static_cast<float>(center),.5f};
    char timing[96]{},source[96]{};
    const char *metadata[]={timing,source};
    if (s.monitorClipIndex<s.clips.size()) {
        const auto &clip=s.clips[s.monitorClipIndex];program.clipName=clip.label;
        char duration[40]{};editor::FormatTimecode(clip.duration,s.timeline.time.rate,false,duration);
        std::snprintf(timing,sizeof(timing),"%s  %.3gx",duration,clip.speed);
        std::snprintf(source,sizeof(source),"%s / track %llu",clip.offline ? "Offline" : clip.missing ? "Missing" : clip.proxy ? "Proxy" : "Original",
            static_cast<unsigned long long>(clip.track));
        program.metadata=metadata;
    }
    for (const auto &marker:std::span(s.markers).first(s.markerCount))
        if (editor::TickToFrame(marker.tick,s.timeline.time.rate)==editor::TickToFrame(s.timeline.time.playhead,s.timeline.time.rate)) {
            program.markerComment=marker.label;break;
        }
    video::Monitor("Program",ImTextureRef(static_cast<ImTextureID>(s.previewRenderer.Texture())),
                   {s.monitorMode==2?width*.48f:width,monitorHeight},s.timeline.time,program,theme);
    s.programMonitorMin=ImGui::GetItemRectMin();s.programMonitorMax=ImGui::GetItemRectMax();
    s.viewportSize={s.programMonitorMax.x-s.programMonitorMin.x,s.programMonitorMax.y-s.programMonitorMin.y};
    if (ImGui::BeginPopupContextItem("monitor metadata")) {
        video::MonitorControls("program",s.programMonitorOptions,s.icons,monitorLabels);
        s.programMonitorMetadataMin=ImGui::GetItemRectMin();s.programMonitorMetadataMax=ImGui::GetItemRectMax();
        s.monitorMetadata=s.programMonitorOptions.metadataPreset;
        ImGui::EndPopup();
    }
    ImGui::EndGroup();
    }
    }
    TransportLanguage(s.timeline.time,s.japanese);
    s.motionPlay=ImGui::GetCursorScreenPos();
    editor::Transport(s.timeline.time, std::span(s.bindings).first(s.bindingCount), s.icons);
    ImGui::EndChild();
    if(inspectorVisible) {
    ImGui::SameLine();
    ImGui::BeginChild("Clip Inspector", {0, top}, ImGuiChildFlags_Borders);
    ImGui::SeparatorText("Clip Inspector");
    if(s.clipPropertyIds[0] && ImGui::Button(s.japanese ? "キーフレームを編集" : "Edit keyframes")) {
        s.propertyCurveChannel=s.clipPropertyIds[0];s.curve.activeChannel=s.propertyCurveChannel;
        s.showDetails=true;s.videoPanel=2;s.curve.fitRequested=true;
    }
    if(s.monitorClipIndex<s.clips.size()) {
        const auto &clip=s.clips[s.monitorClipIndex];
        if(ImGui::CollapsingHeader(s.japanese ? "フェード" : "Fades")) video::FadePicker("inspector-fades",clip,clip.fades,s.revision,s.events,s.clipInspectorLocked);
    }
    editor::PropertyView props[] = {{s.clipPropertyIds[0], "Opacity", "Video", 1, 1, editor::PropertyFlags::Animated},
                                    {s.clipPropertyIds[1], "Scale", "Transform", 1, 1},
                                    {s.clipPropertyIds[2], "Position X", "Transform", 0, 0},
                                    {s.clipPropertyIds[3], "Speed", "Retiming", 1, 1,{},false,IconId::SpeedCurve}};
    for (int i = 0; i < 4; ++i) {
        props[i].value = s.clipPropertyValues[i];
        props[i].flags=static_cast<editor::PropertyFlags>(static_cast<unsigned>(props[i].flags)|
            s.propertyFlags[props[i].id]|(props[i].value!=props[i].defaultValue?2u:0u));
    }
    s.videoProperties.icons=s.icons;
    s.videoProperties.time=s.timeline.time.playhead;
    for (auto &row:props) {
        auto channel=s.propertyKeys.find(row.id);
        if (channel==s.propertyKeys.end() || channel->second.empty()) continue;
        unsigned flags=static_cast<unsigned>(row.flags)|32u;
        if (std::any_of(channel->second.begin(),channel->second.end(),[&](const auto &key){return key.tick==s.timeline.time.playhead;})) flags|=64u;
        row.flags=static_cast<editor::PropertyFlags>(flags);
    }
    PropertyRows visibleProperties{std::span(props).first(FilterProperties(props,s.videoProperties.search))};
    editor::PropertyProvider properties{&visibleProperties,s.revision,static_cast<int>(visibleProperties.rows.size()),PropertyRows::Query};
    if (s.clipPropertyIds[0]) {
        ImGui::BeginDisabled(s.clipInspectorLocked);
        PropertyLanguage(s.videoProperties,s.japanese);
        editor::PropertyGrid("clip",properties,s.videoProperties,s.events);
        ImGui::EndDisabled();
    } else ImGui::TextUnformatted(s.japanese ? "クリップを選択" : "Select a clip");
    ImGui::EndChild();
    }
    Divider("video-divider",displayedRatio,workspaceHeight);
    s.videoTopRatio=displayedRatio/ratioScale;
    imkit::Checkbox(s.japanese ? "詳細パネル" : "Details",&s.showDetails);
    ImGui::SameLine();ImGui::SetNextItemWidth(120);
    const auto selectedTrack=std::find_if(s.tracks.begin(),s.tracks.end(),[&](const auto &t){return t.id==s.placementTrack;});
    if(imkit::BeginCombo("##destination",selectedTrack==s.tracks.end()?"Track":selectedTrack->label)) {
        for(const auto &t:s.tracks) if(t.kind==video::TrackKind::Audio || t.kind==video::TrackKind::Video) {
            ImGui::BeginDisabled(t.locked);if(ImGui::Selectable(t.label,t.id==s.placementTrack)) s.placementTrack=t.id;ImGui::EndDisabled();
        }
        imkit::EndCombo();
    }
    constexpr editor::Command placementCommands[]={editor::Command::InsertSource,editor::Command::OverwriteSource,editor::Command::AppendSource};
    const char *placementNames[]= {s.japanese?"挿入":"Insert",s.japanese?"上書き":"Overwrite",s.japanese?"末尾追加":"Append"};
    for(int i=0;i<3;++i) {
        ImGui::SameLine();const bool pressed=ImGui::Button(placementNames[i]);
        if(pressed || editor::CommandPressed(placementCommands[i],std::span(s.bindings).first(s.bindingCount),ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))) s.PlaceSource(static_cast<video::PlacementMode>(i));
    }
    if(!s.editMessage.empty()) ImGui::TextWrapped("%s",s.editMessage.c_str());
    video::TimelineProvider p{&s, s.revision, static_cast<int>(s.tracks.size())};
    p.waveform={&s,[](void *user,const video::WaveformQuery &query) {return static_cast<EditorWorkspaces*>(user)->QueryWaveform(query);}};
    p.transitionLimit=[](void *u,editor::StableId id,bool outgoing) {
        return static_cast<EditorWorkspaces *>(u)->TransitionLimit(id,outgoing);
    };
    p.tracks = [](void *u, int a, int n) {
        auto &s = *static_cast<EditorWorkspaces *>(u);
        ++s.queryCount;
        s.queriedTracks+=n;
        return std::span<const video::TrackView>(s.tracks).subspan(a, n);
    };
    p.clips = [](void *u, editor::StableId track, editor::Range range) {
        return static_cast<EditorWorkspaces *>(u)->QueryClips(track,range);
    };
    p.layout=[](void *u,double firstPixel,double lastPixel) {
        auto &s=*static_cast<EditorWorkspaces *>(u);
        ++s.queryCount;
        auto first=std::upper_bound(s.trackOffsets.begin(),s.trackOffsets.end(),firstPixel);
        std::size_t index=first==s.trackOffsets.begin() ? 0 : static_cast<std::size_t>(first-s.trackOffsets.begin()-1);
        index=(std::min)(index,s.tracks.size());
        auto last=std::lower_bound(s.trackOffsets.begin()+index,s.trackOffsets.end(),lastPixel);
        auto end=(std::min)(static_cast<std::size_t>(last-s.trackOffsets.begin()),s.tracks.size());
        s.queriedTracks+=end-index;
        return video::TrackLayout{std::span<const video::TrackView>(s.tracks).subspan(index,end-index),s.trackOffsets[index]};
    };
    p.totalHeight=s.trackOffsets.back();
    p.markers = std::span<const editor::Marker>(s.markers).first(s.markerCount);
    p.neighbors = [](void *u, editor::StableId id) {
        auto &s = *static_cast<EditorWorkspaces *>(u);
        video::TimelineProvider::Neighbors result;
        auto it = std::find_if(s.clips.begin(), s.clips.end(), [&](const auto &c) { return c.id == id; });
        if (it != s.clips.end()) {
            if (it != s.clips.begin() && (it - 1)->track == it->track)
                result.previous = &*(it - 1);
            if (it + 1 != s.clips.end() && (it + 1)->track == it->track)
                result.next = &*(it + 1);
        }
        return result;
    };
    p.isEditable=[](void *u,editor::StableId id) {
        const auto &s=*static_cast<EditorWorkspaces *>(u);
        const auto found=std::lower_bound(s.clipById.begin(),s.clipById.end(),id,
            [](const auto &entry,auto id){return entry.first<id;});
        if (found==s.clipById.end() || found->first!=id) return false;
        const auto &clip=s.clips[found->second];
        const auto track=std::find_if(s.tracks.begin(),s.tracks.end(),[&](const auto &t){return t.id==clip.track;});
        return !clip.locked && track!=s.tracks.end() && !track->locked;
    };
    p.canBeginEdit=[](void *u,editor::StableId id,editor::EditKind kind) {
        if (kind!=editor::EditKind::Ripple) return true;
        const auto &s=*static_cast<EditorWorkspaces *>(u);
        const auto clip=std::find_if(s.clips.begin(),s.clips.end(),[&](const auto &c){return c.id==id;});
        if (clip==s.clips.end()) return false;
        return std::none_of(clip+1,s.clips.end(),[&](const auto &c) {
            return c.track==clip->track && c.start>=clip->start+clip->duration && c.locked;
        });
    };
    p.selected = [](void *u, std::span<const editor::StableId> ids) {
        return static_cast<EditorWorkspaces *>(u)->QuerySelectedClips(ids);
    };
    s.timeline.labels={};
    s.timeline.trackLabels={};
    if (s.japanese) {
        s.timeline.labels.unlink="clipのリンクを解除";s.timeline.labels.ungroup="groupから外す";
        s.timeline.labels.transitions.title="トランジション";
        s.timeline.labels.transitions.in="開始側";s.timeline.labels.transitions.out="終了側";
        s.timeline.labels.transitions.inDuration="開始側の長さ";s.timeline.labels.transitions.outDuration="終了側の長さ";
        s.timeline.labels.transitions.kinds={"なし","ディゾルブ","フェード","クロスフェード"};
        s.timeline.labels.duration="長さ";s.timeline.labels.speed="速度";s.timeline.labels.linked="リンク";s.timeline.labels.group="グループ";
        s.timeline.labels.proxy="プロキシ";s.timeline.labels.missing="素材が見つかりません";
        s.timeline.labels.offline="素材がオフライン";s.timeline.labels.locked="ロック中";
        s.timeline.labels.addEnvelope="音量ポイントを追加";s.timeline.labels.removeEnvelope="音量ポイントを削除";
        s.timeline.labels.addCaption="再生位置以降の空きに字幕を追加";s.timeline.labels.rippleDelete="選択クリップをリップル削除";s.timeline.labels.trimStart="開始端をトリム";s.timeline.labels.trimEnd="終了端をトリム";
        s.timeline.labels.envelope="音量エンベロープ";s.timeline.labels.dragKey="キーの時刻をドラッグ";
        s.timeline.labels.linkSelection="選択clipをリンク";s.timeline.labels.groupSelection="選択clipをgroup化";
        s.timeline.labels.tools={"選択","分割","リップル","ロール","スリップ","スライド","手のひら"};
        s.timeline.labels.tooltips={"clipを選択","カーソル位置で分割","trimして後続clipを移動","隣接clipの境界を移動",
            "clip位置を保ち素材範囲を変更","clipを移動して隣接clipをtrim","Timelineをpan"};
        s.timeline.labels.snap="スナップ";s.timeline.labels.magnet="マグネット";s.timeline.labels.magnetTooltip="Timelineの対象へ吸着";
        s.timeline.labels.options="Timeline設定";s.timeline.labels.follow="再生位置に追従";s.timeline.labels.frameGrid="フレーム単位";
        s.timeline.labels.followModes={"なし","滑らか","ページ"};
        s.timeline.labels.snapKinds={"フレーム","再生位置","marker","clip端","keyframe","in／out","選択範囲端"};
        s.timeline.labels.fit="全体表示";s.timeline.labels.fitTooltip="Timeline全体を表示";
        s.timeline.labels.zoom="Timelineの拡大率";s.timeline.labels.zoomOut="縮小";s.timeline.labels.zoomIn="拡大";
        s.timeline.trackLabels.buttons={"表示","消音","ソロ","固定","録音","対象","素材"};
        s.timeline.trackLabels.names={"表示","ミュート","ソロ","ロック","録音待機","編集対象track","素材patch"};
        s.timeline.trackLabels.controls="track操作";s.timeline.trackLabels.height="track高さ";
        s.timeline.trackLabels.on="有効";s.timeline.trackLabels.off="無効";
        s.timeline.trackLabels.kinds={"映像track","音声track","字幕track","効果track","調整track","グループtrack"};
        s.timeline.trackLabels.expand="trackを展開";s.timeline.trackLabels.collapse="trackを折り畳む";
    }
    s.timeline.icons=s.icons;
    s.timeline.bindings=std::span(s.bindings).first(s.bindingCount);
    p.contentRange=s.timelineBounds;
    p.snap = [](void *u, editor::Range range) {
        auto &s = *static_cast<EditorWorkspaces *>(u);
        std::size_t n = 0;
        s.snapCandidates[n++] = {s.timeline.time.playhead, editor::SnapKind::Playhead, 3, 0};
        s.snapCandidates[n++] = {s.timeline.time.inOut.first, editor::SnapKind::InOut, 2, 0};
        s.snapCandidates[n++] = {s.timeline.time.inOut.last, editor::SnapKind::InOut, 2, 0};
        for (const auto &key:s.keys)
            if (key.tick>=range.first && key.tick<=range.last && n<s.snapCandidates.size())
                s.snapCandidates[n++] = {key.tick,editor::SnapKind::Keyframe,2,key.id};
        for (auto marker : std::span(s.markers).first(s.markerCount))
            if (n < s.snapCandidates.size())
                s.snapCandidates[n++] = {marker.tick, editor::SnapKind::Marker, 2, marker.id};
        auto first = std::lower_bound(s.clips.begin(), s.clips.end(), s.timeline.original.track,
                                      [](const auto &c, auto id) { return c.track < id; });
        for (auto it = first;
             it != s.clips.end() && it->track == s.timeline.original.track && n + 2 < s.snapCandidates.size();
             ++it)
            if (it->start >= range.first && it->start <= range.last) {
                s.snapCandidates[n++] = {it->start, editor::SnapKind::ClipEdge, 1, it->id};
                s.snapCandidates[n++] = {it->start + it->duration, editor::SnapKind::ClipEdge, 1, it->id};
            }
        return std::span<const editor::SnapCandidate>(s.snapCandidates).first(n);
    };
    float remaining = ImGui::GetContentRegionAvail().y;
    float timelineHeight=std::max(60.f,remaining-(s.showDetails ? remaining*.42f : 0.f)-3*ImGui::GetFrameHeightWithSpacing()-28);
    s.timelineOrigin = ImGui::GetCursorScreenPos();
    s.timeline.keySelection=&s.keySelection;
    s.timeline.keyCompanions=s.clipKeyCompanions;
    s.ConfigureTimelineEditing(p);
    video::Timeline("Timeline", p, s.timeline, s.selection, s.events, theme, {0, timelineHeight});
    if (!s.showDetails && s.videoPanel<0) {s.ApplyEvents();return;}
    ImGui::BeginChild("Audio color", {0, 0}, ImGuiChildFlags_Borders);
    if (ImGui::BeginTabBar("audio color")) {
        if (ImGui::BeginTabItem("Audio")) {
            s.activeVideoPanel = 0;
            auto mixer=std::find_if(s.audioStrips.begin(),s.audioStrips.end(),
                                   [&](const auto &v){return v.id==s.mixerTrack;});
            ImGui::SetNextItemWidth(220);
            if (imkit::BeginCombo("Channel",mixer==s.audioStrips.end() ? "" : mixer->label)) {
                for (const auto &channel:s.audioStrips) {
                    ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(channel.id)));
                    if (ImGui::Selectable(channel.label,channel.id==s.mixerTrack)) s.mixerTrack=channel.id;
                    ImGui::PopID();
                }
                imkit::EndCombo();
            }
            if (mixer!=s.audioStrips.end()) {
                auto view=*mixer;
                auto track=std::find_if(s.tracks.begin(),s.tracks.end(),[&](const auto &v){return v.id==view.id;});
                if (track!=s.tracks.end()) {
                    view.mute=track->mute; view.solo=track->solo; view.record=track->record; view.locked=track->locked;
                }
                s.mixerState.icons=s.icons;
                video::AudioStrip(view,s.revision,s.mixerState,s.events);
                bool anotherSolo=std::any_of(s.tracks.begin(),s.tracks.end(),
                                             [&](const auto &v){return v.solo && v.id!=view.id;});
                double gain=s.mixerState.drag.active && s.mixerState.drag.draft.target==view.gainId ?
                            s.mixerState.drag.draft.proposed.x : view.gain;
                double pan=s.mixerState.drag.active && s.mixerState.drag.draft.target==view.panId ?
                           s.mixerState.drag.draft.proposed.x : view.pan;
                double angle=(std::clamp(pan,-1.,1.)+1)*3.141592653589793/4;
                if (view.mute || (anotherSolo && !view.solo)) gain=0;
                std::array<float,512> left{},right{};
                for (std::size_t i=0;i<s.pcm.size();++i) {
                    left[i]=static_cast<float>(s.pcm[i]*gain*std::cos(angle));
                    right[i]=static_cast<float>(s.pcm[i]*gain*std::sin(angle));
                }
                video::UpdateMeter(s.meter,left,ImGui::GetIO().DeltaTime);
                video::UpdateMeter(s.rightMeter,right,ImGui::GetIO().DeltaTime);
                ImGui::SameLine();
            }
            if(s.icons) {Icon(*s.icons,IconId::Waveform,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
            video::Waveform("wave", s.audio, {(std::max)(60.f,ImGui::GetContentRegionAvail().x - 80), 100}, theme);
            ImGui::SameLine();
            video::LevelMeter("meter", s.meter, s.rightMeter, {50, 100}, theme);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Color", nullptr, s.videoPanel == 1 ? ImGuiTabItemFlags_SetSelected : 0)) {
            s.activeVideoPanel = 1;
            s.videoPanel = -1;
            float scopeWidth = (std::max)(40.f, (ImGui::GetContentRegionAvail().x-32)/5);
            video::Histogram("hist", s.luma, {scopeWidth, 80}, theme);
            ImGui::SameLine();
            video::ScopeImage("vectorscope", s.scopeVector, 256, 256, {scopeWidth, 80}, theme);
            const ImVec4 tints[] = {theme.editor.axisX,theme.editor.axisY,theme.editor.axisZ};
            for (int channel=0; channel<3; ++channel) {
                ImGui::SameLine();
                ImGui::PushID(channel);
                video::ScopeImage("RGB waveform", s.scopeRGB[channel],64,256,{scopeWidth,80},theme,tints[channel]);
                ImGui::PopID();
            }
            if(s.icons) {Icon(*s.icons,IconId::Palette,{ImGui::GetFontSize()});ImGui::SameLine();}
            ImGui::TextUnformatted(s.japanese?"色補正":"Color grade");
            video::ColorLabels colorLabels;
            if(s.japanese) colorLabels={"リフト","ガンマ","ゲイン","色温度","色かぶり","露出","明るさ"};
            video::ColorControls("Grade",s.colors,s.colorIds,s.revision,s.colorState,s.events,colorLabels);
            if(s.icons) {Icon(*s.icons,IconId::CurveEditor,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
            ImGui::BeginDisabled(s.colorCurve.drag.active);
            const char *channelsEn[]={"Red","Green","Blue"},*channelsJa[]={"赤","緑","青"};
            if(imkit::Combo(s.japanese?"色補正カーブ":"Color curves",&s.colorCurveChannel,s.japanese?channelsJa:channelsEn,3)) {
                s.colorCurveSelection.Clear();s.colorCurve.activeChannel=s.colorCurveChannels[s.colorCurveChannel];s.colorCurve.fitRequested=true;
            }
            float input=static_cast<float>(editor::Seconds(s.colorCurve.time));
            if(imkit::SliderFloat(s.japanese?"入力値":"Input",&input,0,1)) s.colorCurve.time=editor::FromSeconds(input);
            ImGui::EndDisabled();
            editor::CurveEditor("color curve",ColorCurves(s),s.colorCurve,s.colorCurveSelection,s.events,theme,{0,180});
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Keyframes / Curves",nullptr,s.videoPanel==2?ImGuiTabItemFlags_SetSelected:0)) {
            s.videoPanel=-1;
            editor::CurveEditor("clip curve", Curves(s), s.curve, s.keySelection, s.events, theme, {0, 160});
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();
    s.ApplyEvents();
}
} // namespace imkit::gallery
