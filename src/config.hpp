#pragma once

#include "render.hpp"

#include <map>
#include <unordered_set>
#include <vector>

#include "utils/all.hpp"

namespace Config::TimeTableMappingConfig {
    extern std::map<str, std::tuple<std::unordered_set<str>, std::unordered_set<str> > > personal_timetable_entries;
    extern std::map<int, std::tuple<str, str, std::unordered_set<str> > > teacher_mapping;
    extern std::map<std::tuple<str, str, int>, std::tuple<int, int, int> > subject_to_color;
    extern std::tuple<int, int, int> default_subject_color;
}; // namespace Config::TimeTableMappingConfig

namespace Config::LanguageConfig {
    extern std::vector<std::pair<str, str> > weekday_name_mapping;
    extern str tomorrow;
    extern str today;
    extern str yesterday;
    extern str last_week;
    extern str next_week;
    extern str two_week_abbreviation;
    extern str two_week_abbreviation_legend;
    extern str class_timetable;
    extern str room_timetable;
    extern str teacher_timetable;
    extern str personal_timetable;
    extern str unknown_element_extended_text;
    extern str some_hour;
    extern str is_cancelled;
    extern str are_cancelled;
    extern str is_irregular;
    extern str are_irregular;
    extern str instead;
    extern str multiple_lessons_cancelled;
    extern str multiple_lessons_irregular;
    extern str back;
    extern str time;
    extern str unexpected_error;

    void set_internal_lang(const str &lang);
}; // namespace Config::LanguageConfig


namespace Config::HTMLStyleConfig {
    extern std::tuple<int, int, int> table_header_base_rgb;
    extern std::tuple<int, int, int> today_personal_rgb_value;
    extern str timetable_html_header;
    extern str timetable_html_footer;
    extern str timetable_html_footer_two_week;
    extern str personal_timetable_html_header;
    extern str personal_timetable_html_footer;
    extern str unknown_element_symbol;
    extern str lesson_time_ranges_format;
    extern std::vector<str> lesson_time_ranges;

    void set_internal_lang(const str &lang);
}; // namespace Config::HTMLStyleConfig

namespace Config {
    extern Renderer renderer;

    void set_lang(const str &lang);
}; // namespace Config
