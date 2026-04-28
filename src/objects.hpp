#pragma once

#include <algorithm>
#include <chrono>
#include <concepts>
#include <filesystem>
#include <map>
#include <semaphore>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "config.hpp"
#include "logging.hpp"

#define MAX_CONCURRENCY_WEBSITE_CAPTURE 1000

inline Config my_config;

using date = std::chrono::year_month_day;
using day_time = std::chrono::seconds;
using datetime = std::chrono::sys_time<std::chrono::seconds>;
using uuid = boost::uuids::uuid;
using json = nlohmann::json;

template <typename T>
concept HasToString = requires(const T& x) {
    { x.to_string() } -> std::convertible_to<str>;
};

class Date_Utils {
public:
    static day_time datetime_to_time(datetime date_time);

    static date datetime_to_date(datetime date_time);

    static str datetime_to_str(const datetime& tp, const str& format);

    static str date_to_str(const date& d, const str& format);

    static date str_to_date(const str& s, const str& format);

    static day_time str_to_daytime(const str& s, const str& format);

    static day_time str_to_time(str t);

    static datetime combine(const date& d, const day_time& t);

    static int weekday(const date& d);

    static str daytime_to_str(const day_time& t, const str& format);

    static date add_days(const date& d, int days);

    static date add_weeks(const date& d, int weeks);

    static date get_today();

    size_t operator()(const date& d) const;

    size_t operator()(const day_time& d) const;

    size_t operator()(const datetime& dt) const;
};


class Vector_Utils {
public:
    template <typename R1, typename R2>
    static std::vector<typename R1::value_type> concat_ranges(const R1& a, const R2& b);

    template <HasToString T>
    static str vector_to_string(const std::vector<T>& vec);

    template <typename T>
    static bool contains_value(const std::vector<T>& vec, const T& val);
};

class Str_Utils {
public:
    template<typename Container, typename Projection>
    static std::string join(const std::string& sep, const Container& items, Projection proj);

    template<typename Container>
    static str join(const str& sep, const Container& items);

    template<typename Container>
    static str join(const Container& items);
};

class Base_Entity {
public:
    str name;
    str long_name;
    int entity_id;

    Base_Entity(str n, str ln, int id);
    virtual ~Base_Entity();

    [[nodiscard]] str to_string(const str& class_name) const;

    [[nodiscard]] virtual str to_string() const;

    bool operator==(const Base_Entity& other) const;

    std::ostream& print(std::ostream& os) const;
};

class Subject: public Base_Entity {
public:
    Subject(str n, str ln, int i);
    ~Subject() override;

    std::tuple<int, int, int> color();

    [[nodiscard]] str to_string() const override;
};


class Class: public Base_Entity {
public:
    Class(str n, str ln, int i);
    ~Class() override;

    [[nodiscard]] str to_string() const override;
};

class Room : public Base_Entity {
public:
    Room(str n, str ln, int i);
    ~Room() override;

    [[nodiscard]] str to_string() const override;
};

class Teacher : public Base_Entity {
public:
    Teacher(str n, str ln, int i);
    ~Teacher() override;

    [[nodiscard]] std::variant<std::unordered_set<str>, str> subjects() const;

    static str get_name(int raw_teacher_id) ;

    static str get_long_name(int raw_teacher_id) ;

    static Teacher from_teacher_id(int raw_teacher_id) ;

    static Teacher from_teacher_name(const str& raw_teacher_name) ;

    static Teacher from_teacher_long_name(const str &raw_teacher_long_name) ;

    [[nodiscard]] str to_string() const override;
};

class Department : public Base_Entity {
public:
    Department(str n, str ln, int i);
    ~Department() override;

    [[nodiscard]] str to_string() const override;
};

class Base_Date_Entity {
public:
    str name;
    str long_name;
    int entity_id;
    std::optional<date> start_date;
    std::optional<date> end_date;

