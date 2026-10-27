#include <imkit/show/schedule.h>
#include "internal.h"
#include <imkit/components.h>
#include <imkit/widgets.h>
#include <algorithm>
#include <cfloat>
#include <climits>
#include <cstdio>
#include <cstring>
#include <vector>

namespace imkit::show {
using namespace impl;

namespace {
constexpr float Pi = 3.14159265358979323846f;
const char* const MonthNamesShort[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char* const MonthNamesLong[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
const char* const WeekdayShort[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

const char* DateText(std::int32_t day) {
    int y, m, d;
    CivilFromDays(day, y, m, d);
    return FrameText("%04d-%02d-%02d", y, m, d);
}
const char* DayTitle(std::int32_t day) {
    int y, m, d;
    CivilFromDays(day, y, m, d);
    return FrameText("%s, %s %d", WeekdayShort[WeekdayFromDays(day)], MonthNamesShort[m - 1], d);
}
const char* ClockText(std::int64_t utc, TimeZone zone) {
    const LocalTime t = ToLocal(utc, zone);
    return FrameText("%02d:%02d", t.hour, t.minute);
}
const char* RangeText(std::int64_t start, std::int64_t end, TimeZone zone) {
    const LocalTime a = ToLocal(start, zone);
    const LocalTime b = ToLocal(end, zone);
    return FrameText("%s %04d-%02d-%02d  %02d:%02d - %02d:%02d", WeekdayShort[a.weekday], a.year, a.month, a.day, a.hour, a.minute, b.hour, b.minute);
}
std::int32_t WeekStart(std::int32_t day, int firstWeekday) {
    const int offset = (WeekdayFromDays(day) - firstWeekday + 7) % 7;
    return day - offset;
}
std::int32_t AddMonths(std::int32_t day, int months) {
    int y, m, d;
    CivilFromDays(day, y, m, d);
    const int index = y * 12 + (m - 1) + months;
    const int ny = index / 12, nm = index % 12 + 1;
    return DaysFromCivil(ny, nm, std::min(d, DaysInMonth(ny, nm)));
}

// Pill button used by several editors.
struct Pill {
    const ShowColors& c;
    const ComponentOptions& o;
    float u;
    bool operator()(const char* id, const char* text, ImVec4 ink, ImVec4 tone = {}, bool dashed = false) const {
        auto* draw = ImGui::GetWindowDrawList();
        ImGui::PushID(id);
        const ImVec2 ts = ImGui::CalcTextSize(text, nullptr, true);
        const ImVec2 size{ts.x + u * .9f, u * 1.0f};
        const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
        ImGui::InvisibleButton("##pill", size);
        const bool hovered = ImGui::IsItemHovered(), clicked = ImGui::IsItemClicked(), focused = ImGui::IsItemFocused();
        const ImVec4 fill = tone.w > 0 ? Mix(c.input, tone, .2f + (hovered ? .08f : 0.f)) : Mix(c.input, c.text, hovered ? .09f : .03f);
        draw->AddRectFilled(a, b, U32(fill), size.y * .4f);
        if (dashed)
            detail::DrawDashedRect(draw, a, b, size.y * .4f, U32(c.muted, .7f), 1.f, 3.f);
        else
            draw->AddRect(a, b, U32(tone.w > 0 ? Mix(c.border, tone, .6f) : c.border, .8f), size.y * .4f, 0, 1.f);
        draw->AddText({a.x + u * .45f, a.y + (size.y - ts.y) * .5f}, U32(ink), text);
        if (focused && ImGui::GetIO().NavVisible)
            draw->AddRect({a.x - 2, a.y - 2}, {b.x + 2, b.y + 2}, U32(c.focus), size.y * .4f + 2.f, 0, 2.f);
        SemanticItem(o, text, accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
        ImGui::PopID();
        return clicked;
    }
};
} // namespace

// ---- DatePicker ------------------------------------------------------------------------------------------------
bool DatePicker(const char* id, std::int32_t* day, DatePickerOptions o, ComponentOptions options, std::int32_t* rangeEnd) {
    if (!day)
        return false;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const float cell = u * 1.4f;
    ImGui::PushID(id);
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImGuiID viewKey = ImGui::GetID("##view"), awaitKey = ImGui::GetID("##await");
    int y, m, d;
    CivilFromDays(*day, y, m, d);
    int view = st->GetInt(viewKey, INT_MIN);
    if (view == INT_MIN)
        view = y * 12 + m - 1;
    bool changed = false;
    const float width = cell * 7.f;

    ImGui::BeginGroup();
    // Header: previous, title, next.
    {
        const ImVec2 a = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##prev", {cell, cell});
        if (ImGui::IsItemClicked())
            --view;
        SemanticItem(options, "Previous month", accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
        detail::DrawChevron(draw, {a.x + cell * .5f, a.y + cell * .5f}, u * .45f, U32(ImGui::IsItemHovered() ? c.text : c.muted), ImGuiDir_Left, 1.6f);
        ImGui::SameLine(0, 0);
        const char* title = FrameText("%s %d", MonthNamesLong[((view % 12) + 12) % 12], view >= 0 ? view / 12 : (view - 11) / 12);
        const ImVec2 ts = ImGui::CalcTextSize(title);
        const ImVec2 ta = ImGui::GetCursorScreenPos();
        ReadoutItem({width - cell * 2.f, cell});
        draw->AddText({ta.x + (width - cell * 2.f - ts.x) * .5f, ta.y + (cell - ts.y) * .5f}, U32(c.text), title);
        AnnotateText(options, "Month", title, accessibility::SemanticRole::Status);
        ImGui::SameLine(0, 0);
        const ImVec2 b = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##next", {cell, cell});
        if (ImGui::IsItemClicked())
            ++view;
        SemanticItem(options, "Next month", accessibility::SemanticRole::Button, accessibility::SemanticAction::Press);
        detail::DrawChevron(draw, {b.x + cell * .5f, b.y + cell * .5f}, u * .45f, U32(ImGui::IsItemHovered() ? c.text : c.muted), ImGuiDir_Right, 1.6f);
    }
    // Weekday labels.
    {
        const ImVec2 a = ImGui::GetCursorScreenPos();
        ImGui::Dummy({width, cell * .8f});
        for (int i = 0; i < 7; ++i) {
            const char* name = WeekdayShort[(i + o.firstWeekday) % 7];
            const ImVec2 ts = ImGui::CalcTextSize(name);
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .85f, {a.x + static_cast<float>(i) * cell + (cell - ts.x * .85f) * .5f, a.y + cell * .15f}, U32(c.muted), name);
        }
    }
    const int viewYear = view >= 0 ? view / 12 : (view - 11) / 12, viewMonth = ((view % 12) + 12) % 12 + 1;
    const std::int32_t firstOfMonth = DaysFromCivil(viewYear, viewMonth, 1);
    const std::int32_t gridStart = WeekStart(firstOfMonth, o.firstWeekday);
    const std::int32_t lo = rangeEnd ? std::min(*day, *rangeEnd) : *day, hi = rangeEnd ? std::max(*day, *rangeEnd) : *day;
    const bool awaiting = st->GetBool(awaitKey, false);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    for (int i = 0; i < 42; ++i) {
        const std::int32_t cellDay = gridStart + i;
        int cy, cm, cd;
        CivilFromDays(cellDay, cy, cm, cd);
        const ImVec2 a{origin.x + static_cast<float>(i % 7) * cell, origin.y + static_cast<float>(i / 7) * cell};
        const ImVec2 b{a.x + cell, a.y + cell};
        ImGui::SetCursorScreenPos(a);
        ImGui::PushID(i);
        const bool allowed = cellDay >= o.minimumDay && cellDay <= o.maximumDay;
        ImGui::InvisibleButton("##day", {cell, cell});
        const bool hovered = ImGui::IsItemHovered() && allowed, clicked = ImGui::IsItemClicked() && allowed;
        if (clicked) {
            if (rangeEnd && !awaiting) {
                *day = *rangeEnd = cellDay;
                st->SetBool(awaitKey, true);
            } else if (rangeEnd) {
                *rangeEnd = cellDay;
                if (*rangeEnd < *day)
                    std::swap(*day, *rangeEnd);
                st->SetBool(awaitKey, false);
            } else {
                *day = cellDay;
            }
            changed = true;
        }
        SemanticItem(options, DateText(cellDay), accessibility::SemanticRole::Button, accessibility::SemanticAction::Press, cellDay >= lo && cellDay <= hi);
        const bool inRange = rangeEnd && cellDay >= lo && cellDay <= hi;
        const bool selected = cellDay == *day || (rangeEnd && cellDay == *rangeEnd);
        if (inRange)
            draw->AddRectFilled({a.x, a.y + 2}, {b.x, b.y - 2}, U32(c.accent, .16f), 0.f);
        if (selected)
            draw->AddRectFilled({a.x + 2, a.y + 2}, {b.x - 2, b.y - 2}, U32(c.accent), cell * .3f);
        else if (hovered)
            draw->AddRectFilled({a.x + 2, a.y + 2}, {b.x - 2, b.y - 2}, U32(c.text, .08f), cell * .3f);
        if (cellDay == o.today)
            draw->AddRect({a.x + 2, a.y + 2}, {b.x - 2, b.y - 2}, U32(c.accent), cell * .3f, 0, 1.5f);
        const char* text = FrameText("%d", cd);
        const ImVec2 ts = ImGui::CalcTextSize(text);
        const ImVec4 ink = selected ? c.onAccent : !allowed ? Mix(c.muted, c.surface, .5f) : cm == viewMonth ? c.text : c.muted;
        draw->AddText({a.x + (cell - ts.x) * .5f, a.y + (cell - ts.y) * .5f}, U32(ink, cm == viewMonth || selected ? 1.f : .6f), text);
        if (ImGui::IsItemFocused() && ImGui::GetIO().NavVisible)
            draw->AddRect({a.x, a.y}, {b.x, b.y}, U32(c.focus), cell * .3f, 0, 2.f);
        ImGui::PopID();
    }
    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy({width, cell * 6.f});
    ImGui::EndGroup();
    st->SetInt(viewKey, view);
    if (changed) {
        int ny, nm, nd;
        CivilFromDays(*day, ny, nm, nd);
        st->SetInt(viewKey, ny * 12 + nm - 1);
    }
    ImGui::PopID();
    return changed;
}

// ---- TimePicker ------------------------------------------------------------------------------------------------
bool TimePicker(const char* id, int* secondsOfDay, TimePickerOptions o, ComponentOptions options) {
    if (!secondsOfDay)
        return false;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    *secondsOfDay = std::clamp(*secondsOfDay, 0, 86399);
    ImGui::PushID(id);
    bool changed = false;
    int fields[3] = {*secondsOfDay / 3600, *secondsOfDay / 60 % 60, *secondsOfDay % 60};
    const int limits[3] = {24, 60, 60};
    const int steps[3] = {1, std::max(1, o.minuteStep), 1};
    const char* names[3] = {"Hour", "Minute", "Second"};
    const int count = o.seconds ? 3 : 2;
    ImGuiStorage* st = ImGui::GetStateStorage();
    for (int i = 0; i < count; ++i) {
        ImGui::PushID(i);
        const ImVec2 size{u * 2.2f, u * 1.3f};
        const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
        ImGui::InvisibleButton("##field", size);
        const bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive(), focused = ImGui::IsItemFocused();
        const ImGuiID anchorKey = ImGui::GetID("##anchor");
        if (ImGui::IsItemActivated())
            st->SetFloat(anchorKey, io.MousePos.y);
        int next = fields[i];
        if (active) {
            const float moved = st->GetFloat(anchorKey) - io.MousePos.y;
            const int steps_ = static_cast<int>(moved / (u * .55f));
            if (steps_ != 0) {
                next = ((fields[i] + steps_ * steps[i]) % limits[i] + limits[i]) % limits[i];
                st->SetFloat(anchorKey, st->GetFloat(anchorKey) - static_cast<float>(steps_) * u * .55f);
            }
        }
        if (hovered && io.MouseWheel != 0)
            next = ((fields[i] + (io.MouseWheel > 0 ? 1 : -1) * steps[i]) % limits[i] + limits[i]) % limits[i];
        if (focused) {
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
                next = (fields[i] + steps[i]) % limits[i];
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
                next = ((fields[i] - steps[i]) % limits[i] + limits[i]) % limits[i];
        }
        detail::ValueNode node;
        node.name = names[i];
        node.value = fields[i];
        node.minimum = 0;
        node.maximum = limits[i] - 1;
        node.smallChange = steps[i];
        node.largeChange = steps[i] * 5;
        node.text = FrameText("%02d", fields[i]);
        if (const int dir = detail::AnnotateValue(options, node); dir != 0)
            next = ((fields[i] + dir * steps[i]) % limits[i] + limits[i]) % limits[i];
        if (next != fields[i]) {
            fields[i] = next;
            changed = true;
        }
        draw->AddRectFilled(a, b, U32(Mix(c.input, c.text, hovered || active ? .08f : 0.f)), u * .3f);
        draw->AddRect(a, b, U32(active || focused ? c.accent : c.border), u * .3f, 0, active ? 2.f : 1.f);
        const char* text = FrameText("%02d", fields[i]);
        DrawTextCentered(draw, a, b, U32(c.text), text);
        if (hovered) {
            detail::DrawChevron(draw, {(a.x + b.x) * .5f, a.y + u * .18f}, u * .3f, U32(c.muted), ImGuiDir_Up, 1.4f);
            detail::DrawChevron(draw, {(a.x + b.x) * .5f, b.y - u * .18f}, u * .3f, U32(c.muted), ImGuiDir_Down, 1.4f);
        }
        ImGui::PopID();
        if (i + 1 < count) {
            ImGui::SameLine(0, 2.f);
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled(":");
            ImGui::SameLine(0, 2.f);
        }
    }
    ImGui::PopID();
    if (changed)
        *secondsOfDay = fields[0] * 3600 + fields[1] * 60 + (o.seconds ? fields[2] : 0);
    return changed;
}

// ---- DateTimeField ---------------------------------------------------------------------------------------------
bool DateTimeField(const char* id, std::int64_t* utc, TimeZone zone, std::int32_t today, ComponentOptions options) {
    if (!utc)
        return false;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    ImGui::PushID(id);
    const LocalTime t = ToLocal(*utc, zone);
    const char* text = FrameText("%04d-%02d-%02d  %02d:%02d", t.year, t.month, t.day, t.hour, t.minute);
    const ImVec2 size{ImGui::CalcTextSize(text).x + u * 2.2f, u * 1.2f};
    const ImVec2 a = ImGui::GetCursorScreenPos(), b{a.x + size.x, a.y + size.y};
    ImGui::InvisibleButton("##field", size);
    const bool hovered = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked())
        ImGui::OpenPopup("##picker");
    SemanticItem(options, id, accessibility::SemanticRole::ComboBox, accessibility::SemanticAction::Press);
    draw->AddRectFilled(a, b, U32(Mix(c.input, c.text, hovered ? .06f : 0.f)), u * .3f);
    draw->AddRect(a, b, U32(c.border), u * .3f, 0, 1.f);
    draw->AddText({a.x + u * .45f, a.y + (size.y - ImGui::GetTextLineHeight()) * .5f}, U32(c.text), text);
    detail::DrawChevron(draw, {b.x - u * .6f, a.y + size.y * .5f}, u * .4f, U32(c.muted), ImGuiDir_Down, 1.5f);
    bool changed = false;
    if (ImGui::IsPopupOpen("##picker"))
        ImGui::SetNextWindowPos({a.x, b.y + 2.f});
    if (ImGui::BeginPopup("##picker")) {
        std::int32_t day = DaysFromCivil(t.year, t.month, t.day);
        int tod = t.hour * 3600 + t.minute * 60;
        DatePickerOptions dpo;
        dpo.today = today;
        const bool dayChanged = DatePicker("date", &day, dpo, options);
        const bool timeChanged = TimePicker("time", &tod, {}, options);
        if (dayChanged || timeChanged) {
            int y, m, d;
            CivilFromDays(day, y, m, d);
            *utc = ToUtc(y, m, d, tod / 3600, tod / 60 % 60, 0, zone);
            changed = true;
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    return changed;
}

// ---- Lists -----------------------------------------------------------------------------------------------------
void NextRuns(const char* id, std::span<const Occurrence> occurrences, TimeZone zone, ComponentOptions options) {
    const ShowColors c = MakeShowColors(options);
    ImGui::PushID(id);
    if (occurrences.empty()) {
        ImGui::TextColored(c.muted, "%s", Str(options, "show.noUpcoming", "No upcoming runs."));
    }
    for (std::size_t i = 0; i < occurrences.size(); ++i) {
        const Occurrence& o = occurrences[i];
        ImGui::TextColored(i == 0 ? c.accent : c.muted, "%s", i == 0 ? ">" : " ");
        ImGui::SameLine();
        ImGui::TextColored(i == 0 ? c.text : c.muted, "%s", RangeText(o.start, o.end, zone));
    }
    ImGui::PopID();
}

void ConflictPanel(const char* id, std::span<const CalendarEvent> events, std::span<const ConflictPair> conflicts, TimeZone zone, ComponentOptions options) {
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    ImGui::PushID(id);
    if (conflicts.empty()) {
        detail::DrawCheck(draw, {ImGui::GetCursorScreenPos().x + u * .4f, ImGui::GetCursorScreenPos().y + u * .5f}, u * .45f, U32(c.success), 1.8f);
        ImGui::Dummy({u * .9f, u});
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(c.muted, "%s", Str(options, "show.noConflicts", "No overlapping events."));
    }
    for (const ConflictPair& p : conflicts) {
        if (p.first < 0 || p.second < 0 || p.first >= static_cast<int>(events.size()) || p.second >= static_cast<int>(events.size()))
            continue;
        const CalendarEvent& a = events[static_cast<std::size_t>(p.first)];
        const CalendarEvent& b = events[static_cast<std::size_t>(p.second)];
        const ImVec2 at = ImGui::GetCursorScreenPos();
        ImGui::Dummy({u * .9f, u * 2.f});
        GlyphWarning(draw, {at.x + u * .45f, at.y + u * .5f}, u * .7f, U32(c.warning));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("%s  /  %s", Safe(a.title), Safe(b.title));
        const std::int64_t from = std::max(a.start, b.start), to = std::min(a.end, b.end);
        const char* verdict = p.winner < 0 ? Str(options, "show.tie", "Same priority: the host decides.")
                                           : FrameText("%s wins (higher priority)", Safe(events[static_cast<std::size_t>(p.winner)].title));
        ImGui::TextColored(c.muted, "%s   %s", RangeText(from, to, zone), verdict);
        ImGui::EndGroup();
    }
    ImGui::PopID();
}

// ---- Recurrence editor -----------------------------------------------------------------------------------------
bool RecurrenceEditor(const char* id, Recurrence* rule, RecurrenceEditorOptions o, ComponentOptions options) {
    if (!rule)
        return false;
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    const Pill pill{c, options, u};
    bool changed = false;
    ImGui::PushID(id);
    ImGui::BeginGroup();
    const LocalTime first = ToLocal(o.firstStart, o.zone);

    // Frequency.
    const char* freqLabels[] = {"Once", "Daily", "Weekly", "Monthly", "Yearly"};
    int freq = static_cast<int>(rule->frequency);
    if (Segmented("frequency", &freq, freqLabels, options)) {
        rule->frequency = static_cast<Frequency>(freq);
        changed = true;
    }
    if (rule->frequency != Frequency::Once) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(c.muted, "%s", Str(options, "show.every", "Every"));
        ImGui::SameLine();
        int interval = std::max(1, rule->interval);
        ImGui::SetNextItemWidth(u * 4.5f);
        if (ImGui::InputInt("##interval", &interval, 1, 5)) {
            rule->interval = std::clamp(interval, 1, 99);
            changed = true;
        }
        ImGui::SameLine();
        static const char* const units[] = {"", "day(s)", "week(s)", "month(s)", "year(s)"};
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(c.muted, "%s", units[static_cast<int>(rule->frequency)]);
    }
    if (rule->frequency == Frequency::Weekly) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(c.muted, "%s", Str(options, "show.on", "On"));
        ImGui::SameLine();
        const unsigned mask = rule->weekdays ? rule->weekdays : (1u << first.weekday);
        for (int d = 0; d < 7; ++d) {
            const bool on = mask & (1u << d);
            if (pill(WeekdayShort[d], WeekdayShort[d], on ? c.accent : c.muted, on ? c.accent : ImVec4{})) {
                unsigned next = on ? (mask & ~(1u << d)) : (mask | (1u << d));
                if (next == 0)
                    next = 1u << first.weekday;
                rule->weekdays = static_cast<std::uint8_t>(next);
                changed = true;
            }
            ImGui::SameLine(0, u * .25f);
        }
        ImGui::NewLine();
    }
    if (rule->frequency == Frequency::Monthly) {
        int mode = rule->nth != 0 ? 1 : 0;
        const char* modes[] = {"On a day of the month", "On a weekday"};
        if (Segmented("monthly", &mode, modes, options)) {
            if (mode == 1) {
                rule->nth = rule->nth ? rule->nth : 1;
                rule->nthWeekday = first.weekday;
            } else {
                rule->nth = 0;
            }
            changed = true;
        }
        if (mode == 0) {
            int dom = rule->monthDay == 0 ? first.day : rule->monthDay;
            ImGui::SetNextItemWidth(u * 4.5f);
            if (ImGui::InputInt("Day##dom", &dom, 1, 5)) {
                rule->monthDay = std::clamp(dom, -1, 31) == 0 ? 1 : std::clamp(dom, -1, 31);
                changed = true;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("%s", Str(options, "show.lastDayHint", "-1 = last day"));
        } else {
            const char* nths[] = {"1st", "2nd", "3rd", "4th", "Last"};
            int nth = rule->nth == -1 ? 4 : rule->nth - 1;
            ImGui::SetNextItemWidth(u * 5.f);
            if (Combo("##nth", &nth, nths, 5)) {
                rule->nth = nth == 4 ? -1 : nth + 1;
                changed = true;
            }
            ImGui::SameLine();
            const char* days[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
            int wd = rule->nthWeekday;
            ImGui::SetNextItemWidth(u * 7.f);
            if (Combo("##nthDay", &wd, days, 7)) {
                rule->nthWeekday = wd;
                changed = true;
            }
        }
    }
    if (rule->frequency != Frequency::Once) {
        ImGui::Spacing();
        int end = static_cast<int>(rule->end);
        const char* ends[] = {"Never ends", "After a number of runs", "Until a date"};
        if (Segmented("end", &end, ends, options)) {
            rule->end = static_cast<RecurrenceEnd>(end);
            if (rule->end == RecurrenceEnd::Until && rule->untilUtc == 0)
                rule->untilUtc = o.firstStart + 90LL * 86400;
            changed = true;
        }
        if (rule->end == RecurrenceEnd::Count) {
            int n = rule->count;
            ImGui::SetNextItemWidth(u * 5.f);
            if (ImGui::InputInt("Runs##count", &n, 1, 10)) {
                rule->count = std::clamp(n, 1, 9999);
                changed = true;
            }
        } else if (rule->end == RecurrenceEnd::Until) {
            if (DateTimeField("until", &rule->untilUtc, o.zone, o.today, options))
                changed = true;
        }
        // Exceptions.
        ImGui::Spacing();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(c.muted, "%s", Str(options, "show.skip", "Skip"));
        ImGui::SameLine();
        for (int i = 0; i < rule->exceptionCount; ++i) {
            if (pill(FrameText("ex%d", i), FrameText("%s  x", DateText(rule->exceptionDays[static_cast<std::size_t>(i)])), c.warning, c.warning)) {
                for (int k = i; k + 1 < rule->exceptionCount; ++k)
                    rule->exceptionDays[static_cast<std::size_t>(k)] = rule->exceptionDays[static_cast<std::size_t>(k + 1)];
                --rule->exceptionCount;
                changed = true;
                break;
            }
            ImGui::SameLine(0, u * .25f);
        }
        if (rule->exceptionCount < static_cast<int>(rule->exceptionDays.size())) {
            if (pill("addEx", Str(options, "show.addException", "+ date"), c.muted, {}, true))
                ImGui::OpenPopup("##exception");
            if (ImGui::IsPopupOpen("##exception"))
                ImGui::SetNextWindowPos({ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2.f});
            if (ImGui::BeginPopup("##exception")) {
                std::int32_t day = o.today ? o.today : LocalDay(o.now, o.zone);
                DatePickerOptions dpo;
                dpo.today = o.today;
                if (DatePicker("exceptionDate", &day, dpo, options)) {
                    rule->exceptionDays[static_cast<std::size_t>(rule->exceptionCount++)] = day;
                    changed = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }
    // RRULE text and the preview.
    ImGui::Spacing();
    char text[160];
    FormatRRule(*rule, text);
    ImGui::TextColored(c.muted, "%s", text[0] ? text : "(single occurrence)");
    std::array<Occurrence, 8> runs{};
    const std::size_t count = NextOccurrences(*rule, o.firstStart, o.durationSeconds, o.zone, o.now, std::span<Occurrence>(runs.data(), static_cast<std::size_t>(std::clamp(o.previewCount, 1, 8))));
    ImGui::Spacing();
    ImGui::TextColored(c.muted, "%s", Str(options, "show.nextRuns", "Next runs"));
    NextRuns("next", std::span<const Occurrence>(runs.data(), count), o.zone, options);
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

// ---- Cron editor -----------------------------------------------------------------------------------------------
bool CronEditor(const char* id, char* text, std::size_t capacity, CronEditorOptions o, ComponentOptions options) {
    if (!text || capacity < 2)
        return false;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const float u = Unit();
    const Pill pill{c, options, u};
    bool changed = false;
    ImGui::PushID(id);
    ImGui::BeginGroup();
    ImGui::SetNextItemWidth(std::max(u * 10.f, std::min(ImGui::GetContentRegionAvail().x, u * 22.f)));
    if (ImGui::InputText("##expression", text, capacity))
        changed = true;
    CronSchedule schedule;
    char error[96];
    const bool valid = ParseCron(text, schedule, error);
    ImGui::SameLine();
    {
        const ImVec2 a = ImGui::GetCursorScreenPos();
        ImGui::Dummy({u * 1.1f, u});
        if (valid)
            detail::DrawCheck(draw, {a.x + u * .5f, a.y + u * .5f}, u * .5f, U32(c.success), 2.f);
        else
            detail::DrawCross(draw, {a.x + u * .5f, a.y + u * .5f}, u * .45f, U32(c.error), 2.f);
    }
    struct Preset {
        const char* label;
        const char* expression;
    };
    static const Preset presets[] = {{"Every minute", "* * * * *"}, {"Hourly", "0 * * * *"}, {"Daily 18:00", "0 18 * * *"},
                                     {"Weekdays 09:00", "0 9 * * 1-5"}, {"Monthly 1st", "0 0 1 * *"}};
    for (const Preset& p : presets) {
        if (pill(p.label, p.label, c.muted)) {
            std::snprintf(text, capacity, "%s", p.expression);
            changed = true;
        }
        ImGui::SameLine(0, u * .25f);
    }
    ImGui::NewLine();
    ImGui::Spacing();
    if (!valid) {
        ImGui::TextColored(c.error, "%s", error);
    } else {
        char description[200];
        DescribeCron(schedule, description);
        ImGui::TextColored(c.text, "%s", description);
        std::array<std::int64_t, 8> next{};
        const std::size_t n = CronNext(schedule, o.now, o.zone, std::span<std::int64_t>(next.data(), static_cast<std::size_t>(std::clamp(o.previewCount, 1, 8))));
        ImGui::Spacing();
        ImGui::TextColored(c.muted, "%s", Str(options, "show.nextRuns", "Next runs"));
        for (std::size_t i = 0; i < n; ++i) {
            const LocalTime t = ToLocal(next[i], o.zone);
            ImGui::TextColored(i == 0 ? c.text : c.muted, "%s %04d-%02d-%02d  %02d:%02d:%02d", WeekdayShort[t.weekday], t.year, t.month, t.day, t.hour, t.minute, t.second);
        }
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

bool SolarTriggerEditor(const char* id, SolarTrigger* trigger, double latitude, double longitude, std::int32_t today, TimeZone zone, ComponentOptions options) {
    if (!trigger)
        return false;
    const ShowColors c = MakeShowColors(options);
    const float u = Unit();
    bool changed = false;
    ImGui::PushID(id);
    int which = trigger->sunset ? 1 : 0;
    const char* labels[] = {"Sunrise", "Sunset"};
    if (Segmented("event", &which, labels, options)) {
        trigger->sunset = which == 1;
        changed = true;
    }
    ImGui::SameLine();
    int offset = trigger->offsetMinutes;
    ImGui::SetNextItemWidth(u * 5.5f);
    if (ImGui::InputInt("min##offset", &offset, 5, 30)) {
        trigger->offsetMinutes = std::clamp(offset, -600, 600);
        changed = true;
    }
    const std::int64_t t = SolarTriggerTime(*trigger, today, latitude, longitude);
    if (t == 0)
        ImGui::TextColored(c.warning, "%s", Str(options, "show.noSun", "No sunrise or sunset on this day at this latitude."));
    else
        ImGui::TextColored(c.muted, "%s %s", Str(options, "show.todayAt", "Today at"), ClockText(t, zone));
    ImGui::PopID();
    return changed;
}

// ---- Week painter ----------------------------------------------------------------------------------------------
ValueEdit WeekPainter(const char* id, std::bitset<336>* slots, WeekPainterOptions o, ComponentOptions options) {
    ValueEdit edit;
    if (!slots)
        return edit;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    const float gutter = u * 2.2f, header = u * 1.2f;
    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = std::max(u * 12.f, ImGui::GetContentRegionAvail().x);
    if (size.y <= 0)
        size.y = u * (o.compact ? 9.f : 13.f);
    ImGui::PushID(id);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const float cellW = (size.x - gutter) / 7.f, cellH = (size.y - header) / 48.f;
    const ImVec2 g0{p0.x + gutter, p0.y + header};
    ImGui::InvisibleButton("##paint", size);
    const bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImGuiID modeKey = ImGui::GetID("##mode");
    const std::size_t before = slots->count();
    auto cellAt = [&](ImVec2 m) -> int {
        const int day = static_cast<int>(std::floor((m.x - g0.x) / cellW)), slot = static_cast<int>(std::floor((m.y - g0.y) / cellH));
        return day < 0 || day > 6 || slot < 0 || slot > 47 ? -1 : day * 48 + slot;
    };
    const ImGuiID lastXKey = ImGui::GetID("##last.x"), lastYKey = ImGui::GetID("##last.y");
    if (ImGui::IsItemActivated()) {
        edit.started = true;
        const int cell = cellAt(io.MousePos);
        st->SetBool(modeKey, cell >= 0 ? !(*slots)[static_cast<std::size_t>(cell)] : true);
        st->SetFloat(lastXKey, io.MousePos.x);
        st->SetFloat(lastYKey, io.MousePos.y);
    }
    if (active) {
        // Paint every cell between the previous and the current pointer position so fast strokes leave no gaps.
        const ImVec2 previous{st->GetFloat(lastXKey, io.MousePos.x), st->GetFloat(lastYKey, io.MousePos.y)};
        const ImVec2 delta{io.MousePos.x - previous.x, io.MousePos.y - previous.y};
        const int steps = std::max(1, static_cast<int>(std::hypot(delta.x, delta.y) / (std::min(cellW, cellH) * .5f)));
        for (int i = 0; i <= steps; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(steps);
            const int cell = cellAt({previous.x + delta.x * t, previous.y + delta.y * t});
            if (cell >= 0)
                slots->set(static_cast<std::size_t>(cell), st->GetBool(modeKey, true));
        }
        st->SetFloat(lastXKey, io.MousePos.x);
        st->SetFloat(lastYKey, io.MousePos.y);
    }
    edit.original = static_cast<double>(before);
    edit.changed = slots->count() != before;
    edit.finished = ImGui::IsItemDeactivated();
    edit.value = static_cast<double>(slots->count());
    // Grid.
    draw->AddRectFilled(g0, {g0.x + cellW * 7.f, g0.y + cellH * 48.f}, U32(c.input), u * .25f);
    for (int day = 0; day < 7; ++day) {
        const char* name = WeekdayShort[(day + o.firstWeekday) % 7];
        const ImVec2 ts = ImGui::CalcTextSize(name);
        draw->AddText({g0.x + static_cast<float>(day) * cellW + (cellW - ts.x) * .5f, p0.y + (header - ts.y) * .5f}, U32(c.muted), name);
    }
    for (int hour = 0; hour <= 24; hour += 3) {
        const float y = g0.y + cellH * 2.f * static_cast<float>(hour);
        if (hour < 24) {
            const char* label = FrameText("%02d", hour);
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .82f, {p0.x + gutter - ImGui::CalcTextSize(label).x * .82f - 4.f, y - 1.f}, U32(c.muted), label);
        }
        draw->AddLine({g0.x, y}, {g0.x + cellW * 7.f, y}, U32(c.border, hour % 6 == 0 ? .8f : .45f));
    }
    for (int day = 0; day <= 7; ++day)
        draw->AddLine({g0.x + static_cast<float>(day) * cellW, g0.y}, {g0.x + static_cast<float>(day) * cellW, g0.y + cellH * 48.f}, U32(c.border, .45f));
    for (int day = 0; day < 7; ++day) {
        const int srcDay = (day + o.firstWeekday) % 7;
        int run = -1;
        for (int slot = 0; slot <= 48; ++slot) {
            const bool on = slot < 48 && (*slots)[static_cast<std::size_t>(srcDay * 48 + slot)];
            if (on && run < 0)
                run = slot;
            if (!on && run >= 0) {
                draw->AddRectFilled({g0.x + static_cast<float>(day) * cellW + 1.f, g0.y + static_cast<float>(run) * cellH},
                                    {g0.x + static_cast<float>(day + 1) * cellW - 1.f, g0.y + static_cast<float>(slot) * cellH}, U32(c.accent, .85f), 3.f);
                run = -1;
            }
        }
    }
    if (hovered || active) {
        const int cell = cellAt(io.MousePos);
        if (cell >= 0) {
            const int day = cell / 48, slot = cell % 48;
            draw->AddRect({g0.x + static_cast<float>(day) * cellW, g0.y + static_cast<float>(slot) * cellH},
                          {g0.x + static_cast<float>(day + 1) * cellW, g0.y + static_cast<float>(slot + 1) * cellH}, U32(c.text, .7f), 2.f);
            const char* tip = FrameText("%s %02d:%02d", WeekdayShort[day], slot / 2, slot % 2 * 30);
            ImGui::SetTooltip("%s", tip);
        }
    }
    AnnotateText(options, id, FrameText("%d of 336 half-hour slots on", static_cast<int>(slots->count())), accessibility::SemanticRole::Group);
    ImGui::PopID();
    return edit;
}

// ---- Calendar --------------------------------------------------------------------------------------------------
namespace {
struct Segment {
    int event;
    int day;          // Column index.
    float top, bottom; // Seconds from the start of the column's local day.
    int lane, lanes;
};
std::int64_t Snap(std::int64_t seconds, int minutes) {
    const std::int64_t step = std::max(1, minutes) * 60LL;
    return (seconds + (seconds >= 0 ? step / 2 : -step / 2)) / step * step;
}
const char* EventSummary(const CalendarEvent& e, TimeZone zone) {
    return FrameText("%s, %s to %s", Safe(e.title), ClockText(e.start, zone), ClockText(e.end, zone));
}
} // namespace

CalendarRequest Calendar(const char* id, std::span<const CalendarEvent> events, CalendarState& state, CalendarOptions o, ComponentOptions options) {
    CalendarRequest request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    const Pill pill{c, options, u};
    const float round = options.theme ? options.theme->radius.control : u * .3f;
    const std::int32_t today = o.today ? o.today : LocalDay(o.now, o.zone);
    if (state.anchorDay == 0)
        state.anchorDay = today;
    static thread_local std::vector<int> conflictIndex;
    static thread_local std::vector<ConflictPair> pairs;
    pairs.resize(256);
    const std::size_t pairCount = FindConflicts(events, pairs);
    conflictIndex.assign(events.size(), 0);
    for (std::size_t i = 0; i < std::min<std::size_t>(pairCount, pairs.size()); ++i) {
        conflictIndex[static_cast<std::size_t>(pairs[i].first)] = 1;
        conflictIndex[static_cast<std::size_t>(pairs[i].second)] = 1;
    }

    ImGui::PushID(id);
    ImGui::BeginGroup();
    // Toolbar.
    {
        int y, m, d;
        CivilFromDays(state.anchorDay, y, m, d);
        const auto step = [&](int direction) {
            switch (state.view) {
            case CalendarView::Month: state.anchorDay = AddMonths(state.anchorDay, direction); break;
            case CalendarView::Week: state.anchorDay += 7 * direction; break;
            case CalendarView::Day: state.anchorDay += direction; break;
            default: state.anchorDay += 14 * direction; break;
            }
        };
        if (pill("prev", "<", c.text))
            step(-1);
        ImGui::SameLine(0, u * .25f);
        if (pill("next", ">", c.text))
            step(1);
        ImGui::SameLine(0, u * .5f);
        if (pill("today", Str(options, "show.today", "Today"), c.accent, c.accent))
            state.anchorDay = today;
        ImGui::SameLine(0, u * .8f);
        ImGui::AlignTextToFramePadding();
        const std::int32_t ws = WeekStart(state.anchorDay, o.firstWeekday);
        int y2, m2, d2;
        CivilFromDays(ws + 6, y2, m2, d2);
        int y1, m1, d1;
        CivilFromDays(ws, y1, m1, d1);
        const char* title = state.view == CalendarView::Month  ? FrameText("%s %d", MonthNamesLong[m - 1], y)
                            : state.view == CalendarView::Week ? FrameText("%s %d - %s %d, %d", MonthNamesShort[m1 - 1], d1, MonthNamesShort[m2 - 1], d2, y2)
                            : state.view == CalendarView::Day  ? FrameText("%s, %s %d, %d", WeekdayShort[WeekdayFromDays(state.anchorDay)], MonthNamesShort[m - 1], d, y)
                                                               : FrameText("From %s %d, %d", MonthNamesShort[m - 1], d, y);
        ImGui::TextUnformatted(title);
        // The view switch shares the row when there is room, otherwise it wraps below.
        const float switchWidth = u * 16.f;
        const float rightLimit = ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x;
        const float used = ImGui::GetItemRectMax().x;
        if (rightLimit - used >= switchWidth + u)
            ImGui::SameLine(rightLimit - switchWidth - ImGui::GetWindowPos().x);
        int view = static_cast<int>(state.view);
        const char* labels[] = {"Month", "Week", "Day", "Agenda"};
        if (Segmented("view", &view, labels, options))
            state.view = static_cast<CalendarView>(view);
    }
    ImGui::Spacing();

    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = ImGui::GetContentRegionAvail().x;
    if (size.y <= 0)
        size.y = u * 22.f;

    // Events overlapping [first, last] local days, as per-day segments with lanes.
    const bool timeGrid = state.view == CalendarView::Week || state.view == CalendarView::Day;
    if (timeGrid) {
        const int days = state.view == CalendarView::Week ? 7 : 1;
        const std::int32_t firstDay = state.view == CalendarView::Week ? WeekStart(state.anchorDay, o.firstWeekday) : state.anchorDay;
        const float gutter = u * 2.6f;
        const float dayW = (size.x - gutter - ImGui::GetStyle().ScrollbarSize) / static_cast<float>(days);
        const float hourH = o.hourHeight > 0 ? o.hourHeight : u * 2.2f;
        const float total = hourH * 24.f;
        // Day headers.
        {
            const ImVec2 a = ImGui::GetCursorScreenPos();
            ImGui::Dummy({size.x, u * 1.4f});
            for (int i = 0; i < days; ++i) {
                const std::int32_t day = firstDay + i;
                int y, m, d;
                CivilFromDays(day, y, m, d);
                const char* label = FrameText("%s %d", WeekdayShort[WeekdayFromDays(day)], d);
                const ImVec2 ts = ImGui::CalcTextSize(label);
                const float cx = a.x + gutter + (static_cast<float>(i) + .5f) * dayW;
                if (day == today) {
                    draw->AddRectFilled({cx - ts.x * .5f - u * .4f, a.y + u * .15f}, {cx + ts.x * .5f + u * .4f, a.y + u * 1.25f}, U32(c.accent), u * .5f);
                    draw->AddText({cx - ts.x * .5f, a.y + (u * 1.4f - ts.y) * .5f}, U32(c.onAccent), label);
                } else {
                    draw->AddText({cx - ts.x * .5f, a.y + (u * 1.4f - ts.y) * .5f}, U32(c.muted), label);
                }
            }
        }
        // All-day events as a strip.
        {
            const ImVec2 a = ImGui::GetCursorScreenPos();
            int rows = 0;
            for (std::size_t e = 0; e < events.size(); ++e) {
                const CalendarEvent& ev = events[e];
                if (!ev.allDay)
                    continue;
                const std::int32_t sd = LocalDay(ev.start, o.zone), ed = LocalDay(std::max(ev.start, ev.end - 1), o.zone);
                const std::int32_t from = std::max(sd, firstDay), to = std::min(ed, firstDay + days - 1);
                if (from > to)
                    continue;
                const ImVec2 r0{a.x + gutter + static_cast<float>(from - firstDay) * dayW + 1.f, a.y + static_cast<float>(rows) * u * 1.1f};
                const ImVec2 r1{a.x + gutter + static_cast<float>(to - firstDay + 1) * dayW - 1.f, r0.y + u};
                ImGui::PushID(static_cast<int>(ev.id) + 900000);
                ImGui::SetCursorScreenPos(r0);
                ImGui::InvisibleButton("##allday", {r1.x - r0.x, r1.y - r0.y});
                if (ImGui::IsItemClicked())
                    request.select = ev.id;
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    request.activate = ev.id;
                const ImVec4 tone = c.layer(static_cast<std::size_t>(ev.color));
                draw->AddRectFilled(r0, r1, U32(Mix(c.surface, tone, ev.id == state.selected ? .55f : .35f)), round);
                draw->AddRectFilled(r0, {r0.x + 3.f, r1.y}, U32(tone), 2.f);
                DrawTextFit(draw, {r0.x + u * .4f, r0.y + (u - ImGui::GetTextLineHeight()) * .5f}, r1.x - r0.x - u * .6f, U32(c.text), Safe(ev.title));
                SemanticItem(options, Safe(ev.title), accessibility::SemanticRole::Button, accessibility::SemanticAction::Select, ev.id == state.selected);
                ImGui::PopID();
                rows = std::min(rows + 1, 3);
            }
            ImGui::SetCursorScreenPos({a.x, a.y + static_cast<float>(std::max(rows, 0)) * u * 1.1f});
            ImGui::Dummy({size.x, rows ? u * .2f : 0.f});
        }
        ImGui::PushStyleColor(ImGuiCol_ChildBg, {0, 0, 0, 0});
        const bool open = ImGui::BeginChild("##grid", {size.x, std::max(u * 8.f, size.y - u * 4.f)}, ImGuiChildFlags_None);
        if (open) {
            auto* draw = ImGui::GetWindowDrawList(); // The child's list, so drawing is clipped to its viewport.
            if (!state.scrolled) {
                ImGui::SetScrollY(hourH * 7.5f);
                state.scrolled = true;
            }
            const ImVec2 origin = ImGui::GetCursorScreenPos();
            // Hours and lines.
            for (int h = 0; h < 24; ++h) {
                const float y = origin.y + static_cast<float>(h) * hourH;
                draw->AddLine({origin.x + gutter, y}, {origin.x + gutter + dayW * static_cast<float>(days), y}, U32(c.border, .6f));
                if (h > 0) {
                    const char* label = FrameText("%02d:00", h);
                    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .82f, {origin.x + gutter - ImGui::CalcTextSize(label).x * .82f - 6.f, y - ImGui::GetTextLineHeight() * .41f}, U32(c.muted), label);
                }
            }
            for (int i = 0; i <= days; ++i)
                draw->AddLine({origin.x + gutter + static_cast<float>(i) * dayW, origin.y}, {origin.x + gutter + static_cast<float>(i) * dayW, origin.y + total}, U32(c.border, .6f));
            // Day backgrounds: drag to create.
            ImGuiStorage* st = ImGui::GetStateStorage();
            for (int i = 0; i < days; ++i) {
                const std::int32_t day = firstDay + i;
                const std::int64_t dayStart = LocalDayStart(day, o.zone);
                ImGui::PushID(i + 4000);
                ImGui::SetCursorScreenPos({origin.x + gutter + static_cast<float>(i) * dayW, origin.y});
                ImGui::SetNextItemAllowOverlap();
                ImGui::InvisibleButton("##day", {dayW, total});
                const ImGuiID anchorKey = ImGui::GetID("##anchor");
                const auto secondsAt = [&](float y) { return Snap(static_cast<std::int64_t>((y - origin.y) / hourH * 3600.f), o.snapMinutes); };
                if (ImGui::IsItemActivated())
                    st->SetInt(anchorKey, static_cast<int>(secondsAt(io.MousePos.y)));
                if (ImGui::IsItemActive()) {
                    const std::int64_t a = st->GetInt(anchorKey), b = secondsAt(io.MousePos.y);
                    const std::int64_t lo = std::min(a, b), hi = std::max(a, b);
                    if (hi - lo >= o.snapMinutes * 60) {
                        const float y0 = origin.y + static_cast<float>(lo) / 3600.f * hourH, y1 = origin.y + static_cast<float>(hi) / 3600.f * hourH;
                        const float x0 = origin.x + gutter + static_cast<float>(i) * dayW + 2.f;
                        draw->AddRectFilled({x0, y0}, {x0 + dayW - 4.f, y1}, U32(c.accent, .25f), round);
                        draw->AddRect({x0, y0}, {x0 + dayW - 4.f, y1}, U32(c.accent), round, 0, 1.5f);
                        draw->AddText({x0 + 4.f, y0 + 2.f}, U32(c.text), FrameText("%02d:%02d - %02d:%02d", static_cast<int>(lo / 3600), static_cast<int>(lo / 60 % 60), static_cast<int>(hi / 3600), static_cast<int>(hi / 60 % 60)));
                    }
                }
                if (ImGui::IsItemDeactivated()) {
                    const std::int64_t a = st->GetInt(anchorKey), b = secondsAt(io.MousePos.y);
                    if (std::abs(b - a) >= o.snapMinutes * 60) {
                        request.create = true;
                        request.start = dayStart + std::min(a, b);
                        request.end = dayStart + std::max(a, b);
                    }
                }
                ImGui::PopID();
            }
            // Segments and lanes.
            static thread_local std::vector<Segment> segments;
            segments.clear();
            for (std::size_t e = 0; e < events.size(); ++e) {
                const CalendarEvent& ev = events[e];
                if (ev.allDay)
                    continue;
                const std::int32_t sd = LocalDay(ev.start, o.zone), ed = LocalDay(std::max(ev.start, ev.end - 1), o.zone);
                for (std::int32_t day = std::max(sd, firstDay); day <= std::min(ed, firstDay + days - 1); ++day) {
                    const std::int64_t dayStart = LocalDayStart(day, o.zone), dayEnd = LocalDayStart(day + 1, o.zone);
                    const float top = static_cast<float>(std::max(ev.start, dayStart) - dayStart);
                    const float bottom = static_cast<float>(std::min(ev.end, dayEnd) - dayStart);
                    segments.push_back({static_cast<int>(e), day - firstDay, top, bottom, 0, 1});
                }
            }
            std::sort(segments.begin(), segments.end(), [](const Segment& a, const Segment& b) { return a.day != b.day ? a.day < b.day : a.top < b.top; });
            for (std::size_t i = 0; i < segments.size();) {
                std::size_t j = i;
                float clusterEnd = segments[i].bottom;
                std::vector<float> laneEnd;
                while (j < segments.size() && segments[j].day == segments[i].day && segments[j].top < clusterEnd) {
                    std::size_t lane = 0;
                    while (lane < laneEnd.size() && laneEnd[lane] > segments[j].top)
                        ++lane;
                    if (lane == laneEnd.size())
                        laneEnd.push_back(0.f);
                    laneEnd[lane] = segments[j].bottom;
                    segments[j].lane = static_cast<int>(lane);
                    clusterEnd = std::max(clusterEnd, segments[j].bottom);
                    ++j;
                }
                for (std::size_t k = i; k < j; ++k)
                    segments[k].lanes = static_cast<int>(laneEnd.size());
                i = j;
            }
            for (const Segment& seg : segments) {
                const CalendarEvent& ev = events[static_cast<std::size_t>(seg.event)];
                const float laneW = dayW / static_cast<float>(seg.lanes);
                const ImVec2 a{origin.x + gutter + static_cast<float>(seg.day) * dayW + static_cast<float>(seg.lane) * laneW + 2.f, origin.y + seg.top / 3600.f * hourH + 1.f};
                const ImVec2 b{a.x + laneW - 4.f, std::max(a.y + u * .9f, origin.y + seg.bottom / 3600.f * hourH - 1.f)};
                ImGui::PushID(static_cast<int>(ev.id) + seg.day * 100000);
                ImGui::SetCursorScreenPos(a);
                ImGui::SetNextItemAllowOverlap();
                ImGui::InvisibleButton("##event", {b.x - a.x, b.y - a.y});
                const bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
                const ImGuiID anchorKey = ImGui::GetID("##drag");
                if (ImGui::IsItemClicked() && !ev.locked) {
                    request.select = ev.id;
                } else if (ImGui::IsItemClicked()) {
                    request.select = ev.id;
                }
                if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    request.activate = ev.id;
                if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                    request.context = ev.id;
                bool moved = false;
                std::int64_t newStart = ev.start;
                if (active && !ev.locked) {
                    const ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 4.f);
                    if (std::abs(drag.x) > 0 || std::abs(drag.y) > 0) {
                        const std::int64_t dt = Snap(static_cast<std::int64_t>(drag.y / hourH * 3600.f), o.snapMinutes);
                        const int dd = static_cast<int>(std::lround(drag.x / dayW));
                        newStart = ev.start + dt + static_cast<std::int64_t>(dd) * 86400;
                        moved = newStart != ev.start;
                        st->SetInt(anchorKey, moved ? 1 : 0);
                        if (moved) {
                            const std::int64_t shift = newStart - ev.start;
                            const float gx = a.x + static_cast<float>(dd) * dayW, gy = a.y + static_cast<float>(shift - static_cast<std::int64_t>(dd) * 86400) / 3600.f * hourH;
                            draw->AddRect({gx, gy}, {gx + (b.x - a.x), gy + (b.y - a.y)}, U32(c.accent), round, 0, 2.f);
                            draw->AddText({gx + 4.f, gy + 2.f}, U32(c.accent), ClockText(newStart, o.zone));
                        }
                    }
                }
                if (ImGui::IsItemDeactivated() && !ev.locked) {
                    const ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.f);
                    const std::int64_t dt = Snap(static_cast<std::int64_t>(drag.y / hourH * 3600.f), o.snapMinutes);
                    const int dd = static_cast<int>(std::lround(drag.x / dayW));
                    const std::int64_t target = ev.start + dt + static_cast<std::int64_t>(dd) * 86400;
                    if ((std::abs(drag.x) > 4.f || std::abs(drag.y) > 4.f) && target != ev.start) {
                        request.move = ev.id;
                        request.start = target;
                    }
                }
                const bool selected = ev.id == state.selected;
                const ImVec4 tone = c.layer(static_cast<std::size_t>(ev.color));
                ImVec4 fill = Mix(c.surface, tone, selected ? .5f : hovered ? .42f : .32f);
                if (active && moved)
                    fill = Mix(c.surface, tone, .15f);
                draw->AddRectFilled(a, b, U32(fill), round);
                draw->AddRectFilled(a, {a.x + 3.f, b.y}, U32(tone), 2.f);
                const bool conflict = conflictIndex[static_cast<std::size_t>(seg.event)] != 0;
                if (conflict)
                    detail::DrawDashedRect(draw, a, b, round, U32(c.warning), 1.5f, 4.f);
                else if (selected)
                    draw->AddRect(a, b, U32(c.accent), round, 0, 2.f);
                draw->PushClipRect(a, b, true);
                draw->AddText({a.x + u * .4f, a.y + 2.f}, U32(c.text), Safe(ev.title));
                if (b.y - a.y > u * 1.8f)
                    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .85f, {a.x + u * .4f, a.y + 2.f + ImGui::GetTextLineHeight()}, U32(c.muted), FrameText("%s - %s", ClockText(ev.start, o.zone), ClockText(ev.end, o.zone)));
                if (conflict)
                    GlyphWarning(draw, {b.x - u * .5f, a.y + u * .5f}, u * .6f, U32(c.warning));
                if (ev.locked)
                    GlyphLock(draw, {b.x - u * (conflict ? 1.2f : .5f), a.y + u * .5f}, u * .6f, U32(c.muted), true);
                draw->PopClipRect();
                SemanticItem(options, EventSummary(ev, o.zone), accessibility::SemanticRole::Button, accessibility::SemanticAction::Select, selected);
                // Resize handle on the bottom edge.
                if (!ev.locked && seg.day >= 0) {
                    ImGui::SetCursorScreenPos({a.x, b.y - 6.f});
                    ImGui::InvisibleButton("##resize", {b.x - a.x, 6.f});
                    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                    if (ImGui::IsItemActive()) {
                        const ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.f);
                        const std::int64_t newEnd = std::max(ev.start + o.snapMinutes * 60LL, ev.end + Snap(static_cast<std::int64_t>(drag.y / hourH * 3600.f), o.snapMinutes));
                        const float gy = origin.y + static_cast<float>(newEnd - LocalDayStart(LocalDay(ev.start, o.zone), o.zone)) / 3600.f * hourH;
                        draw->AddLine({a.x, gy}, {b.x, gy}, U32(c.accent), 2.f);
                        draw->AddText({a.x + 4.f, gy - ImGui::GetTextLineHeight() - 1.f}, U32(c.accent), ClockText(newEnd, o.zone));
                    }
                    if (ImGui::IsItemDeactivated()) {
                        const ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.f);
                        const std::int64_t newEnd = std::max(ev.start + o.snapMinutes * 60LL, ev.end + Snap(static_cast<std::int64_t>(drag.y / hourH * 3600.f), o.snapMinutes));
                        if (newEnd != ev.end && std::abs(drag.y) > 2.f) {
                            request.resize = ev.id;
                            request.end = newEnd;
                        }
                    }
                }
                ImGui::PopID();
            }
            // Now line.
            if (o.now != 0) {
                const std::int32_t nd = LocalDay(o.now, o.zone);
                if (nd >= firstDay && nd < firstDay + days) {
                    const LocalTime t = ToLocal(o.now, o.zone);
                    const float y = origin.y + static_cast<float>(t.hour * 3600 + t.minute * 60 + t.second) / 3600.f * hourH;
                    const float x0 = origin.x + gutter + static_cast<float>(nd - firstDay) * dayW;
                    draw->AddLine({x0, y}, {x0 + dayW, y}, U32(c.error), 2.f);
                    draw->AddCircleFilled({x0, y}, 4.f, U32(c.error), 12);
                }
            }
            ImGui::SetCursorScreenPos(origin);
            ImGui::Dummy({size.x - ImGui::GetStyle().ScrollbarSize, total});
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    } else if (state.view == CalendarView::Month) {
        int y, m, d;
        CivilFromDays(state.anchorDay, y, m, d);
        const std::int32_t monthStart = DaysFromCivil(y, m, 1);
        const std::int32_t gridStart = WeekStart(monthStart, o.firstWeekday);
        const float cellW = size.x / 7.f;
        const float headH = u * 1.2f;
        const float cellH = std::max(u * 4.f, (size.y - headH) / 6.f);
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        for (int i = 0; i < 7; ++i) {
            const char* name = WeekdayShort[(i + o.firstWeekday) % 7];
            const ImVec2 ts = ImGui::CalcTextSize(name);
            draw->AddText({origin.x + static_cast<float>(i) * cellW + (cellW - ts.x) * .5f, origin.y + (headH - ts.y) * .5f}, U32(c.muted), name);
        }
        const int maxLines = std::max(1, static_cast<int>((cellH - u * 1.3f) / (u * .95f)));
        for (int i = 0; i < 42; ++i) {
            const std::int32_t day = gridStart + i;
            int cy, cm, cd;
            CivilFromDays(day, cy, cm, cd);
            const ImVec2 a{origin.x + static_cast<float>(i % 7) * cellW, origin.y + headH + static_cast<float>(i / 7) * cellH};
            const ImVec2 b{a.x + cellW, a.y + cellH};
            ImGui::PushID(i + 6000);
            ImGui::SetCursorScreenPos(a);
            ImGui::SetNextItemAllowOverlap();
            ImGui::InvisibleButton("##cell", {cellW, cellH});
            const bool hovered = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked()) {
                request.dayClicked = true;
                request.clickedDay = day;
            }
            SemanticItem(options, DateText(day), accessibility::SemanticRole::Cell, accessibility::SemanticAction::Select);
            draw->AddRectFilled(a, b, U32(cm == m ? Mix(c.surface, c.text, hovered ? .06f : .02f) : Mix(c.surface, c.input, .5f)));
            draw->AddRect(a, b, U32(c.border, .6f));
            if (day == today) {
                draw->AddRectFilled({a.x + 4.f, a.y + 3.f}, {a.x + u * 1.3f, a.y + u * 1.1f}, U32(c.accent), u * .5f);
                DrawTextCentered(draw, {a.x + 4.f, a.y + 3.f}, {a.x + u * 1.3f, a.y + u * 1.1f}, U32(c.onAccent), FrameText("%d", cd));
            } else {
                draw->AddText({a.x + 8.f, a.y + 4.f}, U32(cm == m ? c.text : c.muted), FrameText("%d", cd));
            }
            int line = 0, hidden = 0;
            const std::int64_t d0 = LocalDayStart(day, o.zone), d1 = LocalDayStart(day + 1, o.zone);
            for (std::size_t e = 0; e < events.size(); ++e) {
                const CalendarEvent& ev = events[e];
                if (ev.end <= d0 || ev.start >= d1)
                    continue;
                if (line >= maxLines) {
                    ++hidden;
                    continue;
                }
                const ImVec2 ca{a.x + 4.f, a.y + u * 1.3f + static_cast<float>(line) * u * .95f};
                const ImVec2 cb{b.x - 4.f, ca.y + u * .85f};
                ImGui::PushID(static_cast<int>(ev.id));
                ImGui::SetCursorScreenPos(ca);
                ImGui::InvisibleButton("##chip", {cb.x - ca.x, cb.y - ca.y});
                if (ImGui::IsItemClicked())
                    request.select = ev.id;
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    request.activate = ev.id;
                const ImVec4 tone = c.layer(static_cast<std::size_t>(ev.color));
                draw->AddRectFilled(ca, cb, U32(Mix(c.surface, tone, ev.id == state.selected ? .55f : .3f)), 3.f);
                draw->AddRectFilled(ca, {ca.x + 3.f, cb.y}, U32(tone), 2.f);
                draw->PushClipRect(ca, cb, true);
                draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .85f, {ca.x + 6.f, ca.y + 1.f}, U32(c.text), Safe(ev.title));
                if (conflictIndex[e])
                    GlyphWarning(draw, {cb.x - 8.f, (ca.y + cb.y) * .5f}, u * .5f, U32(c.warning));
                draw->PopClipRect();
                SemanticItem(options, EventSummary(ev, o.zone), accessibility::SemanticRole::Button, accessibility::SemanticAction::Select, ev.id == state.selected);
                ImGui::PopID();
                ++line;
            }
            if (hidden > 0)
                draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .85f, {a.x + 8.f, b.y - u * .95f}, U32(c.muted), FrameText("+%d more", hidden));
            ImGui::PopID();
        }
        ImGui::SetCursorScreenPos(origin);
        ImGui::Dummy({size.x, headH + cellH * 6.f});
    } else {
        // Agenda: a list of the next 30 days that have events, in start order.
        static thread_local std::vector<int> order;
        order.clear();
        const std::int64_t from = LocalDayStart(state.anchorDay, o.zone), to = LocalDayStart(state.anchorDay + 30, o.zone);
        for (std::size_t e = 0; e < events.size(); ++e)
            if (events[e].end > from && events[e].start < to)
                order.push_back(static_cast<int>(e));
        std::sort(order.begin(), order.end(), [&](int a, int b) { return events[static_cast<std::size_t>(a)].start < events[static_cast<std::size_t>(b)].start; });
        ImGui::PushStyleColor(ImGuiCol_ChildBg, c.surface);
        if (ImGui::BeginChild("##agenda", {size.x, size.y}, ImGuiChildFlags_Borders)) {
            auto* draw = ImGui::GetWindowDrawList();
            std::int32_t lastDay = INT32_MIN;
            if (order.empty())
                ImGui::TextColored(c.muted, "%s", Str(options, "show.nothingScheduled", "Nothing scheduled in the next 30 days."));
            for (int idx : order) {
                const CalendarEvent& ev = events[static_cast<std::size_t>(idx)];
                const std::int32_t day = LocalDay(ev.start, o.zone);
                if (day != lastDay) {
                    lastDay = day;
                    ImGui::Spacing();
                    ImGui::TextColored(day == today ? c.accent : c.muted, "%s%s", DayTitle(day), day == today ? "  (today)" : "");
                }
                ImGui::PushID(static_cast<int>(ev.id));
                const ImVec2 a = ImGui::GetCursorScreenPos();
                const float rowH = u * 1.4f;
                const float w = ImGui::GetContentRegionAvail().x;
                ImGui::Selectable("##row", ev.id == state.selected, ImGuiSelectableFlags_AllowDoubleClick, {w, rowH});
                if (ImGui::IsItemClicked())
                    request.select = ev.id;
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    request.activate = ev.id;
                const ImVec4 tone = c.layer(static_cast<std::size_t>(ev.color));
                draw->AddRectFilled(a, {a.x + 4.f, a.y + rowH}, U32(tone), 2.f);
                draw->AddText({a.x + u * .6f, a.y + (rowH - ImGui::GetTextLineHeight()) * .5f}, U32(c.muted), ev.allDay ? "All day" : FrameText("%s - %s", ClockText(ev.start, o.zone), ClockText(ev.end, o.zone)));
                draw->AddText({a.x + u * 6.f, a.y + (rowH - ImGui::GetTextLineHeight()) * .5f}, U32(c.text), Safe(ev.title));
                const char* resource = ev.resource >= 0 && ev.resource < static_cast<int>(o.resourceNames.size()) ? o.resourceNames[static_cast<std::size_t>(ev.resource)] : "";
                if (resource && resource[0])
                    draw->AddText({a.x + w - ImGui::CalcTextSize(resource).x - u * (conflictIndex[static_cast<std::size_t>(idx)] ? 1.3f : .5f), a.y + (rowH - ImGui::GetTextLineHeight()) * .5f}, U32(c.muted), resource);
                if (conflictIndex[static_cast<std::size_t>(idx)])
                    GlyphWarning(draw, {a.x + w - u * .5f, a.y + rowH * .5f}, u * .6f, U32(c.warning));
                SemanticItem(options, EventSummary(ev, o.zone), accessibility::SemanticRole::Row, accessibility::SemanticAction::Select, ev.id == state.selected);
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    ImGui::EndGroup();
    ImGui::PopID();
    (void)Pi;
    return request;
}

// ---- Resource timeline -----------------------------------------------------------------------------------------
ResourceTimelineRequest ResourceTimeline(const char* id, std::span<const char* const> resources, std::span<const CalendarEvent> events, std::int32_t day,
                                         StableId selected, ResourceTimelineOptions o, ComponentOptions options) {
    ResourceTimelineRequest request;
    if (resources.empty())
        return request;
    const ShowColors c = MakeShowColors(options);
    auto* draw = ImGui::GetWindowDrawList();
    const auto& io = ImGui::GetIO();
    const float u = Unit();
    const float round = options.theme ? options.theme->radius.control : u * .3f;
    const float labelW = u * 7.f, headH = u * 1.3f, rowH = u * 2.3f;
    ImVec2 size = o.size;
    if (size.x <= 0)
        size.x = ImGui::GetContentRegionAvail().x;
    const float trackW = std::max(u * 12.f, size.x - labelW);
    const float total = headH + rowH * static_cast<float>(resources.size());
    ImGui::PushID(id);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::Dummy({size.x, total});
    const std::int64_t dayStart = LocalDayStart(day, o.zone), dayEnd = LocalDayStart(day + 1, o.zone);
    const float secondsToX = trackW / static_cast<float>(dayEnd - dayStart);
    // Time axis.
    for (int h = 0; h <= 24; h += 2) {
        const float x = origin.x + labelW + static_cast<float>(h) * 3600.f * secondsToX;
        draw->AddLine({x, origin.y + headH}, {x, origin.y + total}, U32(c.border, h % 6 == 0 ? .8f : .45f));
        if (h < 24)
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * .85f, {x + 3.f, origin.y + (headH - ImGui::GetTextLineHeight() * .85f) * .5f}, U32(c.muted), FrameText("%02d", h));
    }
    for (std::size_t r = 0; r < resources.size(); ++r) {
        const float y = origin.y + headH + static_cast<float>(r) * rowH;
        draw->AddRectFilled({origin.x, y}, {origin.x + size.x, y + rowH}, U32(r % 2 ? Mix(c.surface, c.text, .02f) : c.surface));
        draw->AddText({origin.x + u * .5f, y + (rowH - ImGui::GetTextLineHeight()) * .5f}, U32(c.text), Safe(resources[r]));
    }
    static thread_local std::vector<ConflictPair> pairs;
    pairs.resize(256);
    const std::size_t pairCount = FindConflicts(events, pairs);
    ImGuiStorage* st = ImGui::GetStateStorage();
    for (std::size_t e = 0; e < events.size(); ++e) {
        const CalendarEvent& ev = events[e];
        if (ev.resource < 0 || ev.resource >= static_cast<int>(resources.size()) || ev.end <= dayStart || ev.start >= dayEnd)
            continue;
        const float x0 = origin.x + labelW + static_cast<float>(std::max(ev.start, dayStart) - dayStart) * secondsToX;
        const float x1 = origin.x + labelW + static_cast<float>(std::min(ev.end, dayEnd) - dayStart) * secondsToX;
        const float y0 = origin.y + headH + static_cast<float>(ev.resource) * rowH + 4.f;
        const ImVec2 a{x0, y0}, b{std::max(x1, x0 + u), y0 + rowH - 8.f};
        ImGui::PushID(static_cast<int>(ev.id));
        ImGui::SetCursorScreenPos(a);
        ImGui::InvisibleButton("##bar", {b.x - a.x, b.y - a.y});
        const bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
        if (ImGui::IsItemClicked())
            request.select = ev.id;
        const ImGuiID movedKey = ImGui::GetID("##moved");
        ImVec2 ghostA = a, ghostB = b;
        if (active && !ev.locked) {
            const ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 4.f);
            const std::int64_t dt = static_cast<std::int64_t>(drag.x / secondsToX);
            const std::int64_t snapped = (dt + (dt >= 0 ? o.snapMinutes * 30LL : -o.snapMinutes * 30LL)) / (o.snapMinutes * 60LL) * (o.snapMinutes * 60LL);
            const int row = std::clamp(ev.resource + static_cast<int>(std::lround(drag.y / rowH)), 0, static_cast<int>(resources.size()) - 1);
            ghostA = {a.x + static_cast<float>(snapped) * secondsToX, origin.y + headH + static_cast<float>(row) * rowH + 4.f};
            ghostB = {ghostA.x + (b.x - a.x), ghostA.y + (b.y - a.y)};
            draw->AddRect(ghostA, ghostB, U32(c.accent), round, 0, 2.f);
            draw->AddText({ghostA.x + 4.f, ghostA.y + 2.f}, U32(c.accent), ClockText(ev.start + snapped, o.zone));
            st->SetInt(movedKey, 1);
        }
        if (ImGui::IsItemDeactivated() && !ev.locked) {
            const ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.f);
            const std::int64_t dt = static_cast<std::int64_t>(drag.x / secondsToX);
            const std::int64_t snapped = (dt + (dt >= 0 ? o.snapMinutes * 30LL : -o.snapMinutes * 30LL)) / (o.snapMinutes * 60LL) * (o.snapMinutes * 60LL);
            const int row = std::clamp(ev.resource + static_cast<int>(std::lround(drag.y / rowH)), 0, static_cast<int>(resources.size()) - 1);
            if ((std::abs(drag.x) > 4.f || std::abs(drag.y) > 4.f) && (snapped != 0 || row != ev.resource)) {
                request.move = ev.id;
                request.start = ev.start + snapped;
                request.resource = row;
            }
        }
        const ImVec4 tone = c.layer(static_cast<std::size_t>(ev.color));
        draw->AddRectFilled(a, b, U32(Mix(c.surface, tone, ev.id == selected ? .6f : hovered ? .45f : .35f)), round);
        draw->AddRectFilled(a, {a.x + 3.f, b.y}, U32(tone), 2.f);
        bool conflict = false;
        for (std::size_t i = 0; i < std::min<std::size_t>(pairCount, pairs.size()); ++i)
            conflict = conflict || pairs[i].first == static_cast<int>(e) || pairs[i].second == static_cast<int>(e);
        if (conflict)
            detail::DrawDashedRect(draw, a, b, round, U32(c.warning), 1.5f, 4.f);
        else if (ev.id == selected)
            draw->AddRect(a, b, U32(c.accent), round, 0, 2.f);
        draw->PushClipRect(a, b, true);
        draw->AddText({a.x + u * .4f, a.y + (b.y - a.y - ImGui::GetTextLineHeight()) * .5f}, U32(c.text), Safe(ev.title));
        if (conflict)
            GlyphWarning(draw, {b.x - u * .5f, (a.y + b.y) * .5f}, u * .6f, U32(c.warning));
        draw->PopClipRect();
        SemanticItem(options, EventSummary(ev, o.zone), accessibility::SemanticRole::Button, accessibility::SemanticAction::Select, ev.id == selected);
        ImGui::PopID();
    }
    if (o.now >= dayStart && o.now < dayEnd) {
        const float x = origin.x + labelW + static_cast<float>(o.now - dayStart) * secondsToX;
        draw->AddLine({x, origin.y + headH}, {x, origin.y + total}, U32(c.error), 2.f);
    }
    (void)io;
    ImGui::SetCursorScreenPos({origin.x, origin.y + total});
    ImGui::Dummy({0, 0});
    ImGui::PopID();
    return request;
}
} // namespace imkit::show
