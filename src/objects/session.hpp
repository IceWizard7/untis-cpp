#pragma once

#include <optional>
#include <variant>
#include "base_date_entity/all.hpp"
#include "base_entity/all.hpp"
#include "cache.hpp"
#include "logging.hpp"
#include "time_table.hpp"
#include "utils/types.hpp"

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
    Logger my_logger;

    Session(str session_name, bool use_cache, std::optional<str> cache_file, std::optional<Logger> logger, str username,
            str password, str server, str school, str client);

    static uuid get_unique_uuid();

    json rpc_request(const str &method, const json &params, bool retry_on_authentication_error = true);

    static int format_date(const date &d);

    static date parse_date(int d);

    static day_time parse_time(int t);

    static std::map<str, str> create_date_param(date start, date end, const json &kwargs);

    void log_in(uuid unique_id);

    void log_out(const uuid &unique_id);

    [[nodiscard]] std::vector<Class> all_klassen();

    [[nodiscard]] std::vector<Room> all_rooms();

    [[nodiscard]] std::vector<Subject> all_subjects();

    [[nodiscard]] std::vector<Department> all_departments();

    [[nodiscard]] std::vector<Holiday> all_holidays();

    [[nodiscard]] std::vector<SchoolYear> all_schoolyears();

    [[nodiscard]] SchoolYear return_current_year();

    [[nodiscard]] Class get_klasse_by_name(const str &name);

    [[nodiscard]] Room get_room_by_name(const str &name);

    static Teacher get_teacher_by_name(const str &name);

    static Teacher get_teacher_by_long_name(const str &name);

    TimeTable timetable_extended(const std::variant<Class, Room, Teacher> &element, const date &start, const date &end);

    json teachers();

    json statusdata();

    int last_import_time();

    json substitutions(const date &start, const date &end, int department_id = 0);

    [[nodiscard]] std::vector<str> timegrid_units();

    json students();

    json exam_types();

    json exams(const date &start, const date &end, int exam_type_id);

    json timetable_with_absences(const date &start, const date &end);

    json class_reg_events(const date &start, const date &end);

    json class_reg_event_for_id(const date &start, const date &end, const json &type_and_id);

    json class_reg_categories();

    json class_reg_category_groups();

    [[nodiscard]] TimeTable my_timetable(const date &start, const date &end);

    json _search(const str &surname, const str &fore_name, int dob = 0, int what = -1);

    std::map<str, std::variant<str, int, json> > get_student(
            const str &surname, const str &fore_name, int dob = 0);

    std::map<str, std::variant<str, int, json> > get_teacher_from_search(
            const str &surname, const str &fore_name,
            int dob = 0);

    [[nodiscard]] std::variant<std::tuple<str, std::exception>, std::map<str, TimeTable> > multithreading_result(
            float sleep_time, int max_threads, date start, date end, str function_name,
            bool logging, uuid call_id, bool log_out_afterwards, int max_attempts
            );

    // Expose cache functions (from private cache member)
    void read_cache_from_file();

    void write_cache_to_file() const;

    void clear_cache();

    [[nodiscard]] std::optional<double> cache_file_last_changed() const;

private:
    // Internal session tracking
    std::optional<str> jsessionid;
    std::optional<int> person_type;
    std::optional<int> person_id;
    std::optional<int> klasse_id;

    std::unordered_set<uuid> active_session_uuids;
    Cache cache;

    void multithread_worker(
            std::map<str, TimeTable> &raw_result,
            std::optional<std::tuple<str, std::exception> > &error_result, std::mutex &raw_result_lock,
            const Class &klasse, date start, date end, str function_name, uuid call_id,
            int max_attempts);
};