    Base_Date_Entity(str n, str ln, int id, date start_date, date end_date);
    ~Base_Date_Entity();

    static std::optional<date> parse_date(const std::optional<int>& value);

    explicit Base_Date_Entity(const std::unordered_map<str, std::variant<str, int>>& raw_obj);
};

class Holiday : public Base_Date_Entity {};

class SchoolYear : public Base_Date_Entity {};

class Period {
public:
    std::optional<str> raw_period_code;
    datetime start;
    datetime end;
    std::vector<Subject> subjects;
    std::vector<Class> klassen;
    std::vector<Room> rooms;
    std::vector<Room> original_rooms;
    std::vector<Teacher> teachers;
    std::vector<Teacher> original_teachers;
    str student_group;
    str activity_type;
    str bk_remark;
    str bk_text;
    str flags;
    int ls_number;
    str ls_text;
    str subst_text;
    str period_type;
    int period_id;

    Period(
        std::optional<str> raw_period_code,
        datetime start,
        datetime end,
        std::vector<Subject> subjects,
        std::vector<Class> klassen,
        std::vector<Room> rooms,
        std::vector<Room> original_rooms,
        std::vector<Teacher> teachers,
        std::vector<Teacher> original_teachers,
        str student_group,
        str activity_type,
        str bk_remark,
        str bk_text,
        str flags,
        int ls_number,
        str ls_text,
        str subst_text,
        str period_type,
        int period_id
    );

    ~Period();

    [[nodiscard]] str period_code_class(const Class& klassen_object) const;

    [[nodiscard]] str period_code_room(const Room& room_object) const;

    [[nodiscard]] str period_code_teacher(const Teacher& teacher_object) const;

    [[nodiscard]] std::pair<str, std::pair<bool, bool>> get_period_code(const std::variant<Class, Room, Teacher>& featuring_object) const;

    [[nodiscard]] str subjects_str() const;

    [[nodiscard]] str room_str(bool regular_plan) const;

    [[nodiscard]] str teacher_str(bool regular_plan) const;

    [[nodiscard]] str klassen_str() const;

    [[nodiscard]] std::tuple<str, str, str, datetime, datetime> formatted_list_class(bool regular_plan) const;

    [[nodiscard]] std::tuple<str, str, str, datetime, datetime> formatted_list_room(bool regular_plan) const;

    [[nodiscard]] std::tuple<str, str, str, datetime, datetime> formatted_list_teacher(bool regular_plan) const;

    [[nodiscard]] std::tuple<str, str, str, datetime, datetime> formatted_list(const std::variant<Class, Room, Teacher>& featuring_object, bool regular_plan) const;

    [[nodiscard]] str formatted_string(const std::variant<Class, Room, Teacher>& featuring_object, bool regular_plan) const;

    [[nodiscard]] str formatted_string_with_date_part(const std::variant<Class, Room, Teacher>& featuring_object, bool regular_plan) const;

    [[nodiscard]] std::tuple<int, day_time, day_time, std::vector<Subject>, std::vector<Class>, std::vector<Room>, std::vector<Teacher>> regular_plan_identifier() const;

    [[nodiscard]] str to_string() const;

    bool operator==(const Period& other) const;
};

class TimeTable {
    std::vector<Period> periods;

public:
    explicit TimeTable(std::vector<Period> periods_);
    ~TimeTable();

    [[nodiscard]] TimeTable copy_by_date_range(date start_date, date end_date) const;

    void filter_hours_by_subject(const Subject& subject);

    void filter_hours_by_class(const Class& klasse);

    void filter_hours_by_room(const Room& room);

    void filter_hours_by_teacher(const Teacher& teacher);

    void filter_hours_by_personal(const str& name);

    static str format_value(float value, bool percent, bool val_int);

