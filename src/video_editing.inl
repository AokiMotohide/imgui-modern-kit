// Included inside imkit::video; all edit state and data belong to the caller.
double EvaluateFade(Tick tick, Tick duration, const FadeView &fade) {
    if (duration<=0 || tick<0 || tick>duration || fade.inDuration<0 || fade.outDuration<0 || fade.inDuration>duration || fade.outDuration>duration-fade.inDuration) return 0;
    const auto curve=[](double v,FadeCurve c) {
        v=std::clamp(v,0.,1.);
        return c==FadeCurve::EaseIn ? v*v : c==FadeCurve::EaseOut ? 1-(1-v)*(1-v) : v;
    };
    return (fade.inDuration>0 ? curve(double(tick)/fade.inDuration,fade.inCurve) : 1.) *
        (fade.outDuration>0 ? curve(double(duration-tick)/fade.outDuration,fade.outCurve) : 1.);
}
void TransitionShelf(const char *id,const IconAtlas *icons) {
    ImGui::PushID(id);
    const char *names[]{"Dissolve","Crossfade","Dip to black"};
    const TransitionKind kinds[]{TransitionKind::Dissolve,TransitionKind::Crossfade,TransitionKind::Fade};
    for (int i=0;i<3;++i) {
        if(i) ImGui::SameLine();
        if(icons) {Icon(*icons,i==0 ? IconId::Dissolve : i==1 ? IconId::Crossfade : IconId::FadeOut,{16});ImGui::SameLine();}
        ImGui::Button(names[i]);
        if(ImGui::BeginDragDropSource()) {
            const auto kind=kinds[i];
            ImGui::SetDragDropPayload("IMKIT_CUT_TRANSITION",&kind,sizeof(kind));
            ImGui::TextUnformatted(names[i]);ImGui::EndDragDropSource();
        }
    }
    ImGui::PopID();
}
namespace {
bool EditingAction(editor::EventBuffer &out,StableId id,std::uint64_t revision,
                   editor::EditKind kind,editor::Value original,editor::Value proposed) {
    if(out.storage.size()-out.count<2) {out.overflow=true;return false;}
    editor::Transaction tx;
    if(!tx.Begin(id,revision,kind,original,editor::CurrentModifiers(),out)) return false;
    tx.draft.proposed=proposed;return tx.Commit(revision,out);
}
bool FadeControls(const ClipView &clip,const TrackView &track,const TimelineProvider &p,
                  TimelineState &s,ImVec2 a,ImVec2 b,editor::EventBuffer &out,const Theme &theme) {
    const auto *source=p.editing.fades ? p.editing.fades(p.editing.user,clip.id) : nullptr;
    if(!source || clip.duration<=0 || source->inDuration<0 || source->outDuration<0 || source->inDuration>clip.duration || source->outDuration>clip.duration-source->inDuration) return false;
    editor::Value original;original.first=source->inDuration;original.last=source->outDuration;
    original.x=static_cast<int>(source->inCurve);original.y=static_cast<int>(source->outCurve);
    auto value=original;
    auto &tx=s.fadeDrag;
    if(tx.active && tx.draft.target==clip.id) {
        if(track.locked || clip.locked || tx.draft.revision!=p.revision || ImGui::IsKeyPressed(ImGuiKey_Escape)) tx.Cancel(out);
        else if(tx.draft.phase!=editor::Phase::Cancel && tx.draft.phase!=editor::Phase::Commit) {
            value=tx.draft.original;
            auto &length=s.fadeEnd ? value.last : value.first;
            const auto other=s.fadeEnd ? value.first : value.last;
            const auto delta=editor::FromSeconds((ImGui::GetIO().MousePos.x-s.fadeMouseStart)/s.canvas.scale.x*(s.fadeEnd ? -1 : 1));
            length+=std::clamp(delta,-length,std::max<Tick>(0,clip.duration-other-length));
            if(!(value==tx.draft.proposed)) tx.Update(p.revision,value,out);
            if(ImGui::IsMouseReleased(0)) tx.Commit(p.revision,out);
        }
    }
    bool hit=false;
    auto *draw=ImGui::GetWindowDrawList();const float scale=ImGui::GetFontSize()/14.f;
    ImGui::PushID(static_cast<int>(clip.id));ImGui::PushID("fades");
    for(int side=0;side<2;++side) {
        const auto length=side ? value.last : value.first;
        const float edge=side ? b.x : a.x;
        const float x=edge+float(editor::Seconds(length)*s.canvas.scale.x)*(side ? -1 : 1);
        const float handleY=a.y+7*scale;
        const float hx=std::clamp(x,a.x+5*scale,std::max(a.x+5*scale,b.x-5*scale));
        const float bottom=std::max(handleY,b.y-3*scale);
        auto previous=ImVec2{edge,bottom};
        const auto curve=static_cast<FadeCurve>(side ? value.y : value.x);
        for(int n=1;n<=20 && length>0;++n) {
            const double t=n/20.;const double gain=curve==FadeCurve::EaseIn ? t*t : curve==FadeCurve::EaseOut ? 1-(1-t)*(1-t) : t;
            ImVec2 next{edge+(x-edge)*float(t),bottom-float(gain)*(bottom-handleY)};
            draw->AddLine(previous,next,ImGui::GetColorU32(theme.colors.accent),1.5f*scale);previous=next;
        }
        if(hx<s.view.min.x+s.headerWidth || hx>s.view.max.x || handleY<s.view.min.y || handleY>s.view.max.y) continue;
        ImGui::SetCursorScreenPos({hx-6*scale,handleY-6*scale});
        ImGui::InvisibleButton(side ? "fade-out" : "fade-in",{12*scale,12*scale});
        const bool hovered=ImGui::IsItemHovered();hit|=hovered;
        draw->AddCircleFilled({hx,handleY},4*scale,ImGui::GetColorU32(hovered ? theme.colors.accent : theme.colors.text));
        if(hovered) {ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);ImGui::SetTooltip("%s: %.3f s",side ? "Fade out" : "Fade in",editor::Seconds(length));}
        if(ImGui::IsItemActivated() && !track.locked && !clip.locked && !s.drag.active && !tx.active) {
            if(tx.Begin(clip.id,p.revision,editor::EditKind::ClipFades,original,editor::CurrentModifiers(),out)) {
                s.fadeEnd=side!=0;s.fadeMouseStart=ImGui::GetIO().MousePos.x;
            }
        }
        if(ImGui::BeginPopupContextItem()) {
            ImGui::BeginDisabled(track.locked || clip.locked || tx.active);
            double seconds=editor::Seconds(length);int selected=static_cast<int>(curve);
            if(s.icons) {Icon(*s.icons,IconId::FadeCurve,{16});ImGui::SameLine();}
            bool changed=ImGui::InputDouble("Duration (s)",&seconds,.05,.5,"%.3f");
            changed|=ImGui::Combo("Curve",&selected,"Linear\0Ease in\0Ease out\0");
            if(ImGui::Button("Remove fade")) {seconds=0;changed=true;}
            if(changed) {
                auto next=original;
                (side ? next.last : next.first)=std::clamp(editor::FromSeconds(seconds),Tick{0},clip.duration-(side ? original.first : original.last));
                (side ? next.y : next.x)=selected;
                EditingAction(out,clip.id,p.revision,editor::EditKind::ClipFades,original,next);
            }
            ImGui::EndDisabled();ImGui::EndPopup();
        }
    }
    ImGui::PopID();ImGui::PopID();return hit || (tx.active && tx.draft.target==clip.id);
}
bool CutControl(const ClipView &clip,const TrackView &track,const TimelineProvider &p,
                TimelineState &s,ImVec2 a,ImVec2 b,editor::EventBuffer &out,const Theme &theme) {
    if(!p.editing.cut) return false;
    const auto cut=p.editing.cut(p.editing.user,clip.id);
    auto &tx=s.cutDrag;editor::Value original;original.parent=cut.right;original.first=cut.duration;original.x=static_cast<int>(cut.kind);
    auto value=original;
    if(tx.active && tx.draft.target==clip.id) {
        if(cut.locked || clip.locked || track.locked || cut.limit<=0 || cut.right!=tx.draft.original.parent || tx.draft.revision!=p.revision || ImGui::IsKeyPressed(ImGuiKey_Escape)) tx.Cancel(out);
        else if(tx.draft.phase!=editor::Phase::Cancel && tx.draft.phase!=editor::Phase::Commit) {
            value=tx.draft.original;
            const auto delta=editor::FromSeconds((ImGui::GetIO().MousePos.x-s.fadeMouseStart)*2/s.canvas.scale.x);
            value.first+=std::clamp(delta,-value.first,cut.limit-value.first);
            if(!(value==tx.draft.proposed)) tx.Update(p.revision,value,out);
            if(ImGui::IsMouseReleased(0)) tx.Commit(p.revision,out);
        }
    }
    const float half=std::max(7.f,float(editor::Seconds(value.first)*s.canvas.scale.x*.5));
    const float left=std::max(s.view.min.x+s.headerWidth,b.x-half),right=std::min(s.view.max.x,b.x+half);
    if(right<=left || a.y<s.view.min.y || a.y>s.view.max.y) return false;
    ImGui::PushID(static_cast<int>(clip.id));ImGui::PushID("cut");
    ImGui::SetCursorScreenPos({left,a.y+18});ImGui::InvisibleButton("band",{right-left,14});
    const bool hovered=ImGui::IsItemHovered();
    if(value.first>0) {
        auto *d=ImGui::GetWindowDrawList();d->AddRectFilled({left,a.y+18},{right,a.y+32},ImGui::GetColorU32(theme.colors.selection),2);
        d->AddLine({b.x,a.y+18},{b.x,a.y+32},ImGui::GetColorU32(theme.colors.accent),2);
    }
    const bool valid=cut.right && cut.limit>0 && !cut.locked && !clip.locked && !track.locked;
    if(ImGui::BeginDragDropTarget()) {
        if(const auto *payload=ImGui::AcceptDragDropPayload("IMKIT_CUT_TRANSITION",ImGuiDragDropFlags_AcceptBeforeDelivery)) {
            TransitionKind kind=TransitionKind::None;
            if(payload->DataSize==sizeof(kind)) std::memcpy(&kind,payload->Data,sizeof(kind));
            const bool compatible=(track.kind==TrackKind::Audio && kind==TransitionKind::Crossfade) || (track.kind==TrackKind::Video && (kind==TransitionKind::Dissolve || kind==TransitionKind::Fade));
            if(!valid || !compatible) ImGui::SetTooltip("%s",!compatible ? "Transition does not match track type" : cut.rejection);
            if(payload->IsDelivery() && valid && compatible) {auto next=original;next.first=std::min(editor::TicksPerSecond,cut.limit);next.x=static_cast<int>(kind);EditingAction(out,clip.id,p.revision,editor::EditKind::CutTransition,original,next);}
        }
        ImGui::EndDragDropTarget();
    }
    if(hovered && value.first>0) ImGui::SetTooltip("Transition: %.3f s / drag to resize",editor::Seconds(value.first));
    if(ImGui::IsItemActivated() && valid && value.first>0 && !tx.active && !s.fadeDrag.active && !s.drag.active) {
        if(tx.Begin(clip.id,p.revision,editor::EditKind::CutTransition,original,editor::CurrentModifiers(),out)) s.fadeMouseStart=ImGui::GetIO().MousePos.x;
    }
    if(ImGui::BeginPopupContextItem()) {
        ImGui::BeginDisabled(!valid || tx.active);double duration=editor::Seconds(value.first);
        if(track.kind==TrackKind::Video) {
            int kind=value.x==static_cast<int>(TransitionKind::Fade) ? 1 : 0;
            if(ImGui::Combo("Type",&kind,"Dissolve\0Dip to black\0")) {auto next=original;next.x=static_cast<int>(kind ? TransitionKind::Fade : TransitionKind::Dissolve);EditingAction(out,clip.id,p.revision,editor::EditKind::CutTransition,original,next);}
        }
        if(ImGui::InputDouble("Duration (s)",&duration,.05,.5,"%.3f")) {auto next=original;next.first=std::clamp(editor::FromSeconds(duration),Tick{0},cut.limit);EditingAction(out,clip.id,p.revision,editor::EditKind::CutTransition,original,next);}
        if(ImGui::MenuItem("Remove transition",nullptr,false,value.first>0)) {auto next=original;next.first=0;EditingAction(out,clip.id,p.revision,editor::EditKind::CutTransition,original,next);}
        ImGui::EndDisabled();ImGui::EndPopup();
    }
    ImGui::PopID();ImGui::PopID();return hovered || (tx.active && tx.draft.target==clip.id);
}
void TrackActions(const TimelineProvider &p,TimelineState &s,editor::EventBuffer &out,TrackAction action,StableId before=0) {
    if(!s.trackSelection || !s.trackSelection->count) return;
    const auto ids=s.trackSelection->storage.first(s.trackSelection->count);
    if(out.storage.size()-out.count<ids.size()*2) {out.overflow=true;return;}
    // Reserve the complete gesture before emitting any member.
    const auto first=out.count;
    for(const auto id:ids) {editor::Value v;v.offset=static_cast<Tick>(action);v.parent=before;EditingAction(out,id,p.revision,editor::EditKind::TrackEdit,{},v);}
    for(auto &e:out.storage.subspan(first,out.count-first)) {e.operation=ids.front();e.operationSize=ids.size();}
}
void EditingToolbar(const TimelineProvider &p,TimelineState &s,editor::Selection &selection,editor::EventBuffer &out) {
    if(!s.trackSelection) return;
    if(ImGui::Button("Tracks")) ImGui::OpenPopup("track-actions");
    if(ImGui::BeginPopup("track-actions")) {
        if(ImGui::BeginMenu("Add track")) {
            for(int i=0;i<6;++i) if(ImGui::MenuItem(s.trackLabels.kinds[i])) {editor::Value v;v.offset=static_cast<Tick>(TrackAction::Add);v.x=i;EditingAction(out,0,p.revision,editor::EditKind::TrackEdit,{},v);}
            ImGui::EndMenu();
        }
        if(ImGui::MenuItem("Duplicate selected tracks",nullptr,false,s.trackSelection->count>0)) TrackActions(p,s,out,TrackAction::Duplicate);
        if(s.icons) {Icon(*s.icons,IconId::TrackRemove,{16});ImGui::SameLine();}
        if(ImGui::MenuItem("Delete selected tracks",nullptr,false,s.trackSelection->count>0)) {
            s.deletingClipCount=0;
            for(const auto id:s.trackSelection->storage.first(s.trackSelection->count)) if(p.editing.trackClipCount) s.deletingClipCount+=p.editing.trackClipCount(p.editing.user,id);
            if(s.deletingClipCount) s.confirmTrackDelete=true;else TrackActions(p,s,out,TrackAction::Remove);
        }
        ImGui::EndPopup();
    }
    if(s.confirmTrackDelete) {ImGui::OpenPopup("Delete tracks?");s.confirmTrackDelete=false;}
    if(ImGui::BeginPopupModal("Delete tracks?",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Delete %zu tracks and %zu clips?",s.trackSelection->count,s.deletingClipCount);
        if(ImGui::Button("Delete")) {TrackActions(p,s,out,TrackAction::Remove);ImGui::CloseCurrentPopup();}
        ImGui::SameLine();if(ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();ImGui::EndPopup();
    }
    ImGui::SameLine();
    if(ImGui::Button("Edit clips")) ImGui::OpenPopup("clip-actions");
    if(ImGui::BeginPopup("clip-actions")) {
        for(int i=0;i<3;++i) if(ImGui::MenuItem(i==0 ? "Copy" : i==1 ? "Cut" : "Paste",nullptr,false,i==2 || selection.count>0)) {
            editor::Value v;v.offset=i;v.first=s.time.playhead;v.parent=s.trackSelection->active;v.x=static_cast<int>(s.pasteMode);
            EditingAction(out,selection.active,p.revision,editor::EditKind::Clipboard,{},v);
        }
        int mode=static_cast<int>(s.pasteMode);if(ImGui::Combo("Paste mode",&mode,"Insert\0Overwrite\0")) s.pasteMode=static_cast<PlacementMode>(mode);
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if(ImGui::Button("Transitions")) ImGui::OpenPopup("effect-shelf");
    if(ImGui::BeginPopup("effect-shelf")) {TransitionShelf("effects",s.icons);ImGui::EndPopup();}
}
}
void FadePicker(const char *id,const ClipView &clip,const FadeView &fade,std::uint64_t revision,
                editor::EventBuffer &events,bool trackLocked) {
    ImGui::PushID(id);ImGui::BeginDisabled(trackLocked || clip.locked || clip.duration<=0);
    editor::Value original;original.first=fade.inDuration;original.last=fade.outDuration;
    original.x=static_cast<int>(fade.inCurve);original.y=static_cast<int>(fade.outCurve);
    for(int side=0;side<2;++side) {
        ImGui::PushID(side);ImGui::TextUnformatted(side ? "Fade out" : "Fade in");
        auto value=original;double seconds=editor::Seconds(side ? original.last : original.first);
        int curve=static_cast<int>(side ? original.y : original.x);
        bool changed=ImGui::InputDouble("Seconds",&seconds,.05,.5,"%.3f",ImGuiInputTextFlags_EnterReturnsTrue);
        changed|=ImGui::Combo("Curve",&curve,"Linear\0Ease in\0Ease out\0");
        if(ImGui::SmallButton("Remove")) {seconds=0;changed=true;}
        if(changed && clip.duration>0 && std::isfinite(seconds)) {
            (side ? value.last : value.first)=std::clamp(editor::FromSeconds(seconds),Tick{0},std::max<Tick>(0,clip.duration-(side ? original.first : original.last)));
            (side ? value.y : value.x)=curve;
            EditingAction(events,clip.id,revision,editor::EditKind::ClipFades,original,value);
        }
        ImGui::PopID();
    }
    ImGui::EndDisabled();ImGui::PopID();
}
