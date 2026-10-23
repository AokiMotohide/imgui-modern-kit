#pragma once
#include <imkit/patterns.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

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
    return std::clamp(ImGui::GetFrameHeight()*.2f,4.f,8.f);
}
// One-line header above the track: the label on the left, status and value on the
// right. Status is spelled out only when it is not the default Running state.
struct ProgressHeader {
    const char* left="";
    char right[96]{};
    bool any=false;
};
inline ProgressHeader MakeProgressHeader(const ProgressTrackView& view,ComponentOptions options) {
    ProgressHeader header;
    char valueBuffer[32]{};
    const char* value=ProgressValueText(view,valueBuffer,sizeof(valueBuffer));
    const char* status=ProgressStatusLabel(view.status,options);
    const bool showStatus=view.status!=ProgressStatus::Running;
    const bool hasLabel=*SafeProgressText(view.label)!=0;
    header.left=hasLabel?view.label:(showStatus?status:"");
    // A label that already says the status (for example "Paused") is not repeated on the right.
    const bool statusOnRight=hasLabel&&showStatus&&std::strcmp(view.label,status)!=0;
    if(statusOnRight&&*value) std::snprintf(header.right,sizeof(header.right),"%s \xC2\xB7 %s",status,value);
    else if(statusOnRight) std::snprintf(header.right,sizeof(header.right),"%s",status);
    else std::snprintf(header.right,sizeof(header.right),"%s",value);
    header.any=*header.left||*header.right;
    return header;
}
inline float ProgressHeaderGap() { return std::max(3.f,ImGui::GetStyle().ItemSpacing.y*.67f); }
// Width available to the left text once the right text has been reserved.
inline float ProgressHeaderLeftWidth(const ProgressHeader& header,float width) {
    const float right=*header.right?ImGui::CalcTextSize(header.right).x+ImGui::GetStyle().ItemSpacing.x:0.f;
    return std::max(1.f,width-right);
}
inline float ProgressHeaderHeight(const ProgressHeader& header,float width) {
    if(!header.any) return 0.f;
    const float left=*header.left?ImGui::CalcTextSize(header.left,nullptr,false,ProgressHeaderLeftWidth(header,width)).y:0.f;
    return std::max(left,ImGui::GetTextLineHeight());
}
inline float ProgressTrackContentHeight(const ProgressTrackView& view,float width,ComponentOptions options) {
    width=std::max(1.f,width);
    const auto header=MakeProgressHeader(view,options);
    float height=ProgressHeaderHeight(header,width);
    if(header.any) height+=ProgressHeaderGap();
    height+=ProgressTrackBarHeight();
    if(*SafeProgressText(view.supplemental))
        height+=ProgressHeaderGap()+ImGui::CalcTextSize(view.supplemental,nullptr,false,width).y;
    return height;
}
} // namespace imkit::detail
