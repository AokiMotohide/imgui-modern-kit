#include <imkit/imkit.h>
#include <imkit/show.h>
#include <imkit/show/schedule.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace {
int failures = 0;
void Check(bool ok, const char* label) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", label);
        ++failures;
    }
}
} // namespace

using namespace imkit;
using namespace imkit::show;

namespace {
// US Eastern for 2024: UTC-4 from 2024-03-10 07:00 UTC until 2024-11-03 06:00 UTC, otherwise UTC-5.
int Eastern(void*, std::int64_t utc) {
    const std::int64_t start = static_cast<std::int64_t>(DaysFromCivil(2024, 3, 10)) * 86400 + 7 * 3600;
    const std::int64_t end = static_cast<std::int64_t>(DaysFromCivil(2024, 11, 3)) * 86400 + 6 * 3600;
    return utc >= start && utc < end ? -4 * 3600 : -5 * 3600;
}
std::int64_t Utc(int y, int m, int d, int h = 0, int mi = 0, int s = 0) { return ToUtc(y, m, d, h, mi, s, TimeZone{}); }
} // namespace

int main() {
    // Calendar arithmetic.
    Check(DaysFromCivil(1970, 1, 1) == 0 && DaysFromCivil(2000, 3, 1) == 11017, "days since the epoch");
    {
        bool roundTrip = true;
        for (std::int32_t d = -800000; d < 800000; d += 997) {
            int y, m, day;
            CivilFromDays(d, y, m, day);
            roundTrip = roundTrip && DaysFromCivil(y, m, day) == d;
        }
        Check(roundTrip, "civil date conversion round-trips");
    }
    Check(WeekdayFromDays(DaysFromCivil(1970, 1, 1)) == 3 && WeekdayFromDays(DaysFromCivil(2024, 2, 29)) == 3 && WeekdayFromDays(DaysFromCivil(2026, 10, 25)) == 6,
          "weekday numbering starts on Monday");
    Check(DaysInMonth(2024, 2) == 29 && DaysInMonth(2100, 2) == 28 && DaysInMonth(2026, 4) == 30, "month lengths");

    // Time zones.
    const int tokyo = 9 * 3600;
    const TimeZone jst = FixedZone(&tokyo);
    {
        const std::int64_t t = ToUtc(2026, 10, 27, 18, 30, 0, jst);
        const LocalTime l = ToLocal(t, jst);
        Check(l.year == 2026 && l.month == 10 && l.day == 27 && l.hour == 18 && l.minute == 30, "fixed zone round-trips");
        Check(LocalDay(t, jst) == DaysFromCivil(2026, 10, 27) && LocalDayStart(DaysFromCivil(2026, 10, 27), jst) == t - 18 * 3600 - 30 * 60, "local day boundaries");
    }
    const TimeZone eastern{nullptr, &Eastern};
    Check(ToUtc(2024, 3, 10, 2, 30, 0, eastern) == Utc(2024, 3, 10, 7, 30), "a time in the spring-forward gap moves forward");
    Check(ToUtc(2024, 11, 3, 1, 30, 0, eastern) == Utc(2024, 11, 3, 5, 30), "a time in the fall-back overlap uses the first occurrence");
    {
        Recurrence daily;
        daily.frequency = Frequency::Daily;
        std::array<Occurrence, 4> out{};
        const std::int64_t first = ToUtc(2024, 3, 8, 18, 0, 0, eastern);
        const std::size_t n = NextOccurrences(daily, first, 3600, eastern, 0, out);
        const LocalTime a = ToLocal(out[2].start, eastern), b = ToLocal(out[3].start, eastern);
        Check(n == 4 && a.hour == 18 && b.hour == 18 && a.day == 10 && b.day == 11, "a daily 18:00 stays 18:00 across daylight saving");
        Check(out[2].start - out[1].start == 23 * 3600 && out[3].start - out[2].start == 24 * 3600, "the UTC distance shrinks by the lost hour");
    }

    // Recurrence rules.
    {
        const std::int64_t first = Utc(2026, 10, 27, 18);
        std::array<Occurrence, 6> out{};
        Recurrence r;
        r.frequency = Frequency::Daily;
        r.interval = 2;
        r.end = RecurrenceEnd::Count;
        r.count = 3;
        Check(NextOccurrences(r, first, 600, {}, 0, out) == 3 && out[1].start == Utc(2026, 10, 29, 18) && out[2].start == Utc(2026, 10, 31, 18), "every other day, three times");
        Check(NextOccurrences(r, first, 600, {}, Utc(2026, 10, 29, 18, 5), out) == 2 && out[0].start == Utc(2026, 10, 29, 18), "a running occurrence is still listed");

        Recurrence weekly;
        weekly.frequency = Frequency::Weekly;
        weekly.weekdays = static_cast<std::uint8_t>((1u << 0) | (1u << 2) | (1u << 4)); // Mon Wed Fri
        const std::int64_t tuesday = Utc(2026, 10, 27, 9);
        NextOccurrences(weekly, tuesday, 3600, {}, 0, out);
        const int w0 = ToLocal(out[0].start, {}).weekday, w1 = ToLocal(out[1].start, {}).weekday, w2 = ToLocal(out[2].start, {}).weekday;
        Check(w0 == 2 && w1 == 4 && w2 == 0 && out[0].start > tuesday, "weekly picks Wed, Fri, Mon after a Tuesday start");

        Recurrence monthly;
        monthly.frequency = Frequency::Monthly;
        monthly.monthDay = 31;
        NextOccurrences(monthly, Utc(2026, 1, 31, 8), 600, {}, 0, out);
        Check(ToLocal(out[1].start, {}).month == 3 && ToLocal(out[2].start, {}).month == 5, "the 31st skips shorter months");

        Recurrence lastFriday;
        lastFriday.frequency = Frequency::Monthly;
        lastFriday.nth = -1;
        lastFriday.nthWeekday = 4;
        NextOccurrences(lastFriday, Utc(2026, 1, 30, 20), 600, {}, 0, out);
        const LocalTime lf = ToLocal(out[1].start, {});
        Check(lf.weekday == 4 && lf.month == 2 && lf.day == 27, "the last Friday of February 2026 is the 27th");

        Recurrence yearly;
        yearly.frequency = Frequency::Yearly;
        NextOccurrences(yearly, Utc(2024, 2, 29, 12), 600, {}, 0, out);
        Check(ToLocal(out[1].start, {}).year == 2028, "February 29 only recurs in leap years");

        Recurrence skip;
        skip.frequency = Frequency::Daily;
        skip.end = RecurrenceEnd::Count;
        skip.count = 4;
        skip.exceptionDays[0] = DaysFromCivil(2026, 10, 28);
        skip.exceptionCount = 1;
        const std::size_t n = NextOccurrences(skip, first, 600, {}, 0, out);
        Check(n == 3 && out[1].start == Utc(2026, 10, 29, 18) && out[1].index == 2, "an excluded day is skipped but still counts");

        Recurrence until;
        until.frequency = Frequency::Daily;
        until.end = RecurrenceEnd::Until;
        until.untilUtc = Utc(2026, 10, 29, 18);
        Check(NextOccurrences(until, first, 600, {}, 0, out) == 3, "until is inclusive");

        // RRULE text.
        Recurrence rule;
        rule.frequency = Frequency::Weekly;
        rule.interval = 2;
        rule.weekdays = static_cast<std::uint8_t>((1u << 0) | (1u << 2));
        rule.end = RecurrenceEnd::Count;
        rule.count = 10;
        std::array<char, 128> text{};
        FormatRRule(rule, text);
        Check(std::string(text.data()) == "FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,WE;COUNT=10", "RRULE formatting");
        Recurrence parsed;
        Check(ParseRRule(text.data(), parsed) && parsed.frequency == Frequency::Weekly && parsed.interval == 2 && parsed.weekdays == rule.weekdays &&
                  parsed.count == 10 && parsed.end == RecurrenceEnd::Count,
              "RRULE parsing");
        Check(ParseRRule("FREQ=MONTHLY;BYDAY=-1FR;UNTIL=20261231T235959Z", parsed) && parsed.nth == -1 && parsed.nthWeekday == 4 &&
                  parsed.end == RecurrenceEnd::Until && parsed.untilUtc == Utc(2026, 12, 31, 23, 59, 59),
              "RRULE ordinals and UNTIL");
        Check(!ParseRRule("INTERVAL=2", parsed) && !ParseRRule("FREQ=HOURLY", parsed) && !ParseRRule("FREQ=DAILY;INTERVAL=0", parsed), "RRULE rejects bad input");
    }

    // Cron.
    {
        CronSchedule cron;
        std::array<char, 96> error{};
        Check(ParseCron("*/15 9-17 * * MON-FRI", cron, error), "cron with steps, ranges and names");
        std::array<std::int64_t, 4> next{};
        const std::int64_t from = Utc(2026, 10, 27, 8, 50); // A Tuesday.
        Check(CronNext(cron, from, {}, next) == 4 && next[0] == Utc(2026, 10, 27, 9, 0) && next[1] == Utc(2026, 10, 27, 9, 15) && next[3] == Utc(2026, 10, 27, 9, 45), "next cron runs");
        Check(ParseCron("0 0 29 2 *", cron) && CronNext(cron, Utc(2026, 1, 1), {}, next) >= 1 && ToLocal(next[0], {}).year == 2028, "the next leap day");
        Check(ParseCron("0 0 13 * 5", cron), "day of month and weekday");
        std::array<std::int64_t, 8> many{};
        const std::size_t n = CronNext(cron, Utc(2026, 1, 1), {}, many);
        bool hasOtherFriday = false;
        for (std::size_t i = 0; i < n; ++i) {
            const LocalTime t = ToLocal(many[i], {});
            hasOtherFriday = hasOtherFriday || (t.weekday == 4 && t.day != 13);
        }
        Check(hasOtherFriday, "both day fields restricted means either may match");
        Check(!ParseCron("61 * * * *", cron, error) && std::strlen(error.data()) > 0 && !ParseCron("* * * *", cron) && !ParseCron("* * * * * * *", cron) && !ParseCron("0 25 * * *", cron),
              "invalid cron expressions are rejected with a message");
        Check(ParseCron("0 18 * * 1-5", cron), "weekday evening");
        std::array<char, 200> text{};
        DescribeCron(cron, text);
        Check(std::strstr(text.data(), "18:00") && std::strstr(text.data(), "Monday through Friday"), "cron description");
        Check(ParseCron("0 0 * * 7", cron) && cron.dayOfWeek[0], "7 means Sunday");
        Check(ParseCron("30 4 1 1 *", cron, error) && ParseCron("0 0 0 1 1 *", cron) && cron.hasSeconds, "six fields start with seconds");
    }

    // Solar.
    {
        const SolarTimes london = SolarEvents(DaysFromCivil(2024, 6, 21), 51.5074, -0.1278);
        const auto minutesOfDay = [](std::int64_t utc) { return static_cast<int>((utc % 86400 + 86400) % 86400) / 60; };
        Check(london.valid && std::abs(minutesOfDay(london.sunrise) - (3 * 60 + 43)) <= 6 && std::abs(minutesOfDay(london.sunset) - (20 * 60 + 21)) <= 6, "London midsummer sunrise and sunset");
        const SolarTimes tokyoTimes = SolarEvents(DaysFromCivil(2024, 12, 21), 35.68, 139.76);
        Check(tokyoTimes.valid && tokyoTimes.sunset > tokyoTimes.sunrise && tokyoTimes.sunset - tokyoTimes.sunrise < 11 * 3600, "short winter day in Tokyo");
        Check(!SolarEvents(DaysFromCivil(2024, 6, 21), 80, 0).valid && !SolarEvents(DaysFromCivil(2024, 12, 21), 80, 0).valid, "polar day and night have no events");
        Check(!SolarEvents(0, std::nan(""), 0).valid, "non-finite coordinates are rejected");
        SolarTrigger early;
        early.offsetMinutes = -30;
        Check(SolarTriggerTime(early, DaysFromCivil(2024, 6, 21), 51.5074, -0.1278) == london.sunrise - 1800, "solar offsets are in minutes");
    }

    // Conflicts.
    {
        std::array<CalendarEvent, 4> events{};
        events[0] = {1, 100, 200, "A", "", 0, 1, 0};
        events[1] = {2, 150, 250, "B", "", 1, 5, 0};
        events[2] = {3, 200, 300, "C", "", 2, 1, 0};  // Touches A: no conflict.
        events[3] = {4, 120, 180, "D", "", 3, 1, 1};  // Another resource.
        std::array<ConflictPair, 8> pairs{};
        const std::size_t n = FindConflicts(events, pairs);
        Check(n == 2, "overlaps on one resource, not touching ends or other resources");
        bool abPair = false, bcPair = false;
        for (std::size_t i = 0; i < n; ++i) {
            abPair = abPair || (pairs[i].first == 0 && pairs[i].second == 1 && pairs[i].winner == 1);
            bcPair = bcPair || (pairs[i].first == 1 && pairs[i].second == 2 && pairs[i].winner == 1);
        }
        Check(abPair && bcPair, "the higher priority wins");
        events[1].priority = 1;
        FindConflicts(events, pairs);
        Check(pairs[0].winner == -1, "equal priority is a tie");
    }

    // ---- UI -----------------------------------------------------------------------------------------------------
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {1700, 3000};
    io.DeltaTime = 1.f / 60;
    io.Fonts->AddFontDefault();
    unsigned char* pixels;
    int w, h;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    auto theme = MakeTheme(ThemePreset::PrecisionDark);
    ApplyTheme(theme);
    std::array<accessibility::SemanticNode, 1024> nodes{};
    accessibility::AccessibilityFrame semantics{nodes};
    AnimationState animation;
    ComponentOptions options{&theme, &animation, &semantics};

    const std::int64_t now = Utc(2026, 10, 27, 9, 30);
    const std::int32_t today = DaysFromCivil(2026, 10, 27);
    std::int32_t pickedDay = today;
    int timeOfDay = 9 * 3600 + 30 * 60;
    std::bitset<336> slots;
    ValueEdit paintEdit;
    char cronText[64] = "0 18 * * 1-5";
    Recurrence rule;
    rule.frequency = Frequency::Weekly;
    std::vector<CalendarEvent> events;
    events.push_back({1, Utc(2026, 10, 27, 10), Utc(2026, 10, 27, 12), "Morning loop", "", 0, 1, 0});
    events.push_back({2, Utc(2026, 10, 27, 11), Utc(2026, 10, 27, 13), "Press event", "", 2, 5, 0});
    events.push_back({3, Utc(2026, 10, 28, 14), Utc(2026, 10, 28, 15), "Locked show", "", 1, 1, 1, true});
    events.push_back({4, Utc(2026, 10, 29), Utc(2026, 10, 30), "All day", "", 3, 1, 0, false, true});
    CalendarState calendar;
    calendar.anchorDay = today;
    CalendarRequest calRequest;
    const char* resources[] = {"Main LED", "Side wall"};
    ResourceTimelineRequest timelineRequest;
    bool changedDate = false, changedTime = false, changedCron = false, changedRule = false;
    auto frame = [&] {
        semantics.Begin(static_cast<std::uint64_t>(ImGui::GetFrameCount()) + 1);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Host");
        DatePickerOptions dpo;
        dpo.today = today;
        changedDate = DatePicker("date", &pickedDay, dpo, options);
        changedTime = TimePicker("time", &timeOfDay, {}, options);
        paintEdit = WeekPainter("paint", &slots, {}, options);
        changedCron = CronEditor("cron", cronText, sizeof(cronText), {{}, now, 3}, options);
        RecurrenceEditorOptions reo;
        reo.firstStart = now;
        reo.now = now;
        reo.today = today;
        changedRule = RecurrenceEditor("rule", &rule, reo, options);
        CalendarOptions co;
        co.now = now;
        co.today = today;
        co.size = {1200, 700};
        co.resourceNames = resources;
        calRequest = Calendar("calendar", events, calendar, co, options);
        ResourceTimelineOptions rto;
        rto.now = now;
        timelineRequest = ResourceTimeline("timeline", resources, events, today, 0, rto, options);
        ImGui::End();
        ImGui::Render();
    };
    auto find = [&](std::string_view name) -> const accessibility::SemanticNode* {
        for (const auto& n : semantics.Tree().nodes)
            if (n.name == name)
                return &n;
        return nullptr;
    };
    auto centre = [](const accessibility::SemanticNode& n) { return ImVec2{(n.minimum.x + n.maximum.x) * .5f, (n.minimum.y + n.maximum.y) * .5f}; };
    auto click = [&](const accessibility::SemanticNode& n, float dx = 0, float dy = 0) {
        const ImVec2 c = centre(n);
        io.AddMousePosEvent(c.x + dx, c.y + dy);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
    };
    auto release = [&] {
        io.AddMouseButtonEvent(0, false);
        for (int i = 0; i < 32; ++i)
            frame();
    };
    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate(), "the schedule controls publish a valid semantic tree");

    // Date picker.
    if (const auto* day = find("2026-10-30")) {
        click(*day);
        Check(changedDate && pickedDay == DaysFromCivil(2026, 10, 30), "clicking a day selects it");
        release();
    } else
        Check(false, "date cell");
    // Time picker: the wheel steps the hour.
    if (const auto* hour = find("Hour")) {
        const ImVec2 c = centre(*hour);
        io.AddMousePosEvent(c.x, c.y);
        frame();
        io.AddMouseWheelEvent(0, 1);
        frame();
        Check(changedTime && timeOfDay / 3600 == 10, "the wheel changes the hour");
    } else
        Check(false, "hour field");
    // Week painter: dragging paints; the first cell decides the mode.
    if (const auto* grid = find("paint")) {
        const ImVec2 c = centre(*grid);
        io.AddMousePosEvent(c.x, c.y - 40);
        frame();
        io.AddMouseButtonEvent(0, true);
        frame();
        io.AddMousePosEvent(c.x, c.y + 40);
        frame();
        frame();
        Check(slots.count() > 3 && paintEdit.started == false, "dragging paints slots");
        release();
        Check(slots.count() > 3, "painted slots persist");
    } else
        Check(false, "week painter");
    // Cron presets.
    if (const auto* preset = find("Hourly")) {
        click(*preset);
        Check(changedCron && std::string(cronText) == "0 * * * *", "a preset replaces the expression");
        release();
    } else
        Check(false, "cron preset");
    // Recurrence: toggling a weekday chip.
    if (const auto* fri = find("Fri")) {
        click(*fri);
        Check(changedRule && (rule.weekdays & (1u << 4)), "weekday chips edit the rule");
        release();
    } else
        Check(false, "weekday chip");
    // Calendar: select an event, then drag on empty space to create.
    if (const auto* ev = find("Morning loop, 10:00 to 12:00")) {
        click(*ev, -20);
        Check(calRequest.select == 1, "clicking an event selects it");
        release();
    } else
        Check(false, "calendar event");
    if (const auto* mon = find("Today")) {
        click(*mon);
        release();
    }
    // The timeline and the month view publish their own nodes.
    calendar.view = CalendarView::Month;
    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate() && find("2026-10-27"), "the month view is valid and names its days");
    calendar.view = CalendarView::Agenda;
    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate() && find("Morning loop, 10:00 to 12:00"), "the agenda lists events");
    calendar.view = CalendarView::Day;
    for (int i = 0; i < 3; ++i)
        frame();
    Check(semantics.Tree().Validate(), "the day view is valid");
    (void)timelineRequest;
    ImGui::DestroyContext();
    std::printf("Show schedule failures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}
