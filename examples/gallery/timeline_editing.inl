const video::ClipView *EditorWorkspaces::FindClip(editor::StableId id) const {
    const auto it=std::lower_bound(clipById.begin(),clipById.end(),id,[](const auto &entry,auto id){return entry.first<id;});
    return it==clipById.end() || it->first!=id ? nullptr : &clips[it->second];
}
editor::StableId EditorWorkspaces::DestinationTrack(editor::StableId source,editor::StableId anchor,editor::StableId hovered) const {
    const auto index=[&](auto id) {return std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==id;})-tracks.begin();};
    const auto a=index(anchor),h=index(hovered),from=index(source),count=static_cast<std::ptrdiff_t>(tracks.size());
    if(a==count || h==count || from==count) return 0;
    const auto to=from+h-a;
    return to<0 || to>=count || tracks[to].locked || tracks[to].kind!=tracks[from].kind ? 0 : tracks[to].id;
}
bool EditorWorkspaces::CanMoveClips(std::span<const editor::StableId> ids,editor::Tick delta,editor::StableId anchor,editor::StableId hovered) {
    const auto borrowed=QuerySelectedClips(ids);
    std::vector<video::ClipView> moving(borrowed.begin(),borrowed.end());
    if(moving.size()<ids.size()) return false;
    const auto contains=[&](auto id){return std::any_of(moving.begin(),moving.end(),[&](const auto &c){return c.id==id;});};
    for(const auto &c:moving) {
        const auto destination=DestinationTrack(c.track,anchor,hovered);
        if(c.locked || !destination || delta < -c.start || delta>std::numeric_limits<editor::Tick>::max()-c.start-c.duration) return false;
        const auto overlaps=QueryClips(destination,{c.start+delta,c.start+delta+c.duration});
        for(const auto &other:overlaps) if(!contains(other.id)) return false;
    }
    return true;
}
void EditorWorkspaces::ConfigureTimelineEditing(video::TimelineProvider &p) {
    timeline.trackSelection=&trackSelection;
    p.editing.user=this;
    p.editing.trackAfter=[](void *u,editor::StableId id) {
        const auto &rows=static_cast<EditorWorkspaces*>(u)->tracks;
        auto it=std::find_if(rows.begin(),rows.end(),[&](const auto &t){return t.id==id;});
        return it==rows.end() || ++it==rows.end() ? editor::StableId{0} : it->id;
    };
    p.editing.fades=[](void *u,editor::StableId id)->const video::FadeView * {
        const auto *clip=static_cast<EditorWorkspaces*>(u)->FindClip(id);return clip ? &clip->fades : nullptr;
    };
    p.editing.cut=[](void *u,editor::StableId id) {
        const auto &s=*static_cast<EditorWorkspaces*>(u);video::CutTransitionView result;result.left=id;
        const auto *left=s.FindClip(id);if(!left) return result;
        const auto index=static_cast<std::size_t>(left-s.clips.data());
        if(index+1>=s.clips.size()) return result;
        const auto &right=s.clips[index+1];
        const auto track=std::find_if(s.tracks.begin(),s.tracks.end(),[&](const auto &t){return t.id==left->track;});
        result=left->outgoingTransition;result.left=id;result.right=right.id;
        result.locked=left->locked || right.locked || track==s.tracks.end() || track->locked;
        result.limit=video::CenteredTransitionLimit(*left,right,{},{});
        if(track!=s.tracks.end() && result.duration==0) result.kind=track->kind==video::TrackKind::Audio ? video::TransitionKind::Crossfade : video::TransitionKind::Dissolve;
        if(!result.limit || (left->outgoingTransition.right && left->outgoingTransition.right!=right.id)) result.duration=0;
        return result;
    };
    p.editing.destination=[](void *u,auto source,auto anchor,auto hovered){return static_cast<EditorWorkspaces*>(u)->DestinationTrack(source,anchor,hovered);};
    p.editing.canMove=[](void *u,std::span<const editor::StableId> ids,editor::Tick delta,editor::StableId anchor,editor::StableId hovered){return static_cast<EditorWorkspaces*>(u)->CanMoveClips(ids,delta,anchor,hovered);};
    p.editing.box=[](void *u,editor::Range range,double top,double bottom)->std::span<const editor::StableId> {
        auto &s=*static_cast<EditorWorkspaces*>(u);s.boxClipIds.clear();
        auto first=std::upper_bound(s.trackOffsets.begin(),s.trackOffsets.end(),top)-s.trackOffsets.begin();
        if(first) --first;
        for(std::size_t i=first;i<s.tracks.size() && s.trackOffsets[i]<bottom;++i) {
            if(s.tracks[i].locked) continue;
            for(const auto &clip:s.QueryClips(s.tracks[i].id,range)) if(!clip.locked) s.boxClipIds.push_back(clip.id);
        }
        return s.boxClipIds;
    };
    p.editing.trackClipCount=[](void *u,editor::StableId id) {
        const auto &s=*static_cast<EditorWorkspaces*>(u);
        return static_cast<std::size_t>(std::count_if(s.clips.begin(),s.clips.end(),[&](const auto &c){return c.track==id;}));
    };
}
void EditorWorkspaces::SliceTimelineClip(video::ClipView &clip,editor::Tick offset,editor::Tick originalDuration) {
    if(offset>0) clip.fades.inDuration=0;
    if(offset+clip.duration<originalDuration) {clip.fades.outDuration=0;clip.outgoingTransition={};}
    for(auto &key:keys) if(key.channel==clip.keyChannel) key.tick-=offset;
    if(auto properties=clipProperties.find(clip.id);properties!=clipProperties.end())
        for(auto id:properties->second.ids) if(auto channel=propertyKeys.find(id);channel!=propertyKeys.end())
            for(auto &key:channel->second) key.tick-=offset;
    if(auto found=clipEnvelopes.find(clip.id);found!=clipEnvelopes.end() && !found->second.empty()) {
        auto &points=found->second;
        const auto first=video::EvaluateEnvelope(points,offset),last=video::EvaluateEnvelope(points,offset+clip.duration);
        std::erase_if(points,[&](const auto &point){return point.tick<offset || point.tick>offset+clip.duration;});
        for(auto &point:points) point.tick-=offset;
        if(points.empty() || points.front().tick>0) points.insert(points.begin(),{nextId++,0,first});
        if(points.back().tick<clip.duration) points.push_back({nextId++,clip.duration,last});
        clip.envelope=points;
    }
    clip.keys={};clip.keyEvaluation={};
}
bool EditorWorkspaces::ApplyTimelineEdits() {
    using editor::EditKind;using editor::Phase;
    const auto supported=[](EditKind kind){return kind==EditKind::ClipFades || kind==EditKind::CutTransition || kind==EditKind::TrackEdit || kind==EditKind::Clipboard;};
    if(!std::any_of(events.Events().begin(),events.Events().end(),[&](const auto &e){return supported(e.kind);})) return false;
    std::vector<editor::Event> edits;
    for(const auto &e:events.Events()) if(supported(e.kind) && e.phase==Phase::Commit) edits.push_back(e);
    const auto reject=[&](const char *message){editMessage=message;events.Clear();return true;};
    if(edits.empty()) {events.Clear();return true;}
    if(events.overflow || std::any_of(edits.begin(),edits.end(),[&](const auto &e){return e.revision!=revision;})) return reject("Edit cancelled: stale model or event capacity");
    if(std::any_of(edits.begin(),edits.end(),[&](const auto &e){return e.kind!=edits.front().kind;})) return reject("Mixed edit group");
    if(edits.front().kind==EditKind::Clipboard && edits.size()!=1) return reject("Multiple clipboard requests");
    for(const auto &e:edits) if(e.operationSize && static_cast<std::size_t>(std::count_if(edits.begin(),edits.end(),[&](const auto &member){return member.operation==e.operation && member.kind==e.kind;}))!=e.operationSize) return reject("Incomplete edit group");
    video::TimelineProvider provider;ConfigureTimelineEditing(provider);
    const auto trackById=[&](auto id){return std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==id;});};
    for(const auto &e:edits) {
        if(!std::isfinite(e.proposed.x) || !std::isfinite(e.proposed.y)) return reject("Non-finite edit value");
        if(e.kind==EditKind::TrackEdit && (e.proposed.offset<0 || e.proposed.offset>static_cast<int>(video::TrackAction::Reorder) || e.proposed.offset!=edits.front().proposed.offset || e.proposed.parent!=edits.front().proposed.parent)) return reject("Invalid track action");
        if(e.kind==EditKind::TrackEdit && e.proposed.offset==static_cast<int>(video::TrackAction::Add) && (edits.size()!=1 || e.proposed.x<0 || e.proposed.x>5 || std::floor(e.proposed.x)!=e.proposed.x)) return reject("Invalid track kind");
        if(e.kind==EditKind::Clipboard && (e.proposed.offset<0 || e.proposed.offset>2 || e.proposed.x<0 || e.proposed.x>1)) return reject("Invalid clipboard action");
        if(e.kind==EditKind::ClipFades || e.kind==EditKind::CutTransition) {
            const auto *clip=FindClip(e.target);if(!clip || clip->locked) return reject("Clip no longer editable");
            const auto track=trackById(clip->track);if(track==tracks.end() || track->locked) return reject("Track locked");
            if(e.kind==EditKind::ClipFades && (e.proposed.first<0 || e.proposed.last<0 || e.proposed.first>clip->duration || e.proposed.last>clip->duration-e.proposed.first || e.proposed.x<0 || e.proposed.x>2 || e.proposed.y<0 || e.proposed.y>2)) return reject("Invalid fade");
            if(e.kind==EditKind::CutTransition) {
                const auto cut=provider.editing.cut(this,e.target);
                const auto kind=static_cast<video::TransitionKind>(e.proposed.x);
                if(cut.locked || cut.right!=e.proposed.parent || e.proposed.first<0 || e.proposed.first>cut.limit ||
                   (track->kind==video::TrackKind::Audio ? kind!=video::TransitionKind::Crossfade : track->kind!=video::TrackKind::Video || (kind!=video::TransitionKind::Dissolve && kind!=video::TransitionKind::Fade))) return reject("Invalid transition boundary");
            }
        }
        if(e.kind==EditKind::TrackEdit && e.proposed.offset!=static_cast<int>(video::TrackAction::Add)) {
            const auto track=trackById(e.target);if(track==tracks.end() || track->locked) return reject("Track no longer editable");
            if(std::any_of(clips.begin(),clips.end(),[&](const auto &c){return c.track==e.target && c.locked;})) return reject("Track contains a locked clip");
        }
    }
    auto before=CaptureModel();bool changed=false;
    if(edits.front().kind==EditKind::TrackEdit) {
        const auto action=static_cast<video::TrackAction>(edits.front().proposed.offset);
        std::vector<editor::StableId> ids;for(const auto &e:edits) ids.push_back(e.target);
        const auto selected=[&](auto id){return std::find(ids.begin(),ids.end(),id)!=ids.end();};
        std::vector<video::TrackView> rows;for(const auto &t:tracks) if(selected(t.id)) rows.push_back(t);
        if(action==video::TrackAction::Add) {
            video::TrackView t;t.id=nextId++;t.kind=static_cast<video::TrackKind>(edits.front().proposed.x);
            renamedLabels[t.id]=timeline.trackLabels.kinds[static_cast<int>(t.kind)];t.label=renamedLabels[t.id].c_str();tracks.push_back(t);trackSelection.Set(t.id);changed=true;
        } else if(action==video::TrackAction::Remove) {
            std::erase_if(clips,[&](const auto &c){return selected(c.track);});std::erase_if(tracks,[&](const auto &t){return selected(t.id);});
            std::erase_if(audioStrips,[&](const auto &t){return selected(t.id);});trackSelection.Clear();selection.Clear();changed=true;
        } else if(action==video::TrackAction::Reorder) {
            const auto target=edits.front().proposed.parent;
            if(!selected(target) && (target==0 || trackById(target)!=tracks.end())) {
                std::erase_if(tracks,[&](const auto &t){return selected(t.id);});auto where=trackById(target);tracks.insert(where,rows.begin(),rows.end());changed=true;
            }
        } else if(action==video::TrackAction::Duplicate) {
            const auto sourceClips=clips;trackSelection.Clear();
            std::map<editor::StableId,editor::StableId> copies,links,groups;
            const auto remap=[&](auto &mapping,auto id) {if(!id) return editor::StableId{0};auto [it,inserted]=mapping.emplace(id,0);if(inserted) it->second=nextId++;return it->second;};
            for(auto row:rows) {
                const auto old=row.id;row.id=nextId++;renamedLabels[row.id]=std::string(row.label)+" copy";row.label=renamedLabels[row.id].c_str();
                tracks.push_back(row);trackSelection.Set(row.id,true);
                for(const auto &clip:sourceClips) if(clip.track==old) {
                    auto copy=clip;copy.id=nextId++;copies[clip.id]=copy.id;copy.track=row.id;copy.linked=remap(links,clip.linked);copy.group=remap(groups,clip.group);CopyClipEditingData(clip,copy);clips.push_back(copy);
                }
            }
            for(auto &c:clips) if(std::any_of(copies.begin(),copies.end(),[&](const auto &pair){return pair.second==c.id;})) {
                if(auto right=copies.find(c.outgoingTransition.right);right!=copies.end()) {c.outgoingTransition.left=c.id;c.outgoingTransition.right=right->second;}
                else c.outgoingTransition={};
            }
            changed=true;
        }
    } else if(edits.front().kind==EditKind::Clipboard) {
        const auto &e=edits.front();const auto action=static_cast<video::ClipboardAction>(e.proposed.offset);
        if(action!=video::ClipboardAction::Paste) {
            const auto selected=QuerySelectedClips(selection.storage.first(selection.count));
            if(selected.empty()) return reject("No clips selected");
            for(const auto &c:selected) if(c.locked) return reject("Selection contains locked clips");
            auto firstTick=selected.front().start;std::ptrdiff_t firstTrack=tracks.size();
            for(const auto &c:selected) {firstTick=std::min(firstTick,c.start);firstTrack=std::min(firstTrack,trackById(c.track)-tracks.begin());}
            clipboard.clear();
            for(const auto &c:selected) {
                ClipboardClip entry;entry.clip=c;entry.clip.start-=firstTick;entry.label=c.label;
                if(auto properties=clipProperties.find(c.id);properties!=clipProperties.end()) {
                    entry.properties=properties->second.values;
                    for(std::size_t i=0;i<4;++i) {
                        const auto id=properties->second.ids[i];
                        if(auto flags=propertyFlags.find(id);flags!=propertyFlags.end()) entry.flags[i]=flags->second;
                        if(auto channel=propertyKeys.find(id);channel!=propertyKeys.end()) entry.propertyChannels[i]=channel->second;
                    }
                }
                const auto track=trackById(c.track);entry.trackOffset=static_cast<int>(track-tracks.begin()-firstTrack);entry.kind=track->kind;
                entry.envelope.assign(c.envelope.begin(),c.envelope.end());
                for(const auto &key:keys) if(key.channel==c.keyChannel) entry.keys.push_back(key);
                entry.clip.envelope={};entry.clip.keys={};entry.clip.keyEvaluation={};entry.clip.label="";
                clipboard.push_back(std::move(entry));
            }
            if(action==video::ClipboardAction::Cut) {
                std::erase_if(clips,[&](const auto &c){return std::any_of(clipboard.begin(),clipboard.end(),[&](const auto &copy){return copy.clip.id==c.id;});});selection.Clear();changed=true;
            }
        } else {
            auto target=trackById(e.proposed.parent ? e.proposed.parent : placementTrack);
            if(clipboard.empty() || target==tracks.end()) return reject("Choose a destination track and copy clips first");
            std::vector<video::ClipView> added;editor::Tick extent=0;
            for(const auto &copy:clipboard) {
                const auto index=target-tracks.begin()+copy.trackOffset;
                if(index>=static_cast<std::ptrdiff_t>(tracks.size()) || tracks[index].locked || tracks[index].kind!=copy.kind) return reject("Paste destinations do not match source tracks");
                auto clip=copy.clip;clip.track=tracks[index].id;
                if(e.proposed.first<0 || e.proposed.first>std::numeric_limits<editor::Tick>::max()-clip.start-clip.duration) return reject("Paste time is outside the timeline");
                extent=std::max(extent,clip.start+clip.duration);clip.start+=e.proposed.first;added.push_back(clip);
            }
            const auto affected=[&](auto track){return std::any_of(added.begin(),added.end(),[&](const auto &c){return c.track==track;});};
            const bool insert=e.proposed.x==static_cast<int>(video::PlacementMode::Insert);
            for(const auto &c:clips) if(affected(c.track)) {
                const bool touched=insert ? c.start+c.duration>e.proposed.first : std::any_of(added.begin(),added.end(),[&](const auto &n){return n.track==c.track && n.start<c.start+c.duration && n.start+n.duration>c.start;});
                if(touched && (c.locked || c.linked || c.group)) return reject("Paste would modify locked or linked existing clips");
                if(touched && insert && c.start>std::numeric_limits<editor::Tick>::max()-extent-c.duration) return reject("Paste duration overflows timeline");
            }
            std::vector<video::ClipView> result;
            for(auto c:clips) {
                if(!affected(c.track)) {result.push_back(c);continue;}
                if(insert) {
                    if(c.start>=e.proposed.first) c.start+=extent;
                    else if(c.start+c.duration>e.proposed.first) {
                        auto right=c;right.id=nextId++;right.duration=c.start+c.duration-e.proposed.first;right.sourceIn+=static_cast<editor::Tick>((e.proposed.first-c.start)*c.speed);right.start=e.proposed.first+extent;
                        CopyClipEditingData(c,right);SliceTimelineClip(right,e.proposed.first-c.start,c.duration);
                        const auto originalDuration=c.duration;c.duration=e.proposed.first-c.start;SliceTimelineClip(c,0,originalDuration);result.push_back(right);
                    }
                    result.push_back(c);
                } else {
                    std::vector<video::ClipView> fragments{c};
                    for(const auto &n:added) if(n.track==c.track) {
                        std::vector<video::ClipView> remaining;
                        for(auto f:fragments) {
                            const auto end=f.start+f.duration,cutEnd=n.start+n.duration;
                            if(n.start>=end || cutEnd<=f.start) {remaining.push_back(f);continue;}
                            if(cutEnd<end) {auto right=f;right.id=nextId++;right.start=cutEnd;right.duration=end-cutEnd;right.sourceIn+=static_cast<editor::Tick>((cutEnd-f.start)*f.speed);CopyClipEditingData(f,right);SliceTimelineClip(right,cutEnd-f.start,f.duration);remaining.push_back(right);}
                            if(f.start<n.start) {const auto duration=f.duration;f.duration=n.start-f.start;SliceTimelineClip(f,0,duration);remaining.push_back(f);}
                        }
                        fragments=std::move(remaining);
                    }
                    result.insert(result.end(),fragments.begin(),fragments.end());
                }
            }
            selection.Clear();
            std::map<editor::StableId,editor::StableId> copies,links,groups;
            const auto remap=[&](auto &mapping,auto id) {if(!id) return editor::StableId{0};auto [it,inserted]=mapping.emplace(id,0);if(inserted) it->second=nextId++;return it->second;};
            for(std::size_t i=0;i<added.size();++i) {
                auto &c=added[i];const auto sourceId=c.id;c.id=nextId++;copies[sourceId]=c.id;c.linked=remap(links,c.linked);c.group=remap(groups,c.group);
                const auto &copy=clipboard[i];
                renamedLabels[c.id]=copy.label;c.label=renamedLabels[c.id].c_str();c.keyChannel=nextId++;
                auto &properties=clipProperties[c.id];properties.values=copy.properties;
                for(std::size_t component=0;component<4;++component) {
                    const auto id=nextId++;properties.ids[component]=id;clipPropertyOwners[id]={c.id,component};propertyFlags[id]=copy.flags[component];
                    auto &channel=propertyKeys[id];channel=copy.propertyChannels[component];for(auto &key:channel) {key.id=nextId++;key.channel=id;}
                }
                auto &env=clipEnvelopes[c.id];env=copy.envelope;for(auto &point:env) point.id=nextId++;c.envelope=env;
                for(auto key:copy.keys) {key.id=nextId++;key.channel=c.keyChannel;keys.push_back(key);}
                c.keys={};c.keyEvaluation={};
                result.push_back(c);selection.Set(c.id,true);
            }
            for(auto &c:result) if(std::any_of(copies.begin(),copies.end(),[&](const auto &pair){return pair.second==c.id;})) {
                if(auto right=copies.find(c.outgoingTransition.right);right!=copies.end()) {c.outgoingTransition.left=c.id;c.outgoingTransition.right=right->second;}
                else c.outgoingTransition={};
            }
            clips=std::move(result);changed=true;
        }
    } else for(const auto &e:edits) {
        auto clip=std::find_if(clips.begin(),clips.end(),[&](const auto &c){return c.id==e.target;});
        if(e.kind==EditKind::ClipFades) clip->fades={e.proposed.first,e.proposed.last,static_cast<video::FadeCurve>(e.proposed.x),static_cast<video::FadeCurve>(e.proposed.y)};
        else clip->outgoingTransition={clip->id,e.proposed.parent,e.proposed.first,0,static_cast<video::TransitionKind>(e.proposed.x)};
        changed|=!(e.original==e.proposed);
    }
    if(changed) {Remember(std::move(before));++revision;RebuildTrackLayout();RebuildClipIndex();RebuildKeyIndex();SyncClipProperties();}
    events.Clear();editMessage.clear();return true;
}
