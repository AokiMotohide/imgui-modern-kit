#include "editor_workspaces_internal.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
namespace imkit::gallery {
namespace detail {
void PropertyLanguage(editor::PropertyState &state,bool japanese) {
    state.labels={};if (!japanese) return;
    state.labels.copy="値をコピー";state.labels.paste="値を貼り付け";state.labels.mixed="複数の値";state.labels.moveUp="上へ移動";state.labels.moveDown="下へ移動";
    auto &l=state.labels;l.search="検索";l.property="項目";l.value="値";l.key="キー";
    l.modifiedSuffix=" (変更済み)";l.overrideSuffix=" (上書き)";l.lockedSuffix=" [ロック]";
    l.favorite="お気に入り";l.locked="ロック";l.overrideValue="上書き";l.reset="既定値へ戻す";
    l.previousKey="前のキー";l.nextKey="次のキー";l.addKey="キーを追加";l.removeKey="キーを削除";
}
void TransportLanguage(editor::TimeState &time,bool japanese) {
    time.labels={};time.rulerLabels={};
    if (!japanese) return;
    auto &l=time.labels;l.play="再生";l.pause="一時停止";l.stop="停止";
    l.previousFrame="前のフレーム";l.nextFrame="次のフレーム";
    l.in="イン";l.out="アウト";l.loop="ループ";
    l.goToStart="先頭へ";l.goToEnd="末尾へ";l.clearInOut="イン／アウトを解除";
    l.reverse="逆再生";l.forward="順再生";
    time.rulerLabels={"作業範囲","イン／アウト","開始","終了","マーカーを追加","マーカーを削除","マーカー"};
}
void ApplyGizmoPreview(preview::Mesh &mesh,const cg::ViewportState &viewport) {
    cg::ObjectView object;object.id=mesh.id;object.transform=mesh.transform;
    mesh.transform=cg::PreviewTransform(object,viewport);
}
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
    s.assetState.icons=s.icons;s.assetState.labels={};
    if (s.japanese) {
        auto &labels=s.assetState.labels;
        labels.search="検索";labels.grid="グリッド";labels.list="一覧";labels.tag="タグ";
        labels.statuses={"すべての状態","準備完了","読み込み中","プロキシ","見つかりません","エラー"};
        labels.rename="名前変更";labels.duplicate="複製";labels.remove="削除";
    }
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
void CurveLanguage(editor::CurveState &state,bool japanese) {
    state.labels={};
    if (japanese) {
        auto &labels=state.labels;
        labels.add="再生位置にキーを追加";labels.previous="前のキー";labels.next="次のキー";labels.remove="選択キーを削除";
        labels.fit="全チャンネルを表示";labels.ghost="他チャンネルを薄く表示";labels.snap="フレームへ吸着";
        labels.scale="キーの時間を拡大縮小";labels.box="矩形選択";labels.lasso="投げ縄選択";
        labels.extrapolation="外挿";labels.interpolation="補間";labels.handles="ハンドルモード";
        labels.extrapolations={"一定","直線","繰り返し"};labels.interpolations={"一定","直線","ベジェ"};
        labels.handleModes={"自動","自動・制限付き","ベクトル","整列","自由"};
    }
}
editor::CurveProvider Curves(EditorWorkspaces &s) {
    CurveLanguage(s.curve,s.japanese);
    s.curve.bindings=std::span(s.bindings).first(s.bindingCount);
    s.curve.rate=s.timeline.time.rate;
    s.curve.time=s.timeline.time.playhead;
    if(s.propertyCurveChannel) {
        s.curve.activeChannel=s.propertyCurveChannel;
        auto &channel=s.propertyKeys[s.propertyCurveChannel];
        editor::Rect bounds{{0,-1},{1,0}};
        if(!channel.empty()) {
            bounds={{editor::Seconds(channel.front().tick),-channel.front().value},{editor::Seconds(channel.back().tick),-channel.front().value}};
            for(const auto &key:channel) {bounds.min.y=std::min(bounds.min.y,-key.value);bounds.max.y=std::max(bounds.max.y,-key.value);}
        }
        return {&s,s.revision,[](void *u,editor::CurveQuery query) {
            auto &s=*static_cast<EditorWorkspaces*>(u);const auto &keys=s.propertyKeys[s.propertyCurveChannel];
            auto first=std::lower_bound(keys.begin(),keys.end(),query.time.first,[](const auto &k,auto t){return k.tick<t;});
            if(first!=keys.begin()) --first;
            auto last=std::upper_bound(first,keys.end(),query.time.last,[](auto t,const auto &k){return t<k.tick;});
            if(last!=keys.end()) ++last;
            return std::span<const editor::Keyframe>(first,last);
        },[](void *u,editor::StableId id,editor::Tick tick,editor::Extrapolation mode) {
            auto &s=*static_cast<EditorWorkspaces*>(u);return editor::Evaluate(s.propertyKeys[id],tick,mode);
        },bounds,[](void *u,std::span<const editor::StableId> ids) {
            auto &s=*static_cast<EditorWorkspaces*>(u);s.selectedCurveKeys.clear();
            for(const auto &k:s.propertyKeys[s.propertyCurveChannel]) if(std::find(ids.begin(),ids.end(),k.id)!=ids.end()) s.selectedCurveKeys.push_back(k);
            return std::span<const editor::Keyframe>(s.selectedCurveKeys);
        },[](void *u,editor::StableId id,editor::Tick tick,bool next)->const editor::Keyframe* {
            auto &s=*static_cast<EditorWorkspaces*>(u);auto &keys=s.propertyKeys[id];
            auto key=std::lower_bound(keys.begin(),keys.end(),tick,[](const auto &k,auto t){return k.tick<t;});
            if(next) {if(key!=keys.end() && key->tick==tick) ++key;return key==keys.end()?nullptr:&*key;}
            return key==keys.begin()?nullptr:&*--key;
        },[](void *u,editor::Rect bounds) {
            auto &s=*static_cast<EditorWorkspaces*>(u);s.curveSelectionPoints.clear();
            for(const auto &k:s.propertyKeys[s.propertyCurveChannel]) if(editor::Seconds(k.tick)>=bounds.min.x && editor::Seconds(k.tick)<=bounds.max.x)
                s.curveSelectionPoints.push_back({k.id,{editor::Seconds(k.tick),-k.value},k.locked});
            return std::span<const editor::SelectablePoint>(s.curveSelectionPoints);
        }};
    }
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
editor::CurveProvider ColorCurves(EditorWorkspaces &s) {
    CurveLanguage(s.colorCurve,s.japanese);
    s.colorCurve.labels.add=s.japanese?"入力位置にキーを追加":"Add key at input";
    s.colorCurve.icons=s.icons;
    s.colorCurve.bindings=std::span(s.bindings).first(s.bindingCount);
    return {&s,s.revision,[](void *u,editor::CurveQuery q) {
        auto &s=*static_cast<EditorWorkspaces*>(u);
        auto &keys=s.colorCurveKeys[s.colorCurveChannel];
        auto first=std::lower_bound(keys.begin(),keys.end(),q.time.first,[](const auto &k,auto t){return k.tick<t;});
        auto last=std::upper_bound(first,keys.end(),q.time.last,[](auto t,const auto &k){return t<k.tick;});
        for(int i=0;i<2 && first!=keys.begin();++i) --first;
        for(int i=0;i<2 && last!=keys.end();++i) ++last;
        s.colorCurvePreview.resize(static_cast<std::size_t>(last-first));
        s.colorCurve.previewKeys=s.colorCurvePreview;
        return std::span<const editor::Keyframe>(first,last);
    },[](void *u,editor::StableId id,editor::Tick tick,editor::Extrapolation mode) {
        auto &s=*static_cast<EditorWorkspaces*>(u);
        for(int c=0;c<3;++c) if(s.colorCurveChannels[c]==id) return editor::Evaluate(s.colorCurveKeys[c],tick,mode);
        return 0.;
    },editor::Rect{{0,-1},{1,0}},[](void *u,std::span<const editor::StableId> ids) {
        auto &s=*static_cast<EditorWorkspaces*>(u);s.colorCurveSelected.clear();
        for(const auto &k:s.colorCurveKeys[s.colorCurveChannel])
            if(std::find(ids.begin(),ids.end(),k.id)!=ids.end()) s.colorCurveSelected.push_back(k);
        return std::span<const editor::Keyframe>(s.colorCurveSelected);
    },[](void *u,editor::StableId id,editor::Tick tick,bool next)->const editor::Keyframe* {
        auto &s=*static_cast<EditorWorkspaces*>(u);
        for(int c=0;c<3;++c) if(s.colorCurveChannels[c]==id) {
            const auto &keys=s.colorCurveKeys[c];
            auto k=std::lower_bound(keys.begin(),keys.end(),tick,[](const auto &k,auto t){return k.tick<t;});
            if(next) {if(k!=keys.end() && k->tick==tick) ++k;return k==keys.end()?nullptr:&*k;}
            return k==keys.begin()?nullptr:&*--k;
        }
        return nullptr;
    }};
}
void PaneDivider(const char *id,float &width,float height,float total) {
    ImGui::SameLine(0,2);
    ImGui::BeginChild(id,{6,height},ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar);
    editor::Splitter("split",width,total,true,140);
    ImGui::EndChild();ImGui::SameLine(0,2);
}
void Divider(const char *id,float &ratio,float total) {
    ImGui::InvisibleButton(id,{std::max(1.f,ImGui::GetContentRegionAvail().x),6});
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive() && total>0) ratio=std::clamp(ratio+ImGui::GetIO().MouseDelta.y/total,.25f,.72f);
    const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddLine({a.x,(a.y+b.y)*.5f},{b.x,(a.y+b.y)*.5f},ImGui::GetColorU32(ImGui::IsItemActive()?ImGuiCol_SeparatorActive:ImGuiCol_Separator));
}
void Options(EditorWorkspaces &s) {
    if(ImGui::SmallButton(s.japanese ? "サンプル設定" : "Sample settings")) ImGui::OpenPopup("sample-settings");
    if(ImGui::BeginPopup("sample-settings")) {
        bool large=s.large;if(imkit::Checkbox("100k dataset",&large)) s.Dataset(large);
        imkit::Checkbox("日本語",&s.japanese);imkit::Checkbox("Narrow panes",&s.narrow);
        ImGui::Text("%zu queries / %zu clips / %zu commits",s.queryCount,s.queriedClips,s.commits);
        ImGui::EndPopup();
    }
    const bool focused=ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    const auto bindings=std::span(s.bindings).first(s.bindingCount);
    ImGui::SameLine();
    ImGui::BeginDisabled(s.historyCursor==0);
    if (ImGui::SmallButton(s.japanese ? "元に戻す" : "Undo")) s.Undo();
    ImGui::EndDisabled();ImGui::SameLine();
    ImGui::BeginDisabled(s.historyCursor==s.history.size());
    if (ImGui::SmallButton(s.japanese ? "やり直す" : "Redo")) s.Undo(true);
    ImGui::EndDisabled();
    if (editor::CommandPressed(editor::Command::Undo,bindings,focused)) s.Undo();
    if (editor::CommandPressed(editor::Command::Redo,bindings,focused)) s.Undo(true);
    ImGui::SameLine();
    if (ImGui::SmallButton(s.japanese ? "ショートカット" : "Shortcuts")) ImGui::OpenPopup("editor-shortcuts");
    if (ImGui::BeginPopup("editor-shortcuts")) {
        const char *presets[]={"CapCut","Premiere","Blender"};
        if (imkit::Combo(s.japanese ? "基本設定" : "Preset",&s.shortcutPreset,presets,3))
            s.bindingCount=editor::MakeBindings(static_cast<editor::ShortcutPreset>(s.shortcutPreset),s.bindings);
        ImGui::TextUnformatted(s.japanese ? "キーと修飾キーを変更できます。同じ割当は赤で表示します。" : "Choose a key and modifiers. Conflicting bindings are red.");
        const char *names[]={"Play / Pause","Stop","Previous frame","Next frame","Set In","Set Out","Loop","Split","Delete","Duplicate","Select all","Fit","Add key","Previous key","Next key","Undo","Redo","Play reverse","Play forward","Pause","Select tool","Razor tool","Ripple tool","Roll tool","Slip tool","Slide tool","Hand tool","Go to start","Go to end","Clear In / Out","Insert source","Overwrite source","Append source","Copy clips","Cut clips","Paste clips"};
        ImGui::BeginChild("binding-list",{560,360});
        for (std::size_t i=0;i<s.bindingCount;++i) {
            auto &b=s.bindings[i];ImGui::PushID(static_cast<int>(i));
            ImGui::TextUnformatted(names[static_cast<int>(b.command)]);
            const auto key=static_cast<ImGuiKey>(b.chord & ~ImGuiMod_Mask_);
            ImGui::SetNextItemWidth(180);
            if (imkit::BeginCombo("##key",key==ImGuiKey_None ? "None" : ImGui::GetKeyName(key))) {
                if (ImGui::Selectable("None",!b.chord)) b.chord=0;
                for (int k=ImGuiKey_NamedKey_BEGIN;k<=ImGuiKey_F24;++k)
                    if (ImGui::Selectable(ImGui::GetKeyName(static_cast<ImGuiKey>(k)),key==k)) b.chord=(b.chord & ImGuiMod_Mask_) | k;
                imkit::EndCombo();
            }
            for (const auto modifier: {ImGuiMod_Ctrl,ImGuiMod_Shift,ImGuiMod_Alt,ImGuiMod_Super}) {
                ImGui::SameLine();bool enabled=(b.chord & modifier)!=0;
                const char *label=modifier==ImGuiMod_Ctrl ? "Ctrl" : modifier==ImGuiMod_Shift ? "Shift" : modifier==ImGuiMod_Alt ? "Alt" : "Super";
                if (imkit::Checkbox(label,&enabled) && (b.chord & ~ImGuiMod_Mask_))
                    b.chord=enabled ? b.chord | modifier : b.chord & ~modifier;
            }
            const bool conflict=b.chord && std::any_of(s.bindings.begin(),s.bindings.begin()+s.bindingCount,[&](const auto &other){return &other!=&b && other.chord==b.chord;});
            if (conflict) ImGui::TextColored({1,.4f,.3f,1},"%s",s.japanese ? "同じキー割当があります" : "Shared key binding");
            ImGui::PopID();
        }
        ImGui::EndChild();ImGui::EndPopup();
    }
    s.queryCount = s.queriedClips = s.queriedKeys = s.queriedTracks = 0;
}
} // namespace detail
using namespace detail;
void EditorWorkspaces::Dataset(bool big) {
    trackSelection.Clear();clipboard.clear();timeline.boxSelecting=false;
    history.clear();historyCursor=0;
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
    const auto exampleLink=nextId++,exampleGroup=nextId++;
    for (int t = 0; t < trackCount; ++t) {
        video::TrackView track;
        track.id = t + 1;
        constexpr const char *trackLabels[]={"Picture","Sound","Caption","Effect","Adjustment","Group"};
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
            clip.transitionInKind=clip.transitionOutKind=video::TransitionKind::Fade;
            clip.id = 1000 + t * clipsPerTrack + i;
            clip.track = track.id;
            constexpr const char *clipLabels[]={"Studio / Main take","Ambient / stereo","A quiet afternoon",
                "Blur / effect range","Exposure / adjustment range","Sequence / group range"};
            clip.label=clipLabels[t%6];
            clip.start = editor::FromSeconds(i * 4. + (t % 2) * .5);
            clip.duration = editor::FromSeconds(3.5);
            if (!large && i==2 && t<4) {
                clip.start=editor::FromSeconds(8);
                if (t<2) clip.linked=exampleLink;
                if (t>0) clip.group=exampleGroup;
            }
            clip.sourceIn = editor::TicksPerSecond * 5;
            if (t==0 && i==1) {
                clip.transitionIn=editor::FromSeconds(.4);clip.transitionOut=editor::FromSeconds(.6);
                clip.transitionOutKind=video::TransitionKind::Fade;
            }
            clip.proxy = i % 7 == 0;
            if (track.kind == video::TrackKind::Audio) {clip.audioSource=1+i%4;clip.audioChannels=2;clip.sourceIn=editor::FromSeconds(i%8);}
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
    RebuildClipIndex();
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
std::span<const video::ClipView> EditorWorkspaces::QuerySelectedClips(std::span<const editor::StableId> ids) {
    std::size_t count=0;
    const auto append=[&](const video::ClipView &clip) {
        if (std::any_of(selectedClips.begin(),selectedClips.begin()+count,
            [&](const auto &selected){return selected.id==clip.id;})) return true;
        if (count==selectedClips.size()) return false;
        selectedClips[count]=clip;
        const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip.track;});
        selectedClips[count++].locked=clip.locked || track==tracks.end() || track->locked;
        return true;
    };
    for (const auto id:ids) {
        const auto clip=std::find_if(clips.begin(),clips.end(),[&](const auto &c){return c.id==id;});
        if (clip==clips.end() || !append(*clip)) return {};
    }
    // Link and group IDs identify sets; traverse their union transitively on edit start only.
    for (std::size_t i=0;i<count;++i) {
        const auto member=selectedClips[i];
        if (!member.linked && !member.group) continue;
        for (const auto &clip:clips)
            if ((member.linked && member.linked==clip.linked) || (member.group && member.group==clip.group))
                if (!append(clip)) return {};
    }
    return std::span<const video::ClipView>(selectedClips).first(count);
}
void EditorWorkspaces::RebuildClipIndex() {
    std::sort(clips.begin(),clips.end(),[](const auto &a,const auto &b) {
        return a.track!=b.track ? a.track<b.track : a.start<b.start;
    });
    for(std::size_t i=0;i<clips.size();++i) {
        auto &clip=clips[i];const auto duration=std::max(editor::Tick{0},clip.duration);
        clip.fades.inDuration=std::clamp(clip.fades.inDuration,editor::Tick{0},duration);
        clip.fades.outDuration=std::clamp(clip.fades.outDuration,editor::Tick{0},duration-clip.fades.inDuration);
        auto &cut=clip.outgoingTransition;
        if(cut.duration>0) {
            if(i+1==clips.size() || clips[i+1].id!=cut.right) cut={};
            else {
                auto left=clip,right=clips[i+1];left.locked=right.locked=false;
                const auto limit=video::CenteredTransitionLimit(left,right,{},{});
                cut.duration=std::clamp(cut.duration,editor::Tick{0},limit);cut.left=clip.id;
                if(!cut.duration) cut={};
            }
        }
    }
    clipById.clear();clipById.reserve(clips.size());
    for (std::size_t i=0;i<clips.size();++i) clipById.emplace_back(clips[i].id,i);
    std::sort(clipById.begin(),clipById.end());
    clipTreeBase=1;
    while (clipTreeBase<clips.size()) clipTreeBase*=2;
    clipEndTree.assign(clipTreeBase*2,std::numeric_limits<editor::Tick>::min());
    for (std::size_t i=0;i<clips.size();++i) {
        const auto &clip=clips[i];
        const auto duration=std::max(editor::Tick{0},clip.duration);
        clipEndTree[clipTreeBase+i]=clip.start>std::numeric_limits<editor::Tick>::max()-duration
            ? std::numeric_limits<editor::Tick>::max() : clip.start+duration;
    }
    for (auto i=clipTreeBase;i-->1;)
        clipEndTree[i]=std::max(clipEndTree[i*2],clipEndTree[i*2+1]);
    visibleClips.reserve(clips.size());
}
std::span<const video::ClipView> EditorWorkspaces::QueryClips(editor::StableId track,editor::Range range) {
    ++queryCount;visibleClips.clear();clipQueryVisits=0;
    if (clipEndTree.empty() || range.last<range.first) return visibleClips;
    const auto first=std::lower_bound(clips.begin(),clips.end(),track,
        [](const auto &clip,auto id){return clip.track<id;});
    const auto end=std::upper_bound(first,clips.end(),track,
        [](auto id,const auto &clip){return id<clip.track;});
    const auto last=std::upper_bound(first,end,range.last,
        [](auto tick,const auto &clip){return tick<clip.start;});
    const auto lo=static_cast<std::size_t>(first-clips.begin()),hi=static_cast<std::size_t>(last-clips.begin());
    const auto visit=[&](auto &&self,std::size_t node,std::size_t begin,std::size_t end)->void {
        ++clipQueryVisits;
        if (end<=lo || begin>=hi || clipEndTree[node]<range.first) return;
        if (end-begin==1) {visibleClips.push_back(clips[begin]);return;}
        const auto middle=begin+(end-begin)/2;
        self(self,node*2,begin,middle);self(self,node*2+1,middle,end);
    };
    visit(visit,1,0,clipTreeBase);
    queriedClips+=visibleClips.size();
    for (const auto &clip:visibleClips) queriedKeys+=clip.keys.size();
    return visibleClips;
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
        clip.keys={};clip.keyEvaluation={};
        if (!clip.keyChannel) continue;
        const auto range=std::lower_bound(keyChannels.begin(),keyChannels.end(),clip.keyChannel,
            [&](const auto &range,auto id){return keys[range.first].channel<id;});
        if (range==keyChannels.end() || keys[range->first].channel!=clip.keyChannel) continue;
        auto channel=std::span<const editor::Keyframe>(keys).subspan(range->first,range->second-range->first);
        clip.keyEvaluation=channel;
        auto first=std::lower_bound(channel.begin(),channel.end(),editor::Tick{0},[](const auto &key,auto tick){return key.tick<tick;});
        auto last=std::upper_bound(first,channel.end(),clip.duration,[](auto tick,const auto &key){return tick<key.tick;});
        clip.keys={first,last};
    }
}
std::span<const editor::Keyframe> EditorWorkspaces::QueryKeys(editor::CurveQuery query) {
    ++queryCount;
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
    queriedKeys+=visibleKeys.size();
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
        objects[i].kind=i==0 ? cg::ObjectKind::Collection : i==1 ? cg::ObjectKind::Mesh : i==2 ? cg::ObjectKind::Light : cg::ObjectKind::Camera;
        objects[i].parent = i ? 1000000 : 0;
        objects[i].depth = i ? 1 : 0;
        objects[i].hasChildren = i == 0;
        objects[i].transform.translation = {static_cast<double>(i - 1) * 1.5, 0, 0};
    }
    objects[1].geometry=nextId++;
    geometries[objects[1].geometry]={{cubeVertices.begin(),cubeVertices.end()},{cubeIndices.begin(),cubeIndices.end()}};
    components.push_back({{nextId++,objects[1].id,"Mesh renderer","Draw the host geometry using viewport shading.",true},false});
    components.push_back({{nextId++,objects[1].id,"Wireframe override","Override preceding renderer shading with wireframe.",false},true});
    for (int i : {2,3}) {
        auto &geometry=geometries[objects[i].geometry=nextId++];
        geometry.vertices.resize(24);geometry.indices.resize(i==2 ? 24 : 36);
        if (i==2) preview::LightPrimitive(geometry.vertices,geometry.indices);
        else preview::CameraPrimitive(geometry.vertices,geometry.indices);
        components.push_back({{nextId++,objects[i].id,"Helper renderer","Host-owned camera/light preview mesh.",true},false});
    }
    cameraObject=objects[3].id;
    objects[3].transform.translation={3,2,4};objects[3].transform.rotation={std::atan2(2.,5.),std::atan2(-3.,-4.),0};
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
    sourceTime.inOut={0,editor::FromSeconds(3)};
    video::BuildAudioBuckets(pcm, 1, 0, audio);
    std::vector<float> fixturePCM(65536*2);
    for(int source=0;source<4;++source) {
        for(int i=0;i<65536;++i) for(int channel=0;channel<2;++channel) {
            const double time=60.*i/65536;
            const double envelope=source==0 ? 0 : source==1 ? (std::fmod(time,1.3)<.035 ? .95 : .008) :
                source==2 ? .35+.12*std::sin(time*2) : .08+.7*std::abs(std::sin(time*3+channel));
            fixturePCM[i*2+channel]=float(envelope*std::sin(i*.73+channel)*(channel==0?1:.55));
        }
        for(int channel=0;channel<2;++channel) {
            sourceAudio[source][channel].resize(4096);
            video::BuildAudioBuckets(fixturePCM,2,channel,sourceAudio[source][channel]);
        }
    }
    video::UpdateMeter(meter, pcm, 0);
    for(int c=0;c<3;++c) {
        colorCurveChannels[c]=nextId++;
        for(int i=0;i<3;++i) {
            editor::Keyframe key{nextId++,colorCurveChannels[c],editor::FromSeconds(i*.5),i*.5};
            key.interpolation=editor::Interpolation::Linear;colorCurveKeys[c].push_back(key);
        }
    }
    colorCurve.companionDrags=colorCurveCompanions;
    colorCurve.activeChannel=colorCurveChannels[0];colorCurve.fitRequested=true;
    colorCurve.time=editor::FromSeconds(.5);
    for(int i=0;i<3;++i) customProperties[i]={nextId++,i==0?"Parameter A":i==1?"Parameter B":"Parameter C","Custom array",double(i),double(i),{},true};
    RebuildColorScopes();
}
void EditorWorkspaces::RebuildColorScopes() {
    const video::ColorCurveSet curves{colorCurveKeys[0],colorCurveKeys[1],colorCurveKeys[2]};
    for(int y=0;y<64;++y) for(int x=0;x<64;++x)
        pixels[y*64+x]=video::ApplyColorCurves({x/63.f,y/63.f,(x+y)/126.f,1},curves);
    video::BuildScopes(pixels,64,64,{red,green,blue,luma,scopeWave,scopeVector,scopeRGB[0],scopeRGB[1],scopeRGB[2]});
}

