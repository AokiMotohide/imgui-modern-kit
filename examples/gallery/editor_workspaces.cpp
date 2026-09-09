#include "editor_workspaces.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace imkit::gallery {
namespace {
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
    s.queryCount = s.queriedClips = 0;
}
} // namespace
void EditorWorkspaces::Dataset(bool big) {
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
        track.label = t % 3 == 0 ? "V  Picture" : t % 3 == 1 ? "A  Sound" : "T  Caption";
        track.kind = t % 3 == 0   ? video::TrackKind::Video
                     : t % 3 == 1 ? video::TrackKind::Audio
                                  : video::TrackKind::Caption;
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
            clip.label = t % 3 == 0   ? "Studio / Main take"
                         : t % 3 == 1 ? "Ambient / stereo"
                                      : "A quiet afternoon";
            clip.start = editor::FromSeconds(i * 4. + (t % 2) * .5);
            clip.duration = editor::FromSeconds(3.5);
            clip.sourceIn = editor::TicksPerSecond * 5;
            clip.proxy = i % 7 == 0;
            if (track.kind == video::TrackKind::Audio) clip.audioBuckets=audio;
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
    RebuildKeyIndex();
    if (!clips.empty())
        clips.front().keys = std::span<const editor::Keyframe>(keys).first(6);
    selection.Clear();
    selection.Set(1000);
    timeline.drag.active = false;
    mixerTrack = audioStrips.empty() ? 0 : audioStrips.front().id;
    mixerState.drag.active = false;
    curve.drag.active = false;
    curve.companionCount=0;
    for (auto &drag:curveCompanions) drag.active=false;
    RebuildTrackLayout();
    ++revision;
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
    if (!clips.empty()) clips.front().keys=std::span<const editor::Keyframe>(keys).first((std::min)(keys.size(),std::size_t{6}));
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
        objects[i].label = names[i];
        objects[i].parent = i ? 1000000 : 0;
        objects[i].depth = i ? 1 : 0;
        objects[i].hasChildren = i == 0;
        objects[i].transform.translation = {static_cast<double>(i - 1) * 1.5, 0, 0};
    }
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
    preview::Mesh mesh{objects[1].id, cubeVertices, cubeIndices, objects[1].transform};
    mesh.wire=viewport.shading==cg::Shading::Wireframe;
    if (viewport.drag.active) {
        auto v = viewport.drag.draft.proposed;
        if (viewport.drag.draft.kind == editor::EditKind::Translate)
            mesh.transform.translation = {v.x, v.y, v.z};
        if (viewport.drag.draft.kind == editor::EditKind::Rotate)
            mesh.transform.rotation = {v.x, v.y, v.z};
        if (viewport.drag.draft.kind == editor::EditKind::Scale)
            mesh.transform.scale = {v.x, v.y, v.z};
    }
    previewRenderer.Render({&mesh, 1}, viewport.camera);
}
void EditorWorkspaces::ApplyEvents() {
    bool changed = false;
    for (const auto &e : events.Events()) {
        if (e.phase==editor::Phase::Begin && e.kind==editor::EditKind::Property)
            propertyGestureSelection.assign(objectSelection.storage.begin(),objectSelection.storage.begin()+objectSelection.count);
        if (e.phase != editor::Phase::Commit || e.revision != revision)
            continue;
        ++commits;
        for (auto &strip:animationStrips) if (strip.id==e.target) {
            if (e.kind==editor::EditKind::StripSettings) {
                strip.scale=e.proposed.x;strip.repeat=e.proposed.y;strip.blend=e.proposed.z;
                strip.muted=(e.proposed.offset&1)!=0;strip.locked=(e.proposed.offset&2)!=0;changed=true;
            } else if ((e.kind==editor::EditKind::Move || e.kind==editor::EditKind::TrimStart ||
                        e.kind==editor::EditKind::TrimEnd) && !strip.locked) {
                strip.range={e.proposed.first,e.proposed.last};changed=true;
            }
        }
        if (e.kind == editor::EditKind::Select)
            continue;
        if (e.kind == editor::EditKind::Marker && markerCount < markers.size()) {
            markers[markerCount++] = {nextId++, e.proposed.first, "Marker"};
            changed = true;
        }
        if (e.kind == editor::EditKind::Rename)
            renamedLabels[e.target] = e.proposedText.data();
        auto clip = std::find_if(clips.begin(), clips.end(), [&](const auto &c) { return c.id == e.target; });
        if (clip != clips.end()) {
            if (e.kind == editor::EditKind::Rename) {
                clip->label = renamedLabels[e.target].c_str();
                changed = true;
            }
            if (e.kind == editor::EditKind::Split) {
                auto split = video::SplitClip(*clip, e.proposed.first, {});
                if (split.valid) {
                    auto right = *clip;
                    right.id = nextId++;
                    right.start = split.right.start;
                    right.duration = split.right.duration;
                    right.sourceIn = split.right.sourceIn;
                    clip->duration = split.left.duration;
                    clips.push_back(right);
                    changed = true;
                }
            } else if (e.kind == editor::EditKind::Duplicate) {
                auto copy = *clip;
                copy.id = nextId++;
                copy.start = e.proposed.first;
                clips.push_back(copy);
                changed = true;
            } else if (e.kind == editor::EditKind::Move || e.kind == editor::EditKind::TrimStart ||
                       e.kind == editor::EditKind::TrimEnd || e.kind == editor::EditKind::Slip ||
                       e.kind == editor::EditKind::Ripple || e.kind == editor::EditKind::Roll ||
                       e.kind == editor::EditKind::Slide) {
                auto track = clip->track;
                auto oldEnd = clip->start + clip->duration;
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
                if (e.kind == editor::EditKind::Reparent) {
                    bool cycle = false;
                    auto parent = e.proposed.parent;
                    for (int depth = 0; parent && depth < 4; ++depth) {
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
            auto existing=std::find_if(keys.begin(),keys.end(),[&](const auto &key){return key.channel==e.target && key.tick==e.proposed.first;});
            if (existing==keys.end()) {keys.push_back({nextId++,e.target,e.proposed.first,e.proposed.x});changed=true;}
        }
        for (auto &key : keys)
            if (key.id == e.target) {
                if (e.kind==editor::EditKind::Navigate) {timeline.time.playhead=key.tick;keySelection.Set(key.id);}
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
            if (v.id == e.target && e.kind == editor::EditKind::Translate) {
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
        bool isProperty=e.target>=700001 && e.target<=700004;
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
        if (e.target >= 700001 && e.target <= 700004 &&
            (e.kind == editor::EditKind::Property || e.kind == editor::EditKind::Reset)) {
            clipPropertyValues[e.target - 700001] = e.proposed.x;
            changed = true;
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
    events.Clear();
}
void VideoWorkspace(EditorWorkspaces &s, const Theme &theme, ImTextureRef texture) {
    s.Initialize();
    Options(s);
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
    video::Monitor("Program", ImTextureRef(static_cast<ImTextureID>(s.previewRenderer.Texture())),
                   {width * .48f, monitorHeight}, s.timeline.time,
                   {true, true, true, true, s.japanese ? "プログラム" : "Program", {.5f,.5f}, true}, theme);
    editor::Transport(s.timeline.time, std::span(s.bindings).first(s.bindingCount), s.icons);
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Clip Inspector", {0, top}, ImGuiChildFlags_Borders);
    ImGui::SeparatorText("Clip Inspector");
    editor::PropertyView props[] = {{700001, "Opacity", "Video", 1, 1, editor::PropertyFlags::Animated},
                                    {700002, "Scale", "Transform", 1, 1},
                                    {700003, "Position X", "Transform", 0, 0},
                                    {700004, "Speed", "Retiming", 1, 1}};
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
    editor::PropertyGrid("clip", properties, s.videoProperties, s.events);
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
void CGWorkspace(EditorWorkspaces &s, const Theme &theme, ImTextureRef texture) {
    s.Initialize();
    Options(s);
    float width = ImGui::GetContentRegionAvail().x, side = (s.narrow ? 180 : 300)*ImGui::GetFontSize()/14,
          top = ImGui::GetContentRegionAvail().y * .6f;
    ImGui::BeginChild("View stack", {width - side - 10, top}, ImGuiChildFlags_Borders);
    ImGui::Checkbox("OpenGL preview", &s.useGL);
    auto view = cg::BeginViewport(
        "Scene", s.viewport,
        s.useGL ? ImTextureRef(static_cast<ImTextureID>(s.previewRenderer.Texture())) : ImTextureRef{},
        {0, 0}, theme);
    s.viewportOrigin = view.min;
    s.viewportSize = view.size;
    if (!s.useGL) {
        preview::Mesh mesh{s.objects[1].id, s.cubeVertices, s.cubeIndices, s.objects[1].transform};
        mesh.wire=s.viewport.shading==cg::Shading::Wireframe;
        preview::DrawListPreview(*ImGui::GetWindowDrawList(), {&mesh, 1}, s.viewport.camera, view.min,
                                 view.size, s.scratch);
    }
    cg::ViewportObjects(view, s.objects, s.viewport, s.objectSelection, s.revision, s.events, theme);
    for (const auto &o : s.objects)
        if (o.id == s.objectSelection.active) {
            s.viewport.pivotPosition=o.transform.translation;
            cg::TransformGizmo(view, o, s.viewport, s.revision, s.events, theme);
        }
    cg::EndViewport();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Scene properties", {0, top}, ImGuiChildFlags_Borders);
    ImGui::BeginChild("Outliner", {0, top * .42f});
    cg::SceneProvider scene{&s, s.revision, 4, [](void *u, int first, int count, std::string_view) {
                                auto &s = *static_cast<EditorWorkspaces *>(u);
                                return std::span<const cg::ObjectView>(s.objects).subspan(
                                    first, (std::min)(count, 4 - first));
                            }};
    cg::Outliner("Hierarchy", scene, s.outliner, s.objectSelection, s.events);
    ImGui::EndChild();
    ImGui::SeparatorText("Inspector");
    auto object = std::find_if(s.objects.begin(), s.objects.end(),
                               [&](const auto &o) { return o.id == s.objectSelection.active; });
    if (object != s.objects.end()) {
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
                s.edges[i] = {static_cast<editor::StableId>(i + 1), s.uv[i].uv, s.uv[(i + 1) % 4].uv};
            cg::UVProvider p{&s, s.revision,
                             [](void *u, editor::Rect) {
                                 return std::span<const cg::UVVertex>(static_cast<EditorWorkspaces *>(u)->uv);
                             },
                             [](void *u, editor::Rect) {
                                 return std::span<const cg::UVEdge>(
                                     static_cast<EditorWorkspaces *>(u)->edges);
                             }};
            cg::UVEditor("uv", p, texture, s.uvState, s.uvSelection, s.events, theme, {0, 0});
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();
    s.ApplyEvents();
}
void CoreWorkspace(EditorWorkspaces &s, const Theme &theme) {
    s.Initialize();
    editor::Transport(s.timeline.time, std::span(s.bindings).first(s.bindingCount), s.icons);
    editor::TimeRuler("ruler", s.timeline.time, s.curve.canvas, {}, s.revision, s.events, theme);
    editor::CurveEditor("core curve", Curves(s), s.curve, s.keySelection, s.events, theme, {0, 400});
    editor::StatusBar("Host-owned state / revision checked events", s.keySelection);
    s.ApplyEvents();
}
} // namespace imkit::gallery
