#include <imkit/show/image.h>
#include "internal.h"
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>

namespace imkit::show {
using namespace impl;

namespace {
float Finite(float v, float fallback) { return std::isfinite(v) ? v : fallback; }

// Small picture glyph (frame, hill and sun) for the empty state.
void GlyphPicture(ImDrawList* d, ImVec2 c, float s, ImU32 col) {
    const float w = s * .56f, h = s * .44f;
    d->AddRect({c.x - w, c.y - h}, {c.x + w, c.y + h}, col, s * .1f, 0, 1.5f);
    d->AddPolyline(std::array<ImVec2, 4>{ImVec2{c.x - w, c.y + h * .6f}, {c.x - w * .3f, c.y - h * .1f}, {c.x + w * .1f, c.y + h * .35f}, {c.x + w, c.y - h * .2f}}.data(),
                   4, col, 0, 1.5f);
    d->AddCircle({c.x + w * .45f, c.y - h * .5f}, s * .09f, col, 10, 1.5f);
}
} // namespace

ImageViewport FitImage(ImVec2 box, ImVec2 imagePixels, ImageFit fit, ImageCrop crop) {
    ImageViewport v;
    v.max = box;
    if (!(box.x > 0 && box.y > 0))
        return v;
    const float imageAspect = imagePixels.x > 0 && imagePixels.y > 0 ? imagePixels.x / imagePixels.y : 1.f;
    const float boxAspect = box.x / box.y;
    const float zoom = std::clamp(Finite(crop.zoom, 1.f), 1.f, 16.f);
    if (fit == ImageFit::Contain && zoom <= 1.0001f) {
        if (imageAspect > boxAspect) {
            const float h = box.x / imageAspect;
            v.min = {0, (box.y - h) * .5f};
            v.max = {box.x, (box.y + h) * .5f};
        } else {
            const float w = box.y * imageAspect;
            v.min = {(box.x - w) * .5f, 0};
            v.max = {(box.x + w) * .5f, box.y};
        }
        return v;
    }
    float wf = 1.f, hf = 1.f;
    if (fit != ImageFit::Stretch) {
        if (imageAspect > boxAspect)
            wf = boxAspect / imageAspect;
        else
            hf = imageAspect / boxAspect;
    }
    wf /= zoom;
    hf /= zoom;
    const float cx = std::clamp(Finite(crop.centerX, .5f), wf * .5f, 1.f - wf * .5f);
    const float cy = std::clamp(Finite(crop.centerY, .5f), hf * .5f, 1.f - hf * .5f);
    v.uv0 = {cx - wf * .5f, cy - hf * .5f};
    v.uv1 = {cx + wf * .5f, cy + hf * .5f};
    return v;
}

ImageButtonRequest ImageButton(const char* id, const ImageButtonView& view, ImageButtonOptions o, ComponentOptions options) {
    ImageButtonRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = u * 4.f;
    if (size.y <= 0)
        size.y = size.x;
    const bool hasImage = HasImage(view.image);
    const bool toggle = o.mode == ConsoleMode::Toggle;
    const ImVec4 accent = view.accent.w > 0 ? view.accent : c.accent;
    ImGui::PushID(id);
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
    const float r = view.shape == ImageShape::Circle ? std::min(size.x, size.y) * .5f : ControlRound(options) * 1.2f;
    ImGui::InvisibleButton("##image", size);
    bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive(), focused = ImGui::IsItemFocused();
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImGuiID validKey = ImGui::GetID("##valid");

    // Non-rectangular hit test: ignore presses over transparent pixels.
    bool solid = true;
    if (o.alpha && hovered && hasImage) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        const ImageViewport vp = FitImage(size, view.imagePixels, view.fit, view.crop);
        const float px = m.x - a.x, py = m.y - a.y;
        if (px < vp.min.x || px > vp.max.x || py < vp.min.y || py > vp.max.y)
            solid = false;
        else {
            const float tx = (px - vp.min.x) / std::max(1.f, vp.max.x - vp.min.x), ty = (py - vp.min.y) / std::max(1.f, vp.max.y - vp.min.y);
            solid = o.alpha(vp.uv0.x + tx * (vp.uv1.x - vp.uv0.x), vp.uv0.y + ty * (vp.uv1.y - vp.uv0.y), o.alphaUser) >= .1f;
        }
        if (!solid)
            hovered = false;
    }
    if (ImGui::IsItemActivated())
        st->SetBool(validKey, solid);
    const bool valid = st->GetBool(validKey, true);
    request.hovered = hovered;
    if (ImGui::IsItemActivated() && valid) {
        request.pressed = true;
        if (toggle)
            request.changed = true;
        if (o.editable && !hasImage)
            request.chooseImage = true;
    }
    if (ImGui::IsItemDeactivated() && valid)
        request.released = true;
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        request.secondary = true;
        if (o.editable)
            ImGui::OpenPopup("##imagemenu");
    }
    bool dropping = false;
    if (o.editable && o.dropPayload && ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(o.dropPayload, ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
            dropping = true;
            if (p->IsDelivery())
                request.payload = p;
        }
        ImGui::EndDragDropTarget();
    }
    const char* name = view.label && view.label[0] ? view.label : "Image button";
    const bool byTool = toggle ? SemanticItem(options, name, accessibility::SemanticRole::Toggle, accessibility::SemanticAction::Toggle, view.on)
                               : SemanticItem(options, name, accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
    if (byTool) {
        request.pressed = request.released = true;
        if (toggle)
            request.changed = true;
    }

    // Body and picture.
    const bool lit = toggle ? (view.on != request.changed) : (held && valid);
    const ImVec2 inset = held && valid ? ImVec2{1.5f, 1.5f} : ImVec2{0, 0};
    const ImVec2 ia{a.x + inset.x, a.y + inset.y}, ib{b.x - inset.x, b.y - inset.y};
    draw->AddRectFilled(ia, ib, U32(c.input), r);
    if (hasImage) {
        const ImageViewport vp = FitImage({ib.x - ia.x, ib.y - ia.y}, view.imagePixels, view.fit, view.crop);
        draw->AddImageRounded(view.image, {ia.x + vp.min.x, ia.y + vp.min.y}, {ia.x + vp.max.x, ia.y + vp.max.y}, vp.uv0, vp.uv1,
                              ImGui::ColorConvertFloat4ToU32(view.tint), vp.min.x > 0 || vp.min.y > 0 ? 0.f : r);
    } else {
        detail::DrawDashedRect(draw, ia, ib, r, U32(c.muted, dropping ? .95f : .55f), 1.f, std::max(3.f, u * .12f));
        const ImVec2 mid{(ia.x + ib.x) * .5f, (ia.y + ib.y) * .5f};
        GlyphPicture(draw, {mid.x, mid.y - (o.editable ? u * .25f : 0.f)}, std::min(size.x, size.y) * .34f, U32(c.muted));
        if (o.editable)
            DrawTextFit(draw, {ia.x + u * .3f, mid.y + std::min(size.x, size.y) * .2f}, ib.x - ia.x - u * .6f, U32(c.muted),
                        Str(options, "show.dropImage", "Drop an image"));
    }
    if (view.error) {
        draw->AddRectFilled(ia, ib, U32(c.error, .28f), r);
        detail::DrawCross(draw, {(ia.x + ib.x) * .5f, (ia.y + ib.y) * .5f}, std::min(size.x, size.y) * .16f, U32(c.error), 2.f);
    }
    if (hovered)
        draw->AddRectFilled(ia, ib, IM_COL32(255, 255, 255, held ? 0 : 18), r);
    if (held && valid)
        draw->AddRectFilled(ia, ib, IM_COL32(0, 0, 0, 40), r);

    // Label band over the picture, or below it.
    const bool hasLabel = view.label && view.label[0] && o.labelStyle != ImageLabel::None;
    if (hasLabel && o.labelStyle == ImageLabel::Overlay && hasImage) {
        const float band = u * 1.25f;
        draw->PushClipRect({ia.x, ib.y - band}, ib, true);
        draw->AddRectFilled(ia, ib, IM_COL32(0, 0, 0, 150), r);
        draw->PopClipRect();
        const float line = ImGui::GetTextLineHeight(), y = ib.y - u * .3f - line;
        const float capW = view.caption && view.caption[0] ? ImGui::CalcTextSize(view.caption).x * .82f + u * .3f : 0.f;
        draw->PushClipRect(ia, ib, true);
        const bool round = view.shape == ImageShape::Circle;
        const float labelW = std::min(ImGui::CalcTextSize(view.label).x, ib.x - ia.x - u * .8f - capW);
        const float labelX = round ? ia.x + ((ib.x - ia.x) - labelW) * .5f : ia.x + u * .4f;
        DrawTextFit(draw, {labelX, y}, round ? labelW + 1.f : ib.x - ia.x - u * .8f - capW, IM_COL32(255, 255, 255, 255), view.label);
        if (capW > 0)
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .82f, {ib.x - u * .4f - capW + u * .3f, y + line * .09f}, IM_COL32(255, 255, 255, 190), view.caption);
        draw->PopClipRect();
    } else if (hasLabel && o.labelStyle == ImageLabel::Overlay) {
        draw->PushClipRect(ia, ib, true);
        DrawTextFit(draw, {ia.x + u * .4f, ib.y - u * .3f - ImGui::GetTextLineHeight()}, ib.x - ia.x - u * .8f, U32(c.muted), view.label);
        draw->PopClipRect();
    }
    if (view.progress >= 0) {
        const float t = Clamp01(view.progress), h = std::max(3.f, u * .12f);
        draw->PushClipRect({ia.x, ib.y - h}, {ia.x + (ib.x - ia.x) * t, ib.y}, true);
        draw->AddRectFilled(ia, ib, U32(accent), r);
        draw->PopClipRect();
    }

    // Border, state ring, badge and focus.
    draw->AddRect(ia, ib, U32(c.border, .8f), r, 0, 1.f);
    if (lit || dropping) {
        draw->AddRect({ia.x - 2, ia.y - 2}, {ib.x + 2, ib.y + 2}, U32(accent, .3f), r + 2.f, 0, 2.f);
        draw->AddRect(ia, ib, U32(accent), r, 0, 2.f);
    }
    if (view.badge > 0) {
        const ImVec2 bc{ib.x - u * .15f, ia.y + u * .15f};
        const char* text = FrameText("%d", std::min(view.badge, 99));
        const ImVec2 ts = ImGui::CalcTextSize(text);
        const float br = std::max(u * .42f, ts.x * .5f + u * .12f);
        draw->AddCircleFilled(bc, br, U32(c.accent), 18);
        draw->AddText({bc.x - ts.x * .5f, bc.y - ts.y * .5f}, U32(c.onAccent), text);
    }
    if (focused && ImGui::GetIO().NavVisible)
        draw->AddRect({a.x - 3, a.y - 3}, {b.x + 3, b.y + 3}, U32(c.focus), r + 3.f, 0, 2.f);
    DrawBindingDot(draw, c, o.binding, {b.x - u * .3f, a.y + u * .3f}, u * .08f, options);

    if (o.editable && ImGui::BeginPopup("##imagemenu")) {
        if (ImGui::MenuItem(Str(options, "show.chooseImage", "Choose image...")))
            request.chooseImage = true;
        if (ImGui::MenuItem(Str(options, "show.adjustImage", "Adjust position..."), nullptr, false, hasImage))
            request.adjustImage = true;
        if (ImGui::MenuItem(Str(options, "show.clearImage", "Remove image"), nullptr, false, hasImage))
            request.clearImage = true;
        ImGui::EndPopup();
    }
    if (hasLabel && o.labelStyle == ImageLabel::Below) {
        ImGui::SetCursorScreenPos({a.x, b.y + u * .15f});
        const float line = ImGui::GetTextLineHeight();
        const ImVec2 la = ImGui::GetCursorScreenPos();
        DrawTextFit(draw, la, size.x, U32(c.text), view.label);
        ImGui::Dummy({size.x, line});
    }
    ImGui::PopID();
    return request;
}

