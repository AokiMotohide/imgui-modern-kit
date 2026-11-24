#include "editor_workspaces_internal.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
namespace imkit::gallery {
using namespace detail;
void EditorWorkspaces::ApplyEvents() {
    if(ApplyTimelineEdits()) return;
    for(const auto &e:events.Events()) if(e.phase==editor::Phase::Commit && e.operationSize) {
        const auto count=std::count_if(events.Events().begin(),events.Events().end(),[&](const auto &member){return member.phase==editor::Phase::Commit && member.operation==e.operation;});
        if(static_cast<std::size_t>(count)!=e.operationSize) {editMessage="Incomplete edit group";events.Clear();return;}
    }
    bool rejectFrame=false;
    for(const auto &e:events.Events()) if(e.phase==editor::Phase::Commit && e.kind==editor::EditKind::Move && e.proposed.parent) {
        const auto *source=FindClip(e.target);if(!source) continue;
        const auto oldTrack=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==source->track;});
        const auto destination=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==e.proposed.parent;});
        rejectFrame|=source->locked || oldTrack==tracks.end() || destination==tracks.end() ||
            (oldTrack!=tracks.end() && oldTrack->locked) || (destination!=tracks.end() && (destination->locked || destination->kind!=oldTrack->kind));
        rejectFrame|=e.proposed.first<0 || e.proposed.last<=e.proposed.first;
        if(destination!=tracks.end()) for(const auto &other:QueryClips(destination->id,{e.proposed.first,e.proposed.last})) {
            const bool moves=std::any_of(events.Events().begin(),events.Events().end(),[&](const auto &m){return m.phase==editor::Phase::Commit && m.kind==editor::EditKind::Move && m.target==other.id;});
            if(!moves && other.start<e.proposed.last && other.start+other.duration>e.proposed.first) rejectFrame=true;
        }
    }
    for(const auto &e:events.Events()) if(e.phase==editor::Phase::Commit) {
        rejectFrame |= e.revision!=revision;
        if(e.kind==editor::EditKind::Translate || e.kind==editor::EditKind::Rotate || e.kind==editor::EditKind::Scale) {
            auto object=std::find_if(objects.begin(),objects.end(),[&](const auto &o){return o.id==e.target;});
            rejectFrame |= object!=objects.end() && (object->locked || !object->visible);
        }
        for(const auto &[id,channel]:propertyKeys) for(const auto &key:channel) if(key.id==e.target && e.kind!=editor::EditKind::Navigate)
            rejectFrame |= key.locked || (propertyFlags[id]&16u)!=0;
    }
    if(rejectFrame) {events.Clear();return;}
    const bool hasCommit=std::any_of(events.Events().begin(),events.Events().end(),[&](const auto &e) {return e.phase==editor::Phase::Commit && e.revision==revision && e.kind!=editor::EditKind::Select && e.kind!=editor::EditKind::Navigate &&
        !((e.kind==editor::EditKind::Move || e.kind==editor::EditKind::TrimStart || e.kind==editor::EditKind::TrimEnd) && e.original==e.proposed);});
    auto before=hasCommit ? CaptureModel() : nullptr;
    std::vector<video::ClipView> removedClips;
    std::vector<std::pair<editor::StableId,editor::Tick>> deletePositions;
    bool rejectDelete=false;
    for(const auto &e:events.Events()) if(e.phase==editor::Phase::Commit && e.revision==revision && e.kind==editor::EditKind::RippleDelete) {
        auto clip=std::find_if(clips.begin(),clips.end(),[&](const auto &c){return c.id==e.target;});
        if(clip==clips.end()) {rejectDelete=true;continue;}
        if(std::any_of(removedClips.begin(),removedClips.end(),[&](const auto &c){return c.id==e.target;})) {rejectDelete=true;continue;}
        removedClips.push_back(*clip);
    }
    if(!removedClips.empty()) {
        std::sort(removedClips.begin(),removedClips.end(),[](const auto &a,const auto &b){return a.track!=b.track?a.track<b.track:a.start<b.start;});
        for(std::size_t i=0;i<removedClips.size();++i) {
            const auto &c=removedClips[i];
            if(c.duration<=0 || c.start>std::numeric_limits<editor::Tick>::max()-c.duration) {rejectDelete=true;break;}
            if(i && removedClips[i-1].track==c.track && removedClips[i-1].start+removedClips[i-1].duration>c.start) {rejectDelete=true;break;}
        }
        for(const auto &clip:clips) {
            const bool removed=std::any_of(removedClips.begin(),removedClips.end(),[&](const auto &c){return c.id==clip.id;});
            const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip.track;});
            const bool locked=clip.locked || track==tracks.end() || track->locked;
            if(removed) {rejectDelete |= locked;continue;}
            for(const auto &c:removedClips) if((c.linked && c.linked==clip.linked) || (c.group && c.group==clip.group)) rejectDelete=true;
            const auto position=video::RippleDeletePosition(clip,removedClips);
            if(!position) {rejectDelete=true;continue;}
            if(*position!=clip.start) {rejectDelete |= locked;deletePositions.emplace_back(clip.id,*position);}
        }
    }
    bool changed = false;
    const auto clipBodyEdit=[](editor::EditKind kind) {
        return kind==editor::EditKind::Move || kind==editor::EditKind::Duplicate || kind==editor::EditKind::Split ||
            kind==editor::EditKind::TrimStart || kind==editor::EditKind::TrimEnd || kind==editor::EditKind::Slip ||
            kind==editor::EditKind::Ripple || kind==editor::EditKind::Roll || kind==editor::EditKind::Slide || kind==editor::EditKind::Link;
    };
    bool rejectClipBatch=false;
    for (const auto &event:events.Events()) {
        if (event.phase!=editor::Phase::Commit || event.revision!=revision || !clipBodyEdit(event.kind)) continue;
        const auto clip=std::find_if(clips.begin(),clips.end(),[&](const auto &c){return c.id==event.target;});
        if (clip==clips.end()) continue; // Other modules use the same typed edit kinds.
        const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip->track;});
        rejectClipBatch|=clip->locked || track==tracks.end() || track->locked;
        if (event.kind==editor::EditKind::Link) {
            const auto relation=event.proposed.offset%2 ? clip->group : clip->linked;
            rejectClipBatch|=event.proposed.offset<0 || event.proposed.offset>3 || relation!=event.original.parent;
        }
        if (event.kind==editor::EditKind::Split) rejectClipBatch|=!video::SplitClip(*clip,event.proposed.first,{}).valid;
        if (event.kind==editor::EditKind::Ripple) {
            const auto end=clip->start+clip->duration;
            rejectClipBatch|=std::any_of(clips.begin(),clips.end(),[&](const auto &c) {
                return c.track==clip->track && c.start>=end && c.locked;
            });
        }
    }
    // Resolve all ripple follower positions from the same pre-edit snapshot.
    // In particular, a later edited clip must not overwrite a shift from an earlier trim.
    std::vector<std::pair<editor::StableId,editor::Tick>> ripplePositions;
    struct RippleChange {editor::StableId id,track;editor::Tick end,delta;};
    std::vector<RippleChange> rippleChanges;
    if (!rejectClipBatch) for (const auto &e:events.Events()) {
        if (e.phase!=editor::Phase::Commit || e.revision!=revision || e.kind!=editor::EditKind::Ripple) continue;
        const auto source=std::find_if(clips.begin(),clips.end(),[&](const auto &c){return c.id==e.target;});
        if (source!=clips.end()) rippleChanges.push_back({source->id,source->track,source->start+source->duration,e.proposed.last-(source->start+source->duration)});
    }
    if (!rippleChanges.empty()) for (const auto &clip:clips) {
        auto position=clip.start;
        for (const auto &r:rippleChanges) {
            if (r.id==clip.id || r.track!=clip.track || clip.start<r.end) continue;
            const auto delta=r.delta;
            if ((delta>0 && position>std::numeric_limits<editor::Tick>::max()-delta) ||
                (delta<0 && position<std::numeric_limits<editor::Tick>::min()-delta)) {rejectClipBatch=true;break;}
            position+=delta;
        }
        if (position!=clip.start) ripplePositions.emplace_back(clip.id,position);
    }
    editor::StableId createdLink=0,createdGroup=0;
    std::vector<std::pair<editor::StableId,editor::StableId>> duplicateLinks,duplicateGroups,splitLinks,splitGroups;
    const auto remapRelationship=[&](auto &mapping,editor::StableId id) {
        if (!id) return editor::StableId{0};
        const auto entry=std::find_if(mapping.begin(),mapping.end(),[&](const auto &item){return item.first==id;});
        if (entry!=mapping.end()) return entry->second;
        const auto created=nextId++;mapping.emplace_back(id,created);return created;
    };
    for (const auto &e : events.Events()) {
        if (e.phase==editor::Phase::Begin && e.kind==editor::EditKind::Property)
            propertyGestureSelection.assign(objectSelection.storage.begin(),objectSelection.storage.begin()+objectSelection.count);
        if (e.phase != editor::Phase::Commit || e.revision != revision)
            continue;
        ++commits;
        if(e.kind==editor::EditKind::RippleDelete) continue;
        if(e.kind==editor::EditKind::CaptionInsert) {
            const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==e.target;});
            if(track==tracks.end() || track->locked || track->kind!=video::TrackKind::Caption) continue;
            auto start=e.proposed.first;const auto duration=editor::FromSeconds(3);
            for(const auto &c:clips) if(c.track==track->id && c.start<=std::numeric_limits<editor::Tick>::max()-c.duration) {
                if(start>std::numeric_limits<editor::Tick>::max()-duration) break;
                if(c.start<start+duration && c.start+c.duration>start) start=c.start+c.duration;
            }
            if(start>std::numeric_limits<editor::Tick>::max()-duration) continue;
            video::ClipView clip;clip.id=nextId++;clip.track=track->id;clip.start=start;clip.duration=duration;
            clip.keyChannel=nextId++;clip.label="Caption";
            clips.push_back(clip);selection.Set(clip.id);changed=true;continue;
        }
        bool arrayTarget=false,arrayLocked=false;
        for(auto &property:customProperties) if(property.id==e.target) {
            arrayTarget=true;
            if(e.kind==editor::EditKind::Toggle) {
                const auto flag=static_cast<unsigned>(e.proposed.x);
                if(flag==4 || flag==8 || flag==16) {property.flags=static_cast<editor::PropertyFlags>(e.proposed.y?static_cast<unsigned>(property.flags)|flag:static_cast<unsigned>(property.flags)&~flag);changed=true;}
            }
            if(static_cast<unsigned>(property.flags)&16u) {arrayLocked=true;break;}
            if((e.kind==editor::EditKind::Property || e.kind==editor::EditKind::Reset) && std::isfinite(e.proposed.x)) {property.value=e.proposed.x;changed=true;}
            if(e.kind==editor::EditKind::Reorder) {
                auto other=std::find_if(customProperties.begin(),customProperties.end(),[&](const auto &p){return p.id==e.proposed.parent;});
                if(other!=customProperties.end() && !(static_cast<unsigned>(other->flags)&16u)) {std::iter_swap(&property,other);changed=true;}
            }
            if(property.value!=property.defaultValue) property.flags=static_cast<editor::PropertyFlags>(static_cast<unsigned>(property.flags)|2u);
            else property.flags=static_cast<editor::PropertyFlags>(static_cast<unsigned>(property.flags)&~2u);
            break;
        }
        if(arrayTarget && (arrayLocked || e.kind!=editor::EditKind::PropertyKey)) continue;

        bool colorTarget=false,colorEdited=false;
        for(int c=0;c<3;++c) {
            auto &channel=colorCurveKeys[c];
            if(e.target==colorCurveChannels[c]) {
                colorTarget=true;
                if(e.kind==editor::EditKind::KeyInsert && std::none_of(channel.begin(),channel.end(),[&](const auto &k){return k.tick==e.proposed.first;})) {
                    channel.push_back({nextId++,e.target,e.proposed.first,e.proposed.x});colorEdited=true;
                }
            }
            for(std::size_t i=0;i<channel.size();++i) if(channel[i].id==e.target) {
                colorTarget=true;auto &key=channel[i];
                if(e.kind==editor::EditKind::Navigate) {colorCurve.time=key.tick;colorCurveSelection.Set(key.id);}
                if(key.locked) break;
                if(e.kind==editor::EditKind::Remove) {channel.erase(channel.begin()+i);colorCurveSelection.Clear();colorEdited=true;break;}
                if(e.kind==editor::EditKind::Duplicate) {auto copy=key;copy.id=nextId++;copy.tick=e.proposed.first;copy.value=e.proposed.x;channel.push_back(copy);colorEdited=true;break;}
                if(e.kind==editor::EditKind::Keyframe || e.kind==editor::EditKind::KeyScale) {key.tick=e.proposed.first;key.value=e.proposed.x;colorEdited=true;}
                if(e.kind==editor::EditKind::KeyInterpolation && e.proposed.x>=0 && e.proposed.x<3) {key.interpolation=static_cast<editor::Interpolation>(int(e.proposed.x));colorEdited=true;}
                if(e.kind==editor::EditKind::KeyHandleMode && e.proposed.x>=0 && e.proposed.x<5) {key.handles=static_cast<editor::HandleMode>(int(e.proposed.x));colorEdited=true;}
                if(e.kind==editor::EditKind::Handle) {key=editor::MoveHandle(editor::ResolveHandles(channel,i),e.original.offset<0,{e.proposed.x,e.proposed.y});colorEdited=true;}
                break;
            }
            if(colorEdited) std::sort(channel.begin(),channel.end(),[](const auto &a,const auto &b){return a.tick!=b.tick?a.tick<b.tick:a.id<b.id;});
        }
        if(colorTarget) {if(colorEdited) {RebuildColorScopes();changed=true;}continue;}

        bool propertyCurveTarget=false;
        for(auto &[id,channel]:propertyKeys) {
            const bool channelLocked=(propertyFlags[id]&16u)!=0;
            if(e.kind==editor::EditKind::KeyInsert && e.target==id) {
                propertyCurveTarget=true;
                if(!channelLocked && std::none_of(channel.begin(),channel.end(),[&](const auto &k){return k.tick==e.proposed.first;})) {
                    channel.push_back({nextId++,id,e.proposed.first,e.proposed.x});changed=true;
                }
            }
            for(std::size_t i=0;i<channel.size();++i) if(channel[i].id==e.target) {
                propertyCurveTarget=true;auto &key=channel[i];
                if(e.kind==editor::EditKind::Navigate) timeline.time.playhead=key.tick;
                if(key.locked || channelLocked) break;
                if(e.kind==editor::EditKind::Remove) {channel.erase(channel.begin()+i);keySelection.Clear();changed=true;break;}
                if(e.kind==editor::EditKind::Duplicate) {auto copy=key;copy.id=nextId++;copy.tick=e.proposed.first;channel.push_back(copy);changed=true;break;}
                if(e.kind==editor::EditKind::Keyframe || e.kind==editor::EditKind::KeyScale) {key.tick=e.proposed.first;key.value=e.proposed.x;changed=true;}
                if(e.kind==editor::EditKind::KeyInterpolation && e.proposed.x>=0 && e.proposed.x<3) {key.interpolation=static_cast<editor::Interpolation>(int(e.proposed.x));changed=true;}
                if(e.kind==editor::EditKind::KeyHandleMode && e.proposed.x>=0 && e.proposed.x<5) {key.handles=static_cast<editor::HandleMode>(int(e.proposed.x));changed=true;}
                if(e.kind==editor::EditKind::Handle) {key=editor::MoveHandle(editor::ResolveHandles(channel,i),e.original.offset<0,{e.proposed.x,e.proposed.y});changed=true;}
                break;
            }
            if(propertyCurveTarget) {std::sort(channel.begin(),channel.end(),[](const auto &a,const auto &b){return a.tick<b.tick;});break;}
        }
        if(propertyCurveTarget) continue;
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
        if(e.kind==editor::EditKind::Remove) for(std::size_t i=0;i<markerCount;++i) if(markers[i].id==e.target) {
            std::move(markers.begin()+i+1,markers.begin()+markerCount,markers.begin()+i);--markerCount;changed=true;break;
        }
        if (e.kind == editor::EditKind::Marker) {
            if(e.target) {for(auto &m:std::span(markers).first(markerCount)) if(m.id==e.target) m.tick=e.proposed.first;}
            else if(markerCount<markers.size()) markers[markerCount++] = {nextId++, e.proposed.first, "Marker"};
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
            if (!component->view.locked && e.kind==editor::EditKind::Rename) {
                renamedLabels[e.target]=e.proposedText.data();component->view.label=renamedLabels[e.target].c_str();changed=true;
            } else if (!component->view.locked && e.kind==editor::EditKind::Duplicate) {
                auto copy=*component;copy.view.id=nextId++;components.push_back(copy);objectSelection.Set(copy.view.id);changed=true;continue;
            } else if (!component->view.locked && e.kind==editor::EditKind::Reparent) {
                const auto destination=std::find_if(objects.begin(),objects.end(),[&](const auto &o){return o.id==e.proposed.parent;});
                if (destination!=objects.end() && !destination->locked) {component->view.owner=destination->id;changed=true;}
            } else if (e.kind==editor::EditKind::Toggle) {
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
        auto clip = std::find_if(clips.begin(), clips.end(), [&](const auto &c) { return c.id == e.target; });
        if (clip!=clips.end()) {
            if (rejectClipBatch && clipBodyEdit(e.kind)) continue;
            const auto ownerTrack=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip->track;});
            if (clip->locked || ownerTrack==tracks.end() || ownerTrack->locked) continue;
        }
        if (e.kind == editor::EditKind::Rename)
            renamedLabels[e.target] = e.proposedText.data();
        if (e.kind==editor::EditKind::Rename) for (auto &object:objects)
            if (object.id==e.target && !object.locked) {object.label=renamedLabels[e.target].c_str();changed=true;}
        if (e.kind==editor::EditKind::Rename) for (auto &track:tracks)
            if (track.id==e.target && !track.locked) {track.label=renamedLabels[e.target].c_str();changed=true;}
        if (clip != clips.end()) {
            if (e.kind==editor::EditKind::Link && e.proposed.offset>=0 && e.proposed.offset<=3) {
                auto &relation=e.proposed.offset%2 ? clip->group : clip->linked;
                auto set=e.proposed.parent;
                if (e.proposed.offset>=2) {
                    auto &created=e.proposed.offset==2 ? createdLink : createdGroup;
                    if (!created) created=nextId++;
                    set=created;
                }
                if (relation==e.original.parent && relation!=set) {relation=set;changed=true;}
            }
            auto transitionValue=[](const video::ClipView &c) {
                editor::Value value;value.first=c.transitionIn;value.last=c.transitionOut;
                value.x=static_cast<int>(c.transitionInKind);value.y=static_cast<int>(c.transitionOutKind);return value;
            };
            const auto transitionBefore=transitionValue(*clip);
            if (e.kind==editor::EditKind::TransitionType && !clip->locked && e.proposed.first>=0 && e.proposed.first<=3 && e.proposed.last>=0 && e.proposed.last<=3) {
                const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip->track;});
                if (track!=tracks.end() && !track->locked) {
                    clip->transitionInKind=static_cast<video::TransitionKind>(e.proposed.first);
                    clip->transitionOutKind=static_cast<video::TransitionKind>(e.proposed.last);
                    clip->transitionIn=std::min(clip->transitionIn,TransitionLimit(clip->id,false));
                    clip->transitionOut=std::min(clip->transitionOut,TransitionLimit(clip->id,true));changed=true;
                }
            }
            if (e.kind==editor::EditKind::TransitionDuration && !clip->locked) {
                const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip->track;});
                if (track!=tracks.end() && !track->locked) {
                    clip->transitionIn=std::clamp(e.proposed.first,editor::Tick{0},TransitionLimit(clip->id,false));
                    clip->transitionOut=std::clamp(e.proposed.last,editor::Tick{0},
                        std::min(TransitionLimit(clip->id,true),clip->duration-clip->transitionIn));
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
                    const auto localCut=split.left.duration;
                    if (clip->keyChannel && std::any_of(keys.begin(),keys.end(),[&](const auto &key) {
                        return key.channel==clip->keyChannel && key.tick<std::numeric_limits<editor::Tick>::min()+localCut;
                    })) continue;
                    right.linked=remapRelationship(splitLinks,clip->linked);
                    right.group=remapRelationship(splitGroups,clip->group);
                    CopyClipEditingData(*clip,right);
                    if (right.keyChannel) for (auto &key:keys)
                        if (key.channel==right.keyChannel) key.tick-=localCut;
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
                    clip->transitionIn=std::min(clip->transitionIn,clip->duration);
                    clip->transitionOut=0;clip->transitionOutKind=video::TransitionKind::None;
                    clip->fades.outDuration=0;clip->outgoingTransition={};
                    right.fades.inDuration=0;
                    right.transitionIn=0;right.transitionInKind=video::TransitionKind::None;
                    right.transitionOut=std::min(right.transitionOut,right.duration);
                    clips.push_back(right);
                    changed = true;
                }
            } else if (e.kind == editor::EditKind::Duplicate) {
                const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto &t){return t.id==clip->track;});
                if (clip->locked || track==tracks.end() || track->locked) continue;
                auto copy = *clip;
                copy.id = nextId++;
                copy.start = e.proposed.first;
                copy.linked=remapRelationship(duplicateLinks,clip->linked);
                copy.group=remapRelationship(duplicateGroups,clip->group);
                CopyClipEditingData(*clip,copy);
                clips.push_back(copy);
                changed = true;
            } else if (e.kind == editor::EditKind::Move || e.kind == editor::EditKind::TrimStart ||
                       e.kind == editor::EditKind::TrimEnd || e.kind == editor::EditKind::Slip ||
                       e.kind == editor::EditKind::Ripple || e.kind == editor::EditKind::Roll ||
                       e.kind == editor::EditKind::Slide) {
                const auto oldEnd=clip->start+clip->duration;
                if (clip->start==e.proposed.first && oldEnd==e.proposed.last && clip->sourceIn==e.proposed.offset && clip->track==e.proposed.parent)
                    continue;
                clip->start = e.proposed.first;
                clip->duration = e.proposed.last - e.proposed.first;
                clip->sourceIn = e.proposed.offset;
                clip->track = e.proposed.parent;
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
                    if (e.proposed.hasAffine) {
                        o.transform.rotation={e.proposed.affine[0],e.proposed.affine[1],e.proposed.affine[2]};
                        o.transform.shear={e.proposed.affine[3],e.proposed.affine[4],e.proposed.affine[5]};
                    }
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
        bool isProperty=arrayTarget || clipProperty!=clipPropertyOwners.end();
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
    if(!rejectDelete && !removedClips.empty()) {
        const std::map<editor::StableId,editor::Tick> positions(deletePositions.begin(),deletePositions.end());
        for(auto &clip:clips) if(auto position=positions.find(clip.id);position!=positions.end()) clip.start=position->second;
        std::erase_if(clips,[&](const auto &clip){return std::any_of(removedClips.begin(),removedClips.end(),[&](const auto &c){return c.id==clip.id;});});
        for(const auto &clip:removedClips) {clipEnvelopes.erase(clip.id);clipProperties.erase(clip.id);}
        selection.Clear();changed=true;
    }
    if (!rejectClipBatch) for (const auto &[id,position]:ripplePositions) {
        const auto clip=std::find_if(clips.begin(),clips.end(),[&](const auto &c){return c.id==id;});
        if (clip!=clips.end()) {clip->start=position;changed=true;}
    }
    if (changed) {
        if (before) Remember(std::move(before));
        RebuildTrackLayout();
        ++revision;
        RebuildClipIndex();
        RebuildKeyIndex();
    }
    events.Clear();SyncClipProperties();
}
} // namespace imkit::gallery