    [[nodiscard]] std::tuple<int, int, int, int> get_statistics(
        date start_date,
        date end_date,
        const std::variant<Class, Room, Teacher>& featuring_object,
        const std::vector<Class>& filtered_class_objects,
        bool filter_classes,
        const std::vector<Room>& filtered_room_objects,
        bool filter_rooms,
        const std::vector<Teacher>& filtered_teacher_objects,
        bool filter_teachers
    ) const;

    [[nodiscard]] std::vector<str> get_separate_hours(
        date start_date,
        date end_date,
        const std::variant<Class, Room, Teacher>& featuring_object,
        int total_periods,
        const std::vector<Class>& filtered_class_objects,
        bool filter_classes,
        const std::vector<Room>& filtered_room_objects,
        bool filter_rooms,
        const std::vector<Teacher>& filtered_teacher_objects,
        bool filter_teachers,
        bool filter_unused_objects
    ) const;

    static std::tuple<str, str> get_table_name(
        const std::variant<Class, Room, Teacher>& featuring_object,
        date start_date,
        date end_date
    );

    [[nodiscard]] std::vector<Period> unsorted_table() const;

    [[nodiscard]] std::vector<std::pair<day_time, std::vector<std::pair<date, std::vector<Period>>>>> to_table() const;

    [[nodiscard]] std::vector<str> to_class_cancelled_hours() const;

    [[nodiscard]] std::tuple<std::vector<str>, std::vector<str>, std::map<str, std::map<str, std::vector<Period>>>> html_setup(
        int user_id,
        bool website,
        const std::tuple<str, str> &table_name,
        const std::optional<date> &start_date,
        const std::optional<date> &end_date
    ) const;

    static bool html_line_too_long(const std::vector<std::tuple<str, str, str, datetime, datetime>>& distinct_lessons_list_formatted);

    static void html_add_lesson_time_range(std::vector<str>& html, int lesson_count_index, const str& lesson_time_range);

    [[nodiscard]] str to_html(
        const std::variant<Class, Room, Teacher> &featuring_object,
        int user_id,
        bool website,
        const std::tuple<str, str> &table_name,
        std::optional<date> start_date,
        std::optional<date> end_date
    ) const;

    [[nodiscard]] str to_untis_html(
        const std::variant<Class, Room, Teacher>& featuring_object,
        int user_id,
        const std::tuple<str, str>& table_name,
        date start_date,
        date end_date
    ) const;

    [[nodiscard]] str to_website_html(
        const std::variant<Class, Room, Teacher>& featuring_object,
        date start_date,
        date end_date
    ) const;

    [[nodiscard]] str to_personal_html(
        const std::variant<Class, Room, Teacher>& featuring_object,
        date target_date,
        const str& person_name
    ) const;

    [[nodiscard]] str to_regular_html(
        const std::variant<Class, Room, Teacher>& featuring_object,
        int user_id,
        const std::tuple<str, str>& table_name
    ) const;

    static void render_one_image_by_html(
        std::counting_semaphore<MAX_CONCURRENCY_WEBSITE_CAPTURE>& sem,
        std::map<str, std::vector<uint8_t>>& results,
        const std::tuple<str, str>& table_name,
        const str& html_content
    ) ;

    // Returns PNG bytes for a single HTML string
    static std::vector<uint8_t> capture_image_by_html(
        int concurrency_website_capture,
        const std::tuple<str, str>& table_name,
        const str& html_content
    );

    // Returns map of sanitized filename -> PNG bytes
    static std::map<str, std::vector<uint8_t>> capture_all_images(
        const int concurrency_website_capture,
        const std::vector<std::pair<std::tuple<str, str>, str>>& pages
    );

    [[nodiscard]] std::vector<uint8_t> table_to_image(
        int concurrency_website_capture,
        const std::variant<Class, Room, Teacher>& featuring_object,
        int user_id,
        date start_date,
        date end_date
    ) const;

    [[nodiscard]] unsigned long count_appearances(
        const Period& period_to_count
    ) const;

    [[nodiscard]] size_t size() const;

    bool operator==(const TimeTable& other) const;

