#include <imkit/show/schedule.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace imkit::show {
namespace {
constexpr double Pi = 3.14159265358979323846;
std::int64_t FloorDiv(std::int64_t a, std::int64_t b) {
    std::int64_t q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0)))
        --q;
    return q;
}
bool Leap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }
} // namespace

TimeZone FixedZone(const int* offsetSeconds) {
    TimeZone zone;
    zone.user = const_cast<int*>(offsetSeconds);
    zone.offsetSeconds = [](void* user, std::int64_t) { return *static_cast<const int*>(user); };
    return zone;
}

// ---- Calendar arithmetic (proleptic Gregorian, after H. Hinnant) -----------------------------------------------
std::int32_t DaysFromCivil(int year, int month, int day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = static_cast<unsigned>((153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1);
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int>(doe) - 719468;
}
void CivilFromDays(std::int32_t days, int& year, int& month, int& day) {
    const int z = days + 719468;
    const int era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    year = static_cast<int>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    day = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    month = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    year += month <= 2;
}
int WeekdayFromDays(std::int32_t days) {
    int w = (days + 3) % 7; // 1970-01-01 was a Thursday.
    return w < 0 ? w + 7 : w;
}
int DaysInMonth(int year, int month) {
    static const int table[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12)
        return 30;
    return month == 2 && Leap(year) ? 29 : table[month - 1];
}
LocalTime ToLocal(std::int64_t utc, TimeZone zone) {
    const std::int64_t local = utc + zone.Offset(utc);
    const std::int64_t days = FloorDiv(local, 86400);
    const int sec = static_cast<int>(local - days * 86400);
    LocalTime t;
    CivilFromDays(static_cast<std::int32_t>(days), t.year, t.month, t.day);
    t.hour = sec / 3600;
    t.minute = sec / 60 % 60;
    t.second = sec % 60;
    t.weekday = WeekdayFromDays(static_cast<std::int32_t>(days));
    return t;
}
std::int64_t ToUtc(int year, int month, int day, int hour, int minute, int second, TimeZone zone) {
    const std::int64_t t = static_cast<std::int64_t>(DaysFromCivil(year, month, day)) * 86400 + hour * 3600 + minute * 60 + second;
    const int before = zone.Offset(t - 86400), after = zone.Offset(t + 86400);
    const std::int64_t c1 = t - before, c2 = t - after;
    const bool v1 = zone.Offset(c1) == before, v2 = zone.Offset(c2) == after;
    if (v1 && v2)
        return std::min(c1, c2); // Overlap: the first occurrence.
    if (v1)
        return c1;
    if (v2)
        return c2;
    return c1; // Gap: moves forward by the length of the gap.
}
std::int64_t LocalDayStart(std::int32_t days, TimeZone zone) {
    int y, m, d;
    CivilFromDays(days, y, m, d);
    return ToUtc(y, m, d, 0, 0, 0, zone);
}
std::int32_t LocalDay(std::int64_t utc, TimeZone zone) {
    return static_cast<std::int32_t>(FloorDiv(utc + zone.Offset(utc), 86400));
}

// ---- Recurrence ------------------------------------------------------------------------------------------------
namespace {
// The nth (1..4) or last (-1) given weekday of a month as a day number, or INT32_MIN when it does not exist.
std::int32_t NthWeekday(int year, int month, int nth, int weekday) {
    const int dim = DaysInMonth(year, month);
    const int firstWeekday = WeekdayFromDays(DaysFromCivil(year, month, 1));
    const int day1 = 1 + (weekday - firstWeekday + 7) % 7;
    int day = 0;
    if (nth > 0)
        day = day1 + (nth - 1) * 7;
    else if (nth == -1)
        day = day1 + 7 * ((dim - day1) / 7);
    if (day < 1 || day > dim)
        return INT32_MIN;
    return DaysFromCivil(year, month, day);
}
} // namespace

std::size_t NextOccurrences(const Recurrence& rule, std::int64_t firstStart, std::int64_t durationSeconds, TimeZone zone,
                            std::int64_t fromUtc, std::span<Occurrence> out) {
    if (out.empty())
        return 0;
    const LocalTime first = ToLocal(firstStart, zone);
    const std::int32_t startDay = DaysFromCivil(first.year, first.month, first.day);
    const int tod = first.hour * 3600 + first.minute * 60 + first.second;
    const int interval = std::max(1, rule.interval);
    std::size_t written = 0;
    int index = 0;
    const auto skipped = [&](std::int32_t day) {
        for (int i = 0; i < rule.exceptionCount && i < static_cast<int>(rule.exceptionDays.size()); ++i)
            if (rule.exceptionDays[static_cast<std::size_t>(i)] == day)
                return true;
        return false;
    };
    // Returns false when the series or the output is exhausted.
    const auto emit = [&](std::int32_t day) {
        int y, m, d;
        CivilFromDays(day, y, m, d);
        const std::int64_t start = ToUtc(y, m, d, tod / 3600, tod / 60 % 60, tod % 60, zone);
        if (rule.end == RecurrenceEnd::Until && start > rule.untilUtc)
            return false;
        if (rule.end == RecurrenceEnd::Count && index >= rule.count)
            return false;
        const int number = index++;
        if (skipped(day))
            return true;
        const std::int64_t end = start + durationSeconds;
        if (end > fromUtc) {
            out[written++] = {start, end, number};
            if (written == out.size())
                return false;
        }
        return true;
    };
    constexpr int Guard = 200000;
    switch (rule.frequency) {
    case Frequency::Once: emit(startDay); break;
    case Frequency::Daily:
        for (int k = 0; k < Guard; ++k)
            if (!emit(startDay + k * interval))
                break;
        break;
    case Frequency::Weekly: {
        const unsigned mask = rule.weekdays ? rule.weekdays : (1u << first.weekday);
        const std::int32_t weekStart = startDay - first.weekday;
        for (int w = 0; w < Guard / 7; ++w)
            for (int wd = 0; wd < 7; ++wd) {
                if (!(mask & (1u << wd)))
                    continue;
                const std::int32_t day = weekStart + w * interval * 7 + wd;
                if (day < startDay)
                    continue;
                if (!emit(day))
                    return written;
            }
        break;
    }
    case Frequency::Monthly: {
        const int base = first.year * 12 + first.month - 1;
        for (int k = 0; k < Guard / 30; ++k) {
            const int mi = base + k * interval;
            const int y = mi / 12, m = mi % 12 + 1;
            std::int32_t day = INT32_MIN;
            if (rule.nth != 0) {
                day = NthWeekday(y, m, rule.nth, rule.nthWeekday);
            } else {
                int dom = rule.monthDay == 0 ? first.day : rule.monthDay;
                if (dom == -1)
                    dom = DaysInMonth(y, m);
                if (dom >= 1 && dom <= DaysInMonth(y, m))
                    day = DaysFromCivil(y, m, dom);
            }
            if (day == INT32_MIN || day < startDay)
                continue;
            if (!emit(day))
                break;
        }
        break;
    }
    case Frequency::Yearly:
        for (int k = 0; k < Guard / 365; ++k) {
            const int y = first.year + k * interval;
            if (first.month == 2 && first.day == 29 && !Leap(y))
                continue;
            if (!emit(DaysFromCivil(y, first.month, first.day)))
                break;
        }
        break;
    }
    return written;
}

namespace {
const char* const DayCodes[7] = {"MO", "TU", "WE", "TH", "FR", "SA", "SU"};
int DayCode(std::string_view s) {
    for (int i = 0; i < 7; ++i)
        if (s == DayCodes[i])
            return i;
    return -1;
}
bool ToInt(std::string_view s, int& out) {
    if (s.empty())
        return false;
    char* end = nullptr;
    const std::string text(s);
    const long v = std::strtol(text.c_str(), &end, 10);
    if (*end != '\0' || v < -100000 || v > 100000)
        return false;
    out = static_cast<int>(v);
    return true;
}
} // namespace

std::size_t FormatRRule(const Recurrence& rule, std::span<char> out) {
    if (out.empty())
        return 0;
    std::string s;
    if (rule.frequency != Frequency::Once) {
        static const char* const freq[] = {"", "DAILY", "WEEKLY", "MONTHLY", "YEARLY"};
        s = std::string("FREQ=") + freq[static_cast<int>(rule.frequency)];
        char text[64];
        if (rule.interval > 1) {
            std::snprintf(text, sizeof text, ";INTERVAL=%d", rule.interval);
            s += text;
        }
        if (rule.frequency == Frequency::Weekly && rule.weekdays) {
            s += ";BYDAY=";
            bool firstDay = true;
            for (int d = 0; d < 7; ++d)
                if (rule.weekdays & (1u << d)) {
                    if (!firstDay)
                        s += ',';
                    s += DayCodes[d];
                    firstDay = false;
                }
        }
        if (rule.frequency == Frequency::Monthly) {
            if (rule.nth != 0) {
                std::snprintf(text, sizeof text, ";BYDAY=%d%s", rule.nth, DayCodes[std::clamp(rule.nthWeekday, 0, 6)]);
                s += text;
            } else if (rule.monthDay != 0) {
                std::snprintf(text, sizeof text, ";BYMONTHDAY=%d", rule.monthDay);
                s += text;
            }
        }
        if (rule.end == RecurrenceEnd::Count) {
            std::snprintf(text, sizeof text, ";COUNT=%d", rule.count);
            s += text;
        } else if (rule.end == RecurrenceEnd::Until) {
            const TimeZone utc;
            const LocalTime t = ToLocal(rule.untilUtc, utc);
            std::snprintf(text, sizeof text, ";UNTIL=%04d%02d%02dT%02d%02d%02dZ", t.year, t.month, t.day, t.hour, t.minute, t.second);
            s += text;
        }
    }
    const std::size_t n = std::min(s.size(), out.size() - 1);
    std::memcpy(out.data(), s.data(), n);
    out[n] = '\0';
    return n;
}

bool ParseRRule(std::string_view text, Recurrence& rule) {
    Recurrence r;
    bool haveFrequency = false;
    while (!text.empty()) {
        const std::size_t semi = text.find(';');
        const std::string_view part = text.substr(0, semi);
        text = semi == std::string_view::npos ? std::string_view{} : text.substr(semi + 1);
        if (part.empty())
            continue;
        const std::size_t eq = part.find('=');
        if (eq == std::string_view::npos)
            return false;
        const std::string_view key = part.substr(0, eq), value = part.substr(eq + 1);
        if (key == "FREQ") {
            if (value == "DAILY") r.frequency = Frequency::Daily;
            else if (value == "WEEKLY") r.frequency = Frequency::Weekly;
            else if (value == "MONTHLY") r.frequency = Frequency::Monthly;
            else if (value == "YEARLY") r.frequency = Frequency::Yearly;
            else return false;
            haveFrequency = true;
        } else if (key == "INTERVAL") {
            if (!ToInt(value, r.interval) || r.interval < 1)
                return false;
        } else if (key == "COUNT") {
            if (!ToInt(value, r.count) || r.count < 1)
                return false;
            r.end = RecurrenceEnd::Count;
        } else if (key == "BYMONTHDAY") {
            if (!ToInt(value.substr(0, value.find(',')), r.monthDay) || r.monthDay == 0 || r.monthDay > 31 || r.monthDay < -1)
                return false;
        } else if (key == "BYDAY") {
            std::string_view list = value;
            while (!list.empty()) {
                const std::size_t comma = list.find(',');
                const std::string_view item = list.substr(0, comma);
                list = comma == std::string_view::npos ? std::string_view{} : list.substr(comma + 1);
                if (item.size() < 2)
                    return false;
                const int wd = DayCode(item.substr(item.size() - 2));
                if (wd < 0)
                    return false;
                if (item.size() > 2) {
                    int nth = 0;
                    if (!ToInt(item.substr(0, item.size() - 2), nth) || nth == 0 || nth > 4 || nth < -1)
                        return false;
                    r.nth = nth;
                    r.nthWeekday = wd;
                } else {
                    r.weekdays = static_cast<std::uint8_t>(r.weekdays | (1u << wd));
                }
            }
        } else if (key == "UNTIL") {
            int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
            const std::string v(value);
            const int n = std::sscanf(v.c_str(), "%4d%2d%2dT%2d%2d%2d", &y, &mo, &d, &h, &mi, &s);
            if (n < 3 || mo < 1 || mo > 12 || d < 1 || d > DaysInMonth(y, mo))
                return false;
            r.untilUtc = ToUtc(y, mo, d, n >= 6 ? h : 23, n >= 6 ? mi : 59, n >= 6 ? s : 59, TimeZone{});
            r.end = RecurrenceEnd::Until;
        }
        // Other keys (WKST, BYMONTH, ...) are outside the supported subset and ignored.
    }
    if (!haveFrequency)
        return false;
    rule = r;
    return true;
}

// ---- Cron ------------------------------------------------------------------------------------------------------
namespace {
void SetError(std::span<char> error, const char* message) {
    if (!error.empty())
        std::snprintf(error.data(), error.size(), "%s", message);
}
int NameValue(std::string_view s, const char* const* names, int count, int base) {
    if (!names || s.size() != 3)
        return -1;
    for (int i = 0; i < count; ++i) {
        bool same = true;
        for (int k = 0; k < 3; ++k)
            same = same && std::toupper(static_cast<unsigned char>(s[static_cast<std::size_t>(k)])) == names[i][k];
        if (same)
            return base + i;
    }
    return -1;
}
bool Number(std::string_view s, int lo, int hi, const char* const* names, int nameCount, int nameBase, int& out) {
    if (const int named = NameValue(s, names, nameCount, nameBase); named >= 0) {
        out = named;
        return true;
    }
    if (s.empty() || s.size() > 3)
        return false;
    for (char c : s)
        if (!std::isdigit(static_cast<unsigned char>(c)))
            return false;
    out = std::atoi(std::string(s).c_str());
    return out >= lo && out <= hi;
}
bool Field(std::string_view text, int lo, int hi, std::bitset<64>& bits, bool* any, const char* const* names, int nameCount, int nameBase) {
    bits.reset();
    if (any)
        *any = false;
    if (text.empty())
        return false;
    int parts = 0;
    while (!text.empty()) {
        const std::size_t comma = text.find(',');
        std::string_view part = text.substr(0, comma);
        text = comma == std::string_view::npos ? std::string_view{} : text.substr(comma + 1);
        ++parts;
        int step = 1;
        bool hasStep = false;
        if (const std::size_t slash = part.find('/'); slash != std::string_view::npos) {
            if (!Number(part.substr(slash + 1), 1, hi - lo + 1, nullptr, 0, 0, step))
                return false;
            hasStep = true;
            part = part.substr(0, slash);
        }
        int a = lo, b = hi;
        const bool star = part == "*" || part == "?";
        if (!star) {
            const std::size_t dash = part.find('-');
            if (dash == std::string_view::npos) {
                if (!Number(part, lo, hi, names, nameCount, nameBase, a))
                    return false;
                b = hasStep ? hi : a;
            } else if (!Number(part.substr(0, dash), lo, hi, names, nameCount, nameBase, a) ||
                       !Number(part.substr(dash + 1), lo, hi, names, nameCount, nameBase, b) || a > b) {
                return false;
            }
        }
        if (star && !hasStep && any && parts == 1 && text.empty())
            *any = true;
        for (int v = a; v <= b; v += step)
            bits.set(static_cast<std::size_t>(v));
    }
    return bits.any();
}
const char* const MonthNames[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
const char* const DowNames[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
} // namespace

bool ParseCron(std::string_view text, CronSchedule& schedule, std::span<char> error) {
    std::string_view fields[6];
    int count = 0;
    while (!text.empty()) {
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
            text.remove_prefix(1);
        if (text.empty())
            break;
        std::size_t end = 0;
        while (end < text.size() && !std::isspace(static_cast<unsigned char>(text[end])))
            ++end;
        if (count == 6) {
            SetError(error, "Too many fields. Use 5 (or 6 with seconds).");
            return false;
        }
        fields[count++] = text.substr(0, end);
        text = text.substr(end);
    }
    if (count != 5 && count != 6) {
        SetError(error, "Expected 5 fields: minute hour day month weekday.");
        return false;
    }
    CronSchedule s;
    s.hasSeconds = count == 6;
    const int o = s.hasSeconds ? 1 : 0;
    std::bitset<64> bits;
    if (s.hasSeconds) {
        if (!Field(fields[0], 0, 59, bits, nullptr, nullptr, 0, 0)) {
            SetError(error, "The seconds field is not valid (0-59).");
            return false;
        }
        for (std::size_t i = 0; i < 60; ++i)
            s.second[i] = bits[i];
    } else {
        s.second.set(0);
    }
    if (!Field(fields[o], 0, 59, bits, nullptr, nullptr, 0, 0)) {
        SetError(error, "The minute field is not valid (0-59).");
        return false;
    }
    for (std::size_t i = 0; i < 60; ++i)
        s.minute[i] = bits[i];
    if (!Field(fields[o + 1], 0, 23, bits, nullptr, nullptr, 0, 0)) {
        SetError(error, "The hour field is not valid (0-23).");
        return false;
    }
    for (std::size_t i = 0; i < 24; ++i)
        s.hour[i] = bits[i];
    if (!Field(fields[o + 2], 1, 31, bits, &s.dayOfMonthAny, nullptr, 0, 0)) {
        SetError(error, "The day-of-month field is not valid (1-31).");
        return false;
    }
    for (std::size_t i = 0; i < 32; ++i)
        s.dayOfMonth[i] = bits[i];
    if (!Field(fields[o + 3], 1, 12, bits, nullptr, MonthNames, 12, 1)) {
        SetError(error, "The month field is not valid (1-12 or JAN-DEC).");
        return false;
    }
    for (std::size_t i = 0; i < 13; ++i)
        s.month[i] = bits[i];
    if (!Field(fields[o + 4], 0, 7, bits, &s.dayOfWeekAny, DowNames, 7, 0)) {
        SetError(error, "The weekday field is not valid (0-7 or SUN-SAT).");
        return false;
    }
    for (std::size_t i = 0; i < 7; ++i)
        s.dayOfWeek[i] = bits[i];
    if (bits[7])
        s.dayOfWeek.set(0); // 7 is Sunday too.
    schedule = s;
    SetError(error, "");
    return true;
}

std::size_t CronNext(const CronSchedule& schedule, std::int64_t fromUtc, TimeZone zone, std::span<std::int64_t> out) {
    std::size_t written = 0;
    const std::int32_t firstDay = LocalDay(fromUtc, zone);
    for (int offset = 0; offset < 366 * 8 && written < out.size(); ++offset) {
        const std::int32_t day = firstDay + offset;
        int y, m, d;
        CivilFromDays(day, y, m, d);
        if (!schedule.month[static_cast<std::size_t>(m)])
            continue;
        const int weekday = WeekdayFromDays(day);
        const bool domMatch = schedule.dayOfMonth[static_cast<std::size_t>(d)];
        const bool dowMatch = schedule.dayOfWeek[static_cast<std::size_t>((weekday + 1) % 7)];
        const bool match = (schedule.dayOfMonthAny && schedule.dayOfWeekAny) ? true
                           : schedule.dayOfMonthAny                           ? dowMatch
                           : schedule.dayOfWeekAny                            ? domMatch
                                                                              : (domMatch || dowMatch);
        if (!match)
            continue;
        for (int h = 0; h < 24; ++h) {
            if (!schedule.hour[static_cast<std::size_t>(h)])
                continue;
            for (int mi = 0; mi < 60; ++mi) {
                if (!schedule.minute[static_cast<std::size_t>(mi)])
                    continue;
                for (int s = 0; s < 60; ++s) {
                    if (!schedule.second[static_cast<std::size_t>(s)])
                        continue;
                    const std::int64_t t = ToUtc(y, m, d, h, mi, s, zone);
                    if (t > fromUtc) {
                        out[written++] = t;
                        if (written == out.size())
                            return written;
                    }
                }
            }
        }
    }
    return written;
}

std::size_t DescribeCron(const CronSchedule& s, std::span<char> out) {
    if (out.empty())
        return 0;
    std::string text;
    char buf[96];
    const auto list = [](const auto& bits, std::size_t lo, std::size_t hi) {
        std::vector<int> v;
        for (std::size_t i = lo; i <= hi; ++i)
            if (bits[i])
                v.push_back(static_cast<int>(i));
        return v;
    };
    const auto minutes = list(s.minute, 0, 59), hours = list(s.hour, 0, 23);
    const bool everyMinute = minutes.size() == 60, everyHour = hours.size() == 24;
    if (everyMinute && everyHour && !s.hasSeconds) {
        text = "Every minute";
    } else if (everyHour && minutes.size() > 1 && minutes.size() < 60 && minutes[1] - minutes[0] == static_cast<int>(60 / minutes.size()) && minutes[0] == 0) {
        std::snprintf(buf, sizeof buf, "Every %d minutes", 60 / static_cast<int>(minutes.size()));
        text = buf;
    } else if (everyHour && minutes.size() == 1) {
        std::snprintf(buf, sizeof buf, "At minute %d of every hour", minutes[0]);
        text = buf;
    } else if (hours.size() * minutes.size() <= 4) {
        text = "At ";
        bool first = true;
        for (int h : hours)
            for (int m : minutes) {
                std::snprintf(buf, sizeof buf, "%s%02d:%02d", first ? "" : ", ", h, m);
                text += buf;
                first = false;
            }
    } else {
        std::snprintf(buf, sizeof buf, "At %d minute(s) past %d hour(s)", static_cast<int>(minutes.size()), static_cast<int>(hours.size()));
        text = buf;
    }
    if (s.hasSeconds && s.second.count() != 1) {
        text += ", every selected second";
    }
    static const char* const dayNames[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    if (!s.dayOfWeekAny) {
        const auto days = list(s.dayOfWeek, 0, 6);
        if (days.size() == 5 && !s.dayOfWeek[0] && !s.dayOfWeek[6]) {
            text += ", Monday through Friday";
        } else if (days.size() == 2 && s.dayOfWeek[0] && s.dayOfWeek[6]) {
            text += ", on weekends";
        } else {
            text += ", on ";
            for (std::size_t i = 0; i < days.size(); ++i) {
                if (i)
                    text += i + 1 == days.size() ? " and " : ", ";
                text += dayNames[days[i]];
            }
        }
    }
    if (!s.dayOfMonthAny) {
        const auto days = list(s.dayOfMonth, 1, 31);
        text += s.dayOfWeekAny ? ", on day " : ", or on day ";
        for (std::size_t i = 0; i < days.size(); ++i) {
            if (i)
                text += i + 1 == days.size() ? " and " : ", ";
            text += std::to_string(days[i]);
        }
        text += " of the month";
    }
    if (s.month.count() != 12) {
        static const char* const monthNames[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
        text += ", in ";
        bool first = true;
        for (int m = 1; m <= 12; ++m)
            if (s.month[static_cast<std::size_t>(m)]) {
                text += first ? "" : ", ";
                text += monthNames[m - 1];
                first = false;
            }
    }
    const std::size_t n = std::min(text.size(), out.size() - 1);
    std::memcpy(out.data(), text.data(), n);
    out[n] = '\0';
    return n;
}

// ---- Solar -----------------------------------------------------------------------------------------------------
SolarTimes SolarEvents(std::int32_t day, double latitude, double longitude) {
    SolarTimes result;
    if (!std::isfinite(latitude) || !std::isfinite(longitude) || std::abs(latitude) > 90 || std::abs(longitude) > 180)
        return result;
    const double rad = Pi / 180.0;
    const double julianNoon = static_cast<double>(day) + 2440587.5 + 0.5; // 12:00 UTC of the civil date.
    const double n = std::ceil(julianNoon - 2451545.0 + 0.0008);
    const double meanSolarTime = n - longitude / 360.0;
    const double M = std::fmod(357.5291 + 0.98560028 * meanSolarTime, 360.0);
    const double C = 1.9148 * std::sin(M * rad) + 0.0200 * std::sin(2 * M * rad) + 0.0003 * std::sin(3 * M * rad);
    const double lambda = std::fmod(M + C + 180.0 + 102.9372, 360.0);
    const double transit = 2451545.0 + meanSolarTime + 0.0053 * std::sin(M * rad) - 0.0069 * std::sin(2 * lambda * rad);
    const double sinDelta = std::sin(lambda * rad) * std::sin(23.4397 * rad);
    const double cosDelta = std::cos(std::asin(sinDelta));
    const double cosOmega = (std::sin(-0.833 * rad) - std::sin(latitude * rad) * sinDelta) / (std::cos(latitude * rad) * cosDelta);
    if (cosOmega < -1.0 || cosOmega > 1.0)
        return result; // Polar day or night.
    const double omega = std::acos(cosOmega) / rad;
    const auto toUnix = [](double jd) { return static_cast<std::int64_t>(std::llround((jd - 2440587.5) * 86400.0)); };
    result.sunrise = toUnix(transit - omega / 360.0);
    result.sunset = toUnix(transit + omega / 360.0);
    result.valid = true;
    return result;
}
std::int64_t SolarTriggerTime(const SolarTrigger& trigger, std::int32_t day, double latitude, double longitude) {
    const SolarTimes t = SolarEvents(day, latitude, longitude);
    if (!t.valid)
        return 0;
    return (trigger.sunset ? t.sunset : t.sunrise) + static_cast<std::int64_t>(trigger.offsetMinutes) * 60;
}

// ---- Conflicts -------------------------------------------------------------------------------------------------
std::size_t FindConflicts(std::span<const CalendarEvent> events, std::span<ConflictPair> out) {
    static thread_local std::vector<int> order;
    order.resize(events.size());
    for (std::size_t i = 0; i < events.size(); ++i)
        order[i] = static_cast<int>(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        const CalendarEvent& x = events[static_cast<std::size_t>(a)];
        const CalendarEvent& y = events[static_cast<std::size_t>(b)];
        return x.resource != y.resource ? x.resource < y.resource : x.start < y.start;
    });
    std::size_t count = 0;
    for (std::size_t i = 0; i < order.size(); ++i) {
        const CalendarEvent& a = events[static_cast<std::size_t>(order[i])];
        for (std::size_t j = i + 1; j < order.size(); ++j) {
            const CalendarEvent& b = events[static_cast<std::size_t>(order[j])];
            if (b.resource != a.resource || b.start >= a.end)
                break;
            if (b.end <= a.start)
                continue;
            if (count < out.size()) {
                ConflictPair& pair = out[count];
                pair.first = std::min(order[i], order[j]);
                pair.second = std::max(order[i], order[j]);
                const CalendarEvent& f = events[static_cast<std::size_t>(pair.first)];
                const CalendarEvent& s = events[static_cast<std::size_t>(pair.second)];
                pair.winner = f.priority == s.priority ? -1 : (f.priority > s.priority ? pair.first : pair.second);
            }
            ++count;
        }
    }
    return count;
}
} // namespace imkit::show
