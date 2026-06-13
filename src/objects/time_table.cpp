#include "time_table.hpp"

#include <bitset>

#include "config.hpp"
#include "utils/all.hpp"

TimeTable::TimeTable(std::vector<Period> periods_) :
    periods(std::move(periods_)) {
}

TimeTable::~TimeTable() = default;

[[nodiscard]] TimeTable TimeTable::copy_by_date_range(const date start_date, const date end_date) const {
    std::vector<Period> new_periods;

    for (const auto &p: periods) {
        auto this_start = Date_Utils::datetime_to_date(p.start);
        auto this_end = Date_Utils::datetime_to_date(p.end);

        if (start_date <= this_start && this_start <= end_date && start_date <= this_end && this_end <= end_date) {
            new_periods.push_back(p);
        }
    }
    return TimeTable(std::move(new_periods));
}

/// Keep any period that stores that subject in the period (modify TimeTable in place)
void TimeTable::filter_hours_by_subject(const Subject &subject) {
    std::erase_if(periods, [&](const Period &p) {
        return !Vector_Utils::contains_value(p.subjects, subject);
    });
}

/// Keep any period that stores that class in the period (modify TimeTable in place)
void TimeTable::filter_hours_by_class(const Class &klasse) {
    std::erase_if(periods, [&](const Period &p) {
        return !Vector_Utils::contains_value(p.klassen, klasse);
    });
}

/// Keep any period that stores that room in the period (modify TimeTable in place)
void TimeTable::filter_hours_by_room(const Room &room) {
    std::erase_if(periods, [&](const Period &p) {
        return !Vector_Utils::contains_value(Vector_Utils::concat_ranges(p.rooms, p.original_rooms), room);
    });
}

/// Keep any period that stores that teacher in the period (modify TimeTable in place)
void TimeTable::filter_hours_by_teacher(const Teacher &teacher) {
    std::erase_if(periods, [&](const Period &p) {
        return !Vector_Utils::contains_value(Vector_Utils::concat_ranges(p.teachers, p.original_teachers), teacher);
    });
}

// Keep any period that personal attends (modify TimeTable in place)
void TimeTable::filter_hours_by_personal(const str &name) {
    std::unordered_set<str> personal_teachers;
    std::unordered_set<str> personal_subjects;

    const auto it = Config::TimeTableMappingConfig::personal_timetable_entries.find(name);

    if (it != Config::TimeTableMappingConfig::personal_timetable_entries.end()) {
        const auto &[teachers, subjects] = it->second;
        personal_teachers = teachers;
        personal_subjects = subjects;
    } else {
        personal_teachers = {};
        personal_subjects = {};
    }

    std::erase_if(periods, [&](const Period &p) {
        std::vector<str> teacher_names;
        teacher_names.reserve(p.teachers.size() + p.original_teachers.size());

        for (const auto &t: p.teachers)
            teacher_names.push_back(t.name);

        for (const auto &t: p.original_teachers)
            teacher_names.push_back(t.name);

        const bool teacher_match = std::ranges::any_of(
                teacher_names, [&](const str &t_name) {
                    return personal_teachers.contains(t_name);
                });

        const bool subject_match =
                std::ranges::any_of(p.subjects, [&](const Subject &s) {
                    return personal_subjects.contains(s.name);
                });

        return !(teacher_match && subject_match);
    });
}

str TimeTable::format_value(const float value, const bool percent, const bool val_int) {
    str base_string_whole_number = std::to_string(static_cast<int>(value));

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value;
    str base_string = oss.str();

    // Replace . with ,
    for (char &c: base_string) {
        if (c == '.')
            c = ',';
    }

    // Also adapt whole number (shouldn't contain '.', but consistent)
    for (char &c: base_string_whole_number) {
        if (c == '.')
            c = ',';
    }


    if (percent && val_int) {
        return base_string_whole_number + "%";
    }
    if (percent && !val_int) {
        return base_string + "%";
    }
    if (!percent && val_int) {
        return base_string_whole_number;
    }
    return base_string;
}