ValueEdit ImageCropEditor(const char* id, const ImageButtonView& view, ImageCrop* crop, ImVec2 size, ComponentOptions options) {
    ValueEdit edit;
    if (!crop)
        return edit;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    if (size.x <= 0)
        size.x = u * 9.f;
    if (size.y <= 0)
        size.y = size.x;
    const ImageFit fit = view.fit == ImageFit::Contain ? ImageFit::Cover : view.fit; // Panning needs room to move.
    ImGui::PushID(id);
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImGuiID originalKey = ImGui::GetID("##original");
    const ImageCrop before = *crop;

    // The picture: drag to pan, wheel to zoom.
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
    ImGui::InvisibleButton("##crop", size);
    const bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
    if (ImGui::IsItemActivated()) {
        st->SetFloat(originalKey, crop->zoom);
        edit.started = true;
    }
    if (ImGui::IsItemDeactivated())
        edit.finished = true;
    if (active) {
        const ImageViewport vp = FitImage(size, view.imagePixels, fit, *crop);
        const ImVec2 d = ImGui::GetIO().MouseDelta;
        crop->centerX = std::clamp(Finite(crop->centerX, .5f) - d.x / size.x * (vp.uv1.x - vp.uv0.x), 0.f, 1.f);
        crop->centerY = std::clamp(Finite(crop->centerY, .5f) - d.y / size.y * (vp.uv1.y - vp.uv0.y), 0.f, 1.f);
    }
    if (hovered && ImGui::GetIO().MouseWheel != 0.f) {
        st->SetFloat(originalKey, crop->zoom);
        crop->zoom = std::clamp(Finite(crop->zoom, 1.f) * (1.f + ImGui::GetIO().MouseWheel * .1f), 1.f, 8.f);
        edit.started = edit.finished = true;
    }
    const ImageViewport vp = FitImage(size, view.imagePixels, fit, *crop);
    const float r = view.shape == ImageShape::Circle ? std::min(size.x, size.y) * .5f : ControlRound(options) * 1.2f;
    draw->AddRectFilled(a, b, U32(c.input), r);
    if (HasImage(view.image))
        draw->AddImageRounded(view.image, a, b, vp.uv0, vp.uv1, ImGui::ColorConvertFloat4ToU32(view.tint), r);
    if (active)
        for (int k = 1; k <= 2; ++k) { // Rule-of-thirds guides while dragging.
            const float fx = a.x + size.x * static_cast<float>(k) / 3.f, fy = a.y + size.y * static_cast<float>(k) / 3.f;
            draw->AddLine({fx, a.y}, {fx, b.y}, IM_COL32(255, 255, 255, 70), 1.f);
            draw->AddLine({a.x, fy}, {b.x, fy}, IM_COL32(255, 255, 255, 70), 1.f);
        }
    draw->AddRect(a, b, U32(active ? c.accent : c.border, .9f), r, 0, active ? 2.f : 1.f);
    SemanticItem(options, Str(options, "show.adjustImage", "Adjust position..."), accessibility::SemanticRole::Group,
                 accessibility::SemanticAction::None);

    // Zoom slider.
    ImGui::SetNextItemWidth(size.x);
    ImGui::SliderFloat("##zoom", &crop->zoom, 1.f, 8.f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
    if (ImGui::IsItemActivated()) {
        st->SetFloat(originalKey, before.zoom);
        edit.started = true;
    }
    if (ImGui::IsItemDeactivated())
        edit.finished = true;
    edit.changed = crop->centerX != before.centerX || crop->centerY != before.centerY || crop->zoom != before.zoom;
    edit.original = st->GetFloat(originalKey, crop->zoom);
    edit.value = crop->zoom;
    ImGui::PopID();
    return edit;
}
} // namespace imkit::show
