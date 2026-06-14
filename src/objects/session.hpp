#pragma once

#include <functional>
#include <optional>
#include <variant>
#include <cpr/cpr.h>

#include "cache.hpp"
#include "logging.hpp"
#include "time_table.hpp"
#include "base_date_entity/all.hpp"
#include "base_entity/all.hpp"
#include "utils/types.hpp"

template<typename T>
struct SessionCacheJson {
    static json encode(const T &value) {
        return value;
    }

    static T decode(const json &value) {
        return value.get<T>();
    }
};

template<typename T>
struct SessionCacheJson<std::vector<T> > {
    static json encode(const std::vector<T> &values) {
        json result = json::array();

        for (const auto &value: values) {
            result.push_back(SessionCacheJson<T>::encode(value));
        }

        return result;
    }

    static std::vector<T> decode(const json &value) {
        std::vector<T> result;
        result.reserve(value.size());

        for (const auto &item: value) {
            result.push_back(SessionCacheJson<T>::decode(item));
        }

        return result;
    }
};

template<>
struct SessionCacheJson<Class> {
    static json encode(const Class &value);

    static Class decode(const json &value);
};

template<>
struct SessionCacheJson<Room> {
    static json encode(const Room &value);

    static Room decode(const json &value);
};

template<>
struct SessionCacheJson<Subject> {
    static json encode(const Subject &value);

    static Subject decode(const json &value);
};

template<>
struct SessionCacheJson<Department> {
    static json encode(const Department &value);

    static Department decode(const json &value);
};

template<>
struct SessionCacheJson<Teacher> {
    static json encode(const Teacher &value);

    static Teacher decode(const json &value);
};

template<>
struct SessionCacheJson<Holiday> {
    static json encode(const Holiday &value);

    static Holiday decode(const json &value);
};

template<>
struct SessionCacheJson<SchoolYear> {
    static json encode(const SchoolYear &value);

    static SchoolYear decode(const json &value);
};

template<>
struct SessionCacheJson<Period> {
    static json encode(const Period &value);

    static Period decode(const json &value);
};

template<>
struct SessionCacheJson<TimeTable> {
    static json encode(const TimeTable &value);

    static TimeTable decode(const json &value);
};

class Session final {
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

    Session(str session_name, bool use_cache, std::optional<str> cache_file, const std::optional<Logger> &logger,
            str username,
            str password, str server, str school, str client);

    ~Session() = default;

    static uuid get_unique_uuid();

    json rpc_request(const str &method, const json &params, bool retry_on_authentication_error = true);

    static int format_date(const date &d);

    static date parse_date(int d);

    static day_time parse_time(int t);

    static json create_date_param(date start, date end, const json &kwargs);

    void log_in(const uuid &unique_id);

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

    Teacher get_teacher_by_name(const str &name);

    Teacher get_teacher_by_long_name(const str &name);

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
            float sleep_time, int max_threads, date start, date end, const str &function_name,
            bool logging, const uuid &call_id, bool log_out_afterwards, int max_attempts
            );

    // Expose cache functions (from private cache member)
    void read_cache_from_file();

    void write_cache_to_file() const;

    void clear_cache();

    [[nodiscard]] std::optional<double> cache_file_last_changed() const;

private:
    // Internal session tracking
    std::optional<str> my_jsessionid;
    std::optional<int> my_person_type;
    std::optional<int> my_person_id;
    std::optional<int> my_klasse_id;

    std::unordered_set<uuid> active_session_uuids;
    Cache cache;

    static str cache_key(const str &method_name, const json &args_json);

    template<typename T, typename Producer>
    T cached_call(const str &method_name, const json &args_json, Producer producer) {
        if (!use_cache) {
            return producer();
        }

        const str key = cache_key(method_name, args_json);

        if (const auto cached_value = cache.get_json(key); cached_value.has_value()) {
            try {
                return SessionCacheJson<T>::decode(*cached_value);
            } catch (const std::exception &) {
            }
        }

        T value = producer();
        cache.set_json(key, SessionCacheJson<T>::encode(value));
        return value;
    }

    [[nodiscard]] TimeTable parse_timetable(const json &raw_result);

    void multithread_worker(
            std::map<str, TimeTable> &raw_result,
            std::optional<std::tuple<str, std::exception> > &error_result, std::mutex &raw_result_lock,
            const Class &klasse, date start, date end, str function_name, const uuid &call_id,
            int max_attempts);

    json rpc_request_with_session(cpr::Session &http, const str &method, const json &params,
                                  bool retry_on_authentication_error = true);
};
