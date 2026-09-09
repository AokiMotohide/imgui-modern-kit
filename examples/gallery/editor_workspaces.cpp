#include "editor_workspaces.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace imkit::gallery {
namespace {
void ApplyGizmoPreview(preview::Mesh &mesh,const cg::ViewportState &viewport) {
    auto apply=[&](const editor::Transaction *transaction) {
        if (!transaction->active || transaction->draft.target!=mesh.id || transaction->draft.phase==editor::Phase::Cancel) return;
        const auto &v=transaction->draft.proposed;
        if (transaction->draft.kind==editor::EditKind::Translate) mesh.transform.translation={v.x,v.y,v.z};
        if (transaction->draft.kind==editor::EditKind::Rotate) mesh.transform.rotation={v.x,v.y,v.z};
        if (transaction->draft.kind==editor::EditKind::Scale) mesh.transform.scale={v.x,v.y,v.z};
    };
    apply(&viewport.drag);apply(&viewport.pivotDrag);
    for (std::size_t i=0;i<viewport.companionCount;++i) {
        apply(&viewport.companions[i].transform);apply(&viewport.companions[i].position);
    }
}
template<std::size_t N>
int FilterProperties(editor::PropertyView (&rows)[N],const char *search) {
    ImGuiTextFilter filter(search);
    int count=0;
    for (auto row:rows) {
        char text[512];
        std::snprintf(text,sizeof(text),"%s %s",row.category,row.label);
        if (filter.PassFilter(text)) rows[count++]=row;
    }
    return count;
}
struct PropertyRows {
    std::span<const editor::PropertyView> rows;
    static std::span<const editor::PropertyView> Query(void *user,int first,int count,std::string_view) {
        auto rows=static_cast<PropertyRows*>(user)->rows;
        auto begin=(std::min)(rows.size(),static_cast<std::size_t>((std::max)(0,first)));
        return rows.subspan(begin,(std::min)(rows.size()-begin,static_cast<std::size_t>((std::max)(0,count))));
    }
};
double &TransformComponent(cg::Transform &transform,int component) {
    double *fields[]={&transform.translation.x,&transform.translation.y,&transform.translation.z,
        &transform.rotation.x,&transform.rotation.y,&transform.rotation.z,
        &transform.scale.x,&transform.scale.y,&transform.scale.z};
    return *fields[component];
}
editor::StableId ObjectPropertyId(const EditorWorkspaces &state, editor::StableId object, int component) {
    if (component<0 || component>=9) return 0;
    for (std::size_t i=0;i<state.objects.size();++i)
        if (state.objects[i].id==object) return state.objectPropertyIds[i][component];
    return 0;
}
editor::AssetProvider Assets(EditorWorkspaces &s) {
    s.assetState.breadcrumbIds=std::span(s.assetPathIds).first(s.assetPathDepth);
    return {&s,s.revision,static_cast<int>(s.assets.size()),
        [](void *u,int first,int count,std::string_view) {
            auto &s=*static_cast<EditorWorkspaces*>(u);
            auto begin=(std::min)(s.filteredAssetCount,static_cast<std::size_t>((std::max)(0,first)));
            return std::span<const editor::AssetView>(s.filteredAssets).subspan(begin,
                (std::min)(s.filteredAssetCount-begin,static_cast<std::size_t>((std::max)(0,count))));
        },
        [](void *u,std::string_view search) {
            auto &s=*static_cast<EditorWorkspaces*>(u);
            ImGuiTextFilter names(search.data()),tags(s.assetState.tag);
            s.filteredAssetCount=0;
            for (const auto &asset:s.assets)
                if (names.PassFilter(asset.label) && tags.PassFilter(asset.tag) &&
                    (s.assetState.status<0 || s.assetState.status==static_cast<int>(asset.status)) &&
                    (s.assetPathDepth==1 || std::string_view(asset.tag)=="Media"))
                    s.filteredAssets[s.filteredAssetCount++]=asset;
            return static_cast<int>(s.filteredAssetCount);
        }};
}
editor::CurveProvider Curves(EditorWorkspaces &s) {
    s.curve.bindings=std::span(s.bindings).first(s.bindingCount);
    s.curve.rate=s.timeline.time.rate;
    s.curve.time=s.timeline.time.playhead;
    return {&s, s.revision, [](void *u, editor::CurveQuery q) {
                auto &s = *static_cast<EditorWorkspaces *>(u);
                return s.QueryKeys(q);
            }, [](void *u,editor::StableId id,editor::Tick tick,editor::Extrapolation mode) {
                const auto &s=*static_cast<EditorWorkspaces*>(u);
                for (auto [first,last]:s.keyChannels)
                    if (s.keys[first].channel==id)
                        return editor::Evaluate(std::span<const editor::Keyframe>(s.keys).subspan(first,last-first),tick,mode);
                return 0.;
            },s.curveBounds,[](void *u,std::span<const editor::StableId> ids) {
                auto &s=*static_cast<EditorWorkspaces*>(u);s.selectedCurveKeys.clear();
                for (const auto &key:s.keys) if (std::find(ids.begin(),ids.end(),key.id)!=ids.end()) s.selectedCurveKeys.push_back(key);
                return std::span<const editor::Keyframe>(s.selectedCurveKeys);
            },[](void *u,editor::StableId id,editor::Tick tick,bool next)->const editor::Keyframe* {
                const auto &s=*static_cast<EditorWorkspaces*>(u);
                for (auto [first,last]:s.keyChannels) if (s.keys[first].channel==id) {
                    auto begin=s.keys.begin()+first,end=s.keys.begin()+last;
                    auto key=std::lower_bound(begin,end,tick,[](const auto &key,auto tick){return key.tick<tick;});
                    if (next) {if (key!=end && key->tick==tick) ++key;return key==end?nullptr:&*key;}
                    return key==begin?nullptr:&*--key;
                }
                return nullptr;
            },[](void *u,editor::Rect bounds) {
                auto &s=*static_cast<EditorWorkspaces*>(u);
                auto keys=s.QueryKeys({{editor::FromSeconds(bounds.min.x),editor::FromSeconds(bounds.max.x)},-bounds.max.y,-bounds.min.y});
                s.curveSelectionPoints.clear();
                for (const auto &key:keys) s.curveSelectionPoints.push_back({key.id,{editor::Seconds(key.tick),-key.value},key.locked});
                return std::span<const editor::SelectablePoint>(s.curveSelectionPoints);
            }};
}
void Options(EditorWorkspaces &s) {
    bool large = s.large;
    if (ImGui::Checkbox("100k dataset", &large))
        s.Dataset(large);
    ImGui::SameLine();
    ImGui::Checkbox("日本語", &s.japanese);
    ImGui::SameLine();
    ImGui::Checkbox("Narrow panes", &s.narrow);
    ImGui::SameLine();
    ImGui::Text("%zu visible queries / %zu clips / %zu commits", s.queryCount, s.queriedClips, s.commits);
    if (ImGui::BeginPopupContextItem("transition history")) {
        if (ImGui::MenuItem("Undo transition",nullptr,false,s.transitionHistoryCursor>0)) s.UndoTransition();
        if (ImGui::MenuItem("Redo transition",nullptr,false,s.transitionHistoryCursor<s.transitionHistory.size())) s.UndoTransition(true);
        ImGui::EndPopup();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Right-click for transition Undo / Redo");
    s.queryCount = s.queriedClips = 0;
}
} // namespace
void EditorWorkspaces::Dataset(bool big) {
    transitionHistory.clear();transitionHistoryCursor=0;
    clipEnvelopes.clear();clipProperties.clear();clipPropertyOwners.clear();clipPropertyRevision=0;
    large = big;
    tracks.clear();
    clips.clear();
    keys.clear();
    audioStrips.clear();
    int trackCount = large ? 256 : 6;
    clipsPerTrack = large ? 391 : 12;
    tracks.reserve(trackCount);
    clips.reserve(trackCount * clipsPerTrack + 64);
    keys.reserve(large ? 100000 : 12);
    for (int t = 0; t < trackCount; ++t) {
        video::TrackView track;
        track.id = t + 1;
        constexpr const char *trackLabels[]={"V  Picture","A  Sound","T  Caption","FX  Effect","ADJ  Adjustment","GRP  Group"};
        track.label=trackLabels[t%6];
        track.kind=static_cast<video::TrackKind>(t%6);
        tracks.push_back(track);
        if (track.kind == video::TrackKind::Audio) {
            video::AudioStripView strip;
            strip.id=track.id; strip.label=track.label;
            strip.gainId=nextId++; strip.panId=nextId++;
            audioStrips.push_back(strip);
        }
        for (int i = 0; i < clipsPerTrack; ++i) {
            video::ClipView clip;
            clip.id = 1000 + t * clipsPerTrack + i;
            clip.track = track.id;
            constexpr const char *clipLabels[]={"Studio / Main take","Ambient / stereo","A quiet afternoon",
                "Blur / effect range","Exposure / adjustment range","Sequence / group range"};
            clip.label=clipLabels[t%6];
            clip.start = editor::FromSeconds(i * 4. + (t % 2) * .5);
            clip.duration = editor::FromSeconds(3.5);
            clip.sourceIn = editor::TicksPerSecond * 5;
            if (t==0 && i==1) {
                clip.transitionIn=editor::FromSeconds(.4);clip.transitionOut=editor::FromSeconds(.6);
                clip.transitionOutKind=video::TransitionKind::Fade;
            }
            clip.proxy = i % 7 == 0;
            if (track.kind == video::TrackKind::Audio) clip.audioBuckets=audio;
            if (t==1 && i==0) {
                auto &points=clipEnvelopes[clip.id];
                points={{nextId++,editor::FromSeconds(.5),.5},{nextId++,editor::FromSeconds(1.5),1},{nextId++,editor::FromSeconds(3),.7}};
                clip.envelope=points;
            }
            clips.push_back(clip);
        }
    }
    for (int i = 0; i < (large ? 100000 : 12); ++i) {
        editor::Keyframe key;
        key.id = 500000 + i;
        key.channel = large ? 1+i/33334 : 1;
        key.tick = editor::FromSeconds((large ? i%33334 : i) * .5);
        key.value = .5 + .4 * std::sin(i * .8);
        key.left = {-.15, 0};
        key.right = {.15, 0};
        keys.push_back(key);
    }
    if (!large) for (int channel=2;channel<=3;++channel) for (int i=0;i<4;++i) {
        editor::Keyframe key;
        key.id=510000+channel*100+i;key.channel=channel;
        key.tick=editor::FromSeconds(i*1.5);key.value=.5+.3*std::sin(i+channel);
        key.handles=editor::HandleMode::AutoClamped;
        keys.push_back(key);
    }
    if (!clips.empty()) clips.front().keyChannel=1;
    RebuildKeyIndex();
    selection.Clear();
    selection.Set(1000);
    timeline.drag.active = false;
    mixerTrack = audioStrips.empty() ? 0 : audioStrips.front().id;
    mixerState.drag.active = false;
    curve.drag.active = false;
    curve.companionCount=0;
    for (auto &drag:curveCompanions) drag.active=false;
    RebuildTrackLayout();
    ++revision;SyncClipProperties();
}
void EditorWorkspaces::RebuildKeyIndex() {
    std::sort(keys.begin(),keys.end(),[](const auto &a,const auto &b) {
        return a.channel!=b.channel ? a.channel<b.channel : a.tick<b.tick;
    });
    curveBounds={};
    if (!keys.empty()) {
        curveBounds.min=curveBounds.max={editor::Seconds(keys.front().tick),-keys.front().value};
        for (const auto &key:keys) {
            const double time=editor::Seconds(key.tick),value=-key.value;
            curveBounds.min.x=(std::min)(curveBounds.min.x,time);curveBounds.max.x=(std::max)(curveBounds.max.x,time);
            curveBounds.min.y=(std::min)(curveBounds.min.y,value);curveBounds.max.y=(std::max)(curveBounds.max.y,value);
        }
    }
    keyChannels.clear();
    for (std::size_t first=0;first<keys.size();) {
        std::size_t last=first+1;
        while (last<keys.size() && keys[last].channel==keys[first].channel) ++last;
        keyChannels.emplace_back(first,last);first=last;
    }
    visibleKeys.reserve((std::min)(keys.size(),std::size_t{4096}));
    for (auto &clip:clips) {
        clip.keys={};
        if (!clip.keyChannel) continue;
        const auto range=std::lower_bound(keyChannels.begin(),keyChannels.end(),clip.keyChannel,
            [&](const auto &range,auto id){return keys[range.first].channel<id;});
        if (range==keyChannels.end() || keys[range->first].channel!=clip.keyChannel) continue;
        auto channel=std::span<const editor::Keyframe>(keys).subspan(range->first,range->second-range->first);
        auto first=std::lower_bound(channel.begin(),channel.end(),editor::Tick{0},[](const auto &key,auto tick){return key.tick<tick;});
        auto last=std::upper_bound(first,channel.end(),clip.duration,[](auto tick,const auto &key){return tick<key.tick;});
        clip.keys={first,last};
    }
}
std::span<const editor::Keyframe> EditorWorkspaces::QueryKeys(editor::CurveQuery query) {
    visibleKeys.clear();
    for (auto [begin,end]:keyChannels) {
        auto channel=std::span<const editor::Keyframe>(keys).subspan(begin,end-begin);
        auto first=std::lower_bound(channel.begin(),channel.end(),query.time.first,
            [](const auto &key,auto tick){return key.tick<tick;});
        auto last=std::upper_bound(first,channel.end(),query.time.last,
            [](auto tick,const auto &key){return tick<key.tick;});
        for (int i=0;i<2 && first!=channel.begin();++i) --first;
        for (int i=0;i<2 && last!=channel.end();++i) ++last;
        visibleKeys.insert(visibleKeys.end(),first,last);
    }
    curvePreviewKeys.resize(visibleKeys.size());
    curve.previewKeys=curvePreviewKeys;
    return visibleKeys;
}
void EditorWorkspaces::RebuildTrackLayout() {
    timelineBounds={};
    if (!clips.empty()) {
        timelineBounds={clips.front().start,clips.front().start+clips.front().duration};
        for (const auto &clip:clips) {
            timelineBounds.first=(std::min)(timelineBounds.first,clip.start);
            timelineBounds.last=(std::max)(timelineBounds.last,clip.start+clip.duration);
        }
    }
    trackOffsets.resize(tracks.size()+1);
    trackOffsets[0]=0;
    for (std::size_t i=0;i<tracks.size();++i)
        trackOffsets[i+1]=trackOffsets[i]+video::TrackExtent(tracks[i]);
}
void EditorWorkspaces::Initialize() {
    if (initialized)
        return;
    initialized = true;
    for (int i=0;i<3;++i) animationStrips[i]={nextId++,static_cast<editor::StableId>(i+1),
        i==0?"Base motion":i==1?"Accent motion":"Ending motion",
        {editor::FromSeconds(i*2.),editor::FromSeconds(i*2.+4)}};
    stripCanvas.scale={100,1};
    Dataset(false);
    bindingCount = editor::MakeBindings(editor::ShortcutPreset::CapCut, bindings);
    timeline.memberDrags = clipDrags;
    curveCompanions.resize(1024);
    curve.companionDrags=curveCompanions;
    curve.canvas.selectionPath=curveSelectionPath;
    viewport.cameraView=&sceneCamera;
    preview::Cube(cubeVertices, cubeIndices);
    const char *names[] = {"Collection", "Hero cube", "Fill light", "Camera"};
    for (int i = 0; i < 4; ++i) {
        objects[i].id = 1000000 + i;
        objectOrder[i]=objects[i].id;
        objects[i].label = names[i];
        objects[i].parent = i ? 1000000 : 0;
        objects[i].depth = i ? 1 : 0;
        objects[i].hasChildren = i == 0;
        objects[i].transform.translation = {static_cast<double>(i - 1) * 1.5, 0, 0};
    }
    objects[1].geometry=nextId++;
    geometries[objects[1].geometry]={{cubeVertices.begin(),cubeVertices.end()},{cubeIndices.begin(),cubeIndices.end()}};
    components.push_back({{nextId++,objects[1].id,"Mesh renderer","Draw the host geometry using viewport shading.",true},false});
    components.push_back({{nextId++,objects[1].id,"Wireframe override","Override preceding renderer shading with wireframe.",false},true});
    objects[1].transform.scale = {.7, .7, .7};
    objectSelection.Set(objects[1].id);
    const char *assetNames[] = {"Studio take", "Ambience",     "Title",      "Surface",
                                "Camera rig",  "Color preset", "Proxy take", "Reference"};
    for (int i = 0; i < 8; ++i) {
        assets[i].id = 800000 + i;
        assets[i].label = assetNames[i];
        assets[i].tag = i<6 ? "Media" : "Utility";
        assets[i].status = i == 6 ? editor::AssetStatus::Proxy : editor::AssetStatus::Ready;
    }
    uv = {cg::UVVertex{900001, 1, {.1, .1}}, cg::UVVertex{900002, 1, {.9, .1}},
          cg::UVVertex{900003, 1, {.9, .9}}, cg::UVVertex{900004, 1, {.1, .9}}};
    for (std::size_t i = 0; i < pcm.size(); ++i)
        pcm[i] = static_cast<float>(std::sin(i * .27) * (.4 + .3 * std::sin(i * .015)));
    video::BuildAudioBuckets(pcm, 1, 0, audio);
    video::UpdateMeter(meter, pcm, 0);
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
            pixels[y * 64 + x] = {x / 63.f, y / 63.f, (x + y) / 126.f, 1};
    video::BuildScopes(pixels, 64, 64, {red, green, blue, luma, scopeWave, scopeVector,
                                      scopeRGB[0], scopeRGB[1], scopeRGB[2]});
}
void EditorWorkspaces::RenderPreview() {
    if (!initialized || !previewRenderer.Initialized())
        return;
    if (timeline.time.playing) {
        auto &time = timeline.time;
        time.playhead += editor::FromSeconds(ImGui::GetIO().DeltaTime * time.playbackRate);
        if (time.loop && time.inOut.last > time.inOut.first) {
            if (time.playhead >= time.inOut.last)
                time.playhead = time.inOut.first;
            if (time.playhead < time.inOut.first)
                time.playhead = time.inOut.last - 1;
        }
    }
    if (viewportSize.x > 1 && viewportSize.y > 1)
        previewRenderer.Resize(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
    previewRenderer.Render(BuildSceneMeshes(),viewport.camera);
}
std::span<const preview::Mesh> EditorWorkspaces::BuildSceneMeshes() {
    sceneMeshes.clear();sceneMeshes.reserve(objects.size());
    for (std::size_t i=0;i<objects.size();++i) if (objects[i].geometry && objects[i].visible) {
        const auto data=geometries.find(objects[i].geometry);
        if (data==geometries.end()) continue;
        preview::Mesh mesh{objects[i].id,data->second.vertices,data->second.indices,objects[i].transform};
        mesh.wire=viewport.shading==cg::Shading::Wireframe;
        bool render=false;
        for (const auto &component:components) if (component.view.owner==objects[i].id) {
            if (!component.wireOverride) {render=component.view.enabled;mesh.wire=viewport.shading==cg::Shading::Wireframe;}
            else if (component.view.enabled) mesh.wire=true;
        }
        if (render) {ApplyGizmoPreview(mesh,viewport);sceneMeshes.push_back(mesh);}
    }
    return sceneMeshes;
}
bool EditorWorkspaces::UndoTransition(bool redo) {
    if (redo ? transitionHistoryCursor>=transitionHistory.size() : transitionHistoryCursor==0) return false;
    const auto &entry=transitionHistory[redo ? transitionHistoryCursor : transitionHistoryCursor-1];
    auto clip=std::find_if(clips.begin(),clips.end(),[&](const auto &c){return c.id==entry.id;});
    if (clip==clips.end() || clip->locked) return false;
    const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip->track;});
    if (track==tracks.end() || track->locked) return false;
    const auto &expected=redo ? entry.before : entry.after;
    const auto &value=redo ? entry.after : entry.before;
    if (clip->transitionIn!=expected.first || clip->transitionOut!=expected.last ||
        static_cast<int>(clip->transitionInKind)!=expected.x || static_cast<int>(clip->transitionOutKind)!=expected.y ||
        value.first<0 || value.last<0 || value.first>clip->duration || value.last>clip->duration-value.first) return false;
    clip->transitionIn=value.first;clip->transitionOut=value.last;
    clip->transitionInKind=static_cast<video::TransitionKind>(static_cast<int>(value.x));
    clip->transitionOutKind=static_cast<video::TransitionKind>(static_cast<int>(value.y));
    if (redo) ++transitionHistoryCursor;else --transitionHistoryCursor;
    ++revision;return true;
}
void EditorWorkspaces::SyncClipProperties() {
    if (clipPropertySelection==selection.active && clipPropertyRevision==revision) return;
    clipPropertySelection=selection.active;clipPropertyRevision=revision;
    clipPropertyIds={};clipPropertyValues={1,1,0,1};clipInspectorLocked=true;
    auto clip=std::find_if(clips.begin(),clips.end(),[&](const auto &c){return c.id==selection.active;});
    if (clip==clips.end()) return;
    auto [entry,created]=clipProperties.try_emplace(clip->id);
    if (created) for (std::size_t i=0;i<entry->second.ids.size();++i) {
        const auto id=nextId++;entry->second.ids[i]=id;clipPropertyOwners.emplace(id,std::pair{clip->id,i});
    }
    entry->second.values[3]=clip->speed;
    clipPropertyIds=entry->second.ids;clipPropertyValues=entry->second.values;
    const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip->track;});
    clipInspectorLocked=clip->locked || track==tracks.end() || track->locked;
}
void EditorWorkspaces::CopyClipEditingData(const video::ClipView &source,video::ClipView &copy) {
    // Media spans remain borrowed; editable host data receives independent IDs and storage.
    if (const auto properties=clipProperties.find(source.id);properties!=clipProperties.end()) {
        auto &target=clipProperties[copy.id];target.values=properties->second.values;
        for (std::size_t i=0;i<target.ids.size();++i) {
            const auto oldId=properties->second.ids[i],newId=nextId++;
            target.ids[i]=newId;clipPropertyOwners.emplace(newId,std::pair{copy.id,i});
            if (const auto flags=propertyFlags.find(oldId);flags!=propertyFlags.end()) propertyFlags[newId]=flags->second;
            if (const auto channel=propertyKeys.find(oldId);channel!=propertyKeys.end()) {
                auto &newKeys=propertyKeys[newId];newKeys=channel->second;
                for (auto &key:newKeys) {key.id=nextId++;key.channel=newId;}
            }
        }
    }
    if (!source.envelope.empty()) {
        auto &points=clipEnvelopes[copy.id];points.assign(source.envelope.begin(),source.envelope.end());
        for (auto &point:points) point.id=nextId++;
        copy.envelope=points;
    } else copy.envelope={};
    copy.keys={};
    if (source.keyChannel) {
        copy.keyChannel=nextId++;
        const auto count=keys.size();
        for (std::size_t i=0;i<count;++i) if (keys[i].channel==source.keyChannel) {
            auto key=keys[i];key.id=nextId++;key.channel=copy.keyChannel;keys.push_back(key);
        }
    }
    if (const auto label=renamedLabels.find(source.id);label!=renamedLabels.end()) {
        renamedLabels[copy.id]=label->second;copy.label=renamedLabels[copy.id].c_str();
    }
}
void EditorWorkspaces::ApplyEvents() {
    bool changed = false;
    for (const auto &e : events.Events()) {
        if (e.phase==editor::Phase::Begin && e.kind==editor::EditKind::Property)
            propertyGestureSelection.assign(objectSelection.storage.begin(),objectSelection.storage.begin()+objectSelection.count);
        if (e.phase != editor::Phase::Commit || e.revision != revision)
            continue;
        ++commits;
        if (e.kind==editor::EditKind::Reorder) {
            auto from=std::find_if(animationStrips.begin(),animationStrips.end(),[&](const auto &strip){return strip.id==e.target;});
            auto to=std::find_if(animationStrips.begin(),animationStrips.end(),[&](const auto &strip){return strip.id==e.proposed.parent;});
            if (from!=animationStrips.end() && to!=animationStrips.end() && !from->locked && !to->locked) {
                std::iter_swap(from,to);changed=true;
            }
        }
        for (auto &strip:animationStrips) if (strip.id==e.target) {
            if (e.kind==editor::EditKind::StripSettings) {
                if (strip.locked) {
                    if (!(e.proposed.offset&2) && e.proposed.x==strip.scale && e.proposed.y==strip.repeat &&
                        e.proposed.z==strip.blend && ((e.proposed.offset&1)!=0)==strip.muted) {
                        strip.locked=false;changed=true;
                    }
                    continue;
                }
                if (!std::isfinite(e.proposed.x) || !std::isfinite(e.proposed.y) || !std::isfinite(e.proposed.z) ||
                    e.proposed.x<.001 || e.proposed.x>1000 || e.proposed.y<.001 || e.proposed.y>1000 ||
                    e.proposed.z<0 || e.proposed.z>1) continue;
                strip.scale=e.proposed.x;strip.repeat=e.proposed.y;strip.blend=e.proposed.z;
                strip.muted=(e.proposed.offset&1)!=0;strip.locked=(e.proposed.offset&2)!=0;changed=true;
            } else if ((e.kind==editor::EditKind::Move || e.kind==editor::EditKind::TrimStart ||
                        e.kind==editor::EditKind::TrimEnd) && !strip.locked) {
                strip.range={e.proposed.first,e.proposed.last};changed=true;
            }
        }
        if (e.kind==editor::EditKind::AudioEnvelope) {
            auto owner=std::find_if(clips.begin(),clips.end(),[&](const auto &c){return c.id==e.proposed.parent;});
            if (owner==clips.end() || owner->locked || !std::isfinite(e.proposed.x)) continue;
            auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==owner->track;});
            if (track==tracks.end() || track->locked) continue;
            auto &points=clipEnvelopes[owner->id];
            auto point=std::find_if(points.begin(),points.end(),[&](const auto &p){return p.id==e.target;});
            if (e.proposed.offset==1 && e.target==owner->id) {
                const auto tick=std::clamp(e.proposed.first,editor::Tick{0},owner->duration);
                auto same=std::find_if(points.begin(),points.end(),[&](const auto &p){return p.tick==tick;});
                if (same==points.end()) {points.push_back({nextId++,tick,std::clamp(e.proposed.x,0.,2.)});changed=true;}
            } else if (point!=points.end() && !point->locked) {
                if (e.proposed.offset==2) {points.erase(point);changed=true;}
                else if (e.proposed.offset==0) {
                    point->tick=std::clamp(e.proposed.first,editor::Tick{0},owner->duration);
                    point->gain=std::clamp(e.proposed.x,0.,2.);changed=true;
                }
            }
            std::sort(points.begin(),points.end(),[](const auto &a,const auto &b){return a.tick<b.tick;});
            owner->envelope=points;
        }
        if (e.kind == editor::EditKind::Select)
            continue;
        if (e.kind == editor::EditKind::Marker && markerCount < markers.size()) {
            markers[markerCount++] = {nextId++, e.proposed.first, "Marker"};
            changed = true;
        }
        if (e.kind==editor::EditKind::ComponentAdd && (e.proposed.parent==7801 || e.proposed.parent==7802)) {
            const auto owner=std::find_if(objects.begin(),objects.end(),[&](const auto &v){return v.id==e.target;});
            if (owner!=objects.end() && !owner->locked && owner->geometry) {
                const bool wire=e.proposed.parent==7802;
                components.push_back({{nextId++,owner->id,wire?"Wireframe override":"Mesh renderer",
                    wire?"Override preceding renderer shading with wireframe.":"Draw the host geometry using viewport shading.",true},wire});
                changed=true;
            }
        }
        auto component=std::find_if(components.begin(),components.end(),[&](const auto &v){return v.view.id==e.target;});
        if (component!=components.end()) {
            const auto owner=std::find_if(objects.begin(),objects.end(),[&](const auto &v){return v.id==component->view.owner;});
            if (owner==objects.end() || (owner->locked && !(e.kind==editor::EditKind::Toggle && e.proposed.x==1))) continue;
            if (e.kind==editor::EditKind::Toggle) {
                const int field=static_cast<int>(e.proposed.x);
                if (field==2) {component->view.locked=e.proposed.y!=0;changed=true;}
                else if (field==1) {component->view.expanded=e.proposed.y!=0;changed=true;}
                else if (field==0 && !component->view.locked) {component->view.enabled=e.proposed.y!=0;changed=true;}
            } else if (!component->view.locked && e.kind==editor::EditKind::Remove) {
                components.erase(component);changed=true;
            } else if (!component->view.locked && e.kind==editor::EditKind::Reorder &&
                       component->view.owner==e.proposed.parent && (e.proposed.offset==-1 || e.proposed.offset==1)) {
                const auto direction=static_cast<std::ptrdiff_t>(e.proposed.offset);
                for (auto i=std::distance(components.begin(),component)+direction;i>=0 && i<static_cast<std::ptrdiff_t>(components.size());i+=direction)
                    if (components[i].view.owner==component->view.owner) {
                        if (!components[i].view.locked) {std::iter_swap(component,components.begin()+i);changed=true;}break;
                    }
            }
        }
        if (e.kind==editor::EditKind::Duplicate) {
            auto source=std::find_if(objects.begin(),objects.end(),[&](const auto &v){return v.id==e.target;});
            if (source!=objects.end() && !source->locked) {
                auto copy=*source;copy.id=nextId++;copy.hasChildren=false;
                renamedLabels[copy.id]=std::string(source->label)+" copy";copy.label=renamedLabels[copy.id].c_str();
                std::array<editor::StableId,9> properties;
                for (auto &property:properties) property=nextId++;
                if (copy.geometry && e.proposed.offset!=1) {
                    const auto sourceGeometry=copy.geometry;copy.geometry=nextId++;
                    geometries[copy.geometry]=geometries.at(sourceGeometry);
                }
                objects.push_back(copy);objectPropertyIds.push_back(properties);
                const auto componentCount=components.size();
                for (std::size_t i=0;i<componentCount;++i) if (components[i].view.owner==e.target) {
                    auto component=components[i];component.view.id=nextId++;component.view.owner=copy.id;
                    components.push_back(component);
                }
                auto position=std::find(objectOrder.begin(),objectOrder.end(),e.target);
                objectOrder.insert(position==objectOrder.end() ? position : position+1,copy.id);
                objectSelection.Set(copy.id);changed=true;
            }
        }
        if (e.kind==editor::EditKind::LinkGeometry) {
            auto target=std::find_if(objects.begin(),objects.end(),[&](const auto &v){return v.id==e.target;});
            if (target!=objects.end() && !target->locked && target->geometry) {
                if (e.proposed.parent) {
                    const auto source=std::find_if(objects.begin(),objects.end(),[&](const auto &v){return v.id==e.proposed.parent;});
                    if (source!=objects.end() && source->geometry) {target->geometry=source->geometry;changed=true;}
                } else {
                    const auto newGeometry=nextId++;geometries[newGeometry]=geometries.at(target->geometry);
                    target->geometry=newGeometry;changed=true;
                }
            }
        }
        if (e.kind == editor::EditKind::Rename)
            renamedLabels[e.target] = e.proposedText.data();
        if (e.kind==editor::EditKind::Rename) for (auto &object:objects)
            if (object.id==e.target && !object.locked) {object.label=renamedLabels[e.target].c_str();changed=true;}
        auto clip = std::find_if(clips.begin(), clips.end(), [&](const auto &c) { return c.id == e.target; });
        if (clip != clips.end()) {
            auto transitionValue=[](const video::ClipView &c) {
                editor::Value value;value.first=c.transitionIn;value.last=c.transitionOut;
                value.x=static_cast<int>(c.transitionInKind);value.y=static_cast<int>(c.transitionOutKind);return value;
            };
            const auto transitionBefore=transitionValue(*clip);
            if (e.kind==editor::EditKind::TransitionType && !clip->locked && e.proposed.first>=0 && e.proposed.first<=3 && e.proposed.last>=0 && e.proposed.last<=3) {
                const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip->track;});
                if (track!=tracks.end() && !track->locked) {
                    clip->transitionInKind=static_cast<video::TransitionKind>(e.proposed.first);
                    clip->transitionOutKind=static_cast<video::TransitionKind>(e.proposed.last);changed=true;
                }
            }
            if (e.kind==editor::EditKind::TransitionDuration && !clip->locked) {
                const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip->track;});
                if (track!=tracks.end() && !track->locked) {
                    clip->transitionIn=std::clamp(e.proposed.first,editor::Tick{0},clip->duration);
                    clip->transitionOut=std::clamp(e.proposed.last,editor::Tick{0},clip->duration-clip->transitionIn);
                    changed=true;
                }
            }
            if ((e.kind==editor::EditKind::TransitionType || e.kind==editor::EditKind::TransitionDuration) &&
                !(transitionBefore==transitionValue(*clip))) {
                transitionHistory.resize(transitionHistoryCursor);
                transitionHistory.push_back({clip->id,transitionBefore,transitionValue(*clip)});
                transitionHistoryCursor=transitionHistory.size();
            }
            if (e.kind == editor::EditKind::Rename) {
                clip->label = renamedLabels[e.target].c_str();
                changed = true;
            }
            if (e.kind == editor::EditKind::Split) {
                const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip->track;});
                if (clip->locked || track==tracks.end() || track->locked) continue;
                auto split = video::SplitClip(*clip, e.proposed.first, {});
                if (split.valid) {
                    auto right = *clip;
                    right.id = nextId++;
                    right.start = split.right.start;
                    right.duration = split.right.duration;
                    right.sourceIn = split.right.sourceIn;
                    if (!clip->envelope.empty()) {
                        const auto localCut=split.left.duration;
                        const auto gain=video::EvaluateEnvelope(clip->envelope,localCut);
                        std::vector<video::EnvelopePoint> leftPoints,rightPoints;
                        for (const auto &point:clip->envelope) {
                            if (point.tick<=localCut) leftPoints.push_back(point);
                            if (point.tick>=localCut) {
                                auto moved=point;moved.id=nextId++;moved.tick-=localCut;rightPoints.push_back(moved);
                            }
                        }
                        if (leftPoints.empty() || leftPoints.back().tick!=localCut)
                            leftPoints.push_back({nextId++,localCut,gain});
                        if (rightPoints.empty() || rightPoints.front().tick!=0)
                            rightPoints.insert(rightPoints.begin(),{nextId++,0,gain});
                        clipEnvelopes[clip->id]=std::move(leftPoints);
                        clipEnvelopes[right.id]=std::move(rightPoints);
                        clip->envelope=clipEnvelopes.at(clip->id);right.envelope=clipEnvelopes.at(right.id);
                    }
                    clip->duration = split.left.duration;
                    clips.push_back(right);
                    changed = true;
                }
            } else if (e.kind == editor::EditKind::Duplicate) {
                const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip->track;});
                if (clip->locked || track==tracks.end() || track->locked) continue;
                auto copy = *clip;
                copy.id = nextId++;
                copy.start = e.proposed.first;
                CopyClipEditingData(*clip,copy);
                clips.push_back(copy);
                changed = true;
            } else if (e.kind == editor::EditKind::Move || e.kind == editor::EditKind::TrimStart ||
                       e.kind == editor::EditKind::TrimEnd || e.kind == editor::EditKind::Slip ||
                       e.kind == editor::EditKind::Ripple || e.kind == editor::EditKind::Roll ||
                       e.kind == editor::EditKind::Slide) {
                const auto oldEnd=clip->start+clip->duration;
                if (clip->start==e.proposed.first && oldEnd==e.proposed.last && clip->sourceIn==e.proposed.offset)
                    continue;
                auto track = clip->track;
                clip->start = e.proposed.first;
                clip->duration = e.proposed.last - e.proposed.first;
                clip->sourceIn = e.proposed.offset;
                if (e.kind == editor::EditKind::Ripple)
                    for (auto &other : clips)
                        if (other.track == track && other.start >= oldEnd && other.id != e.target)
                            other.start += e.proposed.last - oldEnd;
                changed = true;
            }
        }
        for (auto &strip : audioStrips)
            if (e.kind == editor::EditKind::Property) {
                if (e.target==strip.gainId) { strip.gain=e.proposed.x; changed=true; }
                if (e.target==strip.panId) { strip.pan=e.proposed.x; changed=true; }
            }
        for (auto &track : tracks) {
            if (track.id==e.target && e.kind==editor::EditKind::TrackHeight) {
                track.height=static_cast<float>(e.proposed.x); changed=true;
            }
            if (track.id == e.target && e.kind == editor::EditKind::Toggle) {
                bool *fields[] = {&track.visible, &track.mute,   &track.solo,
                                  &track.locked,  &track.record, &track.target, &track.source, &track.expanded};
                int field = static_cast<int>(e.proposed.x);
                if (field >= 0 && field < 8) {
                    *fields[field] = e.proposed.y != 0;
                    changed = true;
                }
            }
        }
        if (e.kind==editor::EditKind::Reorder && (e.proposed.offset==-1 || e.proposed.offset==1)) {
            const auto object=std::find_if(objects.begin(),objects.end(),[&](const auto &v){return v.id==e.target;});
            auto current=std::find(objectOrder.begin(),objectOrder.end(),e.target);
            if (object!=objects.end() && !object->locked && current!=objectOrder.end() && object->parent==e.proposed.parent) {
                const auto direction=static_cast<std::ptrdiff_t>(e.proposed.offset);
                for (auto index=std::distance(objectOrder.begin(),current)+direction;index>=0 && index<static_cast<std::ptrdiff_t>(objectOrder.size());index+=direction) {
                    const auto sibling=std::find_if(objects.begin(),objects.end(),[&](const auto &v){return v.id==objectOrder[index];});
                    if (sibling==objects.end() || sibling->parent!=object->parent) continue;
                    if (!sibling->locked) {std::iter_swap(current,objectOrder.begin()+index);changed=true;}
                    break;
                }
            }
        }
        if (e.kind==editor::EditKind::Toggle && e.proposed.x==5) {
            for (auto &object:objects) {
                auto ancestor=object.id;
                for (std::size_t depth=0;ancestor && depth<objects.size();++depth) {
                    if (ancestor==e.target) {object.expanded=e.proposed.y!=0;changed=true;break;}
                    const auto parent=std::find_if(objects.begin(),objects.end(),[&](const auto &v){return v.id==ancestor;});
                    ancestor=parent==objects.end() ? 0 : parent->parent;
                }
            }
        }
        for (auto &o : objects) {
            if (o.id == e.target && e.kind == editor::EditKind::Toggle) {
                bool *fields[] = {&o.visible, &o.selectable, &o.renderable, &o.locked, &o.expanded};
                int field = static_cast<int>(e.proposed.x);
                if (field >= 0 && field < 5) {
                    *fields[field] = e.proposed.y != 0;
                    changed = true;
                }
            }
            if (o.id == e.target) {
                if (e.kind == editor::EditKind::Translate) {
                    o.transform.translation = {e.proposed.x, e.proposed.y, e.proposed.z};
                    changed = true;
                }
                if (e.kind == editor::EditKind::Rotate) {
                    o.transform.rotation = {e.proposed.x, e.proposed.y, e.proposed.z};
                    changed = true;
                }
                if (e.kind == editor::EditKind::Scale) {
                    o.transform.scale = {e.proposed.x, e.proposed.y, e.proposed.z};
                    changed = true;
                }
                if (e.kind == editor::EditKind::Reparent && !o.locked) {
                    const auto destination=std::find_if(objects.begin(),objects.end(),[&](const auto &v){return v.id==e.proposed.parent;});
                    bool cycle = e.proposed.parent && (destination==objects.end() || destination->locked);
                    auto parent = e.proposed.parent;
                    for (std::size_t depth = 0; parent && depth < objects.size(); ++depth) {
                        if (parent == o.id) {
                            cycle = true;
                            break;
                        }
                        auto it = std::find_if(objects.begin(), objects.end(),
                                               [&](auto &v) { return v.id == parent; });
                        parent = it == objects.end() ? 0 : it->parent;
                    }
                    if (!cycle) {
                        o.parent = e.proposed.parent;
                        changed = true;
                    }
                }
            }
            for (int component = 0; component < 9; ++component)
                if (e.target == ObjectPropertyId(*this, o.id, component) &&
                    (e.kind == editor::EditKind::Property || e.kind == editor::EditKind::Reset)) {
                    bool allowed=true;
                    if (e.kind==editor::EditKind::Property) {
                        auto selected=objectSelection.storage.first(objectSelection.count);
                        allowed=selected.size()==propertyGestureSelection.size() &&
                            std::equal(selected.begin(),selected.end(),propertyGestureSelection.begin());
                    }
                    for (auto &target:objects)
                        if (target.id==o.id || objectSelection.Contains(target.id))
                            allowed &= !target.locked && !(propertyFlags[ObjectPropertyId(*this,target.id,component)]&16u);
                    if (allowed) for (auto &target:objects)
                        if (target.id==o.id || objectSelection.Contains(target.id)) {
                            TransformComponent(target.transform,component)=e.proposed.x;
                            changed=true;
                        }
                }
        }
        if (e.kind == editor::EditKind::Property) {
            float *rgb = e.target == colorIds.lift ? colors.lift : e.target == colorIds.gamma ? colors.gamma :
                         e.target == colorIds.gain ? colors.gain : nullptr;
            if (rgb) {
                rgb[0]=static_cast<float>(e.proposed.x); rgb[1]=static_cast<float>(e.proposed.y);
                rgb[2]=static_cast<float>(e.proposed.z); changed=true;
            }
            float *scalar = e.target == colorIds.temperature ? &colors.temperature :
                            e.target == colorIds.tint ? &colors.tint :
                            e.target == colorIds.exposure ? &colors.exposure : nullptr;
            if (scalar) { *scalar=static_cast<float>(e.proposed.x); changed=true; }
        }
        if (e.kind==editor::EditKind::KeyInsert) {
            if (e.proposed.parent) {
                auto owner=std::find_if(clips.begin(),clips.end(),[&](const auto &c){return c.id==e.proposed.parent;});
                if (owner==clips.end() || owner->locked || owner->keyChannel!=e.target ||
                    e.proposed.first<0 || e.proposed.first>owner->duration) continue;
                auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==owner->track;});
                if (track==tracks.end() || track->locked) continue;
            }
            auto existing=std::find_if(keys.begin(),keys.end(),[&](const auto &key){return key.channel==e.target && key.tick==e.proposed.first;});
            if (existing==keys.end()) {keys.push_back({nextId++,e.target,e.proposed.first,e.proposed.x});changed=true;}
        }
        for (auto &key : keys)
            if (key.id == e.target) {
                if (e.kind==editor::EditKind::Navigate) {timeline.time.playhead=e.proposed.parent ? e.proposed.first : key.tick;keySelection.Set(key.id);}
                if (key.locked) continue;
                if (e.kind==editor::EditKind::Remove) {
                    keys.erase(keys.begin()+(&key-keys.data()));
                    keySelection.Clear();changed=true;break;
                }
                if (e.kind==editor::EditKind::Duplicate) {
                    auto copy=key;copy.id=nextId++;copy.tick=e.proposed.first;copy.value=e.proposed.x;
                    keys.push_back(copy);changed=true;break;
                }
                if (e.kind==editor::EditKind::KeyInterpolation && e.proposed.x>=0 && e.proposed.x<3) {
                    key.interpolation=static_cast<editor::Interpolation>(static_cast<int>(e.proposed.x));changed=true;
                }
                if (e.kind==editor::EditKind::KeyHandleMode && e.proposed.x>=0 && e.proposed.x<5) {
                    key.handles=static_cast<editor::HandleMode>(static_cast<int>(e.proposed.x));changed=true;
                }
                if (e.kind == editor::EditKind::Keyframe || e.kind==editor::EditKind::KeyScale) {
                    key.tick = e.proposed.first;
                    key.value = e.proposed.x;
                    changed = true;
                }
                if (e.kind == editor::EditKind::Handle) {
                    auto first = std::find_if(keys.begin(), keys.end(),
                                              [&](const auto &v) { return v.channel == key.channel; });
                    auto last = std::find_if(first, keys.end(),
                                             [&](const auto &v) { return v.channel != key.channel; });
                    auto channel = std::span<const editor::Keyframe>(first, last);
                    key = editor::MoveHandle(
                        editor::ResolveHandles(channel, static_cast<std::size_t>(&key - &*first)),
                        e.original.offset < 0, {e.proposed.x, e.proposed.y});
                    changed = true;
                }
            }
        for (auto &v : uv)
            if (v.id == e.target && (e.kind == editor::EditKind::Translate ||
                e.kind == editor::EditKind::Rotate || e.kind == editor::EditKind::Scale)) {
                v.uv = {e.proposed.x, e.proposed.y};
                changed = true;
            }
        if (e.kind==editor::EditKind::Navigate && (e.target==assetPathIds[0] || e.target==assetPathIds[1])) {
            assetPathDepth=e.target==assetPathIds[0]?1:2;
            assetState.search[0]=assetState.tag[0]=0;
            assetState.status=-1;
        }
        for (auto &asset : assets)
            if (asset.id == e.target && e.kind == editor::EditKind::Rename) {
                asset.label = renamedLabels[e.target].c_str();
                changed = true;
            }
        const auto clipProperty=clipPropertyOwners.find(e.target);
        if (clipProperty!=clipPropertyOwners.end()) {
            const auto owner=std::find_if(clips.begin(),clips.end(),[&](const auto &c){return c.id==clipProperty->second.first;});
            if (owner==clips.end() || owner->locked || owner->id!=selection.active) continue;
            const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==owner->track;});
            if (track==tracks.end() || track->locked) continue;
        }
        bool isProperty=clipProperty!=clipPropertyOwners.end();
        for (const auto &object:objects)
            for (int component=0;component<9;++component)
                isProperty |= e.target==ObjectPropertyId(*this, object.id,component);
        if (isProperty && e.kind==editor::EditKind::PropertyKey) {
            auto &channel=propertyKeys[e.target];
            auto next=std::lower_bound(channel.begin(),channel.end(),e.proposed.first,
                [](const auto &key,auto tick){return key.tick<tick;});
            auto action=static_cast<editor::PropertyKeyAction>(e.proposed.offset);
            if (action==editor::PropertyKeyAction::Add) {
                if (next!=channel.end() && next->tick==e.proposed.first) next->value=e.proposed.x;
                else channel.insert(next,{nextId++,e.target,e.proposed.first,e.proposed.x});
                changed=true;
            } else if (action==editor::PropertyKeyAction::Remove) {
                if (next!=channel.end() && next->tick==e.proposed.first) {channel.erase(next);changed=true;}
            } else if (action==editor::PropertyKeyAction::Previous) {
                if (next!=channel.begin()) timeline.time.playhead=(--next)->tick;
            } else if (action==editor::PropertyKeyAction::Next) {
                if (next!=channel.end() && next->tick==e.proposed.first) ++next;
                if (next!=channel.end()) timeline.time.playhead=next->tick;
            }
        }
        if (isProperty && e.kind==editor::EditKind::Toggle) {
            unsigned flag=static_cast<unsigned>(e.proposed.x);
            if (flag==8 || flag==16 || flag==4) {
                if (e.proposed.y) propertyFlags[e.target] |= flag;
                else propertyFlags[e.target] &= ~flag;
                changed=true;
            }
        }
        if (clipProperty!=clipPropertyOwners.end() &&
            (e.kind==editor::EditKind::Property || e.kind==editor::EditKind::Reset) && std::isfinite(e.proposed.x) &&
            !(propertyFlags[e.target]&16u)) {
            const auto [owner,component]=clipProperty->second;
            if ((component==0 && (e.proposed.x<0 || e.proposed.x>1)) ||
                ((component==1 || component==3) && e.proposed.x<=0)) continue;
            auto &value=clipProperties.at(owner).values[component];
            if (value!=e.proposed.x) {
                value=e.proposed.x;changed=true;
                if (component==3) {
                    auto clip=std::find_if(clips.begin(),clips.end(),[&](const auto &c){return c.id==owner;});
                    clip->speed=value;
                }
            }
        }
    }
    if (changed) {
        RebuildTrackLayout();
        ++revision;
        std::sort(clips.begin(), clips.end(), [](const auto &a, const auto &b) {
            return a.track != b.track ? a.track < b.track : a.start < b.start;
        });
        RebuildKeyIndex();
    }
    events.Clear();SyncClipProperties();
}
void VideoWorkspace(EditorWorkspaces &s, const Theme &theme, ImTextureRef texture) {
    s.curve.icons=s.icons;
    s.uvState.icons=s.icons;
    s.Initialize();
    Options(s);s.SyncClipProperties();
    float available = ImGui::GetContentRegionAvail().x;
    float side = (s.narrow ? 180 : 260) * ImGui::GetFontSize()/14;
    float top = (std::max)(220.f, ImGui::GetContentRegionAvail().y * .4f);
    ImGui::BeginChild("Media bin", {side, top}, ImGuiChildFlags_Borders);
    ImGui::SeparatorText(s.japanese ? "素材" : "Media Bin");
    const char *assetPath[]={"All assets","Media"};
    editor::AssetBrowser("media", Assets(s), s.assetState, s.selection, s.events, std::span(assetPath).first(s.assetPathDepth));
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Monitors", {(std::max)(100.f, available - side * 2 - 20), top},
                      ImGuiChildFlags_Borders);
    float width = ImGui::GetContentRegionAvail().x;
    float monitorHeight = (std::max)(100.f, top - 100);
    ImGui::BeginGroup();
    video::Monitor("Source", texture, {width * .48f, monitorHeight}, s.timeline.time,
                   {true, false, true, false, "Source / Studio"}, theme);
    ImGui::EndGroup();
    ImGui::SameLine();
    if (s.monitorRevision!=s.revision || s.monitorClipId!=s.selection.active) {
        s.monitorRevision=s.revision;s.monitorClipId=s.selection.active;
        auto clip=std::find_if(s.clips.begin(),s.clips.end(),[&](const auto &c){return c.id==s.monitorClipId;});
        s.monitorClipIndex=static_cast<std::size_t>(clip-s.clips.begin());
    }
    video::MonitorOptions program{true,true,true,true,s.japanese ? "プログラム" : "Program",{.5f,.5f},true};
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
    video::Monitor("Program", ImTextureRef(static_cast<ImTextureID>(s.previewRenderer.Texture())),
                   {width*.48f,monitorHeight},s.timeline.time,program,theme);
    s.programMonitorMin=ImGui::GetItemRectMin();s.programMonitorMax=ImGui::GetItemRectMax();
    if (ImGui::BeginPopupContextItem("monitor metadata")) {
        const char *labels[]={s.japanese ? "非表示" : "No metadata",s.japanese ? "クリップ" : "Clip metadata",s.japanese ? "詳細" : "Detailed metadata"};
        for (int i=0;i<3;++i)
            if (ImGui::MenuItem(labels[i],nullptr,static_cast<int>(s.monitorMetadata)==i))
                s.monitorMetadata=static_cast<video::MonitorMetadataPreset>(i);
        ImGui::EndPopup();
    }
    editor::Transport(s.timeline.time, std::span(s.bindings).first(s.bindingCount), s.icons);
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Clip Inspector", {0, top}, ImGuiChildFlags_Borders);
    ImGui::SeparatorText("Clip Inspector");
    editor::PropertyView props[] = {{s.clipPropertyIds[0], "Opacity", "Video", 1, 1, editor::PropertyFlags::Animated},
                                    {s.clipPropertyIds[1], "Scale", "Transform", 1, 1},
                                    {s.clipPropertyIds[2], "Position X", "Transform", 0, 0},
                                    {s.clipPropertyIds[3], "Speed", "Retiming", 1, 1}};
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
        editor::PropertyGrid("clip",properties,s.videoProperties,s.events);
        ImGui::EndDisabled();
    } else ImGui::TextUnformatted(s.japanese ? "クリップを選択" : "Select a clip");
    ImGui::EndChild();
    video::TimelineProvider p{&s, s.revision, static_cast<int>(s.tracks.size())};
    p.tracks = [](void *u, int a, int n) {
        auto &s = *static_cast<EditorWorkspaces *>(u);
        ++s.queryCount;
        return std::span<const video::TrackView>(s.tracks).subspan(a, n);
    };
    p.clips = [](void *u, editor::StableId track, editor::Range range) {
        auto &s = *static_cast<EditorWorkspaces *>(u);
        ++s.queryCount;
        auto begin = std::lower_bound(s.clips.begin(), s.clips.end(), track,
                                      [](const auto &c, auto t) { return c.track < t; });
        auto end =
            std::upper_bound(begin, s.clips.end(), track, [](auto t, const auto &c) { return t < c.track; });
        auto first = std::lower_bound(begin, end, range.first - editor::TicksPerSecond * 60,
                                      [](const auto &c, auto t) { return c.start < t; });
        while (first != end && first->start + first->duration < range.first)
            ++first;
        auto last =
            std::upper_bound(first, end, range.last, [](auto t, const auto &c) { return t < c.start; });
        s.queriedClips += last - first;
        return std::span<const video::ClipView>(first, last);
    };
    p.layout=[](void *u,double firstPixel,double lastPixel) {
        auto &s=*static_cast<EditorWorkspaces *>(u);
        ++s.queryCount;
        auto first=std::upper_bound(s.trackOffsets.begin(),s.trackOffsets.end(),firstPixel);
        std::size_t index=first==s.trackOffsets.begin() ? 0 : static_cast<std::size_t>(first-s.trackOffsets.begin()-1);
        index=(std::min)(index,s.tracks.size());
        auto last=std::lower_bound(s.trackOffsets.begin()+index,s.trackOffsets.end(),lastPixel);
        auto end=(std::min)(static_cast<std::size_t>(last-s.trackOffsets.begin()),s.tracks.size());
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
    p.selected = [](void *u, std::span<const editor::StableId> ids) {
        auto &s = *static_cast<EditorWorkspaces *>(u);
        std::size_t n = 0;
        for (auto id : ids) {
            auto it = std::find_if(s.clips.begin(), s.clips.end(), [&](const auto &c) { return c.id == id; });
            if (it != s.clips.end() && n < s.selectedClips.size()) {
                s.selectedClips[n] = *it;
                auto track = std::find_if(s.tracks.begin(), s.tracks.end(),
                                          [&](const auto &t) { return t.id == it->track; });
                s.selectedClips[n++].locked = it->locked || (track != s.tracks.end() && track->locked);
            }
        }
        return std::span<const video::ClipView>(s.selectedClips).first(n);
    };
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
    float timelineHeight = (std::max)(140.f, remaining - 2*ImGui::GetFrameHeightWithSpacing() -
        (s.activeVideoPanel == 1 ? 380.f * ImGui::GetFontSize()/14 : 230.f));
    s.timelineOrigin = ImGui::GetCursorScreenPos();
    s.timeline.keySelection=&s.keySelection;
    s.timeline.keyCompanions=s.clipKeyCompanions;
    video::Timeline("Timeline", p, s.timeline, s.selection, s.events, theme, {0, timelineHeight});
    ImGui::BeginChild("Audio color", {0, 0}, ImGuiChildFlags_Borders);
    if (ImGui::BeginTabBar("audio color")) {
        if (ImGui::BeginTabItem("Audio")) {
            s.activeVideoPanel = 0;
            auto mixer=std::find_if(s.audioStrips.begin(),s.audioStrips.end(),
                                   [&](const auto &v){return v.id==s.mixerTrack;});
            ImGui::SetNextItemWidth(220);
            if (ImGui::BeginCombo("Channel",mixer==s.audioStrips.end() ? "" : mixer->label)) {
                for (const auto &channel:s.audioStrips) {
                    ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(channel.id)));
                    if (ImGui::Selectable(channel.label,channel.id==s.mixerTrack)) s.mixerTrack=channel.id;
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            if (mixer!=s.audioStrips.end()) {
                auto view=*mixer;
                auto track=std::find_if(s.tracks.begin(),s.tracks.end(),[&](const auto &v){return v.id==view.id;});
                if (track!=s.tracks.end()) {
                    view.mute=track->mute; view.solo=track->solo; view.record=track->record; view.locked=track->locked;
                }
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
            const ImVec4 tints[] = {{1,.25f,.25f,1},{.25f,1,.25f,1},{.3f,.5f,1,1}};
            for (int channel=0; channel<3; ++channel) {
                ImGui::SameLine();
                ImGui::PushID(channel);
                video::ScopeImage("RGB waveform", s.scopeRGB[channel],64,256,{scopeWidth,80},theme,tints[channel]);
                ImGui::PopID();
            }
            video::ColorControls("Grade",s.colors,s.colorIds,s.revision,s.colorState,s.events);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Keyframes / Curves")) {
            editor::CurveEditor("clip curve", Curves(s), s.curve, s.keySelection, s.events, theme, {0, 160});
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();
    s.ApplyEvents();
}
void EditorWorkspaces::RebuildOutlinerRows() {
    outlinerRows.clear();outlinerRows.reserve(objects.size());
    orderedObjects.assign(objects.begin(),objects.end());
    auto &ordered=orderedObjects;
    std::sort(ordered.begin(),ordered.end(),[&](const auto &a,const auto &b){
        return std::find(objectOrder.begin(),objectOrder.end(),a.id)<std::find(objectOrder.begin(),objectOrder.end(),b.id);
    });
    ImGuiTextFilter filter(outliner.search);
    auto matches=[&](auto &&self,const cg::ObjectView &object,std::size_t depth)->bool {
        if (depth>objects.size()) return false;
        if (filter.PassFilter(object.label)) return true;
        for (const auto &child:ordered) if (child.parent==object.id && self(self,child,depth+1)) return true;
        return false;
    };
    auto append=[&](auto &&self,const cg::ObjectView &object,int depth)->void {
        if (depth>=static_cast<int>(objects.size()) || !matches(matches,object,0)) return;
        auto row=object;row.depth=depth;
        row.hasChildren=std::any_of(objects.begin(),objects.end(),[&](const auto &child){return child.parent==object.id;});
        outlinerRows.push_back(row);
        if (object.expanded || filter.IsActive())
            for (const auto &child:ordered) if (child.parent==object.id) self(self,child,depth+1);
    };
    for (const auto &object:ordered)
        if (!object.parent || std::none_of(objects.begin(),objects.end(),[&](const auto &parent){return parent.id==object.parent;}))
            append(append,object,0);
}
void CGWorkspace(EditorWorkspaces &s, const Theme &theme, ImTextureRef texture) {
    s.curve.icons=s.icons;
    s.uvState.icons=s.icons;
    s.Initialize();
    Options(s);
    float width = ImGui::GetContentRegionAvail().x, side = (s.narrow ? 180 : 300)*ImGui::GetFontSize()/14,
          top = ImGui::GetContentRegionAvail().y * .6f;
    ImGui::BeginChild("View stack", {width - side - 10, top}, ImGuiChildFlags_Borders);
    s.viewport.icons=s.icons;
    ImGui::Checkbox("OpenGL preview", &s.useGL);
    if (s.viewport.pivot==cg::Pivot::Cursor) {
        double cursor[]={s.cursorPivot.x,s.cursorPivot.y,s.cursorPivot.z};
        ImGui::BeginDisabled(s.viewport.drag.active);
        ImGui::SetNextItemWidth(ImGui::GetFontSize()*20);
        if (ImGui::DragScalarN("Cursor pivot",ImGuiDataType_Double,cursor,3,.01f))
            s.cursorPivot={cursor[0],cursor[1],cursor[2]};
        ImGui::EndDisabled();
    }
    auto view = cg::BeginViewport(
        "Scene", s.viewport,
        s.useGL ? ImTextureRef(static_cast<ImTextureID>(s.previewRenderer.Texture())) : ImTextureRef{},
        {0, 0}, theme);
    s.viewportOrigin = view.min;
    s.viewportSize = view.size;
    if (!s.useGL) {
        preview::DrawListPreview(*ImGui::GetWindowDrawList(),s.BuildSceneMeshes(),s.viewport.camera,
                                 view.min,view.size,s.scratch);
    }
    s.viewportSelectionPoints.resize(s.objects.size());
    s.viewport.selectionPoints=s.viewportSelectionPoints;
    s.viewport.selectionCanvas.selectionPath=s.viewportSelectionPath;
    cg::ViewportObjects(view, s.objects, s.viewport, s.objectSelection, s.revision, s.events, theme);
    if (!s.viewport.drag.active) {
        if (s.viewport.pivot==cg::Pivot::Cursor) s.viewport.pivotPosition=s.cursorPivot;
        else {
            cg::Vec3 sum{},low{},high{};int count=0;
            for (const auto &object:s.objects) if (s.objectSelection.Contains(object.id)) {
                const auto p=object.transform.translation;
                sum.x+=p.x;sum.y+=p.y;sum.z+=p.z;
                if (!count) low=high=p;
                else {low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};
                      high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};}
                if (object.geometry && s.viewport.pivot==cg::Pivot::Bounds) {
                    const auto basis=cg::OrientationBasis(cg::Orientation::Local,object.transform,{});
                    for (const auto &vertex:s.geometries.at(object.geometry).vertices) {
                        const double x=vertex.position[0]*object.transform.scale.x,
                                     y=vertex.position[1]*object.transform.scale.y,
                                     z=vertex.position[2]*object.transform.scale.z;
                        const cg::Vec3 point{p.x+basis.x.x*x+basis.y.x*y+basis.z.x*z,
                                             p.y+basis.x.y*x+basis.y.y*y+basis.z.y*z,
                                             p.z+basis.x.z*x+basis.y.z*y+basis.z.z*z};
                        low={std::min(low.x,point.x),std::min(low.y,point.y),std::min(low.z,point.z)};
                        high={std::max(high.x,point.x),std::max(high.y,point.y),std::max(high.z,point.z)};
                    }
                }
                ++count;
            }
            if (count) s.viewport.pivotPosition=s.viewport.pivot==cg::Pivot::Bounds ?
                cg::Vec3{(low.x+high.x)/2,(low.y+high.y)/2,(low.z+high.z)/2} :
                cg::Vec3{sum.x/count,sum.y/count,sum.z/count};
        }
    }
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
    cg::EndViewport();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Scene properties", {0, top}, ImGuiChildFlags_Borders);
    ImGui::BeginChild("Outliner", {0, top * .42f});
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
    auto object = std::find_if(s.objects.begin(), s.objects.end(),
                               [&](const auto &o) { return o.id == s.objectSelection.active; });
    if (object != s.objects.end() && ImGui::BeginTabBar("Inspector pages")) {
        if (ImGui::BeginTabItem("Transform")) {
            editor::PropertyView rows[] = {
                {ObjectPropertyId(s, object->id, 0), "Position X", "Transform", object->transform.translation.x, 0},
                {ObjectPropertyId(s, object->id, 1), "Position Y", "Transform", object->transform.translation.y, 0},
                {ObjectPropertyId(s, object->id, 2), "Position Z", "Transform", object->transform.translation.z, 0},
                {ObjectPropertyId(s, object->id, 3), "Rotation X (rad)", "Rotation", object->transform.rotation.x, 0},
                {ObjectPropertyId(s, object->id, 4), "Rotation Y (rad)", "Rotation", object->transform.rotation.y, 0},
                {ObjectPropertyId(s, object->id, 5), "Rotation Z (rad)", "Rotation", object->transform.rotation.z, 0},
                {ObjectPropertyId(s, object->id, 6), "Scale X", "Scale", object->transform.scale.x, 1},
                {ObjectPropertyId(s, object->id, 7), "Scale Y", "Scale", object->transform.scale.y, 1},
                {ObjectPropertyId(s, object->id, 8), "Scale Z", "Scale", object->transform.scale.z, 1}};
            for (auto &row:rows)
                row.flags=static_cast<editor::PropertyFlags>(s.propertyFlags[row.id]|
                    (row.value!=row.defaultValue?2u:0u)|(object->locked?16u:0u));
            for (int component=0;component<9;++component) {
                unsigned flags=static_cast<unsigned>(rows[component].flags);
                for (auto &target:s.objects) if (s.objectSelection.Contains(target.id)) {
                    if (TransformComponent(target.transform,component)!=rows[component].value) flags|=1u;
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
            editor::PropertyGrid("object", properties, s.objectProperties, s.events);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Components")) {
            s.inspectorComponents.clear();
            for (const auto &component:s.components) if (component.view.owner==object->id) s.inspectorComponents.push_back(component.view);
            const cg::ComponentTypeView componentTypes[]={{7801,"Mesh renderer"},{7802,"Wireframe override"}};
            cg::ComponentStack("Components",s.inspectorComponents,s.revision,s.events,
                {object->id,object->geometry ? std::span<const cg::ComponentTypeView>(componentTypes) : std::span<const cg::ComponentTypeView>{},object->locked});
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();
    ImGui::BeginChild("Assets", {side, 0}, ImGuiChildFlags_Borders);
    const char *assetPath[]={"All assets","Media"};
    editor::AssetBrowser("assets", Assets(s), s.assetState, s.selection, s.events, std::span(assetPath).first(s.assetPathDepth));
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Animation UV", {0, 0}, ImGuiChildFlags_Borders);
    editor::Transport(s.timeline.time, std::span(s.bindings).first(s.bindingCount), s.icons);
    if (ImGui::BeginTabBar("Animation editors")) {
        if (ImGui::BeginTabItem("Graph Editor",nullptr,s.animationPage==0?ImGuiTabItemFlags_SetSelected:0)) {
            editor::CurveEditor("graph", Curves(s), s.curve, s.keySelection, s.events, theme, {0, 0});
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Dope Sheet",nullptr,s.animationPage==1?ImGuiTabItemFlags_SetSelected:0)) {
            cg::DopeSheet("dope", Curves(s), s.curve, s.keySelection, s.events, theme, {0, 0});
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Animation strips",nullptr,s.animationPage==3?ImGuiTabItemFlags_SetSelected:0)) {
            cg::AnimationStrips("strips",s.animationStrips,s.revision,s.stripCanvas,s.stripDrag,s.events,theme,{0,0});
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("UV / Image",nullptr,s.animationPage==2?ImGuiTabItemFlags_SetSelected:0)) {
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
    editor::Transport(s.timeline.time, std::span(s.bindings).first(s.bindingCount), s.icons);
    editor::TimeRuler("ruler", s.timeline.time, s.curve.canvas, {}, s.revision, s.events, theme);
    editor::CurveEditor("core curve", Curves(s), s.curve, s.keySelection, s.events, theme, {0, 400});
    editor::StatusBar("Host-owned state / revision checked events", s.keySelection);
    s.ApplyEvents();
}
} // namespace imkit::gallery
