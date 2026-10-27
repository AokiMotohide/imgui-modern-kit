#pragma once
#include <imkit/show/common.h>
#include <array>
#include <bitset>
#include <cstdint>
#include <span>
#include <string_view>

// Scheduling: wall-clock helpers, recurrence rules, cron, solar times, conflict detection and the calendar-style
// controls built on them. Times are UTC seconds since 1970 (std::int64_t); the host supplies the time zone through
// a callback, so the library never reads the system clock or a zone database. All functions here are pure.
namespace imkit::show {
using StableId = accessibility::StableId;

// Seconds east of UTC at a UTC instant. A null callback means UTC.
struct TimeZone {
    void* user = nullptr;
    int (*offsetSeconds)(void* user, std::int64_t utcSeconds) = nullptr;
    int Offset(std::int64_t utc) const { return offsetSeconds ? offsetSeconds(user, utc) : 0; }
};
// A fixed offset helper for hosts and tests: pass `&hours3600` as the user pointer.
TimeZone FixedZone(const int* offsetSeconds);

struct LocalTime {
    int year = 1970, month = 1, day = 1; // month 1..12
    int hour = 0, minute = 0, second = 0;
    int weekday = 3;                     // 0 = Monday .. 6 = Sunday.
};
std::int32_t DaysFromCivil(int year, int month, int day);        // Days since 1970-01-01.
void CivilFromDays(std::int32_t days, int& year, int& month, int& day);
int WeekdayFromDays(std::int32_t days);                          // 0 = Monday.
int DaysInMonth(int year, int month);
LocalTime ToLocal(std::int64_t utc, TimeZone zone);
// Local wall-clock fields to UTC. In a spring-forward gap the time moves forward by the gap; in a fall-back overlap
// the first occurrence is used.
std::int64_t ToUtc(int year, int month, int day, int hour, int minute, int second, TimeZone zone);
std::int64_t LocalDayStart(std::int32_t days, TimeZone zone);   // UTC instant of 00:00 local on that day.
std::int32_t LocalDay(std::int64_t utc, TimeZone zone);         // Local calendar day containing the instant.

// ---- Recurrence ---------------------------------------------------------------------------------------------
enum class Frequency { Once, Daily, Weekly, Monthly, Yearly };
enum class RecurrenceEnd { Never, Count, Until };
struct Recurrence {
    Frequency frequency = Frequency::Once;
    int interval = 1;                // Every n days/weeks/months/years.
    std::uint8_t weekdays = 0;       // Weekly: bit 0 = Monday ... bit 6 = Sunday; zero uses the start's weekday.
    int monthDay = 0;                // Monthly: day of month 1..31, -1 = last day; zero uses the start's day.
    int nth = 0;                     // Monthly by weekday: 1..4 or -1 (last); zero selects by day of month instead.
    int nthWeekday = 0;              // Weekday for `nth`, 0 = Monday.
    RecurrenceEnd end = RecurrenceEnd::Never;
    int count = 10;                  // Total occurrences when end == Count (exceptions still count).
    std::int64_t untilUtc = 0;       // Last permitted start when end == Until.
    std::array<std::int32_t, 16> exceptionDays{}; // Local days skipped.
    int exceptionCount = 0;
};
struct Occurrence {
    std::int64_t start = 0, end = 0;
    int index = 0;                   // Position in the series, counting skipped exceptions.
};
// Occurrences that end after `fromUtc`, in order. `firstStart` is the first occurrence (its local time of day is
// repeated). Months without the requested day (for example the 31st) and non-leap February 29 are skipped.
std::size_t NextOccurrences(const Recurrence& rule, std::int64_t firstStart, std::int64_t durationSeconds, TimeZone zone,
                            std::int64_t fromUtc, std::span<Occurrence> out);
// RFC 5545 RRULE subset: FREQ, INTERVAL, BYDAY (with ordinals for monthly), BYMONTHDAY, COUNT, UNTIL (UTC).
std::size_t FormatRRule(const Recurrence& rule, std::span<char> out);
bool ParseRRule(std::string_view text, Recurrence& rule);

// ---- Cron ---------------------------------------------------------------------------------------------------
struct CronSchedule {
    std::bitset<60> second, minute;
    std::bitset<24> hour;
    std::bitset<32> dayOfMonth;      // 1..31
    std::bitset<13> month;           // 1..12
    std::bitset<7> dayOfWeek;        // 0 = Sunday .. 6 = Saturday, as in cron.
    bool dayOfMonthAny = true, dayOfWeekAny = true, hasSeconds = false;
};
// Five fields (minute hour day month weekday) or six with leading seconds. Supports *, lists, ranges, steps and
// names (JAN, MON). On failure `error` (when given) receives a short message.
bool ParseCron(std::string_view text, CronSchedule& schedule, std::span<char> error = {});
// When both day fields are restricted, a day matches if either does (classic cron).
std::size_t CronNext(const CronSchedule& schedule, std::int64_t fromUtc, TimeZone zone, std::span<std::int64_t> out);
std::size_t DescribeCron(const CronSchedule& schedule, std::span<char> out);

// ---- Solar --------------------------------------------------------------------------------------------------
struct SolarTimes {
    std::int64_t sunrise = 0, sunset = 0; // UTC.
    bool valid = false;                    // False in polar day or night.
};
// Sunrise and sunset for a local calendar day (NOAA / Meeus approximation, within a few minutes). Latitude north and
// longitude east are positive, in degrees.
SolarTimes SolarEvents(std::int32_t day, double latitude, double longitude);
struct SolarTrigger {
    bool sunset = false;
    int offsetMinutes = 0;           // Negative = before.
};
std::int64_t SolarTriggerTime(const SolarTrigger& trigger, std::int32_t day, double latitude, double longitude);

// ---- Events and conflicts -----------------------------------------------------------------------------------
struct CalendarEvent {
    StableId id = 0;
    std::int64_t start = 0, end = 0; // UTC; end > start.
    const char* title = "";
    const char* subtitle = "";
    int color = 0;                   // Index into the layer colours.
    int priority = 0;                // Higher wins when events overlap on one resource.
    int resource = 0;                // Output, zone or server the event plays on.
    bool locked = false;
    bool allDay = false;
};
struct ConflictPair {
    int first = 0, second = 0;       // Indices into the event span.
    int winner = -1;                 // Index of the higher-priority event, or -1 for a tie.
};
// Events overlap when they share a resource and their open intervals intersect (touching ends do not conflict).
std::size_t FindConflicts(std::span<const CalendarEvent> events, std::span<ConflictPair> out);

// ---- Pickers ------------------------------------------------------------------------------------------------
struct DatePickerOptions {
    int firstWeekday = 0;            // 0 = Monday.
    std::int32_t minimumDay = -1000000, maximumDay = 1000000;
    std::int32_t today = 0;          // Local day to ring; supplied by the host's clock.
    bool range = false;              // Pick a span: the second click sets the end (stored in `rangeEnd`).
};
// Month grid. `*day` is the selected local day (days since 1970-01-01). In range mode `*rangeEnd` is the other end.
// Returns true when the selection changed.
bool DatePicker(const char* id, std::int32_t* day, DatePickerOptions options = {}, ComponentOptions component = {},
                std::int32_t* rangeEnd = nullptr);
struct TimePickerOptions {
    bool seconds = false;
    int minuteStep = 1;
};
// Hour, minute (and second) fields: click and drag vertically, use the wheel, or press Up/Down when focused.
bool TimePicker(const char* id, int* secondsOfDay, TimePickerOptions options = {}, ComponentOptions component = {});
// A text-style field that opens a date and time popup. Returns true when `*utc` changed.
bool DateTimeField(const char* id, std::int64_t* utc, TimeZone zone, std::int32_t today, ComponentOptions component = {});

// ---- Recurrence and cron editors ---------------------------------------------------------------------------
struct RecurrenceEditorOptions {
    TimeZone zone{};
    std::int64_t firstStart = 0;     // First occurrence, used for the preview and as the default weekday/day.
    std::int64_t durationSeconds = 3600;
    std::int64_t now = 0;            // Host clock; the preview lists occurrences after it.
    std::int32_t today = 0;
    int previewCount = 5;
};
// Frequency, interval, weekday chips, monthly pattern, end condition, exceptions and a preview of the next runs.
bool RecurrenceEditor(const char* id, Recurrence* rule, RecurrenceEditorOptions options, ComponentOptions component = {});
struct CronEditorOptions {
    TimeZone zone{};
    std::int64_t now = 0;
    int previewCount = 5;
};
// `text` is the host's expression buffer. Shows parse status, a plain-language description and the next runs.
bool CronEditor(const char* id, char* text, std::size_t capacity, CronEditorOptions options, ComponentOptions component = {});
bool SolarTriggerEditor(const char* id, SolarTrigger* trigger, double latitude, double longitude, std::int32_t today, TimeZone zone,
                        ComponentOptions component = {});
// Lists upcoming occurrences as local date and time ranges.
void NextRuns(const char* id, std::span<const Occurrence> occurrences, TimeZone zone, ComponentOptions component = {});
// Draws the overlaps FindConflicts found, with the winner of each.
void ConflictPanel(const char* id, std::span<const CalendarEvent> events, std::span<const ConflictPair> conflicts,
                   TimeZone zone, ComponentOptions component = {});

// ---- Week painter -------------------------------------------------------------------------------------------
struct WeekPainterOptions {
    ImVec2 size{0, 0};               // Zero fills the width and uses about 12 frame heights.
    int firstWeekday = 0;
    bool compact = false;
};
// 7 days x 48 half-hour slots (index = day * 48 + slot, day 0 = Monday). Drag to paint; the first cell decides
// whether the stroke paints or erases. `value` of the edit is the number of lit slots.
ValueEdit WeekPainter(const char* id, std::bitset<336>* slots, WeekPainterOptions options = {}, ComponentOptions component = {});

// ---- Calendar -----------------------------------------------------------------------------------------------
enum class CalendarView { Month, Week, Day, Agenda };
struct CalendarState {
    CalendarView view = CalendarView::Week;
    std::int32_t anchorDay = 0;      // Any local day inside the shown period.
    StableId selected = 0;
    bool scrolled = false;           // The week/day view scrolls to the morning once.
};
struct CalendarOptions {
    ImVec2 size{0, 0};               // Zero fills the width and uses about 22 frame heights.
    TimeZone zone{};
    std::int64_t now = 0;            // Host clock for the "now" line.
    std::int32_t today = 0;
    int firstWeekday = 0;
    float hourHeight = 0;            // Zero uses about 2.2 frame heights.
    int snapMinutes = 15;
    std::span<const char* const> resourceNames{}; // Used by the agenda view and tooltips.
};
struct CalendarRequest {
    StableId select = 0;
    StableId activate = 0;           // Double-click.
    StableId context = 0;            // Right-click.
    StableId move = 0;               // An event was dragged: new start in `start`, duration unchanged.
    StableId resize = 0;             // The bottom edge was dragged: new end in `end`.
    std::int64_t start = 0, end = 0;
    bool create = false;             // A range was dragged on empty space: [start, end).
    std::int32_t clickedDay = 0;     // Month view: a day cell was clicked (valid when `dayClicked`).
    bool dayClicked = false;
};
CalendarRequest Calendar(const char* id, std::span<const CalendarEvent> events, CalendarState& state, CalendarOptions options = {},
                         ComponentOptions component = {});

// Rows are resources, the horizontal axis is one local day (24 h). Drag a bar to move it in time or to another row.
struct ResourceTimelineOptions {
    ImVec2 size{0, 0};
    TimeZone zone{};
    std::int64_t now = 0;
    int snapMinutes = 15;
};
struct ResourceTimelineRequest {
    StableId select = 0;
    StableId move = 0;
    std::int64_t start = 0;
    int resource = 0;
};
ResourceTimelineRequest ResourceTimeline(const char* id, std::span<const char* const> resources, std::span<const CalendarEvent> events,
                                         std::int32_t day, StableId selected, ResourceTimelineOptions options = {},
                                         ComponentOptions component = {});
} // namespace imkit::show
