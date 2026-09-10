#include <algorithm>
#include <cmath>
#include <format>
#include <set>
#include <stdexcept>

#include "config.hpp"
#include "time_table.hpp"
#include "utils/date_utils.hpp"

namespace {
    str escape_html(const str& value) {
        str escaped;
        for (const char c : value) {
            switch (c) {
                case '&': escaped += "&amp;"; break;
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                case '"': escaped += "&quot;"; break;
                case '\'': escaped += "&#39;"; break;
                default: escaped += c;
            }
        }
        return escaped;
    }

    str clock_label(day_time time) {
        return Date_Utils::daytime_to_str(time, time.count() % 60 == 0 ? "%H:%M" : "%H:%M:%S");
    }

    str weekday_label(date day) {
        const auto english = Date_Utils::date_to_str(day, "%A");
        for (const auto &[name, translated] : Config::LanguageConfig::weekday_name_mapping) {
            if (name == english) return translated;
        }
        return english;
    }
}

str TimeTable::to_personal_html_v2(const std::variant<Class, Room, Teacher> &featuring_object,
                                  date target_date, const str &person_name, int n_days,
                                  const std::vector<CalendarEvent> &external_events,
                                  double pixels_per_minute) const {
    if (!std::isfinite(pixels_per_minute) || pixels_per_minute < 0.5 || pixels_per_minute > 10) {
        throw std::invalid_argument("pixels_per_minute must be between 0.5 and 10");
    }
    std::vector<CalendarEvent> events;
    std::vector<str> colors;
    std::vector<str> tints;
    std::vector<str> statuses;
    std::set<std::tuple<str, str, str, datetime, datetime, str, bool, bool>> seen_lessons;
    for (const auto &period : periods) {
        const auto [code, changed] = period.get_period_code(featuring_object);
        // Shared lessons can arrive once per class. Ignore class membership and
        // period IDs, while keeping different times, teachers, rooms and statuses.
        const auto lesson_key = std::make_tuple(period.subjects_str(), period.teacher_str(false),
                                                period.room_str(false), period.start, period.end,
                                                code, changed.first, changed.second);
        if (!seen_lessons.insert(lesson_key).second) continue;
        const auto [title, row2, row3, start, end] = period.formatted_list(featuring_object, false);
        events.push_back({title, start, end, row2, row3});
        auto color = Config::TimeTableMappingConfig::default_subject_color;
        if (!period.subjects.empty()) {
            auto subject = period.subjects.front();
            color = subject.color();
        }
        const auto [r, g, b] = color;
        colors.push_back(std::format("rgb({},{},{})", std::clamp(r, 0, 255),
                                     std::clamp(g, 0, 255), std::clamp(b, 0, 255)));
        // A light tint keeps black text readable even for very dark subject colours.
        const auto tint = [](int channel) { return 224 + std::clamp(channel, 0, 255) * 31 / 255; };
        tints.push_back(std::format("rgb({},{},{})", tint(r), tint(g), tint(b)));
        statuses.push_back(code == "missed" ? " cancelled" : code == "extra" ? " extra" :
                           changed.first || changed.second ? " changed" : "");
    }
    for (const auto &event : external_events) {
        events.push_back(event);
        colors.emplace_back("#3875a9");
        tints.emplace_back("#e7eef5");
        statuses.emplace_back(" external");
    }
    const auto placements = Calendar::layout(events, target_date, n_days);
    day_time axis_start = std::chrono::hours{8};
    day_time axis_end = std::chrono::hours{19};
    if (!placements.empty()) {
        for (const auto &p : placements) {
            axis_start = std::min(axis_start, p.start);
            axis_end = std::max(axis_end, p.end);
        }
        axis_start = std::chrono::floor<std::chrono::hours>(axis_start);
        axis_end = std::chrono::ceil<std::chrono::hours>(axis_end);
    }
    std::vector<size_t> day_columns(n_days, 1);
    for (const auto &p : placements) day_columns[p.day_index] = std::max(day_columns[p.day_index], p.columns);
    const auto pixels = [pixels_per_minute](day_time time) { return time.count() / 60.0 * pixels_per_minute; };
    const double height = pixels(axis_end - axis_start);
    const str title = (Config::LanguageConfig::personal_timetable.empty() ? "Personal Timetable" :
                       Config::LanguageConfig::personal_timetable) + " V2 · " + person_name;
    str html = "<!doctype html><html><head><meta charset=\"utf-8\">"
               "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>" +
               escape_html(title) + R"(</title><style>
*{box-sizing:border-box}body{margin:0;background:#f3f5f8;color:#182537;font:14px/1.4 system-ui,sans-serif}
main{max-width:1500px;margin:auto;padding:24px}h1{font-size:22px;margin:0}nav{display:flex;gap:12px;align-items:center;flex-wrap:wrap;margin:14px 0 22px}
nav a{min-width:44px;min-height:44px;display:inline-flex;align-items:center;justify-content:center;background:white;border:1px solid #ccd4de;border-radius:6px;padding:7px 12px;color:#203c62;text-decoration:none}
.day-choice,.day-tab{display:none}
.calendar-scroll{overflow:auto;max-height:min(72vh,900px);max-height:min(72svh,900px);border:1px solid #d7dee7;border-radius:8px;isolation:isolate;scroll-padding-top:66px}
.calendar-scroll:focus-visible{outline:3px solid #3875a9;outline-offset:2px}.calendar-grid{display:grid;grid-template-columns:68px var(--days);min-width:100%;width:max-content;background:white;padding-bottom:12px}
.day-heading{position:sticky;top:0;z-index:4;background:#f6f8fc;min-height:62px;padding:12px 8px;text-align:center;font-weight:650;border-bottom:1px solid #d7dee7}.day-heading.today{background:#e5edff}.day-heading small{display:block;font-weight:400;color:#53647b}
.time-heading{left:0;z-index:6}.time-axis{position:sticky!important;left:0;z-index:3;background:white;border-right:1px solid #d7dee7}
.time-axis,.calendar-day{position:relative;height:var(--height)}.calendar-day{border-left:1px solid #d7dee7;isolation:isolate}
.tick{position:absolute;right:10px;transform:translateY(-50%);font-size:11px;color:#53647b;font-variant-numeric:tabular-nums}.tick:first-child{transform:none}.tick:last-child{transform:translateY(-100%)}
.grid-line{position:absolute;left:0;right:0;border-top:1px solid #e5eaf0;pointer-events:none}.grid-line.half{border-top:1px dotted #edf0f5}
.calendar-event{position:absolute;display:block;overflow:hidden;border-radius:4px;background:var(--event-tint);box-shadow:inset 5px 0 var(--event-color),inset 0 0 0 1px #9baabe;color:#182537;text-decoration:none}
.event-text{display:block;padding:5px 9px}.event-title,.event-detail{display:block;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.event-title{font-weight:650}.event-detail{font-size:12px;color:#46566d}
.calendar-event.compact .event-text{padding:0 6px;font-size:11px;line-height:16px}.calendar-event.compact .event-detail{display:none}.calendar-event.tiny .event-text{visibility:hidden}
.calendar-event.cancelled{background:#fff0f0;color:#a52626}.cancelled .event-title{text-decoration:line-through}.calendar-event.extra{background:#eaf7ee}.calendar-event.changed{background:#fff7df}
a:focus-visible,.calendar-event:focus-visible{outline:3px solid #132f62;outline-offset:2px;z-index:5}
@media screen and (max-width:700px){main{padding:12px}h1{font-size:19px}
.day-choice{display:block;position:absolute;opacity:0;width:1px;height:1px}
.day-tab{display:inline-flex;align-items:center;justify-content:center;min-height:44px;padding:8px 12px;margin:0 6px 10px 0;border:1px solid #bbc8d9;border-radius:6px;background:white;cursor:pointer}
.day-choice:checked+.day-tab{background:#203c62;color:white;border-color:#203c62}
.day-choice:focus-visible+.day-tab{outline:3px solid #3875a9;outline-offset:2px}
.calendar-grid{grid-template-columns:56px var(--mobile-days,var(--days))}.event-text{padding-left:8px}.event-title{font-size:12px}}

@media print{body{background:white}nav{display:none}main{padding:0;max-width:none}.calendar-scroll{overflow:visible;max-height:none}.day-heading,.time-axis{position:relative!important}.calendar-grid{print-color-adjust:exact;-webkit-print-color-adjust:exact}}
)";
    // CSS-only day selection: native radio controls also work without JavaScript.
    html += "@media screen and (max-width:700px){";
    for (int day = 0; day < n_days; ++day) {
        html += std::format(
            "#calendar-choice-{}:checked~.calendar-scroll .calendar-day:not([data-day='{}']),"
            "#calendar-choice-{}:checked~.calendar-scroll .day-heading:not(.time-heading):not([data-day='{}']){{display:none}}"
            "#calendar-choice-{}:checked~.calendar-scroll .calendar-grid{{--mobile-days:minmax({}px,1fr)}}",
            day, day, day, day, day, std::max(size_t{220}, day_columns[day] * (day_columns[day] <= 3 ? 84 : 132)));
    }
    html += "}</style></head><body><main>";
    html += "<h1>" + escape_html(title) + "</h1><nav>";
    const auto nav_link = [&](date day, const str &label) {
        return "<a href=\"?date=" + Date_Utils::date_to_str(day, "%d-%m-%Y") + "\">" + escape_html(label) + "</a>";
    };
    html += nav_link(Date_Utils::add_days(target_date, -n_days), "←");
    html += nav_link(Date_Utils::get_today(), Config::LanguageConfig::today.empty() ? "Today" : Config::LanguageConfig::today);
    html += nav_link(Date_Utils::add_days(target_date, n_days), "→");
    html += "</nav>";
    for (int day = 0; day < n_days; ++day) {
        const auto d = Date_Utils::add_days(target_date, day);
        const auto label = weekday_label(d) + " " + Date_Utils::date_to_str(d, "%d.%m.%Y");
        html += std::format("<input class=\"day-choice\" type=\"radio\" name=\"calendar-day\" id=\"calendar-choice-{}\"{}>"
                            "<label class=\"day-tab\" for=\"calendar-choice-{}\" title=\"{}\">{} {}</label>",
                            day, day == 0 ? " checked" : "", day, escape_html(label),
                            escape_html(weekday_label(d).substr(0, 2)), Date_Utils::date_to_str(d, "%d.%m."));
    }
    html += "<div class=\"calendar-scroll\" role=\"region\" tabindex=\"0\" aria-label=\"" + escape_html(title) +
            "\"><div class=\"calendar-grid\" data-calendar-view=\"v2\" data-axis-start=\"" + std::to_string(axis_start.count()) +
            "\" data-scale=\"" + std::format("{}", pixels_per_minute) + "\" style=\"--days:";
    for (int day = 0; day < n_days; ++day) {
        html += std::format("minmax({}px,1fr) ", std::max(size_t{240}, day_columns[day] * 150));
    }
    html += std::format(";--height:{:.3f}px\"><div class=\"day-heading time-heading\">{}</div>", height,
                        escape_html(Config::LanguageConfig::time));
    for (int day = 0; day < n_days; ++day) {
        const auto date = Date_Utils::add_days(target_date, day);
        html += std::format("<div class=\"day-heading{}\" data-day=\"{}\">{}<small>{}</small></div>",
                            date == Date_Utils::get_today() ? " today" : "", day, escape_html(weekday_label(date)),
                            Date_Utils::date_to_str(date, "%d.%m.%Y"));
    }
    html += "<div class=\"time-axis\">";
    for (auto time = axis_start; time <= axis_end; time += std::chrono::hours{1}) {
        html += std::format("<span class=\"tick\" style=\"top:{:.3f}px\">{}</span>",
                            pixels(time - axis_start), clock_label(time));
    }
    html += "</div>";
    for (int day = 0; day < n_days; ++day) {
        html += "<div class=\"calendar-day\" data-day=\"" + std::to_string(day) + "\" data-date=\"" +
                Date_Utils::date_to_str(Date_Utils::add_days(target_date, day), "%Y-%m-%d") + "\">";
        for (auto time = axis_start; time <= axis_end; time += std::chrono::minutes{30}) {
            html += std::format("<div class=\"grid-line{}\" style=\"top:{:.3f}px\"></div>",
                                time.count() % 3600 ? " half" : "", pixels(time - axis_start));
        }
        for (const auto &p : placements) {
            if (p.day_index != day) continue;
            const auto &event = events[p.event_index];
            const auto event_height = pixels(p.end - p.start);
            const auto times = clock_label(p.start) + "–" + clock_label(p.end);
            const auto text = times + " · " + event.title + " · " + event.location + " · " + event.description;
            html += std::format(
                "<div class=\"calendar-event{}{}\" tabindex=\"0\" role=\"group\" title=\"{}\" aria-label=\"{}\" "
                "data-event=\"{}\" data-start=\"{}\" data-end=\"{}\" data-column=\"{}\" data-columns=\"{}\" "
                "style=\"top:{:.3f}px;height:{:.3f}px;left:calc({:.6f}% + 3px);width:calc({:.6f}% - 6px);--event-color:{};--event-tint:{}\">"
                "<span class=\"event-text\"><span class=\"event-title\">{}</span>"
                "<span class=\"event-detail\">{}</span><span class=\"event-detail\">{}</span></span></div>",
                statuses[p.event_index], event_height < 18 ? " tiny" : event_height < 54 ? " compact" : "",
                escape_html(text), escape_html(text), p.event_index, p.start.count(), p.end.count(),
                p.column, p.columns, pixels(p.start - axis_start), event_height, 100.0 * p.column / p.columns,
                100.0 / p.columns, colors[p.event_index], tints[p.event_index], escape_html(event.title),
                escape_html(event.location), escape_html(event.description));
        }
        html += "</div>";
    }
    html += "</div></div></main></body></html>";
    return html;
}