[[nodiscard]] std::tuple<int, int, int, int>
TimeTable::get_statistics(
        date start_date, date end_date, const std::variant<Class, Room, Teacher> &featuring_object,
        const std::vector<Class> &filtered_class_objects, bool filter_classes,
        const std::vector<Room> &filtered_room_objects, bool filter_rooms,
        const std::vector<Teacher> &filtered_teacher_objects, bool filter_teachers
        ) const {
    std::map<datetime, bool> raw_hours_taught;
    std::map<datetime, bool> raw_hours_missed;
    std::map<datetime, bool> raw_hours_extra;
    std::map<datetime, bool> raw_hours_special_cases;

    for (const auto &p: periods) {
        if (filter_classes) {
            bool found = false;

            for (const auto &k: p.klassen) {
                if (Vector_Utils::contains_value(filtered_class_objects, k)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                continue;
            }
        }

        if (filter_rooms) {
            bool found = false;

            for (const auto &r: Vector_Utils::concat_ranges(p.rooms, p.original_rooms)) {
                if (Vector_Utils::contains_value(filtered_room_objects, r)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                continue;
            }
        }

        if (filter_teachers) {
            bool found = false;

            for (const auto &t: Vector_Utils::concat_ranges(p.teachers, p.original_teachers)) {
                if (Vector_Utils::contains_value(filtered_teacher_objects, t)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                continue;
            }
        }

        if (Date_Utils::datetime_to_time(p.start) == std::chrono::minutes{0} ||
            Date_Utils::datetime_to_time(p.end) == std::chrono::hours{23} + std::chrono::minutes{59}) {
            raw_hours_special_cases[p.start] = true;
        } else {
            auto [lesson_code, rows_changed] = p.get_period_code(featuring_object);
            if (auto period_code = lesson_code; period_code == "regular") {
                raw_hours_taught[p.start] = true;
            } else if (period_code == "extra") {
                raw_hours_extra[p.start] = true;
            } else if (period_code == "missed") {
                raw_hours_missed[p.start] = true;
            }
        }
    }

    // After loop
    std::vector<date> dates_special_cases;
    for (const auto &[dt, value]: raw_hours_special_cases) {
        if (auto d = Date_Utils::datetime_to_date(dt); value && start_date <= d && d <= end_date) {
            dates_special_cases.push_back(d);
        }
    }

    int hours_special_cases = static_cast<int>(dates_special_cases.size());

    int hours_taught = 0;
    int hours_missed = 0;
    int hours_extra = 0;


    for (const auto &[dt, value]: raw_hours_taught) {
        if (auto d = Date_Utils::datetime_to_date(dt);
            value && start_date <= d && d <= end_date && !Vector_Utils::contains_value(dates_special_cases, d)) {
            ++hours_taught;
        }
    }

    for (const auto &[dt, value]: raw_hours_missed) {
        auto d = Date_Utils::datetime_to_date(dt);

        if (value && start_date <= d && d <= end_date && !Vector_Utils::contains_value(dates_special_cases, d)) {
            ++hours_missed;
        }
    }

    for (const auto &[dt, value]: raw_hours_extra) {
        auto d = Date_Utils::datetime_to_date(dt);

        if (value && start_date <= d && d <= end_date && !Vector_Utils::contains_value(dates_special_cases, d)) {
            ++hours_extra;
        }
    }

    return std::make_tuple(hours_special_cases, hours_taught, hours_missed, hours_extra);
}

[[nodiscard]] std::vector<str> TimeTable::get_separate_hours(
        date start_date, date end_date, const std::variant<Class, Room, Teacher> &featuring_object, int total_periods,
        const std::vector<Class> &filtered_class_objects, bool filter_classes,
        const std::vector<Room> &filtered_room_objects, bool filter_rooms,
        const std::vector<Teacher> &filtered_teacher_objects, bool filter_teachers, bool filter_unused_objects) const {
    auto [hours_special_cases, hours_taught, hours_missed, hours_extra] = get_statistics(
            start_date, end_date, featuring_object, filtered_class_objects, filter_classes,
            filtered_room_objects, filter_rooms, filtered_teacher_objects, filter_teachers
            );

    int hours_regular = hours_taught + hours_missed;

    str hours_taught_percent = "0,00%";
    str hours_taught_extra_percent = "0,00%";
    str hours_extra_per_regular = "0,00%";

    if (hours_regular > 0) {
        hours_taught_percent = format_value(
                static_cast<float>(hours_taught) / static_cast<float>(hours_regular) * 100.0f, true, false);
        hours_taught_extra_percent = format_value(static_cast<float>(hours_taught + hours_extra) /
                                                  static_cast<float>(hours_regular) * 100.0f,
                                                  true, false);
        hours_extra_per_regular =
                format_value(static_cast<float>(hours_extra) / static_cast<float>(hours_regular) * 100.0f, true, false);
    }

    if ((hours_taught + hours_missed + hours_extra + hours_special_cases + hours_regular) == 0) {
        if (filter_unused_objects) {
            return {};
        }
    }

    str taught_all_percent = "0,00%";
    if (total_periods > 0) {
        taught_all_percent = format_value(static_cast<float>(hours_taught + hours_extra) /
                                          static_cast<float>(total_periods) * 100.0f,
                                          true, false);
    }

    return {
            std::to_string(hours_taught), std::to_string(hours_missed), std::to_string(hours_extra),
            std::to_string(hours_special_cases), taught_all_percent, hours_taught_percent,
            hours_taught_extra_percent, hours_extra_per_regular
    };
}

std::tuple<str, str> TimeTable::get_table_name(const std::variant<Class, Room, Teacher> &featuring_object,
                                               const date start_date, const date end_date) {
    if (const Class *c_ptr = std::get_if<Class>(&featuring_object)) {
        return std::make_tuple(
                Config::LanguageConfig::class_timetable + " " + c_ptr->name,
                "(" + Date_Utils::date_to_str(start_date, "%d.%m.%Y") + " - " +
                Date_Utils::date_to_str(end_date, "%d.%m.%Y") + ")");
    }
    if (const Room *r_ptr = std::get_if<Room>(&featuring_object)) {
        return std::make_tuple(
                Config::LanguageConfig::room_timetable + " " + r_ptr->name,
                "(" + Date_Utils::date_to_str(start_date, "%d.%m.%Y") + " - " +
                Date_Utils::date_to_str(end_date, "%d.%m.%Y") + ")");
    }
    if (const Teacher *t_ptr = std::get_if<Teacher>(&featuring_object)) {
        return std::make_tuple(
                Config::LanguageConfig::teacher_timetable + " " + t_ptr->name + " (" + t_ptr->long_name +
                ")",
                "(" + Date_Utils::date_to_str(start_date, "%d.%m.%Y") + " - " +
                Date_Utils::date_to_str(end_date, "%d.%m.%Y") + ")");
    }
    return std::make_tuple("", "");
}

[[nodiscard]] std::vector<Period> TimeTable::unsorted_table() const {
    return periods;
}

[[nodiscard]] std::vector<std::pair<day_time, std::vector<std::pair<date, std::vector<Period> > > > >
TimeTable::to_table() const {
    if (periods.empty())
        return {};

    std::unordered_set<day_time, Date_Utils::Date_Hash> times;
    std::unordered_set<date, Date_Utils::Date_Hash> dates;
    std::unordered_set<datetime, Date_Utils::Date_Hash> date_times;
    for (const auto &p: periods) {
        auto a = Date_Utils::datetime_to_time(p.start);
        times.insert(a);
        dates.insert(Date_Utils::datetime_to_date(p.start));
        date_times.insert(p.start);
    }

    std::map<day_time, std::map<date, std::vector<Period> > > time_table;
    for (const auto &t: times) {
        for (const auto &d: dates) {
            time_table[t][d] = {};
        }
    }

    for (const auto &p: periods) {
        for (const auto &dt: date_times) {
            if (p.start <= dt && dt <= p.end) {
                time_table[Date_Utils::datetime_to_time(dt)][Date_Utils::datetime_to_date(dt)].push_back(p);
            }
        }
    }

    std::vector<std::pair<day_time, std::vector<std::pair<date, std::vector<Period> > > > > result;

    for (const auto &[t, date_map]: time_table) {
        std::vector<std::pair<date, std::vector<Period> > > date_vec(date_map.begin(), date_map.end());
        result.emplace_back(t, std::move(date_vec));
    }

    return result;
}

[[nodiscard]] std::vector<str> TimeTable::to_class_cancelled_hours() const {
    std::map<std::pair<datetime, datetime>, std::vector<std::vector<str> > > cancelled_hours_one_time;
    for (const auto &[time, row]: to_table()) {
        for (const auto &cell: row | std::views::values) {
            for (const auto &p: cell) {
                // TODO: The python has a try-catch here
                str long_subject_name = Str_Utils::join(", ", p.subjects, &Subject::long_name);
                str room_name = Str_Utils::join(", ", p.rooms, &Room::name);
                str original_room_name = Str_Utils::join(", ", p.original_rooms, &Room::name);
                str teachers = Str_Utils::join(", ", p.teachers, &Teacher::name);
                str original_teachers = Str_Utils::join(", ", p.original_teachers, &Teacher::name);

                if (teachers.empty()) {
                    teachers = "[" + Config::LanguageConfig::unknown_element_extended_text + "]";
                }

                if (original_teachers.empty()) {
                    original_teachers = teachers;
                }

                if (long_subject_name.empty()) {
                    long_subject_name = Config::LanguageConfig::some_hour;
                }

                if (room_name.empty()) {
                    room_name = Config::LanguageConfig::unknown_element_extended_text;
                }

                if (original_room_name.empty()) {
                    original_room_name = room_name;
                }

                std::pair<datetime, datetime> times = std::make_pair(p.start, p.end);
                auto [p_info, p_code] = std::make_pair(
                        std::format("{} ({}, {})", long_subject_name, room_name, teachers), p.raw_period_code);

                str cancelled_info = std::format(
                        "{} ({}, {}) {}: {}", long_subject_name, room_name, teachers,
                        Config::LanguageConfig::is_cancelled,
                        Date_Utils::datetime_to_str(p.start, "%Y-%m-%d %H:%M:%S"));
                str irregular_info = std::format(
                        "{} ({}, {}) -> {} ({}, {}): {}", long_subject_name,
                        original_room_name, original_teachers, long_subject_name, room_name,
                        teachers, Date_Utils::datetime_to_str(p.start, "%Y-%m-%d %H:%M:%S"));

                auto add_info = [&](const str &info) {
                    const std::vector<str> entry = {p_info, p_code.value_or(""), info};
                    cancelled_hours_one_time[times].push_back(entry);
                };

                if (auto value = p.raw_period_code;
                    value.has_value() && (*value == "cancelled" || *value == "irregular")) {
                    add_info(cancelled_info);
                } else if (original_teachers != teachers || original_room_name != room_name) {
                    add_info(irregular_info);
                }
            }
        }
    }

    std::vector<str> special_hours;

    for (const auto &dates: cancelled_hours_one_time | std::views::keys) {
        str lesson_1_string = cancelled_hours_one_time[dates][0][0];
        str lesson_1_code = cancelled_hours_one_time[dates][0][1];
        str lesson_1_special_string = cancelled_hours_one_time[dates][0][2];
        str lesson_2_string;
        str lesson_2_code;

        if (cancelled_hours_one_time[dates].size() > 1) {
            lesson_2_string = cancelled_hours_one_time[dates][1][0];
            lesson_2_code = cancelled_hours_one_time[dates][1][1];
        }

        if (cancelled_hours_one_time[dates].size() == 1) {
            // 1 Hour / date
            if (lesson_1_code == "irregular") {
                special_hours.push_back(
                        std::format("{} {}: {}", lesson_1_string, Config::LanguageConfig::is_irregular,
                                    Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S")));
            } else {
                special_hours.push_back(lesson_1_special_string);
            }
        } else if (cancelled_hours_one_time[dates].size() == 2) {
            // 2 Hours / date
            if (lesson_1_code == "cancelled" && lesson_2_code == "irregular") {
                special_hours.push_back(
                        std::format("{} {}: {}: {}", Config::LanguageConfig::instead, lesson_1_string,
                                    lesson_2_string,
                                    Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S")));
            } else if (lesson_2_code == "cancelled" && lesson_1_code == "irregular") {
                special_hours.push_back(
                        std::format("{} {}: {}: {}", Config::LanguageConfig::instead, lesson_2_string,
                                    lesson_1_string,
                                    Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S")));
            } else if (lesson_1_code == "cancelled" && lesson_2_code == "cancelled") {
                special_hours.push_back(
                        std::format("{} & {} {}: {}", lesson_1_string, lesson_2_string,
                                    Config::LanguageConfig::are_cancelled,
                                    Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S")));
            } else {
                special_hours.push_back(
                        std::format("{} & {} {}: {}", lesson_1_string, lesson_2_string,
                                    Config::LanguageConfig::are_irregular,
                                    Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S")));
            }
        } else {
            // > 2 Hours / date
            bool all_cancelled =
                    std::ranges::all_of(cancelled_hours_one_time[dates],
                                        [](const std::vector<str> &entry) {
                                            return entry[1] != "irregular";
                                        });

            if (all_cancelled) {
                special_hours.push_back(std::format("{}: {}", Config::LanguageConfig::multiple_lessons_cancelled,
                                                    Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S")));
            } else {
                special_hours.push_back(std::format("{}: {}", Config::LanguageConfig::multiple_lessons_irregular,
                                                    Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S")));
            }
        }
    }

    return special_hours;
}

[[nodiscard]] std::tuple<std::vector<str>, std::vector<str>, std::map<str, std::map<str, std::vector<Period> > > >
TimeTable::html_setup(const int user_id, const bool website, const std::tuple<str, str> &table_name,
                      const std::optional<date> &start_date, const std::optional<date> &end_date) const {
    std::map<str, std::map<str, std::vector<Period> > > final_hours;

    for (const auto &weekday: Config::LanguageConfig::weekday_name_mapping | std::views::keys) {
        if (weekday == "Saturday" || weekday == "Sunday") {
            continue;
        }

        auto it = std::ranges::find(Config::LanguageConfig::weekday_name_mapping, weekday, &std::pair<str, str>::first);
        const str &mapped_weekday = it->second;

        for (const auto &p: periods) {
            if (Date_Utils::datetime_to_str(p.start, "%A") != weekday) {
                continue;
            }

            // Actual start & end time, ex. 08:40 & 09:35, or irregular times: 00:00 & 23:59 (in one lesson)
            day_time period_start_time = Date_Utils::datetime_to_time(p.start);
            day_time period_end_time = Date_Utils::datetime_to_time(p.end);

            for (const auto &time_range: Config::HTMLStyleConfig::lesson_time_ranges) {
                const auto sep = time_range.find(" - ");
                const day_time i_start_time = Date_Utils::str_to_daytime(
                        time_range.substr(0, sep), Config::HTMLStyleConfig::lesson_time_ranges_format);
                const day_time i_end_time = Date_Utils::str_to_daytime(
                        time_range.substr(sep + 3), Config::HTMLStyleConfig::lesson_time_ranges_format);

                if (period_start_time <= i_start_time && i_start_time <= period_end_time) {
                    if (period_start_time <= i_end_time && i_end_time <= period_end_time) {
                        const str time_key =
                                std::format("{} - {}",
                                            Date_Utils::daytime_to_str(
                                                    i_start_time, Config::HTMLStyleConfig::lesson_time_ranges_format),
                                            Date_Utils::daytime_to_str(
                                                    i_end_time, Config::HTMLStyleConfig::lesson_time_ranges_format));

                        final_hours[mapped_weekday][time_key].push_back(p);
                    }
                }
            }
        }
    }

    std::vector<str> weekdays;

    for (const auto &weekday: Config::LanguageConfig::weekday_name_mapping | std::views::keys) {
        if (weekday == "Saturday" || weekday == "Sunday") {
            continue;
        }

        auto it = std::ranges::find(Config::LanguageConfig::weekday_name_mapping, weekday, &std::pair<str, str>::first);
        weekdays.push_back(it->second);
    }

    auto bits4_to_signed_int = [](const str &bits) -> int {
        int n = std::stoi(bits, nullptr, 2);
        if (bits[0] == '1')
            n -= (1 << 4);
        return n;
    };

    // RGB changes:
    // Interpret as 4 bit signed int [-8; +7]
    // For better reading with rgb colour picker, *2 -> [-16; +14]

    str water_mark_bits = std::bitset<64>(user_id).to_string();
    std::tuple<int, int, int> base_rgb_value = Config::HTMLStyleConfig::table_header_base_rgb;
    std::vector<std::tuple<int, int, int> > water_mark_rgb_value;

    // Handle the first 5 full RGB triplets
    for (int num_header = 0; num_header < 5; ++num_header) {
        int offset = num_header * 12;
        int rgb_red = bits4_to_signed_int(water_mark_bits.substr(offset, 4)) * 2 + std::get<0>(base_rgb_value);
        int rgb_green = bits4_to_signed_int(water_mark_bits.substr(offset + 4, 4)) * 2 + std::get<1>(base_rgb_value);
        int rgb_blue = bits4_to_signed_int(water_mark_bits.substr(offset + 8, 4)) * 2 + std::get<2>(base_rgb_value);
        water_mark_rgb_value.emplace_back(rgb_red, rgb_green, rgb_blue);
    }

    int rgb_red = bits4_to_signed_int(water_mark_bits.substr(60, 4)) * 2 + std::get<0>(base_rgb_value);
    water_mark_rgb_value.emplace_back(rgb_red, std::get<1>(base_rgb_value), std::get<2>(base_rgb_value));

    std::vector<str> html;

    if (website && start_date.has_value() && end_date.has_value()) {
        html = {
                Config::HTMLStyleConfig::timetable_html_header,
                "<p>",
                std::format("<a href=\"?date=0\"><button>{}</button></a>", Config::LanguageConfig::back),
                "<br>",
                std::format(
                        "<a href=\"?date={}\"><button>{}</button></a>",
                        Date_Utils::date_to_str(Date_Utils::add_weeks(start_date.value(), -1), "%d-%m-%Y"),
                        Config::LanguageConfig::last_week),
                std::format("{} {}", std::get<0>(table_name), std::get<1>(table_name)),
                std::format(
                        "<a href=\"?date={}\"><button>{}</button></a>",
                        Date_Utils::date_to_str(Date_Utils::add_weeks(start_date.value(), 1), "%d-%m-%Y"),
                        Config::LanguageConfig::next_week),
                "</p>",
                R"(<table border="1" cellspacing="0" cellpadding="5">)",
                std::format(
                        "<tr><th style=\"background-color: rgb({},{},{});\">{}</th>",
                        std::get<0>(Config::HTMLStyleConfig::table_header_base_rgb),
                        std::get<1>(Config::HTMLStyleConfig::table_header_base_rgb),
                        std::get<2>(Config::HTMLStyleConfig::table_header_base_rgb), Config::LanguageConfig::time)
        };

        for (const auto &day: weekdays) {
            html.push_back(std::format("<th style=\"background-color: rgb({}, {}, {});\">{}</th>",
                                       std::get<0>(Config::HTMLStyleConfig::table_header_base_rgb),
                                       std::get<1>(Config::HTMLStyleConfig::table_header_base_rgb),
                                       std::get<2>(Config::HTMLStyleConfig::table_header_base_rgb), day.substr(0, 2)));
        }
        html.emplace_back("</tr>");
    } else {
        html = {
                Config::HTMLStyleConfig::timetable_html_header,
                std::format("<p>{} {}</p>", std::get<0>(table_name), std::get<1>(table_name)),
                R"(<table border="1" cellspacing="0" cellpadding="5">)",
                std::format("<tr><th style=\"background-color: rgb({}, {}, {});\">{}</th>",
                            std::get<0>(Config::HTMLStyleConfig::table_header_base_rgb),
                            std::get<1>(Config::HTMLStyleConfig::table_header_base_rgb),
                            std::get<2>(Config::HTMLStyleConfig::table_header_base_rgb), Config::LanguageConfig::time)
        };

        for (int count = 0; count < weekdays.size(); ++count) {
            const auto &day = weekdays[count];
            html.push_back(std::format("<th style=\"background-color: rgb({}, {}, {});\">{}</th>",
                                       std::get<0>(water_mark_rgb_value.at(count + 1)),
                                       std::get<1>(water_mark_rgb_value.at(count + 1)),
                                       std::get<2>(water_mark_rgb_value.at(count + 1)), day.substr(0, 2)));
        }
        html.emplace_back("</tr>");
    }

    return std::make_tuple(html, weekdays, final_hours);
}

bool TimeTable::html_line_too_long(
        const std::vector<std::tuple<str, str, str, datetime, datetime> > &distinct_lessons_list_formatted) {
    unsigned long max_html_chars_per_row = 10;

    std::vector<unsigned long> chars_rows = {0, 0, 0};

    // Maximum number of characters per row (3 total)
    for (const auto &lesson: distinct_lessons_list_formatted) {
        chars_rows.at(0) += std::get<0>(lesson).length();
        chars_rows[1] += std::get<1>(lesson).length() + 2; // Brackets ()
        chars_rows[2] += std::get<2>(lesson).length() + 2; // Brackets []
    }

    // Do not use num lessons; len('NWP [PAM] (RBU1)') > len('E [SU] [R6B]')

    return std::ranges::any_of(chars_rows, [max_html_chars_per_row](const unsigned long num_chars) {
        return num_chars > max_html_chars_per_row;
    });
}

void TimeTable::html_add_lesson_time_range(std::vector<str> &html, const int lesson_count_index,
                                           const str &lesson_time_range) {
    const auto sep = lesson_time_range.find(" - ");
    const str start = lesson_time_range.substr(0, sep);
    const str end = lesson_time_range.substr(sep + 3);


    html.emplace_back(std::format(R"(
        <tr>
        <td style="padding: 0; margin: 0;">
            <div style="display: flex; align-items: center;">
                <!-- Big number spanning two lines -->
                <div style="font-size: 18pt; font-weight: bold; padding-right: 0pt; width: 30pt;">
                    {}
                </div>
                <!-- Time block -->
                <div style="line-height: 9pt;">
                    <div style="margin-bottom: 2pt;">{}</div>
                    <div>{}</div>
                </div>
            </div>
        </td>)",
                                  lesson_count_index + 1, start, end));
}

[[nodiscard]] str TimeTable::to_html(const std::variant<Class, Room, Teacher> &featuring_object, const int user_id,
                                     const bool website, const std::tuple<str, str> &table_name,
                                     const std::optional<date> start_date, const std::optional<date> end_date) const {
    auto [html, weekdays, final_hours] = html_setup(user_id, website, table_name, start_date, end_date);

    for (int count = 0; count < Config::HTMLStyleConfig::lesson_time_ranges.size(); ++count) {
        const auto &time_range = Config::HTMLStyleConfig::lesson_time_ranges[count];
        html_add_lesson_time_range(html, count, time_range);

        for (const auto &day: weekdays) {
            std::vector<Period> lessons;
            if (final_hours.contains(day) && final_hours.at(day).contains(time_range)) {
                lessons = final_hours.at(day).at(time_range);
            }

            if (lessons.empty()) {
                html.emplace_back("<td></td>");
                continue;
            }

            std::vector<std::tuple<str, str, str, datetime, datetime> > distinct_lessons_list_formatted;

            for (const auto &lesson: lessons) {
                auto list_formatted = lesson.formatted_list(featuring_object, false);
                if (!Vector_Utils::contains_value(distinct_lessons_list_formatted, list_formatted)) {
                    distinct_lessons_list_formatted.push_back(std::move(list_formatted));
                }
            }

            std::vector<str> formatted_lessons;
            std::unordered_set<str> seen_lesson_strings;
            unsigned long total_character = 0;

            for (const auto &lesson: lessons) {
                auto [lesson_code, rows_changed] = lesson.get_period_code(featuring_object);

                auto list_formatted = lesson.formatted_list(featuring_object, false);
                auto string_formatted = lesson.formatted_string(featuring_object, false);
                str text_color;

                if (lesson_code == "missed") {
                    text_color = "color: #D32F2F;";
                } else if (lesson_code == "extra") {
                    text_color = "color: #2E7D32;";
                }

                str short_subject_name_text_color = text_color;

                if (rows_changed.first || rows_changed.second) {
                    if (text_color.empty()) {
                        short_subject_name_text_color = "color: #F9A825;";
                    }
                }

                str formatted_lesson = std::format(
                        "<span>{}</span><br>"
                        "<span{}>[{}]</span><br>"
                        "<span{}>({})</span>",
                        std::get<0>(list_formatted),
                        (rows_changed.first && lesson_code == "regular") ? " style=\"color: #F9A825;\"" : "",
                        std::get<1>(list_formatted),
                        (rows_changed.second && lesson_code == "regular") ? " style=\"color: #F9A825;\"" : "",
                        std::get<2>(list_formatted));

                // Render lesson
                if (lessons.size() == 1) {
                    formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:5px; "
                                                            "vertical-align: top; margin-left:5px; {}\">{}</span>",
                                                            text_color, formatted_lesson));
                } else {
                    if (seen_lesson_strings.contains(string_formatted)) {
                        continue;
                    }
                    seen_lesson_strings.insert(string_formatted);

                    if (website) {
                        if (distinct_lessons_list_formatted.size() < 5) {
                            formatted_lessons.push_back(
                                    std::format("<span style=\"display:inline-block; margin-right:1px; vertical-align: "
                                                "top; margin-left:1px; {}\">{}</span>",
                                                text_color, formatted_lesson));
                        } else {
                            formatted_lessons.push_back(
                                    std::format("<span style=\"display:inline-block; margin-right:5px; vertical-align: "
                                                "top; margin-left:5px; {}\">{}</span>",
                                                text_color, std::get<0>(list_formatted)));
                        }
                    } else {
                        if (!html_line_too_long(distinct_lessons_list_formatted)) {
                            formatted_lessons.push_back(
                                    std::format("<span style=\"display:inline-block; margin-right:1px; vertical-align: "
                                                "top; margin-left:1px; {}\">{}</span>",
                                                text_color, formatted_lesson));
                        } else {
                            // Display only subject name
                            // Make sure they don't go out of border
                            int character_limit_before_line_break = 8;

                            total_character += std::get<0>(list_formatted).length() + 1; // Space
                            unsigned long remainder = total_character % character_limit_before_line_break;

                            // If after those characters, it would be too long -> <br>
                            if (total_character >= character_limit_before_line_break && remainder) {
                                formatted_lessons.emplace_back("<br>");

                                // TODO: Sure?
                                total_character = 0;
                            }

                            formatted_lessons.push_back(
                                    std::format("<span style=\"display:inline-block; margin-right:5px; vertical-align: "
                                                "top; margin-left:5px; {}\">{}</span>",
                                                short_subject_name_text_color, std::get<0>(list_formatted)));
                        }
                    }
                }
            }

            // Stripe colour (based on first eligible subject / fallback to first subject)
            std::tuple<int, int, int> rgba_value = Config::TimeTableMappingConfig::default_subject_color;

            // Collect eligible subjects (None or irregular)
            std::vector<Subject> eligible_subjects;

            for (const auto &lesson: lessons) {
                str p_code = lesson.get_period_code(featuring_object).first;
                if (p_code != "regular" && p_code != "extra") {
                    continue;
                }
                if (lesson.subjects.empty()) {
                    continue;
                }
                Subject su = lesson.subjects.at(0);
                eligible_subjects.push_back(su);
            }

            std::optional<Subject> chosen_subject;

            if (!eligible_subjects.empty()) {
                std::ranges::sort(eligible_subjects, {}, &Subject::name);
                chosen_subject = eligible_subjects.at(0);
            } else {
                if (!lessons.empty() && !lessons.at(0).subjects.empty()) {
                    chosen_subject = lessons.at(0).subjects.at(0);
                }
            }

            if (chosen_subject.has_value()) {
                rgba_value = chosen_subject->color();
            }

            html.push_back(std::format("<td style=\"--stripe-color: rgba({},{},{}); white-space: nowrap;\">{}</td>",
                                       std::get<0>(rgba_value), std::get<1>(rgba_value), std::get<2>(rgba_value),
                                       Str_Utils::join(formatted_lessons)));
        }
        html.emplace_back("</tr>");
    }

    html.emplace_back("</table>");

    html.emplace_back(Config::HTMLStyleConfig::timetable_html_footer);

    str html_content = Str_Utils::join("\n", html);

    return html_content;
}

[[nodiscard]] str TimeTable::to_untis_html(const std::variant<Class, Room, Teacher> &featuring_object,
                                           const int user_id, const std::tuple<str, str> &table_name,
                                           const date start_date, const date end_date) const {
    return to_html(featuring_object, user_id, false, table_name, start_date, end_date);
}

[[nodiscard]] str TimeTable::to_website_html(const std::variant<Class, Room, Teacher> &featuring_object,
                                             const date start_date, const date end_date) const {
    const std::tuple<str, str> table_name = get_table_name(featuring_object, start_date, end_date);
    return to_html(featuring_object, 0, true, table_name, start_date, end_date);
}

[[nodiscard]] str TimeTable::to_personal_html(const std::variant<Class, Room, Teacher> &featuring_object,
                                              date target_date, const str &person_name, const int n_days) const {
    // str english_weekday = Date_Utils::date_to_str(target_date, "%A");
    std::vector<str> english_weekdays;
    std::vector<str> native_weekdays;

    int bounded_n_days = n_days > 7 ? 7 : n_days;

    english_weekdays.reserve(bounded_n_days);
    native_weekdays.reserve(bounded_n_days);

    for (int i = 0; i < bounded_n_days; ++i) {
        str english_weekday = Date_Utils::date_to_str(Date_Utils::add_days(target_date, i), "%A");
        english_weekdays.push_back(english_weekday);

        auto it = std::ranges::find(Config::LanguageConfig::weekday_name_mapping, english_weekday,
                                    &std::pair<str, str>::first);
        const str &native_weekday = it->second;
        native_weekdays.push_back(native_weekday);
    }

    std::map<str, std::map<str, std::vector<Period> > > final_hours;
    for (const auto &native_weekday: native_weekdays) {
        final_hours[native_weekday] = {};
    }

    for (const auto &period: periods) {
        if (!Vector_Utils::contains_value(english_weekdays, Date_Utils::datetime_to_str(period.start, "%A"))) {
            continue;
        }

        const str &english_weekday = Date_Utils::datetime_to_str(period.start, "%A");
        auto it = std::ranges::find(Config::LanguageConfig::weekday_name_mapping, english_weekday,
                                    &std::pair<str, str>::first);
        const str &native_weekday = it->second;

        // Actual start & end time, ex. 08:40 & 09:35, or irregular times: 00:00 & 23:59 (in one lesson)
        day_time period_start_time = Date_Utils::datetime_to_time(period.start);
        day_time period_end_time = Date_Utils::datetime_to_time(period.end);

        for (const auto &time_range: Config::HTMLStyleConfig::lesson_time_ranges) {
            const auto sep = time_range.find(" - ");
            const day_time i_start_time = Date_Utils::str_to_daytime(
                    time_range.substr(0, sep), Config::HTMLStyleConfig::lesson_time_ranges_format);
            const day_time i_end_time = Date_Utils::str_to_daytime(time_range.substr(sep + 3),
                                                                   Config::HTMLStyleConfig::lesson_time_ranges_format);

            if (period_start_time <= i_start_time && i_start_time <= period_end_time) {
                if (period_start_time <= i_end_time && i_end_time <= period_end_time) {
                    const str time_key = std::format(
                            "{} - {}",
                            Date_Utils::daytime_to_str(i_start_time,
                                                       Config::HTMLStyleConfig::lesson_time_ranges_format),
                            Date_Utils::daytime_to_str(i_end_time, Config::HTMLStyleConfig::lesson_time_ranges_format));

                    final_hours[native_weekday][time_key].push_back(period);
                }
            }
        }
    }

    std::tuple<int, int, int> rgb_value = Config::HTMLStyleConfig::table_header_base_rgb;

    if (target_date == Date_Utils::get_today()) {
        rgb_value = Config::HTMLStyleConfig::today_personal_rgb_value;
    }

    std::vector<str> html = {
            Config::HTMLStyleConfig::personal_timetable_html_header,
            "<p>",

            std::format("<a href=\"?date={}\"><button>{}</button></a>",
                        Date_Utils::date_to_str(Date_Utils::add_days(target_date, -1), "%d-%m-%Y"),
                        Config::LanguageConfig::yesterday),

            std::format("{} {} ({} {})", Config::LanguageConfig::personal_timetable, person_name,
                        std::ranges::find(Config::LanguageConfig::weekday_name_mapping,
                                          Date_Utils::date_to_str(target_date, "%A"), &std::pair<str, str>::first)
                        ->second.substr(0, 2),
                        Date_Utils::date_to_str(target_date, "%d.%m.%Y")),

            std::format("<a href=\"?date={}\"><button>{}</button></a>",
                        Date_Utils::date_to_str(Date_Utils::add_days(target_date, 1), "%d-%m-%Y"),
                        Config::LanguageConfig::tomorrow),

            "</p>",
            "<p>",

            std::format("<a href=\"?date={}\"><button>{}</button></a>",
                        Date_Utils::date_to_str(Date_Utils::get_today(), "%d-%m-%Y"), Config::LanguageConfig::today),

            "</p>",
            R"(<table border="1" cellspacing="0" cellpadding="5">)",

            std::format("<tr><th style=\"background-color: rgb({},{},{});\">{}</th>",
                        std::get<0>(Config::HTMLStyleConfig::table_header_base_rgb),
                        std::get<1>(Config::HTMLStyleConfig::table_header_base_rgb),
                        std::get<2>(Config::HTMLStyleConfig::table_header_base_rgb), Config::LanguageConfig::time)
    };

    for (const auto &native_weekday: native_weekdays) {
        html.push_back(std::format("<th style=\"background-color: rgb({},{},{});\">{}</th>", std::get<0>(rgb_value),
                                   std::get<1>(rgb_value), std::get<2>(rgb_value), native_weekday.substr(0, 2)));
    }
    html.emplace_back("<tr>");

    for (int count = 0; count < Config::HTMLStyleConfig::lesson_time_ranges.size(); ++count) {
        const auto &time_range = Config::HTMLStyleConfig::lesson_time_ranges[count];
        html_add_lesson_time_range(html, count, time_range);
        for (const auto &native_weekday: native_weekdays) {
            std::vector<Period> lessons;
            if (final_hours.contains(native_weekday) && final_hours.at(native_weekday).contains(time_range)) {
                lessons = final_hours.at(native_weekday).at(time_range);
            }

            if (lessons.empty()) {
                html.emplace_back("<td></td>");
                continue;
            }

            std::vector<std::tuple<str, str, str, datetime, datetime> > distinct_lessons_list_formatted;

            for (const auto &lesson: lessons) {
                auto list_formatted = lesson.formatted_list(featuring_object, false);
                if (!Vector_Utils::contains_value(distinct_lessons_list_formatted, list_formatted)) {
                    distinct_lessons_list_formatted.push_back(std::move(list_formatted));
                }
            }

            std::vector<str> formatted_lessons;
            std::unordered_set<str> seen_lesson_strings;

            for (const auto &lesson: lessons) {
                auto [lesson_code, rows_changed] = lesson.get_period_code(featuring_object);

                auto list_formatted = lesson.formatted_list(featuring_object, false);
                auto string_formatted = lesson.formatted_string(featuring_object, false);
                str text_color;

                if (lesson_code == "missed") {
                    text_color = "color: #D32F2F;";
                } else if (lesson_code == "extra") {
                    text_color = "color: #2E7D32;";
                }

                str short_subject_name_text_color = text_color;

                if (rows_changed.first || rows_changed.second) {
                    if (text_color.empty()) {
                        short_subject_name_text_color = "color: #F9A825;";
                    }
                }

                str formatted_lesson = std::format(
                        "<span>{}</span><br>"
                        "<span{}>[{}]</span><br>"
                        "<span{}>({})</span>",
                        std::get<0>(list_formatted),
                        (rows_changed.first && lesson_code == "regular") ? " style=\"color: #F9A825;\"" : "",
                        std::get<1>(list_formatted),
                        (rows_changed.second && lesson_code == "regular") ? " style=\"color: #F9A825;\"" : "",
                        std::get<2>(list_formatted));

                if (lessons.size() == 1) {
                    formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:5px; "
                                                            "vertical-align: top; margin-left:5px; {}\">{}</span>",
                                                            text_color, formatted_lesson));
                } else {
                    if (seen_lesson_strings.contains(string_formatted)) {
                        continue;
                    }
                    seen_lesson_strings.insert(string_formatted);

                    if (distinct_lessons_list_formatted.size() < 5) {
                        formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:1px; "
                                                                "vertical-align: top; margin-left:1px; {}\">{}</span>",
                                                                text_color, formatted_lesson));
                    } else {
                        formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:5px; "
                                                                "vertical-align: top; margin-left:5px; {}\">{}</span>",
                                                                short_subject_name_text_color,
                                                                std::get<0>(list_formatted)));
                    }
                }
            }

            // Stripe colour (based on first eligible subject / fallback to first subject)
            std::tuple<int, int, int> rgba_value = Config::TimeTableMappingConfig::default_subject_color;

            // Collect eligible subjects (None or irregular)
            std::vector<Subject> eligible_subjects;

            for (const auto &lesson: lessons) {
                str p_code = lesson.get_period_code(featuring_object).first;
                if (p_code != "regular" && p_code != "extra") {
                    continue;
                }
                if (lesson.subjects.empty()) {
                    continue;
                }
                Subject su = lesson.subjects.at(0);
                eligible_subjects.push_back(su);
            }

            std::optional<Subject> chosen_subject;

            if (!eligible_subjects.empty()) {
                std::ranges::sort(eligible_subjects, {}, &Subject::name);
                chosen_subject = eligible_subjects.at(0);
            } else {
                if (!lessons.empty() && !lessons.at(0).subjects.empty()) {
                    chosen_subject = lessons.at(0).subjects.at(0);
                }
            }

            if (chosen_subject.has_value()) {
                rgba_value = chosen_subject->color();
            }

            html.push_back(std::format("<td style=\"--stripe-color: rgba({},{},{}); white-space: nowrap;\">{}</td>",
                                       std::get<0>(rgba_value), std::get<1>(rgba_value), std::get<2>(rgba_value),
                                       Str_Utils::join(formatted_lessons)));
        }
        html.emplace_back("</tr>");
    }

    html.emplace_back("</table>");

    html.emplace_back(Config::HTMLStyleConfig::timetable_html_footer);

    str html_content = Str_Utils::join("\n", html);

    return html_content;
}

[[nodiscard]] str TimeTable::to_regular_html(const std::variant<Class, Room, Teacher> &featuring_object, int user_id,
                                             const std::tuple<str, str> &table_name) const {
    auto [html, weekdays, final_hours] = html_setup(user_id, false, table_name, std::nullopt, std::nullopt);

    bool any_two_week_lesson = false;

    for (int count = 0; count < Config::HTMLStyleConfig::lesson_time_ranges.size(); ++count) {
        const auto &time_range = Config::HTMLStyleConfig::lesson_time_ranges[count];
        html_add_lesson_time_range(html, count, time_range);

        for (const auto &day: weekdays) {
            std::vector<Period> raw_lessons;
            if (final_hours.contains(day) && final_hours.at(day).contains(time_range)) {
                raw_lessons = final_hours.at(day).at(time_range);
            }

            std::vector<Period> filtered_lessons;
            for (const auto &lesson: raw_lessons) {
                if (lesson.get_period_code(featuring_object).first != "extra") {
                    filtered_lessons.push_back(lesson);
                }
            }

            if (filtered_lessons.empty()) {
                html.emplace_back("<td></td>");
                continue;
            }

            std::vector<std::tuple<str, str, str, datetime, datetime> > distinct_lessons_list_formatted;
            std::unordered_set<str> distinct_lessons_string;

            for (const auto &lesson: filtered_lessons) {
                auto list_formatted = lesson.formatted_list(featuring_object, false);

                str formatted_lesson = std::format("<span>{}</span><br><span>[{}]</span><br><span>({})</span>",
                                                   std::get<0>(list_formatted), std::get<1>(list_formatted),
                                                   std::get<2>(list_formatted));

                if (!distinct_lessons_string.contains(formatted_lesson)) {
                    distinct_lessons_list_formatted.push_back(std::move(list_formatted));
                    distinct_lessons_string.insert(std::move(formatted_lesson));
                }
            }

            std::vector<str> formatted_lessons;
            std::unordered_set<str> seen_formatted_lessons;
            unsigned long total_character = 0;

            for (const auto &lesson: filtered_lessons) {
                auto [lesson_code, _] = lesson.get_period_code(featuring_object);

                auto list_formatted = lesson.formatted_list(featuring_object, false);

                if (lesson_code == "extra") {
                    continue;
                }

                str formatted_lesson = std::format("<span>{}</span><br><span>[{}]</span><br><span>({})</span>",
                                                   std::get<0>(list_formatted), std::get<1>(list_formatted),
                                                   std::get<2>(list_formatted));

                str this_lesson_bi_weekly;

                // Bi-weekly check
                if (count_appearances(lesson) == 1) {
                    this_lesson_bi_weekly = std::format("{}<br>", Config::LanguageConfig::two_week_abbreviation);
                    any_two_week_lesson = true;
                }

                // Render lesson
                if (filtered_lessons.size() == 1) {
                    // 1 Lesson / block (-> bi-weekly, but irrelevant)
                    formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:5px; "
                                                            "vertical-align: top; margin-left:5px;\">{}{}</span>",
                                                            this_lesson_bi_weekly, formatted_lesson));
                } else {
                    // Could still contain bi-weekly lessons; If multiple lessons are in the same block

                    if (seen_formatted_lessons.contains(formatted_lesson)) {
                        continue;
                    }
                    seen_formatted_lessons.insert(formatted_lesson);

                    if (!html_line_too_long(distinct_lessons_list_formatted)) {
                        formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:1px; "
                                                                "vertical-align: top; margin-left:1px;\">{}{}</span>",
                                                                this_lesson_bi_weekly, formatted_lesson));
                    } else {
                        // Display only subject name
                        // Make sure they don't go out of border
                        int character_limit_before_line_break = 8;

                        total_character += std::get<0>(list_formatted).length() + 1; // Space
                        unsigned long remainder = total_character % character_limit_before_line_break;

                        // If after those characters, it would be too long -> <br>
                        if (total_character >= character_limit_before_line_break && remainder) {
                            formatted_lessons.emplace_back("<br>");

                            // TODO: Sure?
                            total_character = 0;
                        }

                        formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:5px; "
                                                                "vertical-align: top; margin-left:5px;\">{}{}</span>",
                                                                this_lesson_bi_weekly, std::get<0>(list_formatted)));
                    }
                }
            }

            // Stripe colour (based on first eligible subject / fallback to first subject)
            std::tuple<int, int, int> rgba_value = Config::TimeTableMappingConfig::default_subject_color;

            // Collect eligible subjects (None or irregular)
            std::vector<Subject> eligible_subjects;

            for (const auto &lesson: filtered_lessons) {
                str p_code = lesson.get_period_code(featuring_object).first;
                if (p_code == "extra") {
                    continue;
                }
                if (lesson.subjects.empty()) {
                    continue;
                }
                Subject su = lesson.subjects.at(0);
                eligible_subjects.push_back(su);
            }

            std::optional<Subject> chosen_subject;

            if (!eligible_subjects.empty()) {
                std::ranges::sort(eligible_subjects, {}, &Subject::name);
                chosen_subject = eligible_subjects.at(0);
            } else {
                if (!filtered_lessons.empty() && !filtered_lessons.at(0).subjects.empty()) {
                    chosen_subject = filtered_lessons.at(0).subjects.at(0);
                }
            }

            if (chosen_subject.has_value()) {
                rgba_value = chosen_subject->color();
            }

            html.push_back(std::format("<td style=\"--stripe-color: rgba({},{},{}); white-space: nowrap;\">{}</td>",
                                       std::get<0>(rgba_value), std::get<1>(rgba_value), std::get<2>(rgba_value),
                                       Str_Utils::join(formatted_lessons)));
        }
        html.emplace_back("</tr>");
    }

    html.emplace_back("</table>");

    if (any_two_week_lesson) {
        html.emplace_back(Config::HTMLStyleConfig::timetable_html_footer_two_week);
    }

    html.emplace_back(Config::HTMLStyleConfig::timetable_html_footer);

    str html_content = Str_Utils::join("\n", html);

    return html_content;
}

void TimeTable::render_one_image_by_html(std::counting_semaphore<MAX_CONCURRENCY_WEBSITE_CAPTURE> &sem,
                                         std::map<str, std::vector<uint8_t> > &results,
                                         const std::tuple<str, str> &table_name, const str &html_content) {
    sem.acquire();
    const str filename = Renderer::sanitize_filename(std::get<0>(table_name) + " " + std::get<1>(table_name) + ".png");
    results[filename] = Renderer::base64_decode(Config::renderer.generate_base64_image(html_content));
    sem.release();
}

std::vector<uint8_t> TimeTable::capture_image_by_html(const int concurrency_website_capture,
                                                      const std::tuple<str, str> &table_name, const str &html_content) {
    std::counting_semaphore<MAX_CONCURRENCY_WEBSITE_CAPTURE> sem(concurrency_website_capture);

    std::map<str, std::vector<uint8_t> > results;
    render_one_image_by_html(sem, results, table_name, html_content);
    return results.begin()->second;
}

// Returns map of sanitized filename -> PNG bytes
std::map<str, std::vector<uint8_t> >
TimeTable::capture_all_images(const int concurrency_website_capture,
                              const std::vector<std::pair<std::tuple<str, str>, str> > &pages) {
    std::counting_semaphore<MAX_CONCURRENCY_WEBSITE_CAPTURE> sem(concurrency_website_capture);
    std::map<str, std::vector<uint8_t> > results;
    std::mutex results_mutex;
    std::vector<std::thread> threads;
    threads.reserve(pages.size());

    for (const auto &[table_name, html_content]: pages) {
        threads.emplace_back([&, table_name, html_content]() {
            sem.acquire();
            const str filename =
                    Renderer::sanitize_filename(std::get<0>(table_name) + " " + std::get<1>(table_name) + ".png");
            auto image = Renderer::base64_decode(Config::renderer.generate_base64_image(html_content));
            sem.release();

            std::lock_guard<std::mutex> lock(results_mutex);
            results[filename] = std::move(image);
        });
    }

    for (auto &t: threads)
        t.join();

    return results;
}

std::vector<uint8_t> TimeTable::table_to_image(const int concurrency_website_capture,
                                               const std::variant<Class, Room, Teacher> &featuring_object,
                                               const int user_id, const date &start_date, const date &end_date) const {
    const std::tuple<str, str> table_name = get_table_name(featuring_object, start_date, end_date);
    const str html_content = to_untis_html(featuring_object, user_id, table_name, start_date, end_date);

    return capture_image_by_html(concurrency_website_capture, table_name, html_content);
}

unsigned long TimeTable::count_appearances(const Period &period_to_count) const {
    unsigned long appearances = 0;
    std::vector<Period> distinct_periods;

    for (const auto &period: periods) {
        // If it's an **exact match**, the date hour etc. match -> continue
        if (Vector_Utils::contains_value(distinct_periods, period)) {
            continue;
        }
        distinct_periods.push_back(period);

        // If the regular identifier is the same, appearances += 1
        if (period_to_count.regular_plan_identifier() == period.regular_plan_identifier()) {
            ++appearances;
        }
    }

    return appearances;
}

size_t TimeTable::size() const {
    return periods.size();
}

bool TimeTable::operator==(const TimeTable &other) const {
    return periods == other.periods;
}

TimeTable TimeTable::operator+(const TimeTable &other) const {
    std::vector<Period> combined;
    combined.reserve(periods.size() + other.periods.size());
    combined.insert(combined.end(), periods.begin(), periods.end());
    combined.insert(combined.end(), other.periods.begin(), other.periods.end());
    return TimeTable(std::move(combined));
}

str TimeTable::to_string() const {
    std::vector<str> period_strings;
    period_strings.reserve(periods.size());

    for (const auto &period: periods) {
        period_strings.push_back(period.to_string());
    }

    return Str_Utils::join("\n", period_strings);
}
