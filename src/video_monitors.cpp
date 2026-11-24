#include "draw_internal.h"
#include <imkit/video.h>
#include <imkit/components.h>
#include <imkit/widgets.h>
#include "transaction_support.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace imkit::video {
void MonitorControls(const char *id,MonitorOptions &options,const IconAtlas *icons,const MonitorLabels &labels) {
    ImGui::PushID(id);
    auto toggle=[&](IconId icon,const char *label,bool &value) {
        if (icons) {Icon(*icons,icon,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
        imkit::Checkbox(label,&value);
    };
    toggle(IconId::SafeArea,labels.safeArea,options.safeArea);
    toggle(IconId::Guides,labels.guides,options.guides);
    imkit::Checkbox(labels.timecode,&options.showTimecode);
    toggle(IconId::TransformBounds,labels.bounds,options.transform);
    bool anchor=options.showAnchor.value_or(options.transform);
    const bool before=anchor;toggle(IconId::AnchorPoint,labels.anchor,anchor);
    if (anchor!=before) options.showAnchor=anchor;
    if (icons) {Icon(*icons,IconId::MetadataOverlay,{16*ImGui::GetFontSize()/14});ImGui::SameLine();}
    int preset=static_cast<int>(options.metadataPreset);
    if (imkit::Combo(labels.metadata,&preset,labels.presets.data(),3)) options.metadataPreset=static_cast<MonitorMetadataPreset>(preset);
    ImGui::PopID();
}
void Monitor(const char *id, ImTextureRef texture, ImVec2 size, const editor::TimeState &time,
             const MonitorOptions &o, const Theme &theme) {
    ImGui::PushID(id);
    auto p = ImGui::GetCursorScreenPos();
    if (texture.GetTexID())
        ImGui::Image(texture, size, o.flipY ? ImVec2{0,1} : ImVec2{0,0}, o.flipY ? ImVec2{1,0} : ImVec2{1,1});
    else
        ImGui::Dummy(size);
    auto *d = ImGui::GetWindowDrawList();
    d->PushClipRect(p, {p.x + size.x, p.y + size.y}, true);
    if (!texture.GetTexID())
        d->AddRectFilled(p, {p.x + size.x, p.y + size.y}, ImGui::GetColorU32(theme.colors.canvas));
    auto color = ImGui::GetColorU32(theme.colors.muted);
    if (o.safeArea)
        for (float inset : {.05f, .1f})
            d->AddRect({p.x + size.x * inset, p.y + size.y * inset},
                       {p.x + size.x * (1 - inset), p.y + size.y * (1 - inset)}, color);
    if (o.guides)
        for (int i = 1; i < 3; ++i) {
            d->AddLine({p.x + size.x * i / 3, p.y}, {p.x + size.x * i / 3, p.y + size.y}, color);
            d->AddLine({p.x, p.y + size.y * i / 3}, {p.x + size.x, p.y + size.y * i / 3}, color);
        }
    if (o.transform) {
        const auto &bounds=o.transformBounds;
        const ImVec2 minimum{p.x+float(size.x*bounds.min.x),p.y+float(size.y*bounds.min.y)};
        const ImVec2 maximum{p.x+float(size.x*bounds.max.x),p.y+float(size.y*bounds.max.y)};
        if (std::isfinite(minimum.x) && std::isfinite(minimum.y) && std::isfinite(maximum.x) && std::isfinite(maximum.y) &&
            minimum.x<maximum.x && minimum.y<maximum.y)
            d->AddRect(minimum,maximum,ImGui::GetColorU32(theme.colors.accent),0.f,2.f,ImDrawFlags_None);
    }
    if (o.showAnchor.value_or(o.transform)) {
        const ImVec2 anchor{p.x+size.x*o.anchor.x,p.y+size.y*o.anchor.y};
        if (std::isfinite(anchor.x) && std::isfinite(anchor.y)) d->AddCircle(anchor,5,color);
    }
    const float padding=std::max(3.f,ImGui::GetFontSize()*.25f);
    const float lineHeight=ImGui::GetFontSize()+2*padding;
    auto overlayText=[&](float top,const char *label) {
        if (!label || !*label || size.x<=4*padding || size.y<lineHeight+2*padding) return;
        const ImVec2 minimum{p.x+padding,top};
        const ImVec2 maximum{std::min(p.x+size.x-padding,minimum.x+ImGui::CalcTextSize(label).x+2*padding),top+lineHeight};
        auto background=theme.colors.surface;background.w=1;
        d->AddRectFilled(minimum,maximum,ImGui::GetColorU32(background),theme.metrics.radius);
        const ImVec4 bounds{minimum.x+padding,minimum.y+padding,maximum.x-padding,maximum.y-padding};
        d->AddText(ImGui::GetFont(),ImGui::GetFontSize(),{bounds.x,bounds.y},ImGui::GetColorU32(theme.colors.text),label,nullptr,0,&bounds);
    };
    overlayText(p.y+padding,o.label);
    if (o.metadataPreset!=MonitorMetadataPreset::Off) {
        float top=p.y+padding+((o.label && *o.label) ? lineHeight+padding : 0);
        const float bottom=p.y+size.y-padding-(o.showTimecode ? lineHeight+padding : 0);
        auto line=[&](const char *label) {
            if (label && *label && top+lineHeight<=bottom) {
                overlayText(top,label);top+=lineHeight+padding;
            }
        };
        line(o.clipName);line(o.markerComment);
        if (o.metadataPreset==MonitorMetadataPreset::Details)
            for (const char *label:o.metadata) {
                if (top+lineHeight>bottom) break;
                line(label);
            }
    }
    if (o.showTimecode && (!o.label || !*o.label || size.y>=2*lineHeight+3*padding)) {
        char label[32]{};
        editor::FormatTimecode(time.playhead, time.rate, time.dropFrame, label);
        overlayText(p.y+size.y-lineHeight-padding,label);
    }
    d->PopClipRect();
    ImGui::PopID();
}
bool Monitor(const char *id,const MonitorView &monitor,ImVec2 size,const editor::TimeState &time,
             const MonitorOptions &o,const Theme &theme,ComponentOptions components) {
    const auto extent=[](float value) {return std::isfinite(value)&&value>0 ? value : 0.f;};
    size={extent(size.x),extent(size.y)};
    ImGui::PushID(id);
    if(size.x<=0 || size.y<=0) {
        ImGui::Dummy(size);ImGui::PopID();return false;
    }
    if(!components.theme) components.theme=&theme;
    const ImVec2 widgetMin=ImGui::GetCursorScreenPos(),widgetMax{widgetMin.x+size.x,widgetMin.y+size.y};
    ImGui::InvisibleButton("##monitor",size);
    const ImVec2 after=ImGui::GetCursorScreenPos();
    auto *draw=ImGui::GetWindowDrawList();
    draw->PushClipRect(widgetMin,widgetMax,true);
    draw->AddRectFilled(widgetMin,widgetMax,ImGui::GetColorU32(theme.colors.canvas));
    const auto layout=editor::ResolveImagePlacement(monitor.image.pixels,{size.x,size.y},monitor.placement);
    const bool ready=monitor.status==editor::PreviewState::Ready && layout.valid && monitor.image.texture.GetTexID()!=ImTextureID{};
    bool action=false;
    if(ready) {
        const ImVec2 imageMin{widgetMin.x+static_cast<float>(layout.display.min.x),
                              widgetMin.y+static_cast<float>(layout.display.min.y)};
        const ImVec2 imageMax{widgetMin.x+static_cast<float>(layout.display.max.x),
                              widgetMin.y+static_cast<float>(layout.display.max.y)};
        const auto textureUv=[&](ImVec2 uv) {
            return ImVec2{monitor.image.uv0.x+(monitor.image.uv1.x-monitor.image.uv0.x)*uv.x,
                          monitor.image.uv0.y+(monitor.image.uv1.y-monitor.image.uv0.y)*uv.y};
        };
        ImVec2 uv0=textureUv(layout.uv0),uv1=textureUv(layout.uv1);
        if(o.flipY) std::swap(uv0.y,uv1.y);
        draw->AddImage(monitor.image.texture,imageMin,imageMax,uv0,uv1);
        draw->PushClipRect(imageMin,imageMax,true);
        const ImVec2 visibleUv{layout.uv1.x-layout.uv0.x,layout.uv1.y-layout.uv0.y};
        const auto point=[&](ImVec2 normalized) {
            return ImVec2{imageMin.x+(normalized.x-layout.uv0.x)/visibleUv.x*(imageMax.x-imageMin.x),
                          imageMin.y+(normalized.y-layout.uv0.y)/visibleUv.y*(imageMax.y-imageMin.y)};
        };
        const auto color=ImGui::GetColorU32(theme.colors.muted);
        if(o.safeArea)
            for(float inset:{.05f,.1f}) draw->AddRect(point({inset,inset}),point({1-inset,1-inset}),color);
        if(o.guides)
            for(int i=1;i<3;++i) {
                const float at=static_cast<float>(i)/3;
                draw->AddLine(point({at,0}),point({at,1}),color);
                draw->AddLine(point({0,at}),point({1,at}),color);
            }
        if(o.transform) {
            const ImVec2 minimum=point({static_cast<float>(o.transformBounds.min.x),static_cast<float>(o.transformBounds.min.y)});
            const ImVec2 maximum=point({static_cast<float>(o.transformBounds.max.x),static_cast<float>(o.transformBounds.max.y)});
            if(std::isfinite(minimum.x)&&std::isfinite(minimum.y)&&std::isfinite(maximum.x)&&std::isfinite(maximum.y) &&
               minimum.x<maximum.x&&minimum.y<maximum.y)
                draw->AddRect(minimum,maximum,ImGui::GetColorU32(theme.colors.accent),0.f,2.f,ImDrawFlags_None);
        }
        if(o.showAnchor.value_or(o.transform)) {
            const ImVec2 anchor=point(o.anchor);
            if(std::isfinite(anchor.x)&&std::isfinite(anchor.y)) draw->AddCircle(anchor,5,color);
        }
        const float padding=std::max(3.f,ImGui::GetFontSize()*.25f);
        const float lineHeight=ImGui::GetFontSize()+2*padding;
        const auto overlayText=[&](float top,const char *label) {
            if(!label||!*label||imageMax.x-imageMin.x<=4*padding||imageMax.y-imageMin.y<lineHeight+2*padding) return;
            const ImVec2 minimum{imageMin.x+padding,top};
            const ImVec2 maximum{std::min(imageMax.x-padding,minimum.x+ImGui::CalcTextSize(label).x+2*padding),top+lineHeight};
            auto background=theme.colors.surface;background.w=1;
            draw->AddRectFilled(minimum,maximum,ImGui::GetColorU32(background),theme.metrics.radius);
            const ImVec4 bounds{minimum.x+padding,minimum.y+padding,maximum.x-padding,maximum.y-padding};
            draw->AddText(ImGui::GetFont(),ImGui::GetFontSize(),{bounds.x,bounds.y},ImGui::GetColorU32(theme.colors.text),label,nullptr,0,&bounds);
        };
        overlayText(imageMin.y+padding,o.label);
        if(o.metadataPreset!=MonitorMetadataPreset::Off) {
            float top=imageMin.y+padding+((o.label&&*o.label)?lineHeight+padding:0);
            const float bottom=imageMax.y-padding-(o.showTimecode?lineHeight+padding:0);
            const auto line=[&](const char *label) {
                if(label&&*label&&top+lineHeight<=bottom) {overlayText(top,label);top+=lineHeight+padding;}
            };
            line(o.clipName);line(o.markerComment);
            if(o.metadataPreset==MonitorMetadataPreset::Details)
                for(const char *label:o.metadata) {if(top+lineHeight>bottom) break;line(label);}
        }
        if(o.showTimecode&&(!o.label||!*o.label||imageMax.y-imageMin.y>=2*lineHeight+3*padding)) {
            char label[32]{};editor::FormatTimecode(time.playhead,time.rate,time.dropFrame,label);
            overlayText(imageMax.y-lineHeight-padding,label);
        }
        draw->PopClipRect();
    } else if(monitor.status!=editor::PreviewState::Ready) {
        StateView state=monitor.stateView;
        const char *key="preview_empty",*fallback="Empty";
        switch(monitor.status) {
        case editor::PreviewState::Loading:key="preview_loading";fallback="Loading";break;
        case editor::PreviewState::Offline:key="preview_offline";fallback="Offline";break;
        case editor::PreviewState::Error:key="preview_error";fallback="Error";state.kind=FeedbackKind::Error;break;
        default:break;
        }
        if(!state.heading||!*state.heading) state.heading=components.locale?components.locale->Text(key,fallback):fallback;
        const float padding=std::max(4.f,ImGui::GetStyle().FramePadding.x);
        const ImVec2 contentMin{widgetMin.x+padding,widgetMin.y+padding};
        const ImVec2 contentSize{std::max(1.f,size.x-2*padding),std::max(1.f,size.y-2*padding)};
        if(monitor.status==editor::PreviewState::Loading) {
            ImGui::SetCursorScreenPos(contentMin);Skeleton("loading-skeleton",contentSize,components);
            ImGui::SetCursorScreenPos(contentMin);Spinner("loading-spinner",components);ImGui::SameLine();
            auto loading=state;loading.action="";EmptyState("loading-state",loading,components);
        } else {
            ImGui::SetCursorScreenPos(contentMin);
            if(monitor.status==editor::PreviewState::Empty) action=EmptyState("empty-state",state,components);
            else if(monitor.status==editor::PreviewState::Offline) action=UnavailableState("offline-state",state,components);
            else action=RetryState("error-state",state,components);
        }
        ImGui::SetCursorScreenPos(after);ImGui::Dummy({0,0});
    }
    draw->PopClipRect();
    ImGui::PopID();return action;
}
void BuildAudioBuckets(std::span<const float> pcm, int channels, int channel, std::span<AudioBucket> out) {
    std::fill(out.begin(), out.end(), AudioBucket{});
    if (channels <= 0 || channel < 0 || channel >= channels || out.empty())
        return;
    auto frames = pcm.size() / channels;
    for (std::size_t i = 0; i < out.size(); ++i) {
        auto first = i * frames / out.size(), last = (i + 1) * frames / out.size();
        if (first == last)
            continue;
        auto &b = out[i];
        float firstSample = pcm[first * channels + channel];
        b.minimum = b.maximum = std::isfinite(firstSample) ? firstSample : 0.f;
        double sum = 0;
        for (auto j = first; j < last; ++j) {
            float v = pcm[j * channels + channel];
            if (!std::isfinite(v))
                v = 0;
            b.minimum = (std::min)(b.minimum, v);
            b.maximum = (std::max)(b.maximum, v);
            b.peak = (std::max)(b.peak, std::abs(v));
            sum += v * v;
        }
        b.rms = static_cast<float>(std::sqrt(sum / (last - first)));
    }
}
void UpdateMeter(MeterState &s, std::span<const float> samples, float dt, float hold) {
    AudioBucket bucket;
    BuildAudioBuckets(samples, 1, 0, {&bucket, 1});
    s.rms = bucket.rms;
    s.peak = bucket.peak;
    s.holdRemaining = (std::max)(0.f, s.holdRemaining - dt);
    if (s.peak >= s.heldPeak) {
        s.heldPeak = s.peak;
        s.holdRemaining = hold;
    } else if (s.holdRemaining == 0)
        s.heldPeak = (std::max)(s.peak, s.heldPeak - dt * .5f);
}
AudioBucket WaveformPixel(const WaveformView &view, editor::Range range) {
    AudioBucket result{};
    if (view.status!=WaveformStatus::Ready || view.buckets.empty() || view.range.last<=view.range.first || range.last<=range.first) return result;
    const long double extent=static_cast<long double>(view.range.last)-view.range.first;
    const auto index=[&](Tick t) {return (static_cast<long double>(t)-view.range.first)*view.buckets.size()/extent;};
    const auto first=static_cast<std::size_t>(std::clamp(std::floor(index(range.first)),0.L,static_cast<long double>(view.buckets.size())));
    const auto last=static_cast<std::size_t>(std::clamp(std::ceil(index(range.last)),0.L,static_cast<long double>(view.buckets.size())));
    for (auto i=first;i<last;++i) {
        const auto &v=view.buckets[i];
        if (std::isfinite(v.minimum)) result.minimum=std::min(result.minimum,v.minimum);
        if (std::isfinite(v.maximum)) result.maximum=std::max(result.maximum,v.maximum);
        if (std::isfinite(v.peak)) result.peak=std::max(result.peak,v.peak);
        if (std::isfinite(v.rms)) result.rms=std::max(result.rms,v.rms);
    }
    return result;
}
void DrawWaveform(const WaveformView &view, editor::Range range, ImVec2 min, ImVec2 max,
                  const WaveformOptions &options, const Theme &theme) {
    if (max.x<=min.x || max.y<=min.y) return;
    auto *d=ImGui::GetWindowDrawList();
    const float left=std::max(min.x,d->GetClipRectMin().x),right=std::min(max.x,d->GetClipRectMax().x);
    if(right<=left) return;
    const auto at=[&](float x) {return static_cast<Tick>(static_cast<long double>(range.first)+
        (static_cast<long double>(range.last)-range.first)*(x-min.x)/(max.x-min.x));};
    range={at(left),at(right)};min.x=left;max.x=right;
    d->PushClipRect(min,max,true);
    const float center=(min.y+max.y)*.5f, amplitude=(max.y-min.y)*.46f;
    if (options.zeroLine) d->AddLine({min.x,center},{max.x,center},ImGui::GetColorU32(theme.colors.muted));
    if (view.status!=WaveformStatus::Ready) {
        d->AddText(min,ImGui::GetColorU32(view.status==WaveformStatus::Error ? theme.colors.destructive : theme.colors.muted),
            view.status==WaveformStatus::Error ? "!" : "...");
    } else {
        const int pixels=std::max(1,static_cast<int>(std::ceil(max.x-min.x)));
        const auto time=[&](int i) {return range.first+static_cast<Tick>((static_cast<long double>(range.last)-range.first)*i/pixels);};
        const float gain=std::isfinite(options.gain) ? std::clamp(options.gain,0.f,64.f) : 1.f;
        for (int i=0;i<pixels;++i) {
            const auto v=WaveformPixel(view,{time(i),time(i+1)});
            const float low=std::clamp(v.minimum*gain,-1.f,1.f), high=std::clamp(v.maximum*gain,-1.f,1.f);
            if (low!=0 || high!=0) {
                const float top=center-high*amplitude, bottom=std::max(center-low*amplitude,top+1);
                auto envelope=theme.colors.text; envelope.w*=.38f;
                d->AddRectFilled({min.x+i,top},{min.x+i+1,bottom},ImGui::GetColorU32(envelope));
                // RMS core: the perceived loudness sits inside the peak envelope.
                const float core=std::clamp(v.rms*gain,0.f,1.f)*amplitude;
                if (core>.5f) d->AddRectFilled({min.x+i,std::max(top,center-core)},{min.x+i+1,std::min(bottom,center+core)},
                                               ImGui::GetColorU32(theme.colors.text));
            }
        }
    }
    d->PopClipRect();
}
void Waveform(const char *id, std::span<const AudioBucket> buckets, ImVec2 size, const Theme &theme) {
    if (size.x<=0 || size.y<=0) return;
    const auto p=ImGui::GetCursorScreenPos();ImGui::InvisibleButton(id,size);
    const editor::Range range{0,static_cast<Tick>(buckets.size())*editor::TicksPerSecond};
    DrawWaveform({buckets,range,WaveformStatus::Ready},range,p,{p.x+size.x,p.y+size.y},{},theme);
}
void LevelMeter(const char *id, const MeterState &left, const MeterState &right, ImVec2 size,
                const Theme &t) {
    auto p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, size);
    auto *d = ImGui::GetWindowDrawList();
    const auto level = [](float value) {
        return std::clamp((20 * std::log10((std::max)(value, 1e-6f)) + 60) / 60, 0.f, 1.f);
    };
    const float gap = std::max(3.f, size.x * .1f), w = std::max(4.f, (size.x - gap) * .5f), round = std::min(w * .4f, 4.f);
    // Zone boundaries in dB: green up to -18, amber up to -6, red above.
    const float zoneLow = level(std::pow(10.f, -18.f / 20.f)), zoneHigh = level(std::pow(10.f, -6.f / 20.f));
    const ImVec4 zones[3] = {t.colors.success, t.colors.warning, t.colors.destructive};
    const float bounds[4] = {0.f, zoneLow, zoneHigh, 1.f};
    int i = 0;
    for (auto s : {left, right}) {
        const float x = p.x + i++ * (w + gap);
        const float bottom = p.y + size.y, top = p.y;
        auto at = [&](float fraction) { return bottom - (bottom - top) * fraction; };
        auto track = t.colors.text; track.w = .09f;
        d->AddRectFilled({x, top}, {x + w, bottom}, ImGui::GetColorU32(track), round);
        const float fill = level(s.rms);
        for (int zone = 0; zone < 3; ++zone) {
            const float from = bounds[zone], to = std::min(bounds[zone + 1], fill);
            if (to <= from) continue;
            const ImDrawFlags corners = (zone == 0 ? ImDrawFlags_RoundCornersBottom : 0) |
                                        (to >= fill ? ImDrawFlags_RoundCornersTop : 0);
            d->AddRectFilled({x, at(to)}, {x + w, at(from)}, ImGui::GetColorU32(zones[zone]), corners ? round : 0.f,
                             corners ? corners : ImDrawFlags_RoundCornersNone);
        }
        for (float tick : {zoneLow, zoneHigh}) {
            auto line = t.colors.canvas; line.w = .8f;
            d->AddLine({x, at(tick)}, {x + w, at(tick)}, ImGui::GetColorU32(line));
        }
        const float hold = at(level(s.heldPeak));
        d->AddRectFilled({x - 1, hold - 1}, {x + w + 1, hold + 1}, ImGui::GetColorU32(s.peak >= 1 ? t.colors.destructive : t.colors.text), 1.f);
        if (s.peak >= 1)
            d->AddRectFilled({x, top}, {x + w, top + 3}, ImGui::GetColorU32(t.colors.destructive), round, ImDrawFlags_RoundCornersTop);
    }
}
void AudioStrip(const AudioStripView &v, std::uint64_t revision, editor::PropertyState &s,
                editor::EventBuffer &out) {
    detail::ResumeTerminal(s.drag, revision, out);
    if (s.drag.active && (v.locked || ImGui::IsKeyPressed(ImGuiKey_Escape) ||
                         (s.drag.draft.target != v.gainId && s.drag.draft.target != v.panId)))
        s.drag.Cancel(out);
    ImGui::PushID(reinterpret_cast<const void *>(static_cast<std::uintptr_t>(v.id)));
    ImGui::BeginGroup();
    ImGui::TextUnformatted(v.label);
    ImGui::BeginDisabled(v.locked);
    auto process = [&](StableId target,double original,double draft,bool changed) {
        if (!target) return;
        if (ImGui::IsItemActivated() && !s.drag.active)
            s.drag.Begin(target,revision,editor::EditKind::Property,{0,0,0,0,original},
                         editor::CurrentModifiers(),out);
        if (s.drag.active && s.drag.draft.target==target) {
            if (changed) s.drag.Update(revision,{0,0,0,0,draft},out);
            if (ImGui::IsItemDeactivated()) s.drag.Commit(revision,out);
        }
    };
    double gain=s.drag.active && s.drag.draft.target==v.gainId ? s.drag.draft.proposed.x : v.gain;
    double minimum=0,maximum=4;
    ImGui::BeginDisabled(!v.gainId);
    bool changed=ImGui::VSliderScalar(v.gainLabel,{32,100},ImGuiDataType_Double,&gain,&minimum,&maximum,"%.2f");
    process(v.gainId,v.gain,gain,changed);
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginGroup();
    double pan=s.drag.active && s.drag.draft.target==v.panId ? s.drag.draft.proposed.x : v.pan;
    minimum=-1;maximum=1;
    ImGui::SetNextItemWidth(ImGui::GetFontSize()*10);
    ImGui::BeginDisabled(!v.panId || v.panId==v.gainId);
    changed=ImGui::SliderScalar(v.panLabel,ImGuiDataType_Double,&pan,&minimum,&maximum,"%.2f");
    process(v.panId,v.pan,pan,changed);
    ImGui::EndDisabled();
    const bool flags[]={v.mute,v.solo,v.record};
    const TrackControl controls[]={TrackControl::Mute,TrackControl::Solo,TrackControl::Record};
    const char *labels[]={v.muteLabel,v.soloLabel,v.recordLabel};
    for (int i=0;i<3;++i) {
        bool value=flags[i];
        ImGui::PushID(i);
        if(s.icons) {const IconId icons[]{IconId::Mute,IconId::Solo,IconId::Microphone};Icon(*s.icons,icons[i],{ImGui::GetFontSize()});ImGui::SameLine();}
        if (imkit::Checkbox(labels[i],&value))
            out.Push({v.id,revision,editor::Phase::Commit,editor::EditKind::Toggle,
                      {0,0,0,0,static_cast<double>(controls[i]),flags[i] ? 1. : 0.},
                      {0,0,0,0,static_cast<double>(controls[i]),value ? 1. : 0.},editor::CurrentModifiers()});
        ImGui::PopID();
    }
    ImGui::EndGroup();
    ImGui::EndDisabled();
    ImGui::EndGroup();
    ImGui::PopID();
}
bool BuildScopes(std::span<const Rgba> pixels, int w, int h, ScopeBuffers out) {
    if (w <= 0 || h <= 0 || pixels.size() != static_cast<std::size_t>(w) * h || out.red.size() != 256 ||
        out.green.size() != 256 || out.blue.size() != 256 || out.luma.size() != 256 ||
        out.waveform.size() != static_cast<std::size_t>(w) * 256 || out.vectorscope.size() != 65536)
        return false;
    const auto waveSize = static_cast<std::size_t>(w) * 256;
    const bool rgb = !out.redWaveform.empty() || !out.greenWaveform.empty() || !out.blueWaveform.empty();
    if (rgb && (out.redWaveform.size() != waveSize || out.greenWaveform.size() != waveSize ||
                out.blueWaveform.size() != waveSize))
        return false;
    for (auto span : {out.red, out.green, out.blue, out.luma, out.waveform, out.vectorscope,
                     out.redWaveform, out.greenWaveform, out.blueWaveform})
        std::fill(span.begin(), span.end(), 0);
    auto bin = [](float value) {
        return static_cast<int>(std::clamp(std::isfinite(value) ? value : 0.f, 0.f, 1.f) * 255);
    };
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        auto p = pixels[i];
        p.r = std::isfinite(p.r) ? p.r : 0.f;
        p.g = std::isfinite(p.g) ? p.g : 0.f;
        p.b = std::isfinite(p.b) ? p.b : 0.f;
        float y = .2126f * p.r + .7152f * p.g + .0722f * p.b;
        ++out.red[bin(p.r)];
        ++out.green[bin(p.g)];
        ++out.blue[bin(p.b)];
        ++out.luma[bin(y)];
        ++out.waveform[bin(y) * w + i % w];
        if (rgb) {
            ++out.redWaveform[bin(p.r) * w + i % w];
            ++out.greenWaveform[bin(p.g) * w + i % w];
            ++out.blueWaveform[bin(p.b) * w + i % w];
        }
        ++out.vectorscope[bin(.5f + (p.r - y) * .635f) * 256 + bin(.5f + (p.b - y) * .539f)];
    }
    return true;
}
void Histogram(const char *id, std::span<const std::uint32_t> bins, ImVec2 size, const Theme &t) {
    auto p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, size);
    auto *d = ImGui::GetWindowDrawList();
    auto panel = t.colors.text; panel.w = .06f;
    d->AddRectFilled(p, {p.x + size.x, p.y + size.y}, ImGui::GetColorU32(panel), t.radius.control);
    if (bins.empty() || size.x < 4 || size.y < 4)
        return;
    auto grid = t.colors.text; grid.w = .08f;
    for (int i = 1; i < 4; ++i) {
        const float x = p.x + size.x * i / 4;
        d->AddLine({x, p.y + 2}, {x, p.y + size.y - 2}, ImGui::GetColorU32(grid));
    }
    if (size.x > 90 && size.y > 36) {
        // Level marks at the black point, mid grey and white point.
        auto mark = t.colors.muted; mark.w *= .8f;
        ImFont *font = ImGui::GetFont();
        const float small = ImGui::GetFontSize() * .78f;
        const ImU32 ink = ImGui::GetColorU32(mark);
        d->AddText(font, small, {p.x + 4, p.y + size.y - small - 2}, ink, "0");
        d->AddText(font, small, {p.x + size.x * .5f - small * .7f, p.y + size.y - small - 2}, ink, "128");
        d->AddText(font, small, {p.x + size.x - small * 1.6f - 3, p.y + size.y - small - 2}, ink, "255");
    }
    const float maximum = static_cast<float>((std::max)(1u, *std::max_element(bins.begin(), bins.end())));
    // One sample per two pixels, taking the tallest bin that falls in the column.
    constexpr int MaxColumns = 512;
    const int columns = std::clamp(static_cast<int>(size.x * .5f), 2, MaxColumns);
    std::array<ImVec2, MaxColumns> points{};
    const float baseline = p.y + size.y - 1;
    for (int c = 0; c < columns; ++c) {
        const std::size_t first = bins.size() * c / columns;
        const std::size_t last = (std::max)(first + 1, bins.size() * (c + 1) / columns);
        std::uint32_t peak = 0;
        for (std::size_t i = first; i < last && i < bins.size(); ++i) peak = (std::max)(peak, bins[i]);
        // Square-root scaling keeps small populations visible next to a dominant bin.
        const float height = std::sqrt(peak / maximum) * (size.y - 4);
        points[c] = {p.x + size.x * c / (columns - 1), baseline - height};
    }
    auto fill = t.colors.accent; fill.w = .28f;
    for (int c = 0; c + 1 < columns; ++c)
        d->AddQuadFilled(points[c], points[c + 1], {points[c + 1].x, baseline}, {points[c].x, baseline}, ImGui::GetColorU32(fill));
    d->AddPolyline(points.data(), columns, ImGui::GetColorU32(t.colors.accent), ImDrawFlags_None, 1.75f);
}
void ScopeImage(const char *id, std::span<const std::uint32_t> bins, int w, int h, ImVec2 size,
                const Theme &t) {
    ScopeImage(id, bins, w, h, size, t, t.colors.success);
}
void ScopeImage(const char *id, std::span<const std::uint32_t> bins, int w, int h, ImVec2 size,
                const Theme &t, ImVec4 tint) {
    auto p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, size);
    auto *d = ImGui::GetWindowDrawList();
    auto panel = t.colors.canvas; panel.w = .7f;
    d->AddRectFilled(p, {p.x + size.x, p.y + size.y}, ImGui::GetColorU32(panel), t.radius.control);
    auto graticule = t.colors.text; graticule.w = .10f;
    for (int i = 1; i < 4; ++i)
        d->AddLine({p.x, p.y + size.y * i / 4}, {p.x + size.x, p.y + size.y * i / 4}, ImGui::GetColorU32(graticule));
    if (w <= 0 || h <= 0 || bins.size() != static_cast<std::size_t>(w) * h)
        return;
    auto max = *std::max_element(bins.begin(), bins.end());
    if (!max)
        return;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            if (bins[y * w + x]) {
                auto c = tint;
                c.w *= static_cast<float>(std::log1p(bins[y * w + x]) / std::log1p(max));
                d->AddRectFilled({p.x + x * size.x / w, p.y + (h - 1 - y) * size.y / h},
                                 {p.x + (x + 1) * size.x / w, p.y + (h - y) * size.y / h},
                                 ImGui::GetColorU32(c));
            }
}
namespace {
editor::Value ColorValue(const float *rgb) { return {0,0,0,0,rgb[0],rgb[1],rgb[2]}; }
void ColorEvent(StableId target, editor::Value original, editor::Value proposed, bool changed,
                std::uint64_t revision, ColorState *state, editor::EventBuffer *events) {
    if (!state || !events || !target)
        return;
    auto &drag = state->drag;
    if (ImGui::IsItemActivated() && !drag.active)
        drag.Begin(target, revision, editor::EditKind::Property, original, editor::CurrentModifiers(), *events);
    if (drag.active && drag.draft.target == target) {
        if (changed)
            drag.Update(revision, proposed, *events);
        if (ImGui::IsItemDeactivated())
            drag.Commit(revision, *events);
    }
}
bool Wheel(float *rgb, float diameter, ImVec2 &center) {
    constexpr float pi = 3.14159265358979323846f;
    const auto p = ImGui::GetCursorScreenPos();
    float radius = diameter * .5f - 4;
    center = {p.x + diameter * .5f, p.y + diameter * .5f};
    ImGui::InvisibleButton("wheel", {diameter,diameter});
    const float mean = (rgb[0] + rgb[1] + rgb[2]) / 3;
    float x = rgb[0] - mean, y = (rgb[1] - rgb[2]) / std::sqrt(3.f);
    bool changed = false;
    if (ImGui::IsItemActive() && ImGui::IsMouseDown(0)) {
        auto mouse = ImGui::GetIO().MousePos;
        x = (mouse.x - center.x) / radius;
        y = (center.y - mouse.y) / radius;
        float length = std::hypot(x,y);
        if (length > 1) { x /= length; y /= length; }
        if (ImGui::IsMouseDoubleClicked(0)) x = y = 0;
        float candidate[] = {mean + x, mean - .5f*x + std::sqrt(3.f)*.5f*y,
                             mean - .5f*x - std::sqrt(3.f)*.5f*y};
        for (int i=0; i<3; ++i) {
            changed |= rgb[i] != candidate[i];
            rgb[i] = candidate[i];
        }
    }
    auto *draw = ImGui::GetWindowDrawList();
    {
        // Smooth hue/saturation disc from one indexed mesh: no seams between sectors.
        constexpr int rings = 14, sectors = 90;
        draw->PrimReserve(rings * sectors * 6, (rings + 1) * (sectors + 1));
        const ImDrawIdx base = static_cast<ImDrawIdx>(draw->_VtxCurrentIdx);
        const ImVec2 white = ImGui::GetFontTexUvWhitePixel();
        for (int ring = 0; ring <= rings; ++ring)
            for (int sector = 0; sector <= sectors; ++sector) {
                const float angle = sector * 2 * pi / sectors, r = radius * ring / rings;
                float cr, cg, cb;
                ImGui::ColorConvertHSVtoRGB(static_cast<float>(sector % sectors) / sectors, static_cast<float>(ring) / rings, .85f, cr, cg, cb);
                draw->PrimWriteVtx({center.x + std::cos(angle) * r, center.y - std::sin(angle) * r}, white,
                                   ImGui::GetColorU32(ImVec4(cr, cg, cb, 1)));
            }
        for (int ring = 0; ring < rings; ++ring)
            for (int sector = 0; sector < sectors; ++sector) {
                const ImDrawIdx a = base + static_cast<ImDrawIdx>(ring * (sectors + 1) + sector), b = a + 1,
                                c = a + static_cast<ImDrawIdx>(sectors + 1), e = c + 1;
                for (ImDrawIdx index : {a, b, e, a, e, c}) draw->PrimWriteIdx(index);
            }
    }
    const auto border = ImGui::GetColorU32(ImGuiCol_Border);
    draw->AddCircle(center,radius,border,64,1.f);
    draw->AddLine({center.x-4,center.y},{center.x+4,center.y},border);
    draw->AddLine({center.x,center.y-4},{center.x,center.y+4},border);
    float length = std::hypot(x,y);
    if (length>1) { x/=length; y/=length; }
    ImVec2 thumb{center.x+x*radius,center.y-y*radius};
    draw->AddCircleFilled(thumb,5,ImGui::GetColorU32(ImGuiCol_WindowBg));
    draw->AddCircle(thumb,5,ImGui::GetColorU32(ImGuiCol_Text),16,2);
    if (ImGui::IsItemFocused())
        draw->AddRect(p,{p.x+diameter,p.y+diameter},ImGui::GetColorU32(ImGuiCol_NavCursor),4.f,2.f,ImDrawFlags_None);
    return changed;
}
bool ColorPanel(const char *id, ColorValues &draft, const ColorValues &original,
                const ColorPropertyIds &ids, std::uint64_t revision, ColorState *state,
                editor::EventBuffer *events, const ColorLabels &labels) {
    ImGui::PushID(id);
    bool changed = false;
    float *rgb[] = {draft.lift,draft.gamma,draft.gain};
    const float *source[] = {original.lift,original.gamma,original.gain};
    const StableId targets[] = {ids.lift,ids.gamma,ids.gain};
    const char *names[] = {labels.lift,labels.gamma,labels.gain};
    const int columns = ImGui::GetContentRegionAvail().x < ImGui::GetFontSize()*30 ? 1 : 3;
    if (ImGui::BeginTable("wheels",columns)) {
        for (int i=0; i<3; ++i) {
            ImGui::TableNextColumn();
            ImGui::PushID(i);
            ImGui::TextUnformatted(names[i]);
            ImGui::BeginDisabled(state && !targets[i]);
            float diameter = std::clamp(ImGui::GetContentRegionAvail().x, 64.f, ImGui::GetFontSize()*8);
            ImVec2 center;
            bool edited = Wheel(rgb[i],diameter,center);
            ColorEvent(targets[i],ColorValue(source[i]),ColorValue(rgb[i]),edited,revision,state,events);
            changed |= edited;
            if (state) { state->wheelCenters[i]=center; state->wheelRadius=diameter*.5f-4; }
            ImGui::SetNextItemWidth(diameter);
            float mean = (rgb[i][0]+rgb[i][1]+rgb[i][2])/3, before = mean;
            edited = imkit::SliderFloat(labels.level,&mean,i==0 ? -1.f : .01f,i==0 ? 1.f : 4.f);
            if (edited) for (int c=0;c<3;++c) rgb[i][c] += mean-before;
            ColorEvent(targets[i],ColorValue(source[i]),ColorValue(rgb[i]),edited,revision,state,events);
            changed |= edited;
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    float *scalars[] = {&draft.temperature,&draft.tint,&draft.exposure};
    float values[] = {original.temperature,original.tint,original.exposure};
    StableId scalarIds[] = {ids.temperature,ids.tint,ids.exposure};
    const char *scalarNames[] = {labels.temperature,labels.tint,labels.exposure};
    for (int i=0; i<3; ++i) {
        ImGui::PushID(i+3);
        ImGui::BeginDisabled(state && !scalarIds[i]);
        bool edited = imkit::SliderFloat(scalarNames[i],scalars[i],i==2 ? -10.f : -1.f,i==2 ? 10.f : 1.f);
        ColorEvent(scalarIds[i],{0,0,0,0,values[i]},{0,0,0,0,*scalars[i]},edited,revision,state,events);
        changed |= edited;
        ImGui::EndDisabled();
        ImGui::PopID();
    }
    ImGui::PopID();
    return changed;
}
}
Rgba ApplyColorCurves(Rgba input,const ColorCurveSet &curves) {
    auto evaluate=[](float value,std::span<const editor::Keyframe> keys) {
        const double normalized=std::isfinite(value) ? std::clamp(double(value),0.,1.) : 0.;
        const double result=keys.empty() ? normalized : editor::Evaluate(keys,editor::FromSeconds(normalized));
        return std::isfinite(result) ? float(std::clamp(result,0.,1.)) : 0.f;
    };
    return {evaluate(input.r,curves.red),evaluate(input.g,curves.green),evaluate(input.b,curves.blue),input.a};
}
bool ColorControls(const char *id, ColorValues &draft) {
    const ColorValues original = draft;
    return ColorPanel(id,draft,original,{},0,nullptr,nullptr,{});
}
void ColorControls(const char *id, const ColorValues &values, const ColorPropertyIds &ids,
                   std::uint64_t revision, ColorState &state, editor::EventBuffer &events,
                   const ColorLabels &labels) {
    detail::ResumeTerminal(state.drag,revision,events);
    if (state.drag.active && ImGui::IsKeyPressed(ImGuiKey_Escape)) state.drag.Cancel(events);
    auto draft = values;
    if (state.drag.active) {
        const auto &event = state.drag.draft;
        float *rgb = event.target == ids.lift ? draft.lift : event.target == ids.gamma ? draft.gamma :
                     event.target == ids.gain ? draft.gain : nullptr;
        if (rgb) { rgb[0]=static_cast<float>(event.proposed.x); rgb[1]=static_cast<float>(event.proposed.y);
                   rgb[2]=static_cast<float>(event.proposed.z); }
        else if (event.target == ids.temperature) draft.temperature=static_cast<float>(event.proposed.x);
        else if (event.target == ids.tint) draft.tint=static_cast<float>(event.proposed.x);
        else if (event.target == ids.exposure) draft.exposure=static_cast<float>(event.proposed.x);
        else state.drag.Cancel(events);
    }
    ColorPanel(id,draft,values,ids,revision,&state,&events,labels);
}
} // namespace imkit::video
