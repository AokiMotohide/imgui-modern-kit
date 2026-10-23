#include "toast_page.h"
#include "gallery.h"
#include <algorithm>
namespace imkit::gallery {
void ToastPage::Show(GalleryState& host) {
    ImGui::SeparatorText("Toasts / トースト");
    ImGui::Checkbox("日本語",&japanese);
    const char* positions[]={"Top left","Top center","Top right","Bottom left","Bottom center","Bottom right"};
    ImGui::Combo("Position",&position,positions,6);layout.position=static_cast<ToastPosition>(position);
    if(ImGui::CollapsingHeader("Layout and lifetime settings")) {
        ImGui::SliderInt("Visible maximum",&maximum,1,8);
        ImGui::SliderFloat("Width (0 = automatic)",&layout.width,0,600);
        ImGui::SliderFloat("Margin (-1 = theme)",&layout.margin,-1,60);
        ImGui::SliderFloat("Gap (-1 = theme)",&layout.gap,-1,30);
        ImGui::InputDouble("Duration (-1 = default, 0 = persistent)",&duration);
    }
    layout.maximum=maximum;
    ImGui::Checkbox("Indeterminate loading",&indeterminate);
    ImGui::SliderFloat("Progress",&progress,0,1);
    auto add=[&](FeedbackKind kind,bool loading=false) {
        ToastView v;v.id=++nextId;v.kind=kind;v.duration=duration;
        v.phase=loading?ToastPhase::Loading:ToastPhase::Message;
        if(loading) {
            v.title=japanese?"ファイルを読み込んでいます":"Loading files";
            v.action=japanese?"詳細を表示":"View details";
            v.stage=japanese?"内容を確認しています":"Inspecting contents";
            v.status=ProgressStatus::Running;v.progress=indeterminate?-1:progress;
        }
        else if(kind==FeedbackKind::Success) v.title=japanese?"読み込みが完了しました":"Files loaded successfully";
        else if(kind==FeedbackKind::Warning) {v.title=japanese?"設定を確認してください":"Review your settings";v.action=japanese?"設定を開く":"Open settings";}
        else if(kind==FeedbackKind::Error) {v.title=japanese?"保存できませんでした":"Unable to save";v.action=japanese?"再試行":"Retry";}
        else v.title=japanese?"情報を更新しました":"Information updated";
        v.description=japanese?"通知の内容と操作はホストが管理します。":"The host owns content and applies every action request.";
        items.push_back(v);
    };
    if(!initialized) {add(FeedbackKind::Success);add(FeedbackKind::Warning);add(FeedbackKind::Error);add(FeedbackKind::Info,true);initialized=true;}
    if(ImGui::Button(japanese?"成功":"Success")) add(FeedbackKind::Success);Record(host,"toast-success");
    ImGui::SameLine();if(ImGui::Button(japanese?"情報":"Info")) add(FeedbackKind::Info);
    if(ImGui::Button(japanese?"警告":"Warning")) add(FeedbackKind::Warning);
    ImGui::SameLine();if(ImGui::Button(japanese?"エラー":"Error")) add(FeedbackKind::Error);
    if(ImGui::Button(japanese?"処理中":"Loading")) add(FeedbackKind::Info,true);Record(host,"toast-loading");
    ImGui::SameLine();if(ImGui::Button(japanese?"完了":"Complete loading")) {
        for(auto& v:items) if(v.phase==ToastPhase::Loading) {
            v.phase=ToastPhase::Message;v.kind=FeedbackKind::Success;
            v.title=japanese?"読み込みが完了しました":"Loading complete";
            v.stage=japanese?"完了":"Complete";v.status=ProgressStatus::Succeeded;v.progress=1.f;v.action="";
        }
    }
    Record(host,"toast-complete");
    ImGui::SameLine();if(ImGui::Button(japanese?"失敗に更新":"Fail loading")) {
        for(auto& v:items) if(v.phase==ToastPhase::Loading) {
            v.phase=ToastPhase::Message;v.kind=FeedbackKind::Error;
            v.title=japanese?"読み込みに失敗しました":"Loading failed";
            v.stage=japanese?"入力を確認してください":"Check the source";
            v.status=ProgressStatus::Failed;v.action=japanese?"詳しく見る":"View details";
        }
    }
    if(ImGui::Button(japanese?"8件追加":"Queue 8")) for(int i=0;i<8;++i) add(FeedbackKind::Success);
    ImGui::SameLine();if(ImGui::Button(japanese?"すべて閉じる":"Clear")) {items.clear();state.Reset();}
    for(auto& v:items) if(v.phase==ToastPhase::Loading) v.progress=indeterminate?-1:progress;
    ImGui::Text("Host queue: %zu | actions: %d",items.size(),actionCount);
    ImGui::TextWrapped("Success/info: 5s. Warning/error/loading: persistent. Hover or keyboard focus pauses expiry. Appearance changes the theme, contrast, density and motion.");
    std::array<accessibility::SemanticNode,128> nodes{};
    accessibility::AccessibilityFrame semantics{nodes};semantics.Begin(ImGui::GetFrameCount());
    LocaleContext locale;locale.lookup=[](void* user,std::string_view key,const char* fallback){return *static_cast<bool*>(user)&&key=="dismiss"?"閉じる":fallback;};locale.user=&japanese;
    std::array<ToastEvent,64> storage{};ToastEventBuffer events{storage};
    ToastViewport("gallery-toasts",items,state,ImGui::GetTime(),events,layout,{&host.theme,nullptr,&semantics,0,&locale});
    for(const auto& n:semantics.Tree().nodes) {
        if(n.role==accessibility::SemanticRole::Button) {
            if(n.name=="Dismiss"||n.name=="閉じる") host.probes["toast-close-"+std::to_string(n.parent)]={n.minimum,n.maximum};
            else host.probes["toast-action-"+std::to_string(n.parent)]={n.minimum,n.maximum};
        } else if(n.parent==0) host.probes["toast-card-"+std::to_string(n.id)]={n.minimum,n.maximum};
    }
    for(std::size_t i=0;i<events.count;++i) {
        const auto event=storage[i];
        if(event.kind==ToastEventKind::Action) ++actionCount;
        else std::erase_if(items,[&](auto& v){return v.id==event.id;});
    }
    if(state.overflow||events.overflow) ImGui::TextUnformatted("Toast storage overflow");
}
}
