#include <imkit/widgets.h>
#include <imkit/patterns.h>
#include "draw_internal.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstring>

namespace imkit {
namespace {
// Selected rows get an accent bar on the leading edge; tabs get an inset pill underneath.
void SelectionMark(bool selected, bool underline = false) {
    if (!selected)
        return;
    const auto a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    const float height = b.y - a.y;
    auto *draw = ImGui::GetWindowDrawList();
    const ImU32 color = ImGui::GetColorU32(ImGuiCol_CheckMark);
    if (underline) {
        const float inset = std::min(10.f, (b.x - a.x) * .2f);
        draw->AddRectFilled({a.x + inset, b.y - 3}, {b.x - inset, b.y - 1}, color, 1.f);
    } else {
        // Drawn in the padding to the left of the row so it never overlaps the label.
        draw->AddRectFilled({a.x - 6, a.y + height * .22f}, {a.x - 3, b.y - height * .22f}, color, 1.5f);
    }
}
// Shrinks the frame height for check marks only, leaving inputs and buttons at the control height.
struct CompactFrame {
    CompactFrame() {
        const auto &style = ImGui::GetStyle();
        const float pad = std::max(1.f, ImGui::GetFontSize() * .225f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {style.FramePadding.x, pad});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, std::min(style.FrameRounding, 6.f));
    }
    ~CompactFrame() { ImGui::PopStyleVar(2); }
    CompactFrame(const CompactFrame &) = delete;
    CompactFrame &operator=(const CompactFrame &) = delete;
};

// Fill-style slider. ImGui keeps all input behaviour (drag, keyboard, Ctrl+click text entry); the
// grab is hidden and the value is drawn as a tinted bar behind the frame text, so the value text
// never collides with a handle. Logarithmic sliders keep the native grab because their value to
// position mapping is not linear.
template <class T> class SliderFill {
  public:
    SliderFill(const T *value, T minimum, T maximum, ImGuiSliderFlags flags, bool vertical, ImVec2 size)
        : value_(value), minimum_(minimum), maximum_(maximum), vertical_(vertical),
          enabled_(!(flags & ImGuiSliderFlags_Logarithmic)) {
        if (!enabled_)
            return;
        draw_ = ImGui::GetWindowDrawList();
        min_ = ImGui::GetCursorScreenPos();
        max_ = {min_.x + (vertical ? size.x : ImGui::CalcItemWidth()), min_.y + (vertical ? size.y : ImGui::GetFrameHeight())};
        Splitter().Split(draw_, 2);
        Splitter().SetCurrentChannel(draw_, 1);
        const ImVec4 clear{0, 0, 0, 0};
        for (ImGuiCol color : {ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive, ImGuiCol_SliderGrab,
                               ImGuiCol_SliderGrabActive})
            ImGui::PushStyleColor(color, clear);
    }
    ~SliderFill() {
        if (!enabled_)
            return;
        ImGui::PopStyleColor(5);
        const bool active = ImGui::IsItemActive(), hovered = ImGui::IsItemHovered();
        Splitter().SetCurrentChannel(draw_, 0);
        const auto &style = ImGui::GetStyle();
        const ImVec4 frame = ImGui::GetStyleColorVec4(active ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
        draw_->AddRectFilled(min_, max_, ImGui::GetColorU32(frame), style.FrameRounding);
        const double span = static_cast<double>(maximum_) - static_cast<double>(minimum_);
        const double raw = span != 0 ? (static_cast<double>(*value_) - static_cast<double>(minimum_)) / span : 0;
        const float ratio = std::isfinite(raw) ? static_cast<float>(std::clamp(raw, 0., 1.)) : 0.f;
        const float inset = std::max(1.f, ImGui::GetFontSize() * .14f);
        const float rounding = std::max(0.f, style.FrameRounding - inset);
        ImVec4 accent = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
        accent.w *= active ? .46f : hovered ? .38f : .30f;
        const ImU32 fill = ImGui::GetColorU32(accent);
        if (vertical_) {
            const float y = max_.y - inset - (max_.y - min_.y - 2 * inset) * ratio;
            if (max_.y - inset - y > .5f) {
                draw_->AddRectFilled({min_.x + inset, y}, {max_.x - inset, max_.y - inset}, fill, rounding);
            }
        } else {
            const float x = min_.x + inset + (max_.x - min_.x - 2 * inset) * ratio;
            if (x - min_.x - inset > .5f) {
                draw_->AddRectFilled({min_.x + inset, min_.y + inset}, {x, max_.y - inset}, fill, rounding);
            }
        }
        Splitter().Merge(draw_);
    }
    SliderFill(const SliderFill &) = delete;
    SliderFill &operator=(const SliderFill &) = delete;

  private:
    // Reused between sliders so no allocation happens after warm-up; sliders never nest.
    static ImDrawListSplitter &Splitter() {
        thread_local ImDrawListSplitter splitter;
        return splitter;
    }
    const T *value_;
    T minimum_, maximum_;
    bool vertical_, enabled_;
    ImDrawList *draw_ = nullptr;
    ImVec2 min_{}, max_{};
};

// Combo preview that fits the frame: the stock arrow is replaced by a chevron, so the text is
// shortened with an ellipsis instead of running underneath it.
const char *FitPreview(const char *text, float width, char (&buffer)[256]) {
    if (!text || ImGui::CalcTextSize(text).x <= width)
        return text;
    const char *ellipsis = "...";
    const float room = std::max(0.f, width - ImGui::CalcTextSize(ellipsis).x);
    std::size_t length = std::min<std::size_t>(std::strlen(text), sizeof(buffer) - 4);
    while (length > 0 && ImGui::CalcTextSize(text, text + length).x > room)
        --length;
    while (length > 0 && (static_cast<unsigned char>(text[length]) & 0xC0) == 0x80)
        --length; // Never cut inside a UTF-8 sequence.
    std::memcpy(buffer, text, length);
    std::strcpy(buffer + length, ellipsis);
    return buffer;
}
const char *ZeroSeparatedItem(void *items, int index) {
    const char *item = static_cast<const char *>(items);
    for (; index > 0 && *item; --index)
        item += std::strlen(item) + 1;
    return *item ? item : nullptr;
}
} // namespace

bool Button(const char *label, const ImVec2 &size) {
    return ImGui::Button(label, size);
}

bool Checkbox(const char *label, bool *value) {
    CompactFrame compact;
    return ImGui::Checkbox(label, value);
}

bool RadioButton(const char *label, bool active) {
    CompactFrame compact;
    return ImGui::RadioButton(label, active);
}

bool RadioButton(const char *label, int *value, int buttonValue) {
    CompactFrame compact;
    return ImGui::RadioButton(label, value, buttonValue);
}

bool SliderFloat(const char *label, float *value, float minimum, float maximum, const char *format,
                 ImGuiSliderFlags flags) {
    SliderFill<float> fill(value, minimum, maximum, flags, false, {});
    return ImGui::SliderFloat(label, value, minimum, maximum, format, flags);
}

bool SliderInt(const char *label, int *value, int minimum, int maximum, const char *format, ImGuiSliderFlags flags) {
    SliderFill<int> fill(value, minimum, maximum, flags, false, {});
    return ImGui::SliderInt(label, value, minimum, maximum, format, flags);
}

bool VSliderFloat(const char *label, const ImVec2 &size, float *value, float minimum, float maximum,
                  const char *format, ImGuiSliderFlags flags) {
    SliderFill<float> fill(value, minimum, maximum, flags, true, size);
    return ImGui::VSliderFloat(label, size, value, minimum, maximum, format, flags);
}

// Multi-component sliders: one fill slider per component sharing the item width, like SliderScalarN.
template <class T, class One>
bool SliderComponents(const char *label, T *values, int count, T minimum, T maximum, const char *format,
                      ImGuiSliderFlags flags, One one) {
    const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    const float width = (ImGui::CalcItemWidth() - spacing * static_cast<float>(count - 1)) / static_cast<float>(count);
    bool changed = false;
    ImGui::BeginGroup();
    ImGui::PushID(label);
    for (int i = 0; i < count; ++i) {
        ImGui::PushID(i);
        if (i > 0)
            ImGui::SameLine(0, spacing);
        ImGui::SetNextItemWidth(width);
        changed |= one("", values + i, minimum, maximum, format, flags);
        ImGui::PopID();
    }
    const char *hidden = std::strstr(label, "##");
    if (hidden != label && *label) {
        ImGui::SameLine(0, spacing);
        ImGui::TextUnformatted(label, hidden);
    }
    ImGui::PopID();
    ImGui::EndGroup();
    return changed;
}
bool SliderFloat2(const char *label, float v[2], float minimum, float maximum, const char *format, ImGuiSliderFlags flags) {
    return SliderComponents(label, v, 2, minimum, maximum, format, flags, imkit::SliderFloat);
}
bool SliderFloat3(const char *label, float v[3], float minimum, float maximum, const char *format, ImGuiSliderFlags flags) {
    return SliderComponents(label, v, 3, minimum, maximum, format, flags, imkit::SliderFloat);
}
bool SliderFloat4(const char *label, float v[4], float minimum, float maximum, const char *format, ImGuiSliderFlags flags) {
    return SliderComponents(label, v, 4, minimum, maximum, format, flags, imkit::SliderFloat);
}
bool SliderInt2(const char *label, int v[2], int minimum, int maximum, const char *format, ImGuiSliderFlags flags) {
    return SliderComponents(label, v, 2, minimum, maximum, format, flags, imkit::SliderInt);
}
bool SliderInt3(const char *label, int v[3], int minimum, int maximum, const char *format, ImGuiSliderFlags flags) {
    return SliderComponents(label, v, 3, minimum, maximum, format, flags, imkit::SliderInt);
}
bool SliderInt4(const char *label, int v[4], int minimum, int maximum, const char *format, ImGuiSliderFlags flags) {
    return SliderComponents(label, v, 4, minimum, maximum, format, flags, imkit::SliderInt);
}

bool BeginCombo(const char *label, const char *preview, ImGuiComboFlags flags) {
    constexpr ImGuiComboFlags native = ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview | ImGuiComboFlags_WidthFitPreview;
    if (flags & native)
        return ImGui::BeginCombo(label, preview, flags);
    const auto &style = ImGui::GetStyle();
    auto *draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::CalcItemWidth(), height = ImGui::GetFrameHeight();
    char buffer[256];
    const char *shown = FitPreview(preview, width - height - style.FramePadding.x, buffer);
    const bool open = ImGui::BeginCombo(label, shown, flags | ImGuiComboFlags_NoArrowButton);
    const float size = ImGui::GetFontSize() * .5f;
    detail::DrawChevron(draw, {pos.x + width - height * .5f - style.FramePadding.x * .25f, pos.y + height * .5f}, size,
                        ImGui::GetColorU32(ImGuiCol_Text, .72f), open ? ImGuiDir_Up : ImGuiDir_Down, 1.5f);
    return open;
}

void EndCombo() {
    ImGui::EndCombo();
}

bool Combo(const char *label, int *current, const char *(*getter)(void *, int), void *user, int count, int heightInItems) {
    const auto &style = ImGui::GetStyle();
    const char *preview = *current >= 0 && *current < count ? getter(user, *current) : nullptr;
    if (heightInItems != -1)
        ImGui::SetNextWindowSizeConstraints({0, 0}, {FLT_MAX, (ImGui::GetFontSize() + style.ItemSpacing.y) * static_cast<float>(heightInItems) - style.ItemSpacing.y + style.WindowPadding.y * 2});
    if (!imkit::BeginCombo(label, preview ? preview : ""))
        return false;
    bool changed = false;
    for (int i = 0; i < count; ++i) {
        ImGui::PushID(i);
        const bool selected = i == *current;
        const char *text = getter(user, i);
        if (ImGui::Selectable(text ? text : "*Unknown item*", selected) && !selected) {
            *current = i;
            changed = true;
        }
        if (selected)
            ImGui::SetItemDefaultFocus();
        ImGui::PopID();
    }
    ImGui::EndCombo();
    return changed;
}

bool Combo(const char *label, int *current, const char *const items[], int count, int heightInItems) {
    return imkit::Combo(
        label, current, [](void *data, int index) { return static_cast<const char *const *>(data)[index]; },
        const_cast<void *>(static_cast<const void *>(items)), count, heightInItems);
}

bool Combo(const char *label, int *current, const char *zeroSeparated, int heightInItems) {
    int count = 0;
    for (const char *item = zeroSeparated; *item; item += std::strlen(item) + 1)
        ++count;
    return imkit::Combo(label, current, ZeroSeparatedItem, const_cast<char *>(zeroSeparated), count, heightInItems);
}

bool InputText(const char *label, char *buffer, std::size_t bufferSize, ImGuiInputTextFlags flags,
               ImGuiInputTextCallback callback, void *userData) {
    return ImGui::InputText(label, buffer, bufferSize, flags, callback, userData);
}

bool Selectable(const char *label, bool selected, ImGuiSelectableFlags flags, const ImVec2 &size) {
    bool pressed = ImGui::Selectable(label, selected, flags, size);
    SelectionMark(selected);
    return pressed;
}

bool Selectable(const char *label, bool *selected, ImGuiSelectableFlags flags, const ImVec2 &size) {
    bool pressed = ImGui::Selectable(label, selected, flags, size);
    SelectionMark(*selected);
    return pressed;
}
bool BeginTabItem(const char *label, bool *open, ImGuiTabItemFlags flags) {
    bool active = ImGui::BeginTabItem(label, open, flags);
    SelectionMark(active, true);
    return active;
}
bool TreeNodeEx(const char *label, ImGuiTreeNodeFlags flags) {
    bool open = ImGui::TreeNodeEx(label, flags);
    SelectionMark((flags & ImGuiTreeNodeFlags_Selected) != 0);
    return open;
}
bool TreeNodeExV(const char *id, ImGuiTreeNodeFlags flags, const char *format, va_list args) {
    bool open = ImGui::TreeNodeExV(id, flags, format, args);
    SelectionMark((flags & ImGuiTreeNodeFlags_Selected) != 0);
    return open;
}
bool TreeNodeExV(const void *id, ImGuiTreeNodeFlags flags, const char *format, va_list args) {
    bool open = ImGui::TreeNodeExV(id, flags, format, args);
    SelectionMark((flags & ImGuiTreeNodeFlags_Selected) != 0);
    return open;
}
bool TreeNodeEx(const char *id, ImGuiTreeNodeFlags flags, const char *format, ...) {
    va_list args;
    va_start(args, format);
    bool open = imkit::TreeNodeExV(id, flags, format, args);
    va_end(args);
    return open;
}
bool TreeNodeEx(const void *id, ImGuiTreeNodeFlags flags, const char *format, ...) {
    va_list args;
    va_start(args, format);
    bool open = imkit::TreeNodeExV(id, flags, format, args);
    va_end(args);
    return open;
}

void ProgressBar(float fraction, const ImVec2 &size, const char *overlay) {
    ProgressTrackView view;
    view.fraction = fraction;
    view.label = overlay ? overlay : "";
    const bool fixedWidth = size.x > 0;
    if (fixedWidth) {
        ImGui::BeginChild("##imkit-progress", {size.x, 0}, ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
    }
    ProgressTrack("##imkit-progress-track", view);
    if (fixedWidth)
        ImGui::EndChild();
}

} // namespace imkit
