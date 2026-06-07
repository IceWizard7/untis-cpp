#pragma once

#define MAX_CONCURRENCY_WEBSITE_CAPTURE 1000

#include <semaphore>
#include <vector>
#include "period.hpp"

class TimeTable {
    std::vector<Period> periods;

public:
    explicit TimeTable(std::vector<Period> periods_);
    ~TimeTable();

    [[nodiscard]] TimeTable copy_by_date_range(date start_date, date end_date) const;

    void filter_hours_by_subject(const Subject &subject);

    void filter_hours_by_class(const Class &klasse);

    void filter_hours_by_room(const Room &room);

    void filter_hours_by_teacher(const Teacher &teacher);

    void filter_hours_by_personal(const str &name);

    static str format_value(float value, bool percent, bool val_int);

    [[nodiscard]] std::tuple<int, int, int, int>
    get_statistics(date start_date, date end_date, const std::variant<Class, Room, Teacher> &featuring_object,
                   const std::vector<Class> &filtered_class_objects, bool filter_classes,
                   const std::vector<Room> &filtered_room_objects, bool filter_rooms,
                   const std::vector<Teacher> &filtered_teacher_objects, bool filter_teachers) const;

    [[nodiscard]] std::vector<str>
    get_separate_hours(date start_date, date end_date, const std::variant<Class, Room, Teacher> &featuring_object,
                       int total_periods, const std::vector<Class> &filtered_class_objects, bool filter_classes,
                       const std::vector<Room> &filtered_room_objects, bool filter_rooms,
                       const std::vector<Teacher> &filtered_teacher_objects, bool filter_teachers,
                       bool filter_unused_objects) const;

    static std::tuple<str, str> get_table_name(const std::variant<Class, Room, Teacher> &featuring_object,
                                               date start_date, date end_date);

    [[nodiscard]] std::vector<Period> unsorted_table() const;

    [[nodiscard]] std::vector<std::pair<day_time, std::vector<std::pair<date, std::vector<Period>>>>> to_table() const;

    [[nodiscard]] std::vector<str> to_class_cancelled_hours() const;

    [[nodiscard]] std::tuple<std::vector<str>, std::vector<str>, std::map<str, std::map<str, std::vector<Period>>>>
    html_setup(int user_id, bool website, const std::tuple<str, str> &table_name, const std::optional<date> &start_date,
               const std::optional<date> &end_date) const;

    static bool html_line_too_long(
            const std::vector<std::tuple<str, str, str, datetime, datetime>> &distinct_lessons_list_formatted);

    static void html_add_lesson_time_range(std::vector<str> &html, int lesson_count_index,
                                           const str &lesson_time_range);

    [[nodiscard]] str to_html(const std::variant<Class, Room, Teacher> &featuring_object, int user_id, bool website,
                              const std::tuple<str, str> &table_name, std::optional<date> start_date,
                              std::optional<date> end_date) const;

    [[nodiscard]] str to_untis_html(const std::variant<Class, Room, Teacher> &featuring_object, int user_id,
                                    const std::tuple<str, str> &table_name, date start_date, date end_date) const;

    [[nodiscard]] str to_website_html(const std::variant<Class, Room, Teacher> &featuring_object, date start_date,
                                      date end_date) const;

    [[nodiscard]] str to_personal_html(const std::variant<Class, Room, Teacher> &featuring_object, date target_date,
                                       const str &person_name, int n_days = 1) const;

    [[nodiscard]] str to_regular_html(const std::variant<Class, Room, Teacher> &featuring_object, int user_id,
                                      const std::tuple<str, str> &table_name) const;

    static void render_one_image_by_html(std::counting_semaphore<MAX_CONCURRENCY_WEBSITE_CAPTURE> &sem,
                                         std::map<str, std::vector<uint8_t>> &results,
                                         const std::tuple<str, str> &table_name, const str &html_content);

    // Returns PNG bytes for a single HTML string
    static std::vector<uint8_t> capture_image_by_html(int concurrency_website_capture,
                                                      const std::tuple<str, str> &table_name, const str &html_content);

    // Returns map of sanitized filename -> PNG bytes
    static std::map<str, std::vector<uint8_t>>
    capture_all_images(int concurrency_website_capture, const std::vector<std::pair<std::tuple<str, str>, str>> &pages);

    [[nodiscard]] std::vector<uint8_t> table_to_image(int concurrency_website_capture,
                                                      const std::variant<Class, Room, Teacher> &featuring_object,
                                                      int user_id, const date &start_date, const date &end_date) const;

    [[nodiscard]] unsigned long count_appearances(const Period &period_to_count) const;

    [[nodiscard]] size_t size() const;

    bool operator==(const TimeTable &other) const;

    TimeTable operator+(const TimeTable &other) const;

    [[nodiscard]] str to_string() const;
};