    TimeTable operator+(const TimeTable& other) const;

    [[nodiscard]] str to_string() const;
};

class Cache {
    struct TimeTableCacheKey {
        date start_date;
        date end_date;
        str method_name;
    };

    struct ElementCacheKey {
        str method_name;
    };

    struct ElementByNameCacheKey {
        str method_name;
        str object_name;
    };

    struct TimeGridCacheKey {
        str method_name;
    };

private:
    std::unordered_map<std::string, json> cache;
    std::optional<std::filesystem::path> cache_file_path;
public:
    explicit Cache(std::optional<std::filesystem::path> cache_file) : cache_file_path(std::move(cache_file)) {}

    [[nodiscard]] std::optional<double> cache_file_last_changed(const std::optional<str>& file_path) const;

    template<typename T>
    std::optional<T> get_from_cache(const std::string& key);

    template<typename T>
    void update_cache(const std::string& key, T value);

    void clear_cache();

    void read_cache_from_file();

    void write_cache_to_file() const;
};

class Session {
public:
    str session_name;
    bool use_cache;
    Logger logger;
    str username;
    str password;
    str server;
    str school;
    str client;

    Session(
        str session_name,
        bool use_cache,
        std::optional<str> cache_file,
        std::optional<Logger> logger,
        str username,
        str password,
        str server,
        str school,
        str client
    );

    static uuid get_unique_uuid();

    json rpc_request(
        const str &method,
        const json &params,
        bool retry_on_authentication_error = true
    );

    static int format_date(const date& d);

    static date parse_date(int d);

    static day_time parse_time(int t);

    static std::map<str, str> create_date_param(date start, date end, const json& kwargs);

    void log_in(uuid unique_id);

    void log_out(uuid unique_id);

    std::vector<Class> all_klassen();

    std::vector<Room> all_rooms() const;

    std::vector<Subject> all_subjects() const;

    std::vector<Department> all_departments() const;

    std::vector<Holiday> all_holidays() const;

    std::vector<SchoolYear> all_schoolyears() const;

    SchoolYear return_current_year() const;

    std::optional<Class> get_klasse_by_name(const str& name);

    std::optional<Room> get_room_by_name(const str& name) const;

    std::optional<Teacher> get_teacher_by_name(const str& name) const;

    std::optional<Teacher> get_teacher_by_long_name(const str& name) const;

    TimeTable timetable_extended(
        const std::variant<Class, Room, Teacher>& element,
        date start,
        date end
    );

    // TODO: teachers

    // TODO: statusdata

    // TODO: last_import_time

    // TODO: substitutions

    std::vector<str> timegrid_units() const;

    // TODO: students

    // TODO: exam_types

    // TODO: exams

    // TODO: timetable_with_absences

    // TODO: class_reg_events

    // TODO: class_reg_event_for_id

    // TODO: class_reg_categories

    // TODO: class_reg_category_groups

    TimeTable my_timetable(
        date start,
        date end
    ) const;

    // TODO: _search

    // TODO: get_student

    // TODO: get_teacher_from_search

private:
    // Internal session tracking
    std::optional<str> jsessionid;
    std::optional<int> person_type;
    std::optional<int> person_id;
    std::optional<int> klasse_id;

    std::unordered_set<uuid> active_session_uuids;
    Cache cache;

    void multithread_worker(
        std::map<str, std::variant<str, std::exception, std::map<str, TimeTable>>> raw_result,
        std::mutex& raw_result_lock,
        Class klasse,
        str raw_date,
        date start,
        date end,
        str function_name,
        uuid call_id,
        int max_attempts
    ) const;

    std::map<str, std::variant<str, std::exception, std::map<str, TimeTable>>> multithreading_result(
        float sleep_time,
        int max_threads,
         str raw_date,
         date start,
         date end,
         str function_name,
         bool logging,
         uuid call_id,
         bool log_out_afterwards,
         int max_attempts
    ) const;
};
