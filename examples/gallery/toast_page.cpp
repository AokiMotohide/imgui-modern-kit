#include "toast_page.h"
#include "gallery.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
namespace imkit::gallery {
namespace {
// Tinted button for one feedback kind: a soft wash of the status colour with status-coloured text.
bool ToneButton(const char* label, ImVec4 tone) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    auto wash = [&](float alpha) { auto c = tone; c.w = alpha; return c; };
    ImGui::PushStyleColor(ImGuiCol_Button, wash(.16f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, wash(.26f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, wash(.36f));
    ImGui::PushStyleColor(ImGuiCol_Text, tone);
    const bool pressed = ImGui::Button(label);
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
    return pressed;
}
// Miniature screen with six anchor cells; the selected cell shows where the stack appears.
bool PositionPicker(GalleryState& host, int& position) {
    const float scale = host.scale;
    const ImVec2 size{std::min(ImGui::GetContentRegionAvail().x, 260.f * scale), 140.f * scale};
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    auto* draw = ImGui::GetWindowDrawList();
    const float rounding = ImGui::GetStyle().FrameRounding;
    draw->AddRectFilled(origin, {origin.x + size.x, origin.y + size.y}, ImGui::GetColorU32(ImGuiCol_FrameBg), rounding + 2);
    draw->AddRect(origin, {origin.x + size.x, origin.y + size.y}, ImGui::GetColorU32(ImGuiCol_Border), rounding + 2);
    const char* names[] = {"Top left", "Top center", "Top right", "Bottom left", "Bottom center", "Bottom right"};
    const float pad = 6.f * scale, cellW = (size.x - pad * 4) / 3, cellH = (size.y - pad * 3) / 2;
    ImVec4 accent = host.theme.semantic.accent;
    bool changed = false;
    ImGui::PushID("position-picker");
    for (int i = 0; i < 6; ++i) {
        const int column = i % 3, row = i / 3;
        const ImVec2 a{origin.x + pad + column * (cellW + pad), origin.y + pad + row * (cellH + pad)};
        const ImVec2 b{a.x + cellW, a.y + cellH};
        ImGui::SetCursorScreenPos(a);
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("##cell", {cellW, cellH})) {
            position = i;
            changed = true;
        }
        const bool hovered = ImGui::IsItemHovered(), selected = position == i;
        if (hovered)
            ImGui::SetTooltip("%s", names[i]);
        auto fill = accent;
        fill.w = selected ? .22f : hovered ? .10f : .0f;
        draw->AddRectFilled(a, b, ImGui::GetColorU32(fill), rounding);
        auto edge = selected ? accent : host.theme.semantic.border;
        edge.w *= selected ? 1.f : .7f;
        draw->AddRect(a, b, ImGui::GetColorU32(edge), rounding, 0, selected ? 1.6f : 1.f);
        // The stack glyph: two short bars anchored toward the chosen corner or edge.
        const float barW = cellW * .5f, barH = std::max(3.f, cellH * .14f), gap = barH * .8f;
        const float x = column == 0 ? a.x + 6.f * scale : column == 2 ? b.x - 6.f * scale - barW : (a.x + b.x - barW) * .5f;
        const bool top = row == 0;
        for (int bar = 0; bar < 2; ++bar) {
            const float y = top ? a.y + 6.f * scale + bar * (barH + gap) : b.y - 6.f * scale - barH - bar * (barH + gap);
            auto glyph = selected ? accent : host.theme.semantic.textSecondary;
            glyph.w *= selected ? 1.f : .55f;
            draw->AddRectFilled({x, y}, {x + barW * (bar ? .75f : 1.f), y + barH}, ImGui::GetColorU32(glyph), barH * .5f);
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy(size);
    return changed;
}
}
void ToastPage::Show(GalleryState& host) {
    ComponentOptions o{&host.theme, &host.animation};
    const float scale = host.scale;
    // Controls sit opposite the toast stack so the stack never covers them.
    const bool stackLeft = position == 0 || position == 3;
    const float available = ImGui::GetContentRegionAvail().x;
    const float controlsWidth = std::min(available, std::max(420.f * scale, std::min(available * .56f, 640.f * scale)));
    if (stackLeft)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + available - controlsWidth);
    const char* languages[] = {"English", "日本語"};
    ImGui::BeginChild("toast-controls", {controlsWidth, 0}, ImGuiChildFlags_AutoResizeY);
    int language = japanese ? 1 : 0;
    if (Segmented("language", &language, languages, o))
        japanese = language == 1;
    ImGui::Spacing();
    layout.position = static_cast<ToastPosition>(position);
    layout.maximum = maximum;

    auto add = [&](FeedbackKind kind, bool loading = false) {
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

    if (BeginGrid(host, "toast-grid", 1)) {
        const bool trigger = BeginSection(host, japanese ? "通知を表示" : "Show a toast",
                                          japanese ? "成功と情報は5秒で消えます。警告・エラー・処理中は閉じるまで残ります。"
                                                   : "Success and info close after 5 s. Warning, error and loading stay until dismissed.");
        if (trigger) {
            if (ToneButton(japanese ? "成功" : "Success", host.theme.semantic.success)) add(FeedbackKind::Success);
            Record(host, "toast-success");
            ImGui::SameLine();
            if (ToneButton(japanese ? "情報" : "Info", host.theme.semantic.accent)) add(FeedbackKind::Info);
            ImGui::SameLine();
            if (ToneButton(japanese ? "警告" : "Warning", host.theme.semantic.warning)) add(FeedbackKind::Warning);
            ImGui::SameLine();
            if (ToneButton(japanese ? "エラー" : "Error", host.theme.semantic.error)) add(FeedbackKind::Error);
            ImGui::Spacing();
            ImGui::TextDisabled("%s", japanese ? "進捗つきの通知" : "Progress toast");
            if (ActionButton(japanese ? "処理中" : "Loading", ActionVariant::Secondary, {}, o)) add(FeedbackKind::Info, true);
            Record(host, "toast-loading");
            ImGui::SameLine();
            if (ActionButton(japanese ? "完了" : "Complete loading", ActionVariant::Secondary, {}, o))
                for(auto& v:items) if(v.phase==ToastPhase::Loading) {
                    v.phase=ToastPhase::Message;v.kind=FeedbackKind::Success;
                    v.title=japanese?"読み込みが完了しました":"Loading complete";
                    v.stage=japanese?"完了":"Complete";v.status=ProgressStatus::Succeeded;v.progress=1.f;v.action="";
                }
            Record(host, "toast-complete");
            ImGui::SameLine();
            if (ActionButton(japanese ? "失敗に更新" : "Fail loading", ActionVariant::SubtleDestructive, {}, o))
                for(auto& v:items) if(v.phase==ToastPhase::Loading) {
                    v.phase=ToastPhase::Message;v.kind=FeedbackKind::Error;
                    v.title=japanese?"読み込みに失敗しました":"Loading failed";
                    v.stage=japanese?"入力を確認してください":"Check the source";
                    v.status=ProgressStatus::Failed;v.action=japanese?"詳しく見る":"View details";
                }
            ImGui::Spacing();
            if (ActionButton(japanese ? "8件追加" : "Queue 8", ActionVariant::Ghost, {}, o)) for(int i=0;i<8;++i) add(FeedbackKind::Success);
            ImGui::SameLine();
            if (ActionButton(japanese ? "すべて閉じる" : "Clear all", ActionVariant::Ghost, {}, o)) {items.clear();state.Reset();}
            ImGui::Spacing();
            char summary[96];
            std::snprintf(summary, sizeof(summary), japanese ? "キュー %zu 件・操作 %d 回" : "%zu in queue · %d actions", items.size(), actionCount);
            ImGui::TextDisabled("%s", summary);
        }
        EndSection(trigger);

        const bool placement = BeginSection(host, japanese ? "表示位置と寿命" : "Placement and lifetime",
                                            japanese ? "通知の積み方と表示時間を設定します。" : "Where the stack appears and how long it lives.");
        if (placement) {
            PositionPicker(host, position);
            layout.position = static_cast<ToastPosition>(position);
            ImGui::Spacing();
            SliderInt(japanese ? "最大表示数" : "Visible maximum", &maximum, 1, 8);
            SliderFloat(japanese ? "幅" : "Width", &layout.width, 0, 600, "%.0f px");
            SliderFloat(japanese ? "余白" : "Margin", &layout.margin, -1, 60, "%.0f px");
            SliderFloat(japanese ? "間隔" : "Gap", &layout.gap, -1, 30, "%.0f px");
            double seconds = duration;
            ImGui::SetNextItemWidth(160.f * scale);
            if (ImGui::InputDouble(japanese ? "表示時間 (秒)" : "Duration (s)", &seconds, 0, 0, "%.1f")) duration = seconds;
            ImGui::TextDisabled("%s", japanese ? "幅 0 = 自動。余白と間隔 -1 = テーマ。時間 -1 = 既定、0 = 常に表示。"
                                               : "Width 0 = automatic. Margin and gap -1 = theme. Duration -1 = default, 0 = persistent.");
        }
        EndSection(placement);

        const bool loading = BeginSection(host, japanese ? "処理中の通知" : "Loading toast",
                                          japanese ? "処理中の通知が示す進捗を操作します。" : "Drive the progress shown by loading toasts.");
        if (loading) {
            Toggle(japanese ? "進捗を特定しない" : "Indeterminate", &indeterminate, o);
            ImGui::BeginDisabled(indeterminate);
            SliderFloat(japanese ? "進捗" : "Progress", &progress, 0, 1, "%.2f");
            ImGui::EndDisabled();
        }
        EndSection(loading);
        EndGrid();
    }
    for(auto& v:items) if(v.phase==ToastPhase::Loading) v.progress=indeterminate?-1:progress;
    ImGui::EndChild();

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
