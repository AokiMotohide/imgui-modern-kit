#include "show_pages.h"
#include "gallery.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>

namespace imkit::gallery {
using namespace show;

namespace {
constexpr int TokyoOffset = 9 * 3600;
constexpr double TokyoLatitude = 35.68, TokyoLongitude = 139.76;
const char* Resources[] = {"Main LED", "Side wall", "Floor"};
} // namespace

void ShowPages::Schedule(GalleryState& host) {
    ComponentOptions o{&host.theme, &host.animation};
    static const int offset = TokyoOffset;
    const TimeZone zone = FixedZone(&offset);
    // The Gallery is the host here: it reads the clock and hands the library its value.
    const std::int64_t now = static_cast<std::int64_t>(std::time(nullptr));
    const std::int32_t today = LocalDay(now, zone);
    SchedSim& s = schedule;

    if (!s.initialized) {
        s.initialized = true;
        const std::int32_t monday = today - WeekdayFromDays(today);
        struct Seed {
            const char* title;
            int day, startMinute, minutes, resource, priority, color;
            bool allDay;
        };
        const Seed seeds[] = {
            {"Daytime loop", 0, 9 * 60, 8 * 60, 0, 1, 0, false}, {"Daytime loop", 1, 9 * 60, 8 * 60, 0, 1, 0, false},
            {"Daytime loop", 2, 9 * 60, 8 * 60, 0, 1, 0, false}, {"Press event", 2, 13 * 60, 3 * 60, 0, 5, 3, false},
            {"Evening show", 2, 18 * 60, 3 * 60, 0, 2, 2, false}, {"Daytime loop", 3, 9 * 60, 8 * 60, 0, 1, 0, false},
            {"Side wall ads", 1, 10 * 60, 4 * 60, 1, 1, 4, false}, {"Floor ambience", 3, 12 * 60, 6 * 60, 2, 1, 1, false},
            {"Weekend festival", 5, 0, 24 * 60, 0, 3, 5, true},
        };
        std::int64_t id = 1;
        for (const Seed& seed : seeds) {
            CalendarEvent e;
            e.id = static_cast<StableId>(id++);
            e.start = LocalDayStart(monday + seed.day, zone) + seed.startMinute * 60LL;
            e.end = e.start + seed.minutes * 60LL;
            e.color = seed.color;
            e.priority = seed.priority;
            e.resource = seed.resource;
            e.allDay = seed.allDay;
            s.events.push_back(e);
            s.titles.emplace_back(seed.title);
        }
        s.nextId = static_cast<StableId>(id);
        s.calendar.anchorDay = today;
        s.rule.frequency = Frequency::Weekly;
        s.rule.weekdays = static_cast<std::uint8_t>((1u << 0) | (1u << 2) | (1u << 4));
        std::snprintf(s.cron, sizeof s.cron, "0 18 * * 1-5");
        s.dateTime = LocalDayStart(today, zone) + 18 * 3600;
        s.pickedDay = today;
        s.timeOfDay = 18 * 3600;
        for (int day = 0; day < 5; ++day)
            for (int slot = 18; slot < 40; ++slot)
                s.hours.set(static_cast<std::size_t>(day * 48 + slot));
    }
    for (std::size_t i = 0; i < s.events.size() && i < s.titles.size(); ++i)
        s.events[i].title = s.titles[i].c_str();

    if (BeginGrid(host, "show-schedule-grid", 2)) {
        const bool s1 = BeginSection(host, "Calendar", "Drag an event to move it, drag its lower edge to resize, drag on empty space to create. Overlaps get a dashed edge.");
        if (s1) {
            CalendarOptions co;
            co.zone = zone;
            co.now = now;
            co.today = today;
            co.size = {0, ImGui::GetFrameHeight() * 22.f};
            co.resourceNames = Resources;
            const CalendarRequest r = Calendar("calendar", s.events, s.calendar, co, o);
            Record(host, "show-calendar");
            auto find = [&](StableId id) { return std::find_if(s.events.begin(), s.events.end(), [&](const CalendarEvent& e) { return e.id == id; }); };
            if (r.select)
                s.calendar.selected = r.select;
            if (r.move)
                if (auto it = find(r.move); it != s.events.end()) {
                    const std::int64_t duration = it->end - it->start;
                    it->start = r.start;
                    it->end = r.start + duration;
                    ++s.edits;
                }
            if (r.resize)
                if (auto it = find(r.resize); it != s.events.end()) {
                    it->end = r.end;
                    ++s.edits;
                }
            if (r.create) {
                CalendarEvent e;
                e.id = s.nextId++;
                e.start = r.start;
                e.end = r.end;
                e.color = static_cast<int>(e.id % 6);
                e.priority = 1;
                s.events.push_back(e);
                s.titles.emplace_back("New event");
                s.calendar.selected = e.id;
                ++s.edits;
            }
            if (r.dayClicked) {
                s.calendar.anchorDay = r.clickedDay;
                s.calendar.view = CalendarView::Day;
            }
            std::array<ConflictPair, 16> pairs{};
            const std::size_t count = FindConflicts(s.events, pairs);
            ImGui::Spacing();
            ConflictPanel("conflicts", s.events, std::span<const ConflictPair>(pairs.data(), std::min<std::size_t>(count, pairs.size())), zone, o);
            ImGui::TextDisabled("Edits: %d   Events: %d", s.edits, static_cast<int>(s.events.size()));
        }
        EndSection(s1);

        const bool s2 = BeginSection(host, "Recurrence", "Frequency, weekday chips, end condition, skipped dates and the next runs.");
        if (s2) {
            RecurrenceEditorOptions ro;
            ro.zone = zone;
            ro.now = now;
            ro.today = today;
            ro.firstStart = LocalDayStart(today, zone) + 18 * 3600;
            ro.durationSeconds = 3 * 3600;
            RecurrenceEditor("recurrence", &s.rule, ro, o);
            Record(host, "show-recurrence");
        }
        EndSection(s2);

        const bool s3 = BeginSection(host, "Resource timeline", "One row per output for the selected day. Drag a bar in time or to another row.");
        if (s3) {
            ResourceTimelineOptions to;
            to.zone = zone;
            to.now = now;
            const ResourceTimelineRequest r = ResourceTimeline("timeline", Resources, s.events, s.calendar.anchorDay, s.calendar.selected, to, o);
            Record(host, "show-timeline");
            if (r.select)
                s.calendar.selected = r.select;
            if (r.move)
                for (CalendarEvent& e : s.events)
                    if (e.id == r.move) {
                        const std::int64_t duration = e.end - e.start;
                        e.start = r.start;
                        e.end = r.start + duration;
                        e.resource = r.resource;
                        ++s.edits;
                    }
        }
        EndSection(s3);

        const bool s4 = BeginSection(host, "Cron and sun", "A cron expression with its meaning and next runs, and a trigger relative to sunrise or sunset.");
        if (s4) {
            CronEditorOptions co;
            co.zone = zone;
            co.now = now;
            CronEditor("cron", s.cron, sizeof(s.cron), co, o);
            Record(host, "show-cron");
            ImGui::Spacing();
            SolarTriggerEditor("solar", &s.solar, TokyoLatitude, TokyoLongitude, today, zone, o);
            Record(host, "show-solar");
        }
        EndSection(s4);

        const bool s5 = BeginSection(host, "Pickers and weekly hours", "Month grid, time fields, a date-time field and a painted weekly schedule (Mon-Sun, 30 minute slots).");
        if (s5) {
            ImGui::BeginGroup();
            DatePickerOptions dp;
            dp.today = today;
            DatePicker("date", &s.pickedDay, dp, o);
            Record(host, "show-datepicker");
            ImGui::EndGroup();
            ImGui::SameLine(0, ImGui::GetFrameHeight());
            ImGui::BeginGroup();
            TimePicker("time", &s.timeOfDay, {}, o);
            ImGui::Spacing();
            DateTimeField("datetime", &s.dateTime, zone, today, o);
            ImGui::Spacing();
            {
                const LocalTime t = ToLocal(s.dateTime, zone);
                ImGui::TextDisabled("%04d-%02d-%02d %02d:%02d (UTC+9)", t.year, t.month, t.day, t.hour, t.minute);
            }
            ImGui::EndGroup();
            ImGui::Spacing();
            WeekPainterOptions wo;
            wo.size = {0, ImGui::GetFrameHeight() * 11.f};
            WeekPainter("hours", &s.hours, wo, o);
            Record(host, "show-weekpainter");
            ImGui::TextDisabled("On-air slots: %d of 336 (%.1f h per week)", static_cast<int>(s.hours.count()), static_cast<double>(s.hours.count()) / 2.0);
        }
        EndSection(s5);
        EndGrid();
    }
}
} // namespace imkit::gallery
