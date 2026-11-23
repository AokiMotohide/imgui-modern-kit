#include "gallery_host.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace imkit::gallery::host {

int VerifyTimelineModel() {
    using namespace imkit;
    auto storage=std::make_unique<gallery::EditorWorkspaces>();auto &s=*storage;s.Initialize();
    s.tracks={{71,"Video A"},{72,"Video B"},{73,"Video C"},{74,"Audio",video::TrackKind::Audio}};
    s.clips.clear();
    for(int i=0;i<2;++i) {video::ClipView c;c.id=81+i;c.track=71;c.label="Clip";c.start=editor::FromSeconds(1+2*i);c.duration=editor::FromSeconds(2);c.sourceIn=editor::FromSeconds(10);s.clips.push_back(c);}
    s.selection.Set(81);s.trackSelection.Set(71);s.RebuildTrackLayout();s.RebuildClipIndex();
    video::TimelineProvider p;s.ConfigureTimelineEditing(p);
    int failures=0;auto check=[&](bool ok,const char *message){std::printf("%s %s\n",ok ? "PASS" : "FAIL",message);if(!ok) ++failures;};
    const auto edit=[&](editor::StableId id,editor::EditKind kind,editor::Value value) {
        s.events.Clear();s.events.Push({id,s.revision,editor::Phase::Begin,kind});
        s.events.Push({id,s.revision,editor::Phase::Commit,kind,{},value});s.ApplyEvents();
    };
    check(s.CanMoveClips(s.selection.storage.first(1),0,71,71),"touching clip boundary is not a move collision");
    const auto beforeTouch=s.revision;
    edit(81,editor::EditKind::Move,{editor::FromSeconds(5),editor::FromSeconds(7),editor::FromSeconds(10),71,1});
    check(s.FindClip(81)->start==editor::FromSeconds(5) && s.revision>beforeTouch,
          "host commits move flush against neighboring clip");
    check(s.Undo() && s.FindClip(81)->start==editor::FromSeconds(1),"boundary move undo restores original position");
    edit(81,editor::EditKind::ClipFades,{editor::FromSeconds(.5),editor::FromSeconds(.25),0,0,1,2});
    check(s.FindClip(81)->fades.inDuration==editor::FromSeconds(.5),"host applies independent fade");
    check(s.Undo() && s.FindClip(81)->fades.inDuration==0,"fade undo restores model");
    check(s.Undo(true) && s.FindClip(81)->fades.outCurve==video::FadeCurve::EaseOut,"fade redo restores curve");
    s.tracks[0].locked=true;const auto rev=s.revision;edit(81,editor::EditKind::ClipFades,{});
    check(s.revision==rev && s.FindClip(81)->fades.inDuration>0,"locked fade is rejected");s.tracks[0].locked=false;
    auto cut=p.editing.cut(&s,81);check(cut.right==82 && cut.limit>0,"adjacent cut exposes media handles");
    edit(81,editor::EditKind::CutTransition,{editor::FromSeconds(1),0,0,82,static_cast<double>(video::TransitionKind::Dissolve)});
    check(s.FindClip(81)->outgoingTransition.right==82,"shared transition references both clips");
    check(s.Undo() && s.FindClip(81)->outgoingTransition.duration==0,"shared transition undo");
    check(s.CanMoveClips(s.selection.storage.first(1),0,71,72),"cross-track move accepted on compatible track");
    check(!s.CanMoveClips(s.selection.storage.first(1),0,71,74),"cross-track move rejects incompatible track");
    check(!s.CanMoveClips(s.selection.storage.first(1),editor::FromSeconds(2),71,71),"move rejects unselected collision");
    s.selection.Set(82,true);check(s.CanMoveClips(s.selection.storage.first(2),0,71,72),"move excludes complete selected set from collisions");
    s.events.Clear();
    for (auto id:{81,82}) {
        const auto *clip=s.FindClip(id);
        editor::Value original{clip->start,clip->start+clip->duration,clip->sourceIn,clip->track,clip->speed};
        auto proposed=original;proposed.first+=editor::TicksPerSecond;proposed.last+=editor::TicksPerSecond;
        s.events.Push({static_cast<editor::StableId>(id),s.revision,editor::Phase::Commit,editor::EditKind::Move,original,proposed});
    }
    s.ApplyEvents();
    check(s.FindClip(81)->start==editor::FromSeconds(2) && s.FindClip(82)->start==editor::FromSeconds(4),
          "host commits complete multi-clip move preserving spacing");
    check(s.Undo() && s.FindClip(81)->start==editor::FromSeconds(1) && s.FindClip(82)->start==editor::FromSeconds(3),
          "one undo restores complete multi-clip move");
    const auto selected=p.editing.box(&s,{editor::FromSeconds(1.5),editor::FromSeconds(3.5)},0,60);
    check(selected.size()==2,"rectangle intersects clips even without their centers");
    s.trackSelection.Set(71);s.trackSelection.Set(72,true);
    const auto oldTracks=s.tracks.size();
    edit(0,editor::EditKind::TrackEdit,{0,0,static_cast<editor::Tick>(video::TrackAction::Add),0,static_cast<double>(video::TrackKind::Effect)});
    check(s.tracks.size()==oldTracks+1,"host adds requested track kind");
    check(s.Undo() && s.tracks.size()==oldTracks && s.trackSelection.count==2,"track undo restores independent selection");
    s.events.Clear();
    for(auto id:{71,72}) {editor::Event e{static_cast<editor::StableId>(id),s.revision,editor::Phase::Commit,editor::EditKind::TrackEdit};e.proposed.offset=static_cast<int>(video::TrackAction::Reorder);e.proposed.parent=74;e.operation=71;e.operationSize=2;s.events.Push(e);}
    s.ApplyEvents();check(s.tracks[0].id==73 && s.tracks[1].id==71 && s.tracks[2].id==72,"multi-track reorder preserves relative order");
    check(s.Undo() && s.tracks[0].id==71,"multi-track reorder has one undo");
    s.events.Clear();editor::Event partial{71,s.revision,editor::Phase::Commit,editor::EditKind::TrackEdit};partial.proposed.offset=static_cast<int>(video::TrackAction::Remove);partial.operation=71;partial.operationSize=2;s.events.Push(partial);s.ApplyEvents();
    check(s.tracks.size()==oldTracks && s.FindClip(81),"partial track batch is rejected");
    s.selection.Set(81);s.selection.Set(82,true);
    edit(81,editor::EditKind::Clipboard,{0,0,static_cast<int>(video::ClipboardAction::Copy)});
    check(s.clipboard.size()==2,"copy captures selected clips");
    const auto oldClips=s.clips.size();
    edit(81,editor::EditKind::Clipboard,{editor::FromSeconds(8),0,static_cast<int>(video::ClipboardAction::Paste),72,static_cast<double>(video::PlacementMode::Insert)});
    check(s.clips.size()==oldClips+2 && s.selection.count==2,"paste creates independently identified clips");
    check(s.FindClip(s.selection.active)->track==72,"paste uses destination track");
    check(s.Undo() && s.clips.size()==oldClips,"paste is one undo operation");
    edit(71,editor::EditKind::TrackEdit,{0,0,static_cast<int>(video::TrackAction::Remove)});
    check(s.clips.empty() && s.tracks.size()==oldTracks-1,"track removal includes its clips");
    check(s.Undo() && s.FindClip(81) && s.tracks.size()==oldTracks,"track deletion undo restores clips");
    return failures ? 1 : 0;
}
int VerifyInspectorModel() {
    using namespace imkit;
    auto storage=std::make_unique<gallery::EditorWorkspaces>();
    auto &state=*storage;
    state.Initialize();
    int failures=0;
    auto check=[&](bool ok,const char *name) {
        std::printf("%s %s\n",ok?"PASS":"FAIL",name);
        if (!ok) ++failures;
    };
    {
        auto model=std::make_unique<imkit::gallery::EditorWorkspaces>();model->Initialize();
        model->assetSelection.Set(model->assets[0].id);model->placementTrack=model->tracks[0].id;
        model->timeline.time.playhead=imkit::editor::FromSeconds(1);
        const auto oldSize=model->clips.size(),oldRevision=model->revision;
        check(model->PlaceSource(imkit::video::PlacementMode::Insert),"source insert completes");
        const auto inserted=model->selection.active;
        check(model->clips.size()>oldSize && model->revision>oldRevision,"insert splits and creates material");
        check(model->Undo() && model->clips.size()==oldSize,"general undo restores insertion");
        check(model->Undo(true) && model->selection.active==inserted,"redo restores inserted selection");
        model->tracks[0].locked=true;
        const auto lockedSize=model->clips.size(),lockedRevision=model->revision;
        check(!model->PlaceSource(imkit::video::PlacementMode::Overwrite) && model->clips.size()==lockedSize && model->revision==lockedRevision,"locked source placement is atomic");
        model->tracks[0].locked=false;
        model->timeline.time.playhead=imkit::editor::FromSeconds(120);
        check(model->PlaceSource(imkit::video::PlacementMode::Append),"source append completes");
        check(model->Undo(),"source append undo");
        auto object=model->objects[1];
        model->events.Push({object.id,model->revision,imkit::editor::Phase::Commit,imkit::editor::EditKind::Translate,{}, {0,0,0,0,3,4,5}});
        model->ApplyEvents();check(model->objects[1].transform.translation.x==3,"transform host commit");
        check(model->Undo() && model->objects[1].transform.translation.x==object.transform.translation.x,"general undo restores transform");
    }
    {
        auto listStorage=std::make_unique<gallery::EditorWorkspaces>();auto &list=*listStorage;list.Initialize();
        const auto first=list.customProperties[0].id,second=list.customProperties[1].id;
        list.events.Push({first,list.revision,editor::Phase::Commit,editor::EditKind::Reorder,{},editor::Value{0,0,1,second}});list.ApplyEvents();
        check(list.customProperties[1].id==first && list.customProperties[0].id==second,"host reorders custom array by explicit sibling IDs");
        list.events.Push({first,list.revision,editor::Phase::Commit,editor::EditKind::Property,{},editor::Value{0,0,0,0,42}});list.ApplyEvents();
        check(list.customProperties[1].value==42,"array edit follows stable ID after reorder");
        list.clips.resize(3);for(int i=0;i<3;++i) {list.clips[i].linked=0;list.clips[i].group=0;list.clips[i].track=1;list.clips[i].start=i*10;list.clips[i].duration=10;}
        const auto removed=list.clips[0].id;list.clips[1].locked=true;
        list.events.Push({removed,list.revision,editor::Phase::Commit,editor::EditKind::RippleDelete});list.ApplyEvents();
        check(list.clips.size()==3 && list.clips[2].start==20,"locked follower rejects ripple delete atomically");
        list.clips[1].locked=false;
        list.events.Push({removed,list.revision,editor::Phase::Commit,editor::EditKind::RippleDelete});list.ApplyEvents();
        check(list.clips.size()==2 && list.clips[0].start==0 && list.clips[1].start==10,"ripple delete removes target and shifts followers once");
        auto track=std::find_if(list.tracks.begin(),list.tracks.end(),[](const auto &t){return t.kind==video::TrackKind::Caption;});
        list.events.Push({track->id,list.revision,editor::Phase::Commit,editor::EditKind::CaptionInsert});list.ApplyEvents();
        check(list.clips.size()==3 && list.selection.active==list.clips.back().id,"caption insertion creates and selects a host clip");
    }
    {
        auto markerStorage=std::make_unique<gallery::EditorWorkspaces>();auto &marker=*markerStorage;marker.Initialize();
        marker.events.Push({0,marker.revision,editor::Phase::Commit,editor::EditKind::Marker,{},editor::Value{100}});marker.ApplyEvents();
        const auto count=marker.markerCount,id=marker.markers[count-1].id;
        marker.events.Push({id,marker.revision,editor::Phase::Commit,editor::EditKind::Marker,{},editor::Value{200}});marker.ApplyEvents();
        check(marker.markerCount==count && marker.markers[count-1].tick==200,"host marker move updates existing ID");
        marker.events.Push({id,marker.revision,editor::Phase::Commit,editor::EditKind::Remove});marker.ApplyEvents();
        check(marker.markerCount==count-1,"host removes selected marker");
    }
    {
        auto colorStorage=std::make_unique<gallery::EditorWorkspaces>();auto &color=*colorStorage;color.Initialize();
        const auto id=color.colorCurveKeys[0][1].id;
        const float before=color.pixels[32].r;
        editor::Value value;value.first=editor::FromSeconds(.5);value.x=.8;
        color.events.Push({id,color.revision,editor::Phase::Cancel,editor::EditKind::Keyframe,{},value});
        color.ApplyEvents();check(color.pixels[32].r==before,"color curve Cancel preserves synthetic pixels");
        color.events.Push({id,color.revision,editor::Phase::Commit,editor::EditKind::Keyframe,{},value});
        color.ApplyEvents();check(color.colorCurveKeys[0][1].value==.8 && color.pixels[32].r>before+.2f,
            "color curve Commit updates host keys and synthetic scope source");
        check(color.pixels[32].g==0 && color.pixels[32].a==1,"red curve preserves other channels and alpha");
    }
    {
        auto affineStorage=std::make_unique<gallery::EditorWorkspaces>();auto &affine=*affineStorage;affine.Initialize();
        editor::Value value;value.x=2;value.y=3;value.z=4;value.affine={.2,.3,.4,.5,.6,.7};value.hasAffine=true;
        affine.events.Push({affine.objects[0].id,affine.revision,editor::Phase::Commit,editor::EditKind::Scale,{},value});
        affine.ApplyEvents();const auto &result=affine.objects[0].transform;
        check(result.scale.x==2 && result.rotation.z==.4 && result.shear.x==.5 && result.shear.z==.7,
              "host scale Commit preserves affine rotation and shear payload");
        const auto object=std::find_if(affine.objects.begin(),affine.objects.end(),[](const auto &o){return o.geometry!=0;});
        affine.objectSelection.Set(object->id);object->transform={{10,0,0},{},{1,1,1},{1,0,0}};
        affine.geometries.at(object->geometry).vertices={{{2,0,0}},{{4,2,0}}};
        const auto bounds=affine.SelectionPivot(cg::Pivot::Bounds),median=affine.SelectionPivot(cg::Pivot::Median);
        check(bounds.x==14 && bounds.y==1 && bounds.z==0,"Bounds pivot uses sheared geometry without including external object origin");
        check(median.x==10 && median.y==0,"Median pivot retains object-origin semantics");
    }
    {
        auto linkedStorage=std::make_unique<gallery::EditorWorkspaces>();
        auto &linked=*linkedStorage;linked.Initialize();
        linked.clips.resize(3);
        linked.clips[0].linked=123;linked.clips[1].linked=123;
        linked.clips[1].group=456;linked.clips[2].group=456;
        const std::array ids{linked.clips[0].id};
        auto members=linked.QuerySelectedClips(ids);
        check(members.size()==3,"selected clip query resolves transitive linked and group membership");
        const auto firstStart=linked.clips.front().start;const auto revisionBefore=linked.revision;
        linked.clips[1].locked=true;
        for (const auto &clip:linked.clips)
            linked.events.Push({clip.id,linked.revision,editor::Phase::Commit,editor::EditKind::Move,{},
                editor::Value{clip.start+100,clip.start+clip.duration+100,clip.sourceIn,clip.track,clip.speed}});
        linked.ApplyEvents();
        check(linked.clips.front().start==firstStart && linked.revision==revisionBefore,
              "late locked member rejects entire host clip commit batch before mutation");
        linked.clips[1].locked=false;
        const auto sourceCount=linked.clips.size();
        for (const auto &clip:linked.clips)
            linked.events.Push({clip.id,linked.revision,editor::Phase::Commit,editor::EditKind::Duplicate,{},
                editor::Value{editor::FromSeconds(100)}});
        linked.ApplyEvents();
        std::vector<video::ClipView> duplicates;
        for (const auto &clip:linked.clips) if (clip.start==editor::FromSeconds(100)) duplicates.push_back(clip);
        check(linked.clips.size()==sourceCount*2 && duplicates.size()==3 &&
              duplicates[0].linked!=123 && duplicates[0].linked!=0 && duplicates[0].linked==duplicates[1].linked &&
              duplicates[1].group!=456 && duplicates[1].group!=0 && duplicates[1].group==duplicates[2].group,
              "duplicate batch retains internal relationships with independent link and group IDs");
        check(linked.QuerySelectedClips(ids).size()==3,"original linked selection excludes duplicated relationship sets");
        linked.tracks.front().locked=true;members=linked.QuerySelectedClips(ids);
        check(members.size()==3 && std::all_of(members.begin(),members.end(),[](const auto &c){return c.locked;}),
              "related clip query propagates locked owner tracks");
        const auto model=linked.clips.front();
        for (int i=0;i<64;++i) {auto copy=model;copy.id=80000+i;linked.clips.push_back(copy);}
        check(linked.QuerySelectedClips(ids).empty(),"related selection scratch overflow returns no partial members");
    }
    {
        auto splitStorage=std::make_unique<gallery::EditorWorkspaces>();auto &split=*splitStorage;split.Initialize();
        split.clips.resize(2);
        for (auto &clip:split.clips) {clip.start=0;clip.duration=1000;clip.linked=321;clip.group=654;}
        split.RebuildClipIndex();
        const std::array originalIds{split.clips[0].id,split.clips[1].id};
        for (auto id:originalIds)
            split.events.Push({id,split.revision,editor::Phase::Commit,editor::EditKind::Split,{},editor::Value{500}});
        split.ApplyEvents();
        const auto originals=split.QuerySelectedClips(std::span<const editor::StableId>(originalIds).first(1));
        check(originals.size()==2 && originals[0].start==0 && originals[1].start==0,
              "split left relationship set excludes right halves");
        const auto right=std::find_if(split.clips.begin(),split.clips.end(),[](const auto &clip){return clip.start==500;});
        check(right!=split.clips.end(),"related split creates right halves");
        if (right!=split.clips.end()) {
            const auto rightId=right->id;
            const auto rights=split.QuerySelectedClips(std::span<const editor::StableId>(&rightId,1));
            check(rights.size()==2 && rights[0].start==500 && rights[1].start==500 &&
                  rights[0].linked!=321 && rights[0].group!=654 && rights[0].linked==rights[1].linked &&
                  rights[0].group==rights[1].group,"split right halves retain their own independent linked and group sets");
        }
    }
    {
        auto queryStorage=std::make_unique<gallery::EditorWorkspaces>();
        auto &query=*queryStorage;
        for (int i=0;i<100000;++i) {
            video::ClipView clip;clip.id=1000+i;clip.track=1;clip.start=i*100;clip.duration=10;
            query.clips.push_back(clip);
        }
        query.clips.front().start=-editor::TicksPerSecond*120;
        query.clips.front().duration=editor::TicksPerSecond*120+10000000;
        query.RebuildClipIndex();
        const auto capacity=query.visibleClips.capacity();
        auto visible=query.QueryClips(1,{9000000,9000005});
        check(visible.size()==2 && visible.front().id==1000 && visible.back().id==91000,
              "interval query includes long clip starting outside former 60-second lookback");
        check(query.clipQueryVisits<150 && query.visibleClips.capacity()==capacity,
              "sparse 100k clip query prunes ended clips without scratch growth");
        check(query.QueryClips(2,{0,10000000}).empty(),"clip interval query isolates track IDs");
        query.clips.front().duration=1;query.RebuildClipIndex();
        check(query.QueryClips(1,{9000000,9000005}).size()==1,"clip index rebuild reflects edited duration");
    }
    {
        auto copyStorage=std::make_unique<gallery::EditorWorkspaces>();
        auto &copies=*copyStorage;copies.Initialize();
        const auto original=copies.clips.front().id,property=copies.clipPropertyIds[1];
        copies.clipProperties.at(original).values[1]=1.75;
        copies.propertyFlags[property]=8;
        copies.propertyKeys[property]={{copies.nextId++,property,0,1.75}};
        copies.clipEnvelopes[original]={{copies.nextId++,0,.75},{copies.nextId++,1000,1}};
        copies.clips.front().envelope=copies.clipEnvelopes.at(original);
        auto duplicate=[&] {
            copies.events.Push({original,copies.revision,editor::Phase::Commit,editor::EditKind::Duplicate,{},
                editor::Value{editor::FromSeconds(100)}});copies.ApplyEvents();
        };
        const auto count=copies.clips.size();copies.tracks.front().locked=true;duplicate();
        check(copies.clips.size()==count,"locked track rejects host clip duplication");copies.tracks.front().locked=false;
        duplicate();
        const auto copy=std::find_if(copies.clips.begin(),copies.clips.end(),[&](const auto &c) {
            return c.track==copies.tracks.front().id && c.start==editor::FromSeconds(100);
        });
        check(copy!=copies.clips.end() && copies.clips.size()==count+1,"host duplicates clip");
        if (copy!=copies.clips.end()) {
            const auto copyProperty=copies.clipProperties.at(copy->id).ids[1];
            check(copyProperty!=property && copies.clipProperties.at(copy->id).values[1]==1.75 &&
                  copies.propertyFlags.at(copyProperty)==8 && copies.propertyKeys.at(copyProperty).front().channel==copyProperty &&
                  copies.propertyKeys.at(copyProperty).front().id!=copies.propertyKeys.at(property).front().id,
                  "duplicate copies Inspector values flags and property keys with independent IDs");
            check(copy->keyChannel!=copies.clips.front().keyChannel && !copy->keys.empty() &&
                  copy->keys.front().id!=copies.clips.front().keys.front().id &&
                  copy->keys.front().value==copies.clips.front().keys.front().value,
                  "duplicate receives independent inline key channel and keys");
            copies.clipEnvelopes.at(copy->id).front().gain=.25;
            check(copy->envelope.front().id!=copies.clips.front().envelope.front().id &&
                  copies.clips.front().envelope.front().gain==.75,
                  "duplicate envelope edits preserve original clip");
        }
    }
    {
        auto splitStorage=std::make_unique<gallery::EditorWorkspaces>();auto &split=*splitStorage;split.Initialize();
        const auto id=split.clips.front().id;const auto start=split.clips.front().start;
        split.clipEnvelopes[id]={{split.nextId++,0,.5},{split.nextId++,1000,1.5}};
        split.clips.front().envelope=split.clipEnvelopes.at(id);
        split.clips.front().transitionIn=800;split.clips.front().transitionOut=1000;
        split.clips.front().transitionInKind=video::TransitionKind::Fade;
        split.clips.front().transitionOutKind=video::TransitionKind::Dissolve;
        const auto sourceChannel=split.clips.front().keyChannel;
        split.keys.push_back({split.nextId++,sourceChannel,1000,.625});split.RebuildKeyIndex();
        split.clipProperties.at(id).values[1]=1.25;
        const std::vector<editor::Keyframe> beforeKeys(split.clips.front().keyEvaluation.begin(),split.clips.front().keyEvaluation.end());
        auto applySplit=[&] {
            split.events.Push({id,split.revision,editor::Phase::Commit,editor::EditKind::Split,{},editor::Value{start+500}});
            split.ApplyEvents();
        };
        const auto count=split.clips.size();split.tracks.front().locked=true;applySplit();
        check(split.clips.size()==count,"locked track rejects host clip split");split.tracks.front().locked=false;applySplit();
        const auto right=std::find_if(split.clips.begin(),split.clips.end(),[&](const auto &c){return c.track==split.tracks.front().id && c.start==start+500;});
        check(right!=split.clips.end() && split.clips.size()==count+1,"host splits clip at requested tick");
        if (right!=split.clips.end()) {
            const auto leftClip=std::find_if(split.clips.begin(),split.clips.end(),[&](const auto &c){return c.id==id;});
            check(leftClip->transitionIn==500 && leftClip->transitionInKind==video::TransitionKind::Fade &&
                  leftClip->transitionOut==0 && leftClip->transitionOutKind==video::TransitionKind::None &&
                  right->transitionIn==0 && right->transitionInKind==video::TransitionKind::None &&
                  right->transitionOut==1000 && right->transitionOutKind==video::TransitionKind::Dissolve,
                  "split preserves outer transitions with duration clamp and clears newly cut edges");
            check(!right->keyEvaluation.empty() && right->keyEvaluation.front().tick<0 &&
                  std::abs(editor::Evaluate(right->keyEvaluation,0)-editor::Evaluate(beforeKeys,500))<1e-10 &&
                  std::abs(editor::Evaluate(right->keyEvaluation,250)-editor::Evaluate(beforeKeys,750))<1e-10,
                  "split evaluation retains outside keys and preserves boundary interpolation");
            const auto rightKey=std::find_if(right->keys.begin(),right->keys.end(),[](const auto &key){return key.tick==500 && key.value==.625;});
            check(right->keyChannel!=sourceChannel && rightKey!=right->keys.end() &&
                  split.clipProperties.at(right->id).values[1]==1.25 &&
                  split.clipProperties.at(right->id).ids[1]!=split.clipProperties.at(id).ids[1],
                  "split copies Inspector and rebases independent right key channel");
            const auto &left=split.clipEnvelopes.at(id);
            check(left.size()==2 && left.back().tick==500 && left.back().gain==1 &&
                  right->envelope.size()==2 && right->envelope.front().tick==0 && right->envelope.front().gain==1 &&
                  right->envelope.back().tick==500 && right->envelope.back().gain==1.5 &&
                  right->envelope.front().id!=left.back().id,
                  "split interpolates envelope boundary and shifts independent right points to local time");
            check(video::EvaluateEnvelope(left,250)==.75 && video::EvaluateEnvelope(right->envelope,250)==1.25,
                  "split envelope preserves original gain on both sides");
        }
    }
    {
        auto rippleStorage=std::make_unique<gallery::EditorWorkspaces>();auto &ripple=*rippleStorage;ripple.Initialize();
        const auto source=ripple.clips.front(),following=ripple.clips[1];const auto beforeRevision=ripple.revision;
        auto apply=[&] {
            ripple.events.Push({source.id,ripple.revision,editor::Phase::Commit,editor::EditKind::Ripple,{},
                {source.start,source.start+source.duration+100,source.sourceIn,source.track,source.speed}});ripple.ApplyEvents();
        };
        ripple.clips[1].locked=true;apply();
        check(ripple.revision==beforeRevision && ripple.clips.front().duration==source.duration &&
              ripple.clips[1].start==following.start,"locked following clip rejects whole host ripple before source trim");
        ripple.clips[1].locked=false;apply();
        check(ripple.clips.front().duration==source.duration+100 && ripple.clips[1].start==following.start+100,
              "unlocked ripple applies source trim and following shift together");
        const auto first=ripple.clips[0],second=ripple.clips[1],third=ripple.clips[2];
        for (const auto &clip : {second,first}) ripple.events.Push({clip.id,ripple.revision,editor::Phase::Commit,editor::EditKind::Ripple,{},
            {clip.start,clip.start+clip.duration+50,clip.sourceIn,clip.track,clip.speed}});
        ripple.ApplyEvents();
        check(ripple.clips[0].duration==first.duration+50 && ripple.clips[1].duration==second.duration+50 &&
              ripple.clips[1].start==second.start+50 && ripple.clips[2].start==third.start+100,
              "multiple ripple trims sum original follower shifts regardless of commit order");
    }
    const auto firstClipId=state.clips.front().id,secondClipId=state.clips[1].id;
    const auto firstScaleId=state.clipPropertyIds[1];
    state.events.Push({firstScaleId,state.revision,editor::Phase::Commit,editor::EditKind::Property,{},editor::Value{0,0,0,0,1.5}});state.ApplyEvents();
    state.selection.Set(secondClipId);state.SyncClipProperties();
    check(state.clipPropertyIds[1]!=firstScaleId && state.clipPropertyValues[1]==1,"clip selection uses independent property IDs and defaults");
    state.events.Push({firstScaleId,state.revision,editor::Phase::Commit,editor::EditKind::Property,{},editor::Value{0,0,0,0,2}});state.ApplyEvents();
    state.selection.Set(firstClipId);state.SyncClipProperties();
    check(state.clipPropertyValues[1]==1.5,"clip property value survives selection and rejects stale owner edit");
    state.tracks.front().locked=true;
    state.events.Push({firstScaleId,state.revision,editor::Phase::Commit,editor::EditKind::Property,{},editor::Value{0,0,0,0,2}});state.ApplyEvents();
    check(state.clipProperties.at(firstClipId).values[1]==1.5,"locked track rejects clip Inspector edit");state.tracks.front().locked=false;
    const auto lockedClip=state.clips.front();const auto lockedRevision=state.revision;
    state.tracks.front().locked=true;
    for (auto kind:{editor::EditKind::Move,editor::EditKind::TrimStart,editor::EditKind::TrimEnd,
                   editor::EditKind::Ripple,editor::EditKind::Roll,editor::EditKind::Slip,editor::EditKind::Slide}) {
        state.events.Push({lockedClip.id,state.revision,editor::Phase::Commit,kind,{},
            {lockedClip.start+1,lockedClip.start+lockedClip.duration+1,lockedClip.sourceIn+1,lockedClip.track,lockedClip.speed}});
        state.ApplyEvents();
        check(state.revision==lockedRevision && state.clips.front().start==lockedClip.start &&
              state.clips.front().duration==lockedClip.duration && state.clips.front().sourceIn==lockedClip.sourceIn,
              "locked track rejects host clip edit without changing revision");
    }
    state.tracks.front().locked=false;
    const auto unchangedRevision=state.revision;
    const auto &unchangedClip=state.clips.front();
    editor::Value sameClip{unchangedClip.start,unchangedClip.start+unchangedClip.duration,unchangedClip.sourceIn,unchangedClip.track,unchangedClip.speed};
    for (auto kind:{editor::EditKind::Move,editor::EditKind::TrimStart,editor::EditKind::TrimEnd,
                   editor::EditKind::Ripple,editor::EditKind::Roll,editor::EditKind::Slip,editor::EditKind::Slide}) {
        state.events.Push({unchangedClip.id,state.revision,editor::Phase::Commit,kind,sameClip,sameClip});state.ApplyEvents();
        check(state.revision==unchangedRevision,"unchanged clip edit preserves host revision and skips index rebuild");
    }
    const auto clipChannel=state.clips.front().keyChannel;
    const auto countBeforeInsert=state.keys.size();
    editor::Event clipInsert{clipChannel,state.revision,editor::Phase::Commit,editor::EditKind::KeyInsert};
    clipInsert.proposed.parent=state.clips.front().id;clipInsert.proposed.first=12345;clipInsert.proposed.x=.4;
    state.events.Push(clipInsert);state.ApplyEvents();
    check(state.keys.size()==countBeforeInsert+1 && std::any_of(state.clips.front().keys.begin(),state.clips.front().keys.end(),
        [](const auto &key){return key.tick==12345 && key.value==.4;}),"clip insertion applies to explicit host channel and refreshes span");
    const auto savedKeys=state.keys;
    std::erase_if(state.keys,[&](const auto &key){return key.channel==clipChannel;});state.RebuildKeyIndex();
    check(state.clips.front().keyChannel==clipChannel && state.clips.front().keys.empty(),
          "empty clip channel keeps its explicit ID instead of adopting another channel");
    clipInsert.revision=state.revision;state.events.Push(clipInsert);state.ApplyEvents();
    check(state.clips.front().keys.size()==1 && state.clips.front().keys.front().channel==clipChannel,
          "first key can be inserted again into an emptied clip channel");
    state.clips[1].keyChannel=clipChannel;
    state.keys=savedKeys;state.RebuildKeyIndex();
    check(state.clips[1].keys.data()==state.clips.front().keys.data() && !state.clips[1].keys.empty(),
          "all clips referencing an explicit channel refresh their borrowed key spans");
    state.clips[1].keyChannel=0;state.RebuildKeyIndex();
    const auto transitionClip=state.clips.front().id;
    state.events.Push({transitionClip,state.revision,editor::Phase::Commit,editor::EditKind::TransitionDuration,{},
        {editor::FromSeconds(.2),editor::FromSeconds(.3)}});state.ApplyEvents();
    check(state.clips.front().transitionIn==editor::FromSeconds(.2) && state.UndoTransition() &&
          state.clips.front().transitionIn==0,"transition duration host apply and Undo");
    check(state.UndoTransition(true) && state.clips.front().transitionOut==editor::FromSeconds(.3),"transition duration Redo");
    state.events.Push({transitionClip,state.revision,editor::Phase::Commit,editor::EditKind::TransitionType,{}, {1,3}});state.ApplyEvents();
    check(state.clips.front().transitionInKind==video::TransitionKind::Dissolve && state.UndoTransition() &&
          state.clips.front().transitionInKind==video::TransitionKind::Fade,"transition kind host apply and Undo");
    state.UndoTransition();
    auto audioClip=std::find_if(state.clips.begin(),state.clips.end(),[](const auto &c){return !c.envelope.empty();});
    const auto envelopeCount=audioClip->envelope.size();
    auto envelopeEdit=[&](editor::StableId target,editor::Tick action,editor::Tick tick,double gain) {
        editor::Event event{target,state.revision,editor::Phase::Commit,editor::EditKind::AudioEnvelope};
        event.proposed.parent=audioClip->id;event.proposed.offset=action;event.proposed.first=tick;event.proposed.x=gain;
        state.events.Push(event);state.ApplyEvents();
    };
    envelopeEdit(audioClip->id,1,editor::FromSeconds(2),.8);
    auto addedPoint=std::find_if(audioClip->envelope.begin(),audioClip->envelope.end(),[](const auto &p){return p.tick==editor::FromSeconds(2);});
    check(audioClip->envelope.size()==envelopeCount+1 && addedPoint!=audioClip->envelope.end(),"envelope insert refreshes host span");
    if (addedPoint!=audioClip->envelope.end()) {
        const auto pointId=addedPoint->id;
        envelopeEdit(pointId,0,editor::FromSeconds(2.2),1.4);
        check(std::any_of(audioClip->envelope.begin(),audioClip->envelope.end(),[&](const auto &p){return p.id==pointId && p.gain==1.4;}),"envelope edit preserves point identity");
        envelopeEdit(pointId,2,0,1);
        check(audioClip->envelope.size()==envelopeCount,"envelope remove refreshes host span");
    }
    const auto rendererComponent=state.components[0].view.id,wireComponent=state.components[1].view.id;
    state.events.Push({wireComponent,state.revision,editor::Phase::Commit,editor::EditKind::Toggle,{}, {0,0,0,0,0,1}});state.ApplyEvents();
    check(state.BuildSceneMeshes().front().wire,"enabled wire component changes preview rendering");
    state.events.Push({wireComponent,state.revision,editor::Phase::Commit,editor::EditKind::Reorder,{},
        {0,0,-1,state.objects[1].id}});state.ApplyEvents();
    check(!state.BuildSceneMeshes().front().wire,"component order changes renderer override result");
    state.events.Push({rendererComponent,state.revision,editor::Phase::Commit,editor::EditKind::Toggle,{}, {0,0,0,0,0,0}});state.ApplyEvents();
    const auto disabledMeshes=state.BuildSceneMeshes();
    check(std::none_of(disabledMeshes.begin(),disabledMeshes.end(),[&](const auto &mesh){return mesh.id==state.objects[1].id;}),"disabled renderer component removes preview mesh");
    state.events.Push({rendererComponent,state.revision,editor::Phase::Commit,editor::EditKind::Toggle,{}, {0,0,0,0,0,1}});state.ApplyEvents();
    state.events.Push({wireComponent,state.revision,editor::Phase::Commit,editor::EditKind::Reorder,{},
        {0,0,1,state.objects[1].id}});state.ApplyEvents();
    state.events.Push({wireComponent,state.revision,editor::Phase::Commit,editor::EditKind::Toggle,{}, {0,0,0,0,0,0}});state.ApplyEvents();
    state.RebuildOutlinerRows();check(state.outlinerRows.size()==state.objects.size()+state.components.size(),"Outliner expanded hierarchy includes all rows");
    state.objects[0].expanded=false;state.RebuildOutlinerRows();
    check(state.outlinerRows.size()==1,"Outliner collapse hides descendants");
    std::snprintf(state.outliner.search,sizeof(state.outliner.search),"Camera");state.RebuildOutlinerRows();
    check(state.outlinerRows.size()==2 && state.outlinerRows[1].id==state.objects[3].id &&
          state.outlinerRows[1].depth==1,"Outliner search includes match and ancestor through collapsed hierarchy");
    state.outliner.search[0]=0;state.objects[0].expanded=true;
    const auto oldParent=state.objects[3].parent;state.objects[3].parent=state.objects[1].id;
    state.RebuildOutlinerRows();
    check(state.outlinerRows.size()==state.objects.size()+state.components.size() && state.outlinerRows[2].id==state.objects[3].id &&
          state.outlinerRows[2].depth==2 && state.outlinerRows[1].hasChildren,"Outliner rebuild reflects reparented hierarchy");
    state.objects[3].parent=oldParent;
    const auto initialOrder=state.objectOrder;
    editor::Event reorderObject{state.objects[2].id,state.revision,editor::Phase::Commit,editor::EditKind::Reorder};
    reorderObject.proposed.parent=state.objects[2].parent;reorderObject.proposed.offset=-1;
    state.events.Push(reorderObject);state.ApplyEvents();state.RebuildOutlinerRows();
    check(state.outlinerRows[1].id==state.objects[2].id && state.objects[1].id==1000001,
          "Outliner sibling reorder preserves object storage identity");
    state.objects[1].locked=true;reorderObject.revision=state.revision;reorderObject.proposed.offset=1;
    state.events.Push(reorderObject);state.ApplyEvents();
    check(state.objectOrder[1]==state.objects[2].id,"Outliner reorder cannot cross locked sibling");
    state.objects[1].locked=false;state.objectOrder=initialOrder;
    state.objectSelection.Set(state.objects[1].id);
    state.objectSelection.Set(state.objects[2].id,true);
    auto send=[&](editor::Phase phase,int component,double value,editor::EditKind kind=editor::EditKind::Property) {
        state.events.Clear();
        state.events.Push({state.objectPropertyIds[1][component],state.revision,phase,kind,{},editor::Value{0,0,0,0,value}});
        state.ApplyEvents();
    };
    send(editor::Phase::Begin,4,.6);send(editor::Phase::Commit,4,.6);
    check(state.objects[1].transform.rotation.y==.6 && state.objects[2].transform.rotation.y==.6,
          "explicit rotation property applies to selected objects");
    send(editor::Phase::Begin,8,1.8);send(editor::Phase::Commit,8,1.8);
    check(state.objects[1].transform.scale.z==1.8 && state.objects[2].transform.scale.z==1.8,
          "scale property applies to selected objects");
    send(editor::Phase::Commit,8,1,editor::EditKind::Reset);
    check(state.objects[1].transform.scale.z==1 && state.objects[2].transform.scale.z==1,"scale reset uses unit value");
    state.objects[2].locked=true;
    send(editor::Phase::Begin,4,2);send(editor::Phase::Commit,4,2);
    check(state.objects[1].transform.rotation.y==.6 && state.objects[2].transform.rotation.y==.6,
          "locked selected object rejects whole edit");
    state.objects[2].locked=false;
    state.propertyFlags[state.objectPropertyIds[2][4]]=16;
    send(editor::Phase::Begin,4,2);send(editor::Phase::Commit,4,2);
    check(state.objects[1].transform.rotation.y==.6,"locked selected property rejects whole edit");
    state.propertyFlags[state.objectPropertyIds[2][4]]=0;
    send(editor::Phase::Begin,4,2);
    state.objectSelection.Set(state.objects[1].id);
    send(editor::Phase::Commit,4,2);
    check(state.objects[1].transform.rotation.y==.6,"selection change rejects gesture commit");
    auto key=[&](editor::PropertyKeyAction action,editor::Tick tick) {
        state.events.Clear();
        state.events.Push({state.objectPropertyIds[1][4],state.revision,editor::Phase::Commit,
            editor::EditKind::PropertyKey,{},editor::Value{tick,0,static_cast<editor::Tick>(action),0,.6}});
        state.ApplyEvents();
    };
    key(editor::PropertyKeyAction::Add,100);key(editor::PropertyKeyAction::Add,200);
    check(state.propertyKeys[state.objectPropertyIds[1][4]].size()==2,"property keys stored in host channel");
    key(editor::PropertyKeyAction::Previous,200);
    check(state.timeline.time.playhead==100,"previous property key navigation");
    key(editor::PropertyKeyAction::Next,100);
    check(state.timeline.time.playhead==200,"next property key navigation");
    key(editor::PropertyKeyAction::Remove,100);
    check(state.propertyKeys[state.objectPropertyIds[1][4]].size()==1,"property key removal");
    const auto originalKey=state.keys[1];
    const auto keyCount=state.keys.size();
    state.events.Clear();
    state.events.Push({originalKey.id,state.revision,editor::Phase::Commit,editor::EditKind::Duplicate,{},
        editor::Value{originalKey.tick+editor::TicksPerSecond,0,0,0,originalKey.value+.1}});
    state.ApplyEvents();
    auto source=std::find_if(state.keys.begin(),state.keys.end(),[&](const auto &key){return key.id==originalKey.id;});
    auto duplicate=std::find_if(state.keys.begin(),state.keys.end(),[&](const auto &key){return key.id!=originalKey.id &&
        key.channel==originalKey.channel && key.tick==originalKey.tick+editor::TicksPerSecond && key.value==originalKey.value+.1;});
    check(state.keys.size()==keyCount+1 && source!=state.keys.end() && source->tick==originalKey.tick && duplicate!=state.keys.end(),
        "curve duplication assigns a new ID and preserves source key");
    state.Dataset(true);
    auto visible=state.QueryKeys({{editor::FromSeconds(100),editor::FromSeconds(101)},-10,10});
    check(state.keyChannels.size()==3 && visible.size()<32,"100k curve query returns bounded keys from three channels");
    bool ordered=true;
    for (std::size_t i=1;i<visible.size();++i)
        ordered &= visible[i-1].channel<visible[i].channel ||
            (visible[i-1].channel==visible[i].channel && visible[i-1].tick<=visible[i].tick);
    check(ordered,"curve query keeps each channel contiguous and time sorted");
    bool matches=true;
    for (auto mode:{editor::HandleMode::Auto,editor::HandleMode::AutoClamped,editor::HandleMode::Vector,
                   editor::HandleMode::Aligned,editor::HandleMode::Free}) {
        for (auto &key:state.keys) key.handles=mode;
        visible=state.QueryKeys({{editor::FromSeconds(100.1),editor::FromSeconds(100.9)},-10,10});
        for (auto [first,last]:state.keyChannels) {
            auto channel=std::span<const editor::Keyframe>(state.keys).subspan(first,last-first);
            auto a=std::find_if(visible.begin(),visible.end(),[&](const auto &key){return key.channel==channel.front().channel;});
            auto b=std::find_if(a,visible.end(),[&](const auto &key){return key.channel!=channel.front().channel;});
            for (double seconds:{100.1,100.25,100.75,100.9}) {
                const auto sample=editor::FromSeconds(seconds);
                matches &= std::abs(editor::Evaluate(channel,sample)-editor::Evaluate({a,b},sample))<1e-9;
            }
        }
    }
    check(matches,"five handle modes preserve full-channel evaluation at visible boundaries");
    const auto stripId=state.animationStrips[0].id,neighborId=state.animationStrips[1].id;
    state.events.Clear();
    state.events.Push({stripId,state.revision,editor::Phase::Update,editor::EditKind::StripSettings,{},
        {0,0,1,0,2,3,.4}});state.ApplyEvents();
    check(state.animationStrips[0].scale==1,"strip Update preserves host settings");
    state.events.Clear();
    state.events.Push({stripId,state.revision,editor::Phase::Commit,editor::EditKind::StripSettings,{},
        {0,0,1,0,2,3,.4}});state.ApplyEvents();
    check(state.animationStrips[0].scale==2 && state.animationStrips[0].repeat==3 &&
        state.animationStrips[0].blend==.4 && state.animationStrips[0].muted,"strip Commit applies settings");
    state.events.Clear();
    state.events.Push({stripId,state.revision,editor::Phase::Commit,editor::EditKind::Reorder,{},
        {0,0,1,neighborId}});state.ApplyEvents();
    check(state.animationStrips[1].id==stripId && state.animationStrips[0].id==neighborId,
        "strip Reorder swaps host order by StableId");
    state.animationStrips[1].locked=true;
    state.events.Clear();
    state.events.Push({stripId,state.revision,editor::Phase::Commit,editor::EditKind::StripSettings,{},
        {0,0,0,0,8,9,.8}});state.ApplyEvents();
    check(state.animationStrips[1].locked && state.animationStrips[1].scale==2,
        "locked strip rejects simultaneous unlock and settings mutation");
    state.events.Clear();
    state.events.Push({stripId,state.revision,editor::Phase::Commit,editor::EditKind::StripSettings,{},
        {0,0,1,0,2,3,.4}});state.ApplyEvents();
    check(!state.animationStrips[1].locked,"locked strip accepts explicit unchanged-settings unlock");
    {
        auto tree=std::make_unique<gallery::EditorWorkspaces>();tree->Initialize();
        tree->outliner.kindFilter=static_cast<int>(cg::ObjectKind::Modifier);tree->RebuildOutlinerRows();
        check(tree->outlinerRows.size()==3 && tree->outlinerRows.back().kind==cg::ObjectKind::Modifier,
              "Outliner modifier filter preserves collection and owner path");
        const auto sourceComponent=tree->components[1].view.id;
        tree->events.Push({sourceComponent,tree->revision,editor::Phase::Commit,editor::EditKind::Duplicate});tree->ApplyEvents();
        const auto duplicateComponent=tree->components.back().view.id;
        check(duplicateComponent!=sourceComponent && tree->objectSelection.active==duplicateComponent,
              "Outliner component duplicate owns a new selected stable ID");
        tree->events.Push({duplicateComponent,tree->revision,editor::Phase::Commit,editor::EditKind::Reparent,{},
            {0,0,0,tree->objects[2].id}});tree->ApplyEvents();
        check(tree->components.back().view.owner==tree->objects[2].id,"Outliner component reparent updates stack owner");
    }
    const auto sourceId=state.objects[1].id;const auto sourceX=state.objects[1].transform.translation.x;
    const auto meshCountBeforeDuplicate=state.BuildSceneMeshes().size();
    state.events.Push({sourceId,state.revision,editor::Phase::Commit,editor::EditKind::Duplicate});state.ApplyEvents();
    check(state.objects.size()==5 && state.objects.back().id!=sourceId && state.objectSelection.active==state.objects.back().id,
          "Outliner duplicate creates and selects a new object ID");
    bool distinctProperties=true;
    for (auto id:state.objectPropertyIds.back()) for (std::size_t i=0;i+1<state.objectPropertyIds.size();++i)
        for (auto previous:state.objectPropertyIds[i]) distinctProperties &= id!=previous;
    check(distinctProperties && state.BuildSceneMeshes().size()==meshCountBeforeDuplicate+1,"duplicate has independent property IDs and a preview mesh");
    state.events.Push({state.objects.back().id,state.revision,editor::Phase::Commit,editor::EditKind::Translate,{},
        {0,0,0,0,12,0,0}});state.ApplyEvents();
    check(state.objects.back().transform.translation.x==12 && state.objects[1].transform.translation.x==sourceX,
          "duplicate transform edit preserves source object");
    const auto independentGeometry=state.objects.back().geometry, sourceGeometry=state.objects[1].geometry;
    const auto independentX=state.geometries.at(independentGeometry).vertices.front().position[0];
    editor::Event linked{sourceId,state.revision,editor::Phase::Commit,editor::EditKind::Duplicate};linked.proposed.offset=1;
    state.events.Push(linked);state.ApplyEvents();
    const auto linkedId=state.objects.back().id;
    check(state.objects.back().geometry==sourceGeometry && independentGeometry!=sourceGeometry,
          "linked duplicate shares geometry while ordinary duplicate owns copied data");
    state.geometries.at(sourceGeometry).vertices.front().position[0]+=.25f;
    auto meshes=state.BuildSceneMeshes();
    auto linkedMesh=std::find_if(meshes.begin(),meshes.end(),[&](const auto &v){return v.id==linkedId;});
    check(linkedMesh!=meshes.end() && linkedMesh->vertices.front().position[0]==independentX+.25f &&
          state.geometries.at(independentGeometry).vertices.front().position[0]==independentX,
          "geometry edits reach linked preview and preserve independent duplicate");
    state.events.Push({linkedId,state.revision,editor::Phase::Commit,editor::EditKind::LinkGeometry});state.ApplyEvents();
    check(state.objects.back().geometry!=sourceGeometry &&
          state.geometries.at(state.objects.back().geometry).vertices.front().position[0]==independentX+.25f,
          "make single user copies current shared geometry");
    editor::Event relink{linkedId,state.revision,editor::Phase::Commit,editor::EditKind::LinkGeometry};relink.proposed.parent=sourceId;
    state.events.Push(relink);state.ApplyEvents();
    check(state.objects.back().geometry==sourceGeometry,"link geometry reconnects to active object data");
    const auto componentTotal=state.components.size();
    state.events.Push({sourceId,state.revision,editor::Phase::Commit,editor::EditKind::ComponentAdd,{}, {0,0,0,7802}});state.ApplyEvents();
    check(state.components.size()==componentTotal+1 && state.components.back().view.owner==sourceId &&
          state.components.back().wireOverride,"component add creates selected host type for owner");
    state.objects[1].locked=true;
    state.events.Push({sourceId,state.revision,editor::Phase::Commit,editor::EditKind::ComponentAdd,{}, {0,0,0,7801}});state.ApplyEvents();
    check(state.components.size()==componentTotal+1,"locked object rejects component addition");
    state.objects[1].locked=false;
    std::puts("Evidence: host model/event application; no native OS or GUI input.");
    return failures?1:0;
}
} // namespace imkit::gallery::host
