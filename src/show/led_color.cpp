#include <imkit/show/led.h>
#include "internal.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>

namespace imkit::show {
using namespace impl;

namespace {
float Linear01(float v) { return Clamp01(v); }
float EncodeSrgb(float v) { return v <= .0031308f ? 12.92f * v : 1.055f * std::pow(v, 1.f / 2.4f) - .055f; }
std::uint8_t Byte(float v) { return static_cast<std::uint8_t>(std::lround(Clamp01(v) * 255.f)); }

// One edit phase for a multi-value control: call right after the control's item.
ValueEdit GestureEdit(float valueNow, bool changed) {
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImGuiID key = ImGui::GetID("##original");
    ValueEdit e;
    if (ImGui::IsItemActivated()) {
        st->SetFloat(key, valueNow);
        e.started = true;
    }
    e.finished = ImGui::IsItemDeactivated();
    e.changed = changed;
    e.original = st->GetFloat(key, valueNow);
    e.value = valueNow;
    return e;
}
} // namespace

// ---- Colour maths ----------------------------------------------------------------------------------------------
ImVec4 KelvinToRgb(float kelvin) {
    // Planckian locus approximation (Kim et al., 2002) valid for 1667..25000 K, then CIE XYZ to linear sRGB.
    const double t = std::clamp(static_cast<double>(std::isfinite(kelvin) ? kelvin : 6500.f), 1667.0, 25000.0);
    const double t2 = t * t, t3 = t2 * t;
    const double x = t <= 4000.0 ? -.2661239e9 / t3 - .2343589e6 / t2 + .8776956e3 / t + .179910
                                  : -3.0258469e9 / t3 + 2.1070379e6 / t2 + .2226347e3 / t + .240390;
    const double x2 = x * x, x3 = x2 * x;
    double y;
    if (t <= 2222.0)
        y = -1.1063814 * x3 - 1.34811020 * x2 + 2.18555832 * x - .20219683;
    else if (t <= 4000.0)
        y = -.9549476 * x3 - 1.37418593 * x2 + 2.09137015 * x - .16748867;
    else
        y = 3.0817580 * x3 - 5.87338670 * x2 + 3.75112997 * x - .37001483;
    const double X = x / y, Y = 1.0, Z = (1.0 - x - y) / y;
    double r = 3.2406 * X - 1.5372 * Y - .4986 * Z, g = -.9689 * X + 1.8758 * Y + .0415 * Z, b = .0557 * X - .2040 * Y + 1.0570 * Z;
    r = std::max(0.0, r);
    g = std::max(0.0, g);
    b = std::max(0.0, b);
    const double peak = std::max({r, g, b, 1e-6});
    return {EncodeSrgb(static_cast<float>(r / peak)), EncodeSrgb(static_cast<float>(g / peak)), EncodeSrgb(static_cast<float>(b / peak)), 1.f};
}

LightColor RgbToRgbw(float r, float g, float b) {
    LightColor out;
    r = Linear01(r);
    g = Linear01(g);
    b = Linear01(b);
    const float w = std::min({r, g, b});
    out.r = r - w;
    out.g = g - w;
    out.b = b - w;
    out.w = w;
    return out;
}

ImVec4 MixLightColor(const LightColor& l) {
    const float r = Clamp01(l.r) + Clamp01(l.w) * 1.f + Clamp01(l.a) * 1.f + Clamp01(l.uv) * .45f;
    const float g = Clamp01(l.g) + Clamp01(l.w) * .96f + Clamp01(l.a) * .62f + Clamp01(l.uv) * .1f;
    const float b = Clamp01(l.b) + Clamp01(l.w) * .88f + Clamp01(l.a) * .1f + Clamp01(l.uv) * 1.f;
    const float k = Clamp01(l.intensity);
    return {Clamp01(r) * k, Clamp01(g) * k, Clamp01(b) * k, 1.f};
}

// ---- LED colour picker -----------------------------------------------------------------------------------------
LedColorRequest LedColorPicker(const char* id, LightColor* color, float* kelvin, LedColorOptions o, ComponentOptions options) {
    LedColorRequest request;
    if (!color)
        return request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const float w = o.width > 0 ? o.width : u * 14.f;
    const float round = ControlRound(options);
    ImGui::PushID(id);
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImGuiID hueKey = ImGui::GetID("##hue");
    float h, s, v;
    ImGui::ColorConvertRGBtoHSV(Clamp01(color->r), Clamp01(color->g), Clamp01(color->b), h, s, v);
    float hue = st->GetFloat(hueKey, h);
    if (s > .002f && v > .002f)
        hue = h;
    auto note = [&](const ValueEdit& e, LedColorField field) {
        if (e) {
            request.edit = e;
            request.field = field;
        }
    };
    auto setHsv = [&](float nh, float ns, float nv) {
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(nh, ns, nv, r, g, b);
        color->r = r;
        color->g = g;
        color->b = b;
        hue = nh;
    };
    ImGui::BeginGroup();

    // Preview of the mixed light.
    {
        const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + u * 1.2f};
        ReadoutItem({w, u * 1.2f});
        draw->AddRectFilled(a, b, U32(MixLightColor(*color)), round);
        draw->AddRect(a, b, U32(c.border), round, 0, detail::FrameLineWidth(1.f));
        AnnotateText(options, "Light colour", FrameText("#%02X%02X%02X", Byte(color->r), Byte(color->g), Byte(color->b)), accessibility::SemanticRole::Status);
    }
    // Saturation / value area.
    {
        const float ah = std::max(u * 4.f, w * .55f);
        const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + ah};
        ImGui::PushID("area");
        ImGui::InvisibleButton("##area", {w, ah});
        bool changed = false;
        if (ImGui::IsItemActive()) {
            const ImVec2 m = ImGui::GetIO().MousePos;
            const float ns = Clamp01((m.x - a.x) / w), nv = 1.f - Clamp01((m.y - a.y) / ah);
            changed = ns != s || nv != v;
            setHsv(hue, ns, nv);
            s = ns;
            v = nv;
        }
        note(GestureEdit(v, changed), LedColorField::Area);
        float cr, cg, cb;
        ImGui::ColorConvertHSVtoRGB(hue, 1.f, 1.f, cr, cg, cb);
        const ImU32 pure = U32({cr, cg, cb, 1.f}), white = IM_COL32(255, 255, 255, 255), clear = IM_COL32(0, 0, 0, 0), black = IM_COL32(0, 0, 0, 255);
        draw->AddRectFilledMultiColor(a, b, white, pure, pure, white);
        draw->AddRectFilledMultiColor(a, b, clear, clear, black, black);
        draw->AddRect(a, b, U32(c.border), round * .6f, 0, detail::FrameLineWidth(1.f));
        const ImVec2 m{a.x + s * w, a.y + (1.f - v) * ah};
        draw->AddCircle(m, u * .32f, IM_COL32(0, 0, 0, 200), 16, 3.f);
        draw->AddCircle(m, u * .32f, IM_COL32(255, 255, 255, 255), 16, 1.6f);
        ImGui::PopID();
    }
    // Hue strip.
    {
        const float hh = u * .7f;
        ImGui::Dummy({0, u * .1f});
        const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + hh};
        ImGui::PushID("hue");
        ImGui::InvisibleButton("##hue", {w, hh});
        bool changed = false;
        if (ImGui::IsItemActive()) {
            const float nh = Clamp01((ImGui::GetIO().MousePos.x - a.x) / w);
            changed = nh != hue;
            if (changed)
                setHsv(nh, s, v);
        }
        note(GestureEdit(hue, changed), LedColorField::Hue);
        for (int i = 0; i < 6; ++i) {
            float r0, g0, b0, r1, g1, b1;
            ImGui::ColorConvertHSVtoRGB(static_cast<float>(i) / 6.f, 1.f, 1.f, r0, g0, b0);
            ImGui::ColorConvertHSVtoRGB(static_cast<float>(i + 1) / 6.f, 1.f, 1.f, r1, g1, b1);
            const ImU32 c0 = U32({r0, g0, b0, 1.f}), c1 = U32({r1, g1, b1, 1.f});
            draw->AddRectFilledMultiColor({a.x + w * static_cast<float>(i) / 6.f, a.y}, {a.x + w * static_cast<float>(i + 1) / 6.f, b.y}, c0, c1, c1, c0);
        }
        draw->AddRect(a, b, U32(c.border), 2.f, 0, detail::FrameLineWidth(1.f));
        const float mx = a.x + hue * w;
        draw->AddRectFilled({mx - 3.f, a.y - 2.f}, {mx + 3.f, b.y + 2.f}, IM_COL32(255, 255, 255, 255), 2.f);
        draw->AddRect({mx - 3.f, a.y - 2.f}, {mx + 3.f, b.y + 2.f}, IM_COL32(0, 0, 0, 200), 2.f, 0, 1.f);
        ImGui::PopID();
    }
    st->SetFloat(hueKey, hue);

    // One gradient track per channel.
    auto track = [&](const char* name, float* value, ImVec4 high, LedColorField field, const char* spoken) {
        const float th = u * 1.0f;
        const float labelW = u * 1.4f, valueW = u * 2.4f;
        ImGui::PushID(name);
        const ImVec2 a = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##track", {w, th});
        EditFrame frame(value);
        const float x0 = a.x + labelW, x1 = a.x + w - valueW;
        if (frame.Active())
            *value = Clamp01((ImGui::GetIO().MousePos.x - x0) / std::max(1.f, x1 - x0));
        if (const int dir = detail::AnnotateValue(options, {accessibility::SemanticRole::Slider, spoken, "", "", static_cast<double>(*value), 0, 1, .01, .1}))
            frame.Instant(Clamp01(*value + static_cast<float>(dir) * .01f));
        note(frame.Done(), field);
        const float ty = a.y + th * .5f, tr = std::max(6.f, u * .38f);
        draw->AddText({a.x, a.y + (th - ImGui::GetTextLineHeight()) * .5f}, U32(c.muted), name);
        const ImU32 lo = U32(Recessed(c, .8f)), hi = U32(high);
        draw->AddRectFilledMultiColor({x0, ty - tr * .5f}, {x1, ty + tr * .5f}, lo, hi, hi, lo);
        draw->AddRect({x0, ty - tr * .5f}, {x1, ty + tr * .5f}, U32(c.border), tr * .4f, 0, detail::FrameLineWidth(1.f));
        const ImVec2 tc{x0 + *value * (x1 - x0), ty};
        draw->AddCircleFilled(tc, th * .4f, IM_COL32(255, 255, 255, 255), 16);
        draw->AddCircle(tc, th * .4f, IM_COL32(0, 0, 0, 160), 16, 1.5f);
        const char* text = FrameText("%d", static_cast<int>(Byte(*value)));
        draw->AddText({a.x + w - ImGui::CalcTextSize(text).x, a.y + (th - ImGui::GetTextLineHeight()) * .5f}, U32(c.text), text);
        if (ImGui::IsItemFocused() && ImGui::GetIO().NavVisible)
            draw->AddRect({a.x - 2, a.y - 2}, {a.x + w + 2, a.y + th + 2}, U32(c.focus), 4.f, 0, 2.f);
        ImGui::PopID();
    };
    ImGui::Dummy({0, u * .15f});
    track("R", &color->r, {1, 0, 0, 1}, LedColorField::Red, "Red");
    track("G", &color->g, {0, 1, 0, 1}, LedColorField::Green, "Green");
    track("B", &color->b, {0, .3f, 1, 1}, LedColorField::Blue, "Blue");
    if (o.white)
        track("W", &color->w, {1, .96f, .88f, 1}, LedColorField::White, "White");
    if (o.amber)
        track("A", &color->a, {1, .62f, .1f, 1}, LedColorField::Amber, "Amber");
    if (o.uv)
        track("UV", &color->uv, {.45f, .1f, 1, 1}, LedColorField::Uv, "Ultraviolet");
    if (o.dimmer)
        track("%", &color->intensity, {1, 1, 1, 1}, LedColorField::Intensity, "Intensity");

    // Colour temperature.
    if (o.cct && kelvin) {
        const float th = u * 1.0f, labelW = u * 1.4f, valueW = u * 2.4f;
        const float lo = std::min(o.cctMinimum, o.cctMaximum), hi = std::max(o.cctMinimum, o.cctMaximum);
        ImGui::PushID("cct");
        const ImVec2 a = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##cct", {w, th});
        EditFrame frame(kelvin);
        const float x0 = a.x + labelW, x1 = a.x + w - valueW;
        if (frame.Active())
            *kelvin = lo + Clamp01((ImGui::GetIO().MousePos.x - x0) / std::max(1.f, x1 - x0)) * (hi - lo);
        if (const int dir = detail::AnnotateValue(options, {accessibility::SemanticRole::Slider, "Colour temperature", "", "", static_cast<double>(*kelvin), lo, hi, 50, 500}))
            frame.Instant(std::clamp(*kelvin + static_cast<float>(dir) * 50.f, lo, hi));
        const ValueEdit e = frame.Done();
        if (e) {
            const ImVec4 rgb = KelvinToRgb(*kelvin);
            color->r = rgb.x;
            color->g = rgb.y;
            color->b = rgb.z;
            request.edit = e;
            request.field = LedColorField::Kelvin;
        }
        const float ty = a.y + th * .5f, tr = std::max(6.f, u * .38f);
        draw->AddText({a.x, a.y + (th - ImGui::GetTextLineHeight()) * .5f}, U32(c.muted), "K");
        for (int i = 0; i < 8; ++i) {
            const ImU32 c0 = U32(KelvinToRgb(lo + (hi - lo) * static_cast<float>(i) / 8.f)), c1 = U32(KelvinToRgb(lo + (hi - lo) * static_cast<float>(i + 1) / 8.f));
            draw->AddRectFilledMultiColor({x0 + (x1 - x0) * static_cast<float>(i) / 8.f, ty - tr * .5f}, {x0 + (x1 - x0) * static_cast<float>(i + 1) / 8.f, ty + tr * .5f}, c0, c1, c1, c0);
        }
        draw->AddRect({x0, ty - tr * .5f}, {x1, ty + tr * .5f}, U32(c.border), tr * .4f, 0, detail::FrameLineWidth(1.f));
        const ImVec2 tc{x0 + Clamp01((*kelvin - lo) / std::max(1.f, hi - lo)) * (x1 - x0), ty};
        draw->AddCircleFilled(tc, th * .4f, IM_COL32(255, 255, 255, 255), 16);
        draw->AddCircle(tc, th * .4f, IM_COL32(0, 0, 0, 160), 16, 1.5f);
        const char* text = FrameText("%d", static_cast<int>(std::lround(*kelvin)));
        draw->AddText({a.x + w - ImGui::CalcTextSize(text).x, a.y + (th - ImGui::GetTextLineHeight()) * .5f}, U32(c.text), text);
        ImGui::PopID();
    }

    // Hex entry and swatches.
    if (o.hex) {
        // A small pool keyed by the picker's ID, so two visible pickers (a popup over the main one) never share a typing buffer.
        static thread_local char pool[4][16];
        // The buffer follows the colour until the user starts typing; the previous frame's activity is kept in the state storage.
        const ImGuiID hexKey = ImGui::GetID("##hexActive");
        char* hex = pool[hexKey % 4];
        if (!st->GetBool(hexKey))
            std::snprintf(hex, 16, "#%02X%02X%02X", Byte(color->r), Byte(color->g), Byte(color->b));
        ImGui::SetNextItemWidth(u * 6.f);
        const bool entered = ImGui::InputText("##hex", hex, 16, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        st->SetBool(hexKey, ImGui::IsItemActive());
        if (entered) {
            unsigned r = 0, g = 0, b = 0;
            const char* p = hex[0] == '#' ? hex + 1 : hex;
            if (std::sscanf(p, "%2x%2x%2x", &r, &g, &b) == 3) {
                ValueEdit e;
                e.started = e.changed = e.finished = true;
                e.original = color->r;
                color->r = static_cast<float>(r) / 255.f;
                color->g = static_cast<float>(g) / 255.f;
                color->b = static_cast<float>(b) / 255.f;
                e.value = color->r;
                note(e, LedColorField::Hex);
            }
        }
    }
    if (!o.swatches.empty()) {
        if (o.hex)
            ImGui::SameLine();
        for (std::size_t i = 0; i < o.swatches.size(); ++i) {
            if (i > 0)
                ImGui::SameLine(0, u * .2f);
            ImGui::PushID(static_cast<int>(i));
            const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + u * 1.0f, a.y + u * 1.0f};
            ImGui::InvisibleButton("##swatch", {u * 1.0f, u * 1.0f});
            if (ImGui::IsItemClicked()) {
                ValueEdit e;
                e.started = e.changed = e.finished = true;
                e.original = color->r;
                color->r = o.swatches[i].x;
                color->g = o.swatches[i].y;
                color->b = o.swatches[i].z;
                e.value = color->r;
                note(e, LedColorField::Swatch);
            }
            draw->AddRectFilled(a, b, U32({o.swatches[i].x, o.swatches[i].y, o.swatches[i].z, 1.f}), round * .6f);
            draw->AddRect(a, b, U32(ImGui::IsItemHovered() ? c.text : c.border), round * .6f, 0, detail::FrameLineWidth(1.f));
            SemanticItem(options, FrameText("Swatch #%02X%02X%02X", Byte(o.swatches[i].x), Byte(o.swatches[i].y), Byte(o.swatches[i].z)),
                         accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
            ImGui::PopID();
        }
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return request;
}

// ---- Gradient --------------------------------------------------------------------------------------------------
ImVec4 SampleGradient(std::span<const GradientStop> stops, float t, GradientInterpolation mode) {
    if (stops.empty())
        return {0, 0, 0, 0};
    t = Clamp01(t);
    int lo = -1, hi = -1;
    for (int i = 0; i < static_cast<int>(stops.size()); ++i) {
        const float p = stops[static_cast<std::size_t>(i)].position;
        if (p <= t && (lo < 0 || p >= stops[static_cast<std::size_t>(lo)].position))
            lo = i;
        if (p >= t && (hi < 0 || p <= stops[static_cast<std::size_t>(hi)].position))
            hi = i;
    }
    if (lo < 0)
        return stops[static_cast<std::size_t>(hi)].color;
    if (hi < 0 || lo == hi)
        return stops[static_cast<std::size_t>(lo)].color;
    const GradientStop& a = stops[static_cast<std::size_t>(lo)];
    const GradientStop& b = stops[static_cast<std::size_t>(hi)];
    const float span = b.position - a.position;
    if (span <= 1e-6f)
        return b.color;
    float f = (t - a.position) / span;
    if (mode == GradientInterpolation::Step)
        return a.color;
    if (mode == GradientInterpolation::Smooth)
        f = f * f * (3.f - 2.f * f);
    return Mix(a.color, b.color, f);
}

GradientRequest GradientEditor(const char* id, const GradientEditorView& view, int* selected, GradientEditorOptions o, ComponentOptions options) {
    GradientRequest request;
    if (!view.stops || !view.count || view.capacity < 1)
        return request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    int& count = *view.count;
    count = std::clamp(count, 0, view.capacity);
    int local = -1;
    int& sel = selected ? *selected : local;
    const float w = std::max(u * 8.f, o.width > 0 ? o.width : ImGui::GetContentRegionAvail().x);
    const float barH = o.height > 0 ? o.height : u * 2.f, markerH = u * 1.1f;
    const float round = ControlRound(options) * .8f;
    auto stops = [&] { return std::span<GradientStop>(view.stops, static_cast<std::size_t>(count)); };
    ImGui::PushID(id);
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + w, a.y + barH};

    // The bar: double-click adds a stop.
    ImGui::InvisibleButton("##bar", {w, barH});
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && count < view.capacity) {
        const float t = Clamp01((io.MousePos.x - a.x) / w);
        view.stops[count] = {t, SampleGradient(stops(), t, view.interpolation)};
        sel = count;
        request.added = true;
        request.stop = count;
        ++count;
    }
    {
        const int steps = std::clamp(static_cast<int>(w / 3.f), 8, 256);
        draw->PushClipRect(a, b, true);
        for (int i = 0; i < steps; ++i) {
            const float t0 = static_cast<float>(i) / static_cast<float>(steps), t1 = static_cast<float>(i + 1) / static_cast<float>(steps);
            const ImVec2 p0{a.x + w * t0, a.y}, p1{a.x + w * t1 + 1.f, b.y};
            if (view.interpolation == GradientInterpolation::Step) {
                draw->AddRectFilled(p0, p1, U32(SampleGradient(stops(), (t0 + t1) * .5f, view.interpolation)));
            } else {
                const ImU32 c0 = U32(SampleGradient(stops(), t0, view.interpolation)), c1 = U32(SampleGradient(stops(), t1, view.interpolation));
                draw->AddRectFilledMultiColor(p0, p1, c0, c1, c1, c0);
            }
        }
        draw->PopClipRect();
        draw->AddRect(a, b, U32(c.border), round, 0, detail::FrameLineWidth(1.f));
    }

    // Markers below the bar.
    const float markerW = u * .9f;
    int removeIndex = -1;
    for (int i = 0; i < count; ++i) {
        GradientStop& stop = view.stops[i];
        stop.position = Clamp01(stop.position);
        const float cx = a.x + stop.position * w;
        ImGui::PushID(i);
        ImGui::SetCursorScreenPos({cx - markerW * .5f, b.y + u * .1f});
        ImGui::InvisibleButton("##stop", {markerW, markerH});
        if (ImGui::IsItemActivated()) {
            sel = i;
            request.stop = i;
        }
        const float before = stop.position;
        if (ImGui::IsItemActive())
            stop.position = Clamp01((io.MousePos.x - a.x) / w);
        const ValueEdit e = GestureEdit(stop.position, stop.position != before);
        if (e) {
            request.edit = e;
            request.stop = i;
        }
        if (const int dir = detail::AnnotateValue(options, {accessibility::SemanticRole::Slider, FrameText("Gradient stop %d", i + 1), "", "",
                                                            static_cast<double>(stop.position), 0, 1, .01, .1})) {
            stop.position = Clamp01(stop.position + static_cast<float>(dir) * .01f);
            request.edit.started = request.edit.changed = request.edit.finished = true;
            request.edit.value = stop.position;
            request.stop = i;
        }
        if ((ImGui::IsItemFocused() || ImGui::IsItemHovered()) && sel == i && count > o.minimumStops &&
            (ImGui::IsKeyPressed(ImGuiKey_Delete, false) || ImGui::IsKeyPressed(ImGuiKey_Backspace, false)))
            removeIndex = i;
        const bool isSel = sel == i;
        const ImVec2 apex{cx, b.y + 1.f};
        draw->AddTriangleFilled(apex, {cx - markerW * .35f, b.y + u * .45f}, {cx + markerW * .35f, b.y + u * .45f}, U32(isSel ? c.accent : c.muted));
        const ImVec2 sa{cx - markerW * .4f, b.y + u * .45f}, sb{cx + markerW * .4f, b.y + u * .1f + markerH};
        draw->AddRectFilled(sa, sb, U32({stop.color.x, stop.color.y, stop.color.z, 1.f}), 3.f);
        draw->AddRect(sa, sb, U32(isSel ? c.accent : c.border), 3.f, 0, isSel ? 2.f : detail::FrameLineWidth(1.f));
        ImGui::PopID();
    }
    ImGui::SetCursorScreenPos({a.x, b.y + u * .1f});
    ImGui::Dummy({w, markerH + u * .1f});

    // Selected stop: colour, position and removal.
    if (sel >= 0 && sel < count) {
        GradientStop& stop = view.stops[sel];
        const ImVec2 sa = ImGui::GetCursorScreenPos(), sb{sa.x + u * 1.8f, sa.y + u * 1.2f};
        ImGui::InvisibleButton("##color", {u * 1.8f, u * 1.2f});
        if (ImGui::IsItemClicked())
            ImGui::OpenPopup("##stopcolor");
        draw->AddRectFilled(sa, sb, U32({stop.color.x, stop.color.y, stop.color.z, 1.f}), round);
        draw->AddRect(sa, sb, U32(c.border), round, 0, detail::FrameLineWidth(1.f));
        SemanticItem(options, "Stop colour", accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
        if (ImGui::BeginPopup("##stopcolor")) {
            LightColor light{stop.color.x, stop.color.y, stop.color.z};
            LedColorOptions co;
            co.dimmer = false;
            co.swatches = o.swatches;
            const LedColorRequest r = LedColorPicker("picker", &light, nullptr, co, options);
            if (r.edit) {
                stop.color = {light.r, light.g, light.b, 1.f};
                request.colorChanged = true;
                request.stop = sel;
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(c.muted, "%d%%", static_cast<int>(std::lround(stop.position * 100.f)));
        ImGui::SameLine();
        ImGui::BeginDisabled(count <= o.minimumStops);
        if (PillButton("remove", Str(options, "show.removeStop", "Remove"), c, options, c.muted))
            removeIndex = sel;
        ImGui::EndDisabled();
    }
    if (removeIndex >= 0 && removeIndex < count && count > o.minimumStops) {
        for (int i = removeIndex; i + 1 < count; ++i)
            view.stops[i] = view.stops[i + 1];
        --count;
        request.removed = true;
        request.stop = removeIndex;
        sel = std::min(sel, count - 1);
    }
    ImGui::PopID();
    return request;
}

// ---- Colour order ----------------------------------------------------------------------------------------------
std::size_t ColorOrderChoices(bool withWhite, std::span<std::array<char, 5>> out) {
    const int n = withWhite ? 4 : 3;
    const char letters[5] = {'R', 'G', 'B', 'W', 0};
    int index[4] = {0, 1, 2, 3};
    std::size_t written = 0;
    do {
        if (written < out.size()) {
            for (int i = 0; i < n; ++i)
                out[written][static_cast<std::size_t>(i)] = letters[index[i]];
            out[written][static_cast<std::size_t>(n)] = 0;
        }
        ++written;
    } while (std::next_permutation(index, index + n));
    return std::min(written, out.size());
}

ColorOrderRequest ColorOrderPicker(const char* id, char* order, bool withWhite, ComponentOptions options) {
    ColorOrderRequest request;
    if (!order)
        return request;
    const ShowColors c = MakeShowColors(options);
    std::array<std::array<char, 5>, 24> choices{};
    const std::size_t count = ColorOrderChoices(withWhite, choices);
    ImGui::PushID(id);
    const float right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
    for (std::size_t i = 0; i < count; ++i) {
        // The accessibility node borrows the name until the frame ends, so it must not point into this function's stack.
        const char* text = FrameText("%s", choices[i].data());
        const bool on = std::strncmp(order, text, 4) == 0;
        if (i > 0) {
            const float next = ImGui::CalcTextSize(text).x + Unit() * 1.2f;
            if (FitsOnLine(right, next, ImGui::GetStyle().ItemSpacing.x))
                ImGui::SameLine();
        }
        if (PillButton(text, text, c, options, on ? c.text : c.muted, on ? c.accent : ImVec4{}) && !on) {
            std::snprintf(order, 5, "%s", text);
            request.changed = true;
        }
    }
    ImGui::SameLine();
    if (PillButton("test", Str(options, "show.testOrder", "Test"), c, options, c.text, {}))
        request.test = true;
    ImGui::PopID();
    return request;
}
} // namespace imkit::show
