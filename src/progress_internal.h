#pragma once
#include <imkit/patterns.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace imkit::detail {
inline const char* SafeProgressText(const char* text) { return text?text:""; }
inline const char* ProgressStatusLabel(ProgressStatus status,ComponentOptions options) {
    const auto text=[&](const char* key,const char* fallback) {
        return options.locale?options.locale->Text(key,fallback):fallback;
    };
    switch(status) {
    case ProgressStatus::Queued:return text("progress_status_queued","Queued");
    case ProgressStatus::Running:return text("progress_status_running","Running");
    case ProgressStatus::Paused:return text("progress_status_paused","Paused");
    case ProgressStatus::Succeeded:return text("progress_status_succeeded","Succeeded");
    case ProgressStatus::Failed:return text("progress_status_failed","Failed");
    case ProgressStatus::Cancelled:return text("progress_status_cancelled","Cancelled");
    }
    return text("progress_status_running","Running");
}
inline const char* ProgressValueText(const ProgressTrackView& view,char* buffer,std::size_t capacity) {
    if(*SafeProgressText(view.valueText)) return view.valueText;
    if(!std::isfinite(view.fraction)||view.fraction<0.f||capacity==0) return "";
    std::snprintf(buffer,capacity,"%.0f%%",std::clamp(view.fraction,0.f,1.f)*100.f);
    return buffer;
}
inline float ProgressTrackBarHeight() {
    return std::clamp(ImGui::GetFrameHeight()*.18f,4.f,8.f);
}
inline float ProgressTrackContentHeight(const ProgressTrackView& view,float width,ComponentOptions options) {
    width=std::max(1.f,width);
    float height=0.f;bool hasItem=false;
    const auto addText=[&](const char* text) {
        if(!*SafeProgressText(text)) return;
        if(hasItem) height+=ImGui::GetStyle().ItemSpacing.y;
        height+=ImGui::CalcTextSize(text,nullptr,false,width).y;hasItem=true;
    };
    char valueBuffer[32]{};
    addText(view.label);
    addText(ProgressStatusLabel(view.status,options));
    addText(ProgressValueText(view,valueBuffer,sizeof(valueBuffer)));
    if(hasItem) height+=ImGui::GetStyle().ItemSpacing.y;
    height+=ProgressTrackBarHeight();hasItem=true;
    addText(view.supplemental);
    return height;
}
} // namespace imkit::detail
