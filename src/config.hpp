#pragma once

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "render.hpp"

using str = std::string;

class TimeTableMappingConfig {
public:
    std::map<str, std::tuple<std::unordered_set<str>, std::unordered_set<str>>> personal_timetable_entries;
    std::map<int, std::tuple<str, str, std::unordered_set<str>>> teacher_mapping;
    std::map<std::tuple<str, str, int>, std::tuple<int, int, int>> subject_to_color;
    std::tuple<int, int, int> default_subject_color;

    TimeTableMappingConfig();
    ~TimeTableMappingConfig();
};

class LanguageConfig {
public:
    std::vector<std::pair<str, str>> weekday_name_mapping;
    str tomorrow;
    str today;
    str yesterday;
    str last_week;
    str next_week;
    str two_week_abbreviation;
    str two_week_abbreviation_legend;
    str class_timetable;
    str room_timetable;
    str teacher_timetable;
    str personal_timetable;
    str unknown_element_extended_text;
    str some_hour;
    str is_cancelled;
    str are_cancelled;
    str is_irregular;
    str are_irregular;
    str instead;
    str multiple_lessons_cancelled;
    str multiple_lessons_irregular;
    str back;
    str time;
    str unexpected_error;

    void set_internal_lang(const str& lang);

    LanguageConfig();
    ~LanguageConfig();
};


class HTMLStyleConfig {
public:
    std::tuple<int, int, int> table_header_base_rgb;
    std::tuple<int, int, int> today_personal_rgb_value;
    str timetable_html_header;
    str timetable_html_footer;
    str timetable_html_footer_two_week;
    str personal_timetable_html_header;
    str personal_timetable_html_footer;
    str unknown_element_symbol;
    str lesson_time_ranges_format;
    std::vector<str> lesson_time_ranges;

    void set_internal_lang(const str& lang, const LanguageConfig& language_config);

    HTMLStyleConfig();
    ~HTMLStyleConfig();
};

class Config {
public:
    TimeTableMappingConfig timetable_mapping_config;
    LanguageConfig language_config;
    HTMLStyleConfig html_style_config;
    Renderer renderer;

    Config();
    ~Config();

    void set_lang(const str& lang);
};