void EditorWorkspaces::SyncSceneCamera() {
    const auto object=std::find_if(objects.begin(),objects.end(),[&](const auto &o){return o.id==cameraObject;});
    if (object==objects.end()) {viewport.cameraView=nullptr;return;}
    preview::Mesh pose;pose.id=object->id;pose.transform=object->transform;ApplyGizmoPreview(pose,viewport);
    sceneCamera=cg::CameraFromTransform(pose.transform,sceneCamera);viewport.cameraView=&sceneCamera;
    if (viewport.camera.projection==cg::Projection::Camera) {viewport.camera=sceneCamera;viewport.camera.projection=cg::Projection::Camera;}
}
void EditorWorkspaces::RenderPreview() {
    if (initialized) SyncSceneCamera();
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
        if (!previewHelpers && (objects[i].kind==cg::ObjectKind::Camera || objects[i].kind==cg::ObjectKind::Light)) continue;
        const auto data=geometries.find(objects[i].geometry);
        if (data==geometries.end()) continue;
        preview::Mesh mesh{objects[i].id,data->second.vertices,data->second.indices,objects[i].transform};
        mesh.wire=viewport.shading==cg::Shading::Wireframe;
        bool render=false;
        for (const auto &component:components) if (component.view.owner==objects[i].id) {
            if (!component.wireOverride) {render=component.view.enabled;mesh.wire=viewport.shading==cg::Shading::Wireframe;}
            else if (component.view.enabled) mesh.wire=true;
        }
        // Camera/light helpers must not obscure the editable surface, including camera view.
        if (objects[i].kind==cg::ObjectKind::Camera || objects[i].kind==cg::ObjectKind::Light) mesh.wire=true;
        if (render && !(objects[i].id==cameraObject && viewport.camera.projection==cg::Projection::Camera)) {
            ApplyGizmoPreview(mesh,viewport);sceneMeshes.push_back(mesh);
        }
    }
    return sceneMeshes;
}
editor::Tick EditorWorkspaces::TransitionLimit(editor::StableId id,bool outgoing) const {
    const auto found=std::lower_bound(clipById.begin(),clipById.end(),id,
        [](const auto &entry,auto value){return entry.first<value;});
    if (found==clipById.end() || found->first!=id) return 0;
    const auto index=found->second;
    const auto &clip=clips[index];
    const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip.track;});
    if (clip.locked || track==tracks.end() || track->locked) return 0;
    const auto kind=outgoing ? clip.transitionOutKind : clip.transitionInKind;
    if (kind==video::TransitionKind::None) return 0;
    if (kind==video::TransitionKind::Fade) return clip.duration;
    if ((outgoing && index+1==clips.size()) || (!outgoing && index==0)) return 0;
    return outgoing ? video::CenteredTransitionLimit(clip,clips[index+1],{},{}) :
                      video::CenteredTransitionLimit(clips[index-1],clip,{},{});
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
    copy.keys={};copy.keyEvaluation={};
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
std::unique_ptr<EditorWorkspaces::Snapshot> EditorWorkspaces::CaptureModel() const {
    auto model=std::make_unique<Snapshot>();
    model->tracks=tracks;
    model->audioStrips=audioStrips;
    model->mixerTrack=mixerTrack;
    model->clips=clips;
    model->tracksSelected.assign(trackSelection.storage.begin(),trackSelection.storage.begin()+trackSelection.count);
    model->activeTrack=trackSelection.active;
    model->clipEnvelopes=clipEnvelopes;
    model->keys=keys;
    model->objects=objects;
    model->geometries=geometries;
    model->components=components;
    model->objectOrder=objectOrder;
    model->objectPropertyIds=objectPropertyIds;
    model->uv=uv;
    model->assets=assets;
    model->clipProperties=clipProperties;
    model->clipPropertyOwners=clipPropertyOwners;
    model->markers=markers;
    model->markerCount=markerCount;
    model->renamedLabels=renamedLabels;
    model->propertyFlags=propertyFlags;
    model->propertyKeys=propertyKeys;
    model->animationStrips=animationStrips;
    model->customProperties=customProperties;
    model->colors=colors;
    model->colorCurveKeys=colorCurveKeys;
    model->colorCurveChannels=colorCurveChannels;
    model->cameraObject=cameraObject;
    model->nextId=nextId;
    model->clipsSelected.assign(selection.storage.begin(),selection.storage.begin()+selection.count);
    model->objectsSelected.assign(objectSelection.storage.begin(),objectSelection.storage.begin()+objectSelection.count);
    model->keysSelected.assign(keySelection.storage.begin(),keySelection.storage.begin()+keySelection.count);
    model->stripsSelected.assign(stripSelection.storage.begin(),stripSelection.storage.begin()+stripSelection.count);model->activeStrip=stripSelection.active;
    model->activeClip=selection.active;model->activeObject=objectSelection.active;model->activeKey=keySelection.active;
    return model;
}
void EditorWorkspaces::SwapModel(Snapshot &model) {
    std::swap(tracks,model.tracks);
    std::swap(audioStrips,model.audioStrips);
    std::swap(mixerTrack,model.mixerTrack);
    std::swap(clips,model.clips);
    std::swap(clipEnvelopes,model.clipEnvelopes);
    std::swap(keys,model.keys);
    std::swap(objects,model.objects);
    std::swap(geometries,model.geometries);
    std::swap(components,model.components);
    std::swap(objectOrder,model.objectOrder);
    std::swap(objectPropertyIds,model.objectPropertyIds);
    std::swap(uv,model.uv);
    std::swap(assets,model.assets);
    std::swap(clipProperties,model.clipProperties);
    std::swap(clipPropertyOwners,model.clipPropertyOwners);
    std::swap(markers,model.markers);
    std::swap(markerCount,model.markerCount);
    std::swap(renamedLabels,model.renamedLabels);
    std::swap(propertyFlags,model.propertyFlags);
    std::swap(propertyKeys,model.propertyKeys);
    std::swap(animationStrips,model.animationStrips);
    std::swap(customProperties,model.customProperties);
    std::swap(colors,model.colors);
    std::swap(colorCurveKeys,model.colorCurveKeys);
    std::swap(colorCurveChannels,model.colorCurveChannels);
    std::swap(cameraObject,model.cameraObject);
    std::swap(nextId,model.nextId);
    const auto swapSelection=[](editor::Selection &s,std::vector<editor::StableId> &ids,editor::StableId &active) {
        std::vector<editor::StableId> old(s.storage.begin(),s.storage.begin()+s.count);
        s.count=std::min(ids.size(),s.storage.size());std::copy_n(ids.begin(),s.count,s.storage.begin());
        ids=std::move(old);std::swap(s.active,active);
    };
    swapSelection(selection,model.clipsSelected,model.activeClip);
    swapSelection(trackSelection,model.tracksSelected,model.activeTrack);
    swapSelection(objectSelection,model.objectsSelected,model.activeObject);
    swapSelection(keySelection,model.keysSelected,model.activeKey);
    swapSelection(stripSelection,model.stripsSelected,model.activeStrip);
    // All snapshot views borrow the restored model; never retain pointers into the history entry.
    for (auto &c:clips) {
        const auto e=clipEnvelopes.find(c.id);c.envelope=e==clipEnvelopes.end() ? std::span<const video::EnvelopePoint>{} : e->second;
        if (auto n=renamedLabels.find(c.id);n!=renamedLabels.end()) c.label=n->second.c_str();
    }
    for (auto &t:tracks) if(auto n=renamedLabels.find(t.id);n!=renamedLabels.end()) t.label=n->second.c_str();
    for (auto &o:objects) if(auto n=renamedLabels.find(o.id);n!=renamedLabels.end()) o.label=n->second.c_str();
    for (auto &a:assets) if(auto n=renamedLabels.find(a.id);n!=renamedLabels.end()) a.label=n->second.c_str();
    for (auto &c:components) if(auto n=renamedLabels.find(c.view.id);n!=renamedLabels.end()) c.view.label=n->second.c_str();
    ++revision;RebuildTrackLayout();RebuildKeyIndex();RebuildClipIndex();RebuildColorScopes();
    clipPropertyRevision=0;SyncClipProperties();events.Clear();
}
void EditorWorkspaces::Remember(std::unique_ptr<Snapshot> snapshot) {
    history.resize(historyCursor);
    if (history.size()==16) {history.erase(history.begin());--historyCursor;}
    history.push_back(std::move(snapshot));historyCursor=history.size();
}
bool EditorWorkspaces::Undo(bool redo) {
    if (viewport.drag.active || timeline.drag.active || timeline.transitionDrag.active || timeline.envelopeDrag.active || timeline.keyDrag.active ||
        timeline.captionDrag.active || timeline.heightDrag.active || stripDrag.active || uvState.drag.active ||
        curve.drag.active || colorCurve.drag.active || videoProperties.drag.active || objectProperties.drag.active || mixerState.drag.active) return false;
    if (redo ? historyCursor==history.size() : historyCursor==0) return false;
    if (!redo) --historyCursor;
    SwapModel(*history[historyCursor]);
    if (redo) ++historyCursor;
    return true;
}
video::WaveformView EditorWorkspaces::QueryWaveform(const video::WaveformQuery &query) {
    const editor::Range extent{0,editor::FromSeconds(60)};
    if (query.source<1 || query.source>sourceAudio.size() || query.channel<0 || query.channel>1) return {{},extent,video::WaveformStatus::Error};
    const video::WaveformView source{sourceAudio[query.source-1][query.channel],extent,video::WaveformStatus::Ready};
    const int count=std::clamp(query.pixels,1,static_cast<int>(waveScratch.size()));
    const auto tick=[&](int i) {return query.range.first+static_cast<editor::Tick>((static_cast<long double>(query.range.last)-query.range.first)*i/count);};
    for (int i=0;i<count;++i) waveScratch[i]=video::WaveformPixel(source,{tick(i),tick(i+1)});
    return {std::span<const video::AudioBucket>(waveScratch).first(count),query.range,video::WaveformStatus::Ready};
}
bool EditorWorkspaces::PlaceSource(video::PlacementMode mode) {
    editMessage.clear();
    const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==placementTrack;});
    const auto asset=std::find_if(assets.begin(),assets.end(),[&](const auto &a){return a.id==assetSelection.active;});
    const auto range=sourceTime.inOut;
    if (track==tracks.end() || track->locked || asset==assets.end() || range.first<0 || range.last<=range.first || range.last>editor::FromSeconds(60)) {
        editMessage=japanese ? "素材、対象トラック、In/Outを確認してください" : "Choose a source, unlocked track and valid In/Out";return false;
    }
    if (viewport.drag.active || timeline.drag.active || events.count) return false;
    auto start=timeline.time.playhead;
    if (mode==video::PlacementMode::Append) {
        start=0;for(const auto &c:clips) if(c.track==placementTrack) start=std::max(start,c.start+c.duration);
    }
    const auto duration=range.last-range.first;
    if (start<0 || start>std::numeric_limits<editor::Tick>::max()-duration) return false;
    const auto end=start+duration;
    // Reject the entire request if an affected linked/group member lies outside target tracks.
    const auto targeted=[&](editor::StableId id) {auto t=std::find_if(tracks.begin(),tracks.end(),[&](const auto &v){return v.id==id;});return t!=tracks.end() && (id==placementTrack || t->target);};
    for(const auto &c:clips) {
        const bool affected=mode==video::PlacementMode::Insert ? targeted(c.track) && c.start+c.duration>start : c.track==placementTrack && c.start<end && c.start+c.duration>start;
        if (!affected) continue;
        auto owner=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==c.track;});
        bool reject=c.locked || owner==tracks.end() || owner->locked || c.start>std::numeric_limits<editor::Tick>::max()-duration;
        for(const auto &member:clips) if((c.linked && member.linked==c.linked) || (c.group && member.group==c.group))
            reject|=member.locked || (mode==video::PlacementMode::Insert ? !targeted(member.track) : member.track!=placementTrack);
        if(reject) {editMessage=japanese ? "ロックまたはリンク対象のため変更できません" : "Locked or linked material prevents this edit";return false;}
    }
    auto before=CaptureModel();
    std::vector<video::ClipView> result;result.reserve(clips.size()+8);
    const auto rebase=[&](video::ClipView &clip,editor::Tick offset) {
        const auto channel=clip.keyChannel;clip.keyChannel=nextId++;
        std::vector<editor::Keyframe> copied;
        for(const auto &key:keys) if(key.channel==channel) {auto k=key;k.id=nextId++;k.channel=clip.keyChannel;k.tick-=offset;copied.push_back(k);}
        keys.insert(keys.end(),copied.begin(),copied.end());
        if(auto found=clipEnvelopes.find(clip.id);found!=clipEnvelopes.end()) {
            const double atStart=video::EvaluateEnvelope(found->second,offset);
            const double atEnd=video::EvaluateEnvelope(found->second,offset+clip.duration);
            auto &points=found->second;for(auto &point:points) point.tick-=offset;
            std::erase_if(points,[&](const auto &point){return point.tick<=0 || point.tick>=clip.duration;});
            points.insert(points.begin(),{nextId++,0,atStart});points.push_back({nextId++,clip.duration,atEnd});clip.envelope=points;
        }
        clip.transitionIn=0;clip.transitionOut=std::min(clip.transitionOut,clip.duration);
    };
    for(const auto &original:clips) {
        auto c=original;const auto oldEnd=c.start+c.duration;
        if(mode==video::PlacementMode::Insert && targeted(c.track)) {
            if(c.start>=start) c.start+=duration;
            else if(oldEnd>start) {
                auto right=c;right.id=nextId++;right.start=end;right.duration=oldEnd-start;
                right.sourceIn+=static_cast<editor::Tick>((start-c.start)*c.speed);
                CopyClipEditingData(original,right);rebase(right,start-c.start);c.duration=start-c.start;c.transitionOut=0;result.push_back(right);
            }
        } else if(mode==video::PlacementMode::Overwrite && c.track==placementTrack && c.start<end && oldEnd>start) {
            if(c.start>=start && oldEnd<=end) continue;
            if(c.start<start && oldEnd>end) {
                auto right=c;right.id=nextId++;right.start=end;right.duration=oldEnd-end;
                right.sourceIn+=static_cast<editor::Tick>((end-c.start)*c.speed);
                CopyClipEditingData(original,right);rebase(right,end-c.start);result.push_back(right);c.duration=start-c.start;c.transitionOut=0;
            } else if(c.start<start) {c.duration=start-c.start;c.transitionOut=0;}
            else {const auto offset=end-c.start;c.sourceIn+=static_cast<editor::Tick>(offset*c.speed);c.start=end;c.duration=oldEnd-end;rebase(c,offset);}
        }
        result.push_back(c);
    }
    video::ClipView added;added.id=nextId++;added.keyChannel=nextId++;added.track=placementTrack;
    added.start=start;added.duration=duration;added.sourceIn=range.first;added.label=asset->label;
    if(track->kind==video::TrackKind::Audio) {added.audioSource=1+(asset->id%4);added.audioChannels=2;}
    result.push_back(added);clips=std::move(result);selection.Set(added.id);Remember(std::move(before));
    ++revision;RebuildClipIndex();RebuildKeyIndex();SyncClipProperties();return true;
}
#include "timeline_editing.inl"
} // namespace imkit::gallery
