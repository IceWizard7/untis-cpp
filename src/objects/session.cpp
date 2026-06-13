#include "session.hpp"

#include <cpr/cpr.h>
#include <iostream>
#include <random>
#include <stdexcept>

#include "exceptions.hpp"

namespace {
    json encode_entity(const Base_Entity &value) {
        return {{"name", value.name}, {"longName", value.long_name}, {"id", value.entity_id}};
    }

    str json_long_name(const json &value) {
        if (value.contains("longName") && !value.at("longName").is_null()) {
            return value.at("longName").get<str>();
        }

        return value.value("long_name", "");
    }

    int json_int_or_zero(const json &value, const char *key) {
        if (!value.contains(key) || value.at(key).is_null()) {
            return 0;
        }

        return value.at(key).get<int>();
    }

    json encode_date_entity(const Base_Date_Entity &value) {
        return {{"name", value.name},
                {"longName", value.long_name},
                {"id", value.entity_id},
                {"startDate",
                 value.start_date.has_value() ? json(Session::format_date(*value.start_date)) : json(nullptr)},
                {"endDate", value.end_date.has_value() ? json(Session::format_date(*value.end_date)) : json(nullptr)}};
    }

    std::unordered_map<str, std::variant<str, int> > decode_date_entity_raw(const json &value) {
        using FieldValue = std::variant<str, int>;

        return {{"name", FieldValue{value.value("name", "")}},
                {"longName", FieldValue{json_long_name(value)}},
                {"id", FieldValue{json_int_or_zero(value, "id")}},
                {"startDate", FieldValue{json_int_or_zero(value, "startDate")}},
                {"endDate", FieldValue{json_int_or_zero(value, "endDate")}}};
    }

    long long datetime_to_cache_seconds(const datetime &value) {
        return value.time_since_epoch().count();
    }

    datetime cache_seconds_to_datetime(const json &value) {
        return datetime{std::chrono::seconds{value.get<long long>()}};
    }
}

json SessionCacheJson<Class>::encode(const Class &value) {
    return encode_entity(value);
}

Class SessionCacheJson<Class>::decode(const json &value) {
    return {value.value("name", ""), json_long_name(value), json_int_or_zero(value, "id")};
}

json SessionCacheJson<Room>::encode(const Room &value) {
    return encode_entity(value);
}

Room SessionCacheJson<Room>::decode(const json &value) {
    return {value.value("name", ""), json_long_name(value), json_int_or_zero(value, "id")};
}

json SessionCacheJson<Subject>::encode(const Subject &value) {
    return encode_entity(value);
}

Subject SessionCacheJson<Subject>::decode(const json &value) {
    return {value.value("name", ""), json_long_name(value), json_int_or_zero(value, "id")};
}

json SessionCacheJson<Department>::encode(const Department &value) {
    return encode_entity(value);
}

Department SessionCacheJson<Department>::decode(const json &value) {
    return {value.value("name", ""), json_long_name(value), json_int_or_zero(value, "id")};
}

json SessionCacheJson<Teacher>::encode(const Teacher &value) {
    return encode_entity(value);
}

Teacher SessionCacheJson<Teacher>::decode(const json &value) {
    return {value.value("name", ""), json_long_name(value), json_int_or_zero(value, "id")};
}

json SessionCacheJson<Holiday>::encode(const Holiday &value) {
    return encode_date_entity(value);
}

Holiday SessionCacheJson<Holiday>::decode(const json &value) {
    return Holiday(decode_date_entity_raw(value));
}

json SessionCacheJson<SchoolYear>::encode(const SchoolYear &value) {
    return encode_date_entity(value);
}

SchoolYear SessionCacheJson<SchoolYear>::decode(const json &value) {
    return SchoolYear(decode_date_entity_raw(value));
}

json SessionCacheJson<Period>::encode(const Period &value) {
    return {{"rawPeriodCode", value.raw_period_code.has_value() ? json(*value.raw_period_code) : json(nullptr)},
            {"start", datetime_to_cache_seconds(value.start)},
            {"end", datetime_to_cache_seconds(value.end)},
            {"subjects", SessionCacheJson<std::vector<Subject> >::encode(value.subjects)},
            {"klassen", SessionCacheJson<std::vector<Class> >::encode(value.klassen)},
            {"rooms", SessionCacheJson<std::vector<Room> >::encode(value.rooms)},
            {"originalRooms", SessionCacheJson<std::vector<Room> >::encode(value.original_rooms)},
            {"teachers", SessionCacheJson<std::vector<Teacher> >::encode(value.teachers)},
            {"originalTeachers", SessionCacheJson<std::vector<Teacher> >::encode(value.original_teachers)},
            {"studentGroup", value.student_group},
            {"activityType", value.activity_type},
            {"bkRemark", value.bk_remark},
            {"bkText", value.bk_text},
            {"flags", value.flags},
            {"lsNumber", value.ls_number},
            {"lsText", value.ls_text},
            {"substText", value.subst_text},
            {"periodType", value.period_type},
            {"periodId", value.period_id}};
}

Period SessionCacheJson<Period>::decode(const json &value) {
    std::optional<str> raw_period_code = std::nullopt;

    if (value.contains("rawPeriodCode") && !value.at("rawPeriodCode").is_null()) {
        raw_period_code = value.at("rawPeriodCode").get<str>();
    }

    return {raw_period_code,
            cache_seconds_to_datetime(value.at("start")),
            cache_seconds_to_datetime(value.at("end")),
            SessionCacheJson<std::vector<Subject> >::decode(value.value("subjects", json::array())),
            SessionCacheJson<std::vector<Class> >::decode(value.value("klassen", json::array())),
            SessionCacheJson<std::vector<Room> >::decode(value.value("rooms", json::array())),
            SessionCacheJson<std::vector<Room> >::decode(value.value("originalRooms", json::array())),
            SessionCacheJson<std::vector<Teacher> >::decode(value.value("teachers", json::array())),
            SessionCacheJson<std::vector<Teacher> >::decode(value.value("originalTeachers", json::array())),
            value.value("studentGroup", ""),
            value.value("activityType", ""),
            value.value("bkRemark", ""),
            value.value("bkText", ""),
            value.value("flags", ""),
            value.value("lsNumber", 0),
            value.value("lsText", ""),
            value.value("substText", ""),
            value.value("periodType", ""),
            value.value("periodId", 0)};
}

json SessionCacheJson<TimeTable>::encode(const TimeTable &value) {
    return SessionCacheJson<std::vector<Period> >::encode(value.unsorted_table());
}

TimeTable SessionCacheJson<TimeTable>::decode(const json &value) {
    return TimeTable(SessionCacheJson<std::vector<Period> >::decode(value));
}

Session::Session(str session_name, bool use_cache, std::optional<str> cache_file, const std::optional<Logger> &logger,
                 str username, str password, str server, str school, str client) :
    session_name(std::move(session_name)), use_cache(use_cache), logger(logger.value_or(Logger{})),
    username(std::move(username)), password(std::move(password)), school(std::move(school)), client(std::move(client)),
    cache(std::move(cache_file)) {
    if (!server.starts_with("http")) {
        server = std::format("https://{}", server);
    }

    while (!server.empty() && server.back() == '/') {
        server.pop_back();
    }

    this->server = std::move(server);
}

str Session::cache_key(const str &method_name, const json &args_json) {
    return json::array({method_name, args_json.is_null() ? json::array() : args_json}).dump();
}

uuid Session::get_unique_uuid() {
    // UUID version 4
    thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution dist(0, 255);

    std::array<unsigned char, 16> bytes{};

    for (auto &byte: bytes) {
        byte = static_cast<unsigned char>(dist(rng));
    }

    // UUID version 4
    bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);

    // RFC 4122 variant
    bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (std::size_t i = 0; i < bytes.size(); ++i) {
        oss << std::setw(2) << static_cast<int>(bytes[i]);

        if (i == 3 || i == 5 || i == 7 || i == 9) {
            oss << '-';
        }
    }

    return oss.str();
}

json Session::rpc_request(const str &method, const json &params, bool retry_on_authentication_error) {
    json payload = {{"id", get_unique_uuid()}, // TODO: WHY???
                    {"method", method},
                    {"params", params},
                    {"jsonrpc", "2.0"}};

    str url = std::format("{}/jsonrpc.do?school={}", server, school);

    cpr::Header headers{{"Content-Type", "application/json"}};

    cpr::Cookies cookies;
    if (my_jsessionid) {
        cookies = cpr::Cookies{{"JSESSIONID", *my_jsessionid}};
    }

    cpr::Response response = cpr::Post(cpr::Url{url}, headers, cookies, cpr::Body{payload.dump()});

    if (response.status_code >= 400) {
        throw std::runtime_error(std::format("HTTP error {}", response.status_code));
    }

    json data = json::parse(response.text);

    if (data.contains("error")) {
        auto error = data["error"];
        str msg = error.value("message", "");

        if (msg == "not authenticated") {
            logger.log_warning(std::format("WebUntis API NotAuthenticatedError ({}): {}", method, error.dump()));

            if (retry_on_authentication_error) {
                logger.log_debug("Logging in & retrying Authentication...");

                auto call_id = get_unique_uuid();

                log_in(call_id);

                auto result = rpc_request(method, params, false);

                log_out(call_id);

                return result;
            }

            throw NotAuthenticatedError(error);
        }

        if (msg.find("no right for ") != str::npos) {
            throw NoRightForMethod(error, method);
        }

        if (msg.find("Method not found") != str::npos) {
            throw MethodNotFound(error, method);
        }

        throw std::runtime_error(std::format("WebUntis API Error ({}): {}", method, error.dump()));
    }

    return data["result"];
}

int Session::format_date(const date &d) {
    return stoi(Date_Utils::date_to_str(d, "%Y%m%d"));
}

date Session::parse_date(const int d) {
    return Date_Utils::str_to_date(std::to_string(d), "%Y%m%d");
}

day_time Session::parse_time(const int t) {
    return Date_Utils::str_to_time(std::to_string(t));
}

json Session::create_date_param(const date start, const date end, const json &kwargs) {
    if (start > end) {
        throw std::runtime_error("Start date cannot be later than end date.");
    }
    json params = {{"startDate", format_date(start)}, {"endDate", format_date(end)}};
    if (!kwargs.is_null()) {
        params.update(kwargs);
    }
    return params;
}

void Session::log_in(const uuid &unique_id) {
    if (active_session_uuids.empty()) {
        const json &params = {{"user", username}, {"password", password}, {"client", client}};

        // Use raw request for login to catch the session ID
        json payload = {{"id", get_unique_uuid()}, // TODO: WHY???
                        {"method", "authenticate"},
                        {"params", params},
                        {"jsonrpc", "2.0"}};

        str url = std::format("{}/jsonrpc.do?school={}", server, school);

        cpr::Header headers{{"Content-Type", "application/json"}};

        cpr::Response response = cpr::Post(cpr::Url{url}, headers, cpr::Body{payload.dump()});

        if (response.status_code >= 400) {
            throw std::runtime_error(std::format("HTTP error {}", response.status_code));
        }

        json data = json::parse(response.text);

        if (data.contains("error")) {
            auto error = data["error"];
            throw std::runtime_error(std::format("Login failed: {}", error.dump()));
        }

        auto result = data["result"];

        my_jsessionid = result["sessionId"];
        my_person_type = result["personType"];
        my_person_id = result["personId"];
        my_klasse_id = result["klasseId"];

        logger.log_info(std::format("Logged in ({})!", session_name));
    }
    active_session_uuids.insert(unique_id);
}

void Session::log_out(const uuid &unique_id) {
    active_session_uuids.erase(unique_id);
    if (active_session_uuids.empty() && my_jsessionid.has_value()) {
        try {
            rpc_request("logout", {});
        } catch (const std::exception &) {
        }
        my_jsessionid.reset();

        logger.log_info(std::format("Logged out ({})!", session_name));
    }
}

std::vector<Class> Session::all_klassen() {
    return cached_call<std::vector<Class> >("Session.all_klassen", json::array(), [this] {
        std::vector<Class> all_kl;
        for (const auto &k: rpc_request("getKlassen", {})) {
            all_kl.emplace_back(k.value("name", ""), k.value("longName", ""), k.value("id", 0));
        }
        return all_kl;
    });
}

std::vector<Room> Session::all_rooms() {
    return cached_call<std::vector<Room> >("Session.all_rooms", json::array(), [this] {
        std::vector<Room> all_ro;
        for (const auto &r: rpc_request("getRooms", {})) {
            all_ro.emplace_back(r.value("name", ""), r.value("longName", ""), r.value("id", 0));
        }
        return all_ro;
    });
}

std::vector<Subject> Session::all_subjects() {
    return cached_call<std::vector<Subject> >("Session.all_subjects", json::array(), [this] {
        std::vector<Subject> all_su;
        for (const auto &s: rpc_request("getSubjects", {})) {
            all_su.emplace_back(s.value("name", ""), s.value("longName", ""), s.value("id", 0));
        }
        return all_su;
    });
}

std::vector<Department> Session::all_departments() {
    return cached_call<std::vector<Department> >("Session.all_departments", json::array(), [this] {
        std::vector<Department> all_de;
        for (const auto &d: rpc_request("getDepartments", {})) {
            all_de.emplace_back(d.value("name", ""), d.value("longName", ""), d.value("id", 0));
        }
        return all_de;
    });
}

std::vector<Holiday> Session::all_holidays() {
    return cached_call<std::vector<Holiday> >("Session.all_holidays", json::array(), [this] {
        std::vector<Holiday> all_ho;

        using FieldValue = std::variant<str, int>;
        using RawObj = std::unordered_map<str, FieldValue>;

        for (const auto &h: rpc_request("getHolidays", {})) {
            all_ho.emplace_back(RawObj{{"name", FieldValue{h.value("name", "")}},
                                       {"longName", FieldValue{h.value("longName", "")}},
                                       {"id", FieldValue{h.value("id", 0)}},
                                       {"startDate", FieldValue{h.value("startDate", 0)}},
                                       {"endDate", FieldValue{h.value("endDate", 0)}}});
        }

        return all_ho;
    });
}

std::vector<SchoolYear> Session::all_schoolyears() {
    return cached_call<std::vector<SchoolYear> >("Session.all_schoolyears", json::array(), [this] {
        std::vector<SchoolYear> all_sy;

        using FieldValue = std::variant<str, int>;
        using RawObj = std::unordered_map<str, FieldValue>;

        for (const auto &s: rpc_request("getSchoolyears", {})) {
            all_sy.emplace_back(RawObj{{"name", FieldValue{s.value("name", "")}},
                                       {"longName", FieldValue{s.value("longName", "")}},
                                       {"id", FieldValue{s.value("id", 0)}},
                                       {"startDate", FieldValue{s.value("startDate", 0)}},
                                       {"endDate", FieldValue{s.value("endDate", 0)}}});
        }

        return all_sy;
    });
}

SchoolYear Session::return_current_year() {
    const std::vector<SchoolYear> years = all_schoolyears();
    if (years.empty()) {
        throw std::runtime_error("No school years found");
    }

    // TODO: This doesn't even make sense in the Python equivalent.
    // id field doesn't exist
    // Not in C++, nor in Python.
    /*
    const auto it = std::max_element(
        years.begin(),
        years.end(),
        [](const SchoolYear& a, const SchoolYear& b) {
            return a.id < b.id;
        }
    );
    */

    // return *it;
    throw std::runtime_error("Session::return_current_year is not implemented yet.");
}

Class Session::get_klasse_by_name(const str &name) {
    return cached_call<Class>("Session.get_klasse_by_name", json::array({name}), [this, &name] {
        for (const auto &k: all_klassen()) {
            if (k.name == name) {
                return k;
            }
        }
        throw std::out_of_range(std::format("Class {} was not found.", name));
    });
}

Room Session::get_room_by_name(const str &name) {
    return cached_call<Room>("Session.get_room_by_name", json::array({name}), [this, &name] {
        for (const auto &r: all_rooms()) {
            if (r.name == name) {
                return r;
            }
        }
        throw std::out_of_range(std::format("Room {} was not found.", name));
    });
}

Teacher Session::get_teacher_by_name(const str &name) {
    return cached_call<Teacher>("Session.get_teacher_by_name", json::array({name}), [&name] {
        try {
            return Teacher::from_teacher_name(name);
        } catch (const std::exception &) {
            throw std::out_of_range(std::format("Teacher (with name) {} was not found.", name));
        }
    });
}

Teacher Session::get_teacher_by_long_name(const str &name) {
    return cached_call<Teacher>("Session.get_teacher_by_long_name", json::array({name}), [&name] {
        try {
            return Teacher::from_teacher_long_name(name);
        } catch (const std::exception &) {
            throw std::out_of_range(std::format("Teacher (with long name) {} was not found.", name));
        }
    });
}

void Session::read_cache_from_file() {
    cache.read_cache_from_file();
}

void Session::write_cache_to_file() const {
    cache.write_cache_to_file();
}

void Session::clear_cache() {
    cache.clear_cache();
}

std::optional<double> Session::cache_file_last_changed() const {
    return cache.cache_file_last_changed();
}

TimeTable Session::parse_timetable(const json &raw_result) {
    if (!raw_result.is_array()) {
        return TimeTable({});
    }

    std::map<int, json> all_su = {};
    std::map<int, json> all_kl = {};
    std::map<int, json> all_ro = {};

    for (const auto &s: all_subjects()) {
        all_su[s.entity_id] = SessionCacheJson<Subject>::encode(s);
    }

    for (const auto &k: all_klassen()) {
        all_kl[k.entity_id] = SessionCacheJson<Class>::encode(k);
    }

    for (const auto &r: all_rooms()) {
        all_ro[r.entity_id] = SessionCacheJson<Room>::encode(r);
    }

    const auto array_or_empty = [](const json &value, const char *key) {
        if (value.contains(key) && value.at(key).is_array()) {
            return value.at(key);
        }

        return json::array();
    };

    std::vector<Period> periods;

    for (const auto &raw_p: raw_result) {
        try {
            datetime start_dt = Date_Utils::combine(parse_date(raw_p.at("date")), parse_time(raw_p.at("startTime")));
            datetime end_dt = Date_Utils::combine(parse_date(raw_p.at("date")), parse_time(raw_p.at("endTime")));

            std::vector<Subject> subjects;
            std::vector<Class> klassen;
            std::vector<Room> rooms;
            std::vector<Room> original_rooms;
            std::vector<Teacher> teachers;
            std::vector<Teacher> original_teachers;

            for (const auto &s: array_or_empty(raw_p, "su")) {
                const int subject_id = json_int_or_zero(s, "id");
                const auto master_su = all_su.contains(subject_id) ? all_su.at(subject_id) : json{};

                subjects.emplace_back(master_su.value("name", ""), json_long_name(master_su), subject_id);
            }

            for (const auto &k: array_or_empty(raw_p, "kl")) {
                const int klasse_id = json_int_or_zero(k, "id");
                const auto master_kl = all_kl.contains(klasse_id) ? all_kl.at(klasse_id) : json{};

                klassen.emplace_back(master_kl.value("name", ""), json_long_name(master_kl), klasse_id);
            }

            for (const auto &r: array_or_empty(raw_p, "ro")) {
                const int room_id = json_int_or_zero(r, "id");
                const auto master_ro = all_ro.contains(room_id) ? all_ro.at(room_id) : json{};

                rooms.emplace_back(master_ro.value("name", ""), json_long_name(master_ro), room_id);
            }

            for (const auto &r: array_or_empty(raw_p, "ro")) {
                if (!r.contains("orgid")) {
                    continue;
                }

                const int original_room_id = json_int_or_zero(r, "orgid");
                const auto master_ro = all_ro.contains(original_room_id) ? all_ro.at(original_room_id) : json{};

                original_rooms.emplace_back(master_ro.value("name", ""), json_long_name(master_ro), original_room_id);
            }

            for (const auto &t: array_or_empty(raw_p, "te")) {
                if (t.contains("id")) {
                    teachers.emplace_back(Teacher::from_teacher_id(json_int_or_zero(t, "id")));
                }

                if (t.contains("orgid")) {
                    original_teachers.emplace_back(Teacher::from_teacher_id(json_int_or_zero(t, "orgid")));
                }
            }

            periods.emplace_back(raw_p.value("code", ""), start_dt, end_dt, subjects, klassen, rooms, original_rooms,
                                 teachers, original_teachers, raw_p.value("sg", ""), raw_p.value("activityType", ""),
                                 raw_p.value("bkRemark", ""), raw_p.value("bkText", ""), raw_p.value("flags", ""),
                                 raw_p.value("lsnumber", 0), raw_p.value("lstext", ""), raw_p.value("substText", ""),
                                 raw_p.value("type", ""), raw_p.value("id", 0));
        } catch (const std::exception &) {
        }
    }

    return TimeTable(periods);
}

TimeTable Session::timetable_extended(const std::variant<Class, Room, Teacher> &element, const date &start,
                                      const date &end) {
    std::map<str, int> element_type_table = {
            {"klasse", 1}, {"teacher", 2}, {"subject", 3}, {"room", 4}, {"student", 5}};
    int element_type = 0;
    int entity_id = 0;

    if (const Class *c_ptr = std::get_if<Class>(&element)) {
        element_type = element_type_table["klasse"];
        entity_id = c_ptr->entity_id;
    } else if (const Room *r_ptr = std::get_if<Room>(&element)) {
        element_type = element_type_table["room"];
        entity_id = r_ptr->entity_id;
    } else if (const Teacher *t_ptr = std::get_if<Teacher>(&element)) {
        element_type = element_type_table["teacher"];
        entity_id = t_ptr->entity_id;
    }

    json options = {{"startDate", format_date(start)},
                    {"endDate", format_date(end)},
                    {"element", {{"id", entity_id}, {"type", element_type}}},
                    {"showBooking", true},
                    {"showInfo", true},
                    {"showSubstText", true},
                    {"showLsText", true},
                    {"showLsNumber", true},
                    {"showStudentgroup", true}};

    return cached_call<TimeTable>(
            "Session.timetable_extended",
            json::array({format_date(start), format_date(end), {{"id", entity_id}, {"type", element_type}}}),
            [this, options] {
                json raw_result;

                try {
                    raw_result = rpc_request("getTimetable", {{"options", options}});
                } catch (const std::exception &e) {
                    my_logger.log_error("Error in getTimetable: " + str(e.what()));
                    return TimeTable({});
                }

                return parse_timetable(raw_result);
            });
}

json Session::teachers() {
    return cached_call<json>("Session.teachers", json::array(), [this] {
        return rpc_request("getTeachers", {});
    });
}

json Session::statusdata() {
    return cached_call<json>("Session.statusdata", json::array(), [this] {
        return rpc_request("getStatusData", {});
    });
}

int Session::last_import_time() {
    return cached_call<int>("Session.last_import_time", json::array(), [this] {
        return rpc_request("getLatestImportTime", {}).get<int>();
    });
}

json Session::substitutions(const date &start, const date &end, const int department_id) {
    const json params = create_date_param(start, end, {{"departmentId", department_id}});
    return cached_call<json>(
            "Session.substitutions", json::array({format_date(start), format_date(end), department_id}),
            [this, params] {
                return rpc_request("getSubstitutions", params);
            });
}

[[nodiscard]] std::vector<str> Session::timegrid_units() {
    return cached_call<std::vector<str> >("Session.timegrid_units", json::array(), [this] {
        json raw_json = rpc_request("getTimegridUnits", {});

        auto get_optional_int = [](const json &obj, const char *key) -> std::optional<int> {
            const auto it = obj.find(key);

            if (it == obj.end() || it->is_null()) {
                return std::nullopt;
            }

            return it->get<int>();
        };

        auto convert_time = [](const std::optional<int> &value) -> str {
            if (!value.has_value()) {
                return "";
            }

            const day_time &datetime_obj = Date_Utils::str_to_daytime(std::to_string(*value), "%H%M");

            return Date_Utils::daytime_to_str(datetime_obj, Config::HTMLStyleConfig::lesson_time_ranges_format);
        };

        std::vector<str> lesson_time_ranges;

        for (const auto &item: raw_json) {
            const auto &time_units = item.at("timeUnits");

            for (const auto &time_unit_list: time_units) {
                for (const auto &time_unit: time_unit_list) {
                    auto start_time = get_optional_int(time_unit, "startTime");
                    auto end_time = get_optional_int(time_unit, "endTime");

                    lesson_time_ranges.push_back(
                            std::format("{} - {}", convert_time(start_time), convert_time(end_time)));
                }
            }
        }

        return lesson_time_ranges;
    });
}

json Session::students() {
    return cached_call<json>("Session.students", json::array(), [this] {
        return rpc_request("getStudents", {});
    });
}

json Session::exam_types() {
    return cached_call<json>("Session.exam_types", json::array(), [this] {
        return rpc_request("getExamTypes", {});
    });
}

json Session::exams(const date &start, const date &end, const int exam_type_id) {
    const json params = create_date_param(start, end, {{"examTypeId", exam_type_id}});
    return cached_call<json>(
            "Session.exams", json::array({format_date(start), format_date(end), exam_type_id}), [this, params] {
                return rpc_request("getExams", params);
            });
}

json Session::timetable_with_absences(const date &start, const date &end) {
    const json params = {{"options", create_date_param(start, end, {})}};
    return cached_call<json>(
            "Session.timetable_with_absences", json::array({format_date(start), format_date(end)}), [this, params] {
                return rpc_request("getTimetableWithAbsences", params);
            });
}

json Session::class_reg_events(const date &start, const date &end) {
    const json params = create_date_param(start, end, {});
    return cached_call<json>(
            "Session.class_reg_events", json::array({format_date(start), format_date(end)}), [this, params] {
                return rpc_request("getClassregEvents", params);
            });
}

json Session::class_reg_event_for_id(const date &start, const date &end, const json &type_and_id) {
    std::map<str, int> element_type_table = {
            {"klasse", 1}, {"teacher", 2}, {"subject", 3}, {"room", 4}, {"student", 5}};
    const auto it = type_and_id.begin();

    const str &element_type = it.key();
    const int element_id = it.value().get<int>();

    const json params =
            create_date_param(start, end, {{"id", element_id}, {"type", element_type_table[element_type]}});
    return cached_call<json>(
            "Session.class_reg_event_for_id",
            json::array({format_date(start), format_date(end), type_and_id}),
            [this, params] {
                return rpc_request("getClassregEvents", params);
            });
}

json Session::class_reg_categories() {
    return cached_call<json>("Session.class_reg_categories", json::array(), [this] {
        return rpc_request("getClassregCategories", {});
    });
}

json Session::class_reg_category_groups() {
    return cached_call<json>("Session.class_reg_category_groups", json::array(), [this] {
        return rpc_request("getClassregCategoryGroups", {});
    });
}

[[nodiscard]] TimeTable Session::my_timetable(const date &start, const date &end) {
    if (!my_person_id.has_value() || !my_person_type.has_value()) {
        throw std::runtime_error("Person ID or Type not available. Are you logged in?");
    }
    json options = {{"startDate", format_date(start)},
                    {"endDate", format_date(end)},
                    {"element", {{"id", *my_person_id}, {"type", *my_person_type}}},
                    {"showBooking", true},
                    {"showInfo", true},
                    {"showSubstText", true},
                    {"showLsText", true},
                    {"showLsNumber", true},
                    {"showStudentgroup", true}};

    return cached_call<TimeTable>(
            "Session.my_timetable",
            json::array({format_date(start), format_date(end), {{"id", *my_person_id}, {"type", *my_person_type}}}),
            [this, options] {
                json raw_result;

                try {
                    raw_result = rpc_request("getTimetable", {{"options", options}});
                } catch (const std::exception &e) {
                    my_logger.log_error("Error in getTimetable: " + str(e.what()));
                    return TimeTable({});
                }

                return parse_timetable(raw_result);
            });
}

json Session::_search(const str &surname, const str &fore_name, const int dob, const int what) {
    const json params = {{"sn", surname}, {"fn", fore_name}, {"dob", dob}, {"type", what}};
    return cached_call<json>(
            "Session._search", json::array({surname, fore_name, dob, what}), [this, params] {
                return rpc_request("getPersonId", params);
            });
}

std::map<str, std::variant<str, int, json> > Session::get_student(const str &surname, const str &fore_name,
                                                                  const int dob) {
    json id_val = _search(surname, fore_name, dob, 5);
    if (id_val.empty()) {
        throw std::runtime_error("Student not found");
    }

    using FieldValue = std::variant<str, int, json>;
    using FieldMap = std::map<str, FieldValue>;

    return FieldMap{{"id", FieldValue{id_val}},
                    {"name", FieldValue{surname}},
                    {"longName", FieldValue{surname}},
                    {"foreName", FieldValue{fore_name}}};
}

std::map<str, std::variant<str, int, json> > Session::get_teacher_from_search(const str &surname, const str &fore_name,
                                                                              const int dob) {
    json id_val = _search(surname, fore_name, dob, 2);
    if (id_val.empty()) {
        throw std::runtime_error("Teacher not found");
    }

    using FieldValue = std::variant<str, int, json>;
    using FieldMap = std::map<str, FieldValue>;

    return FieldMap{{"id", FieldValue{id_val}},
                    {"name", FieldValue{surname}},
                    {"longName", FieldValue{surname}},
                    {"foreName", FieldValue{fore_name}},
                    {"title", FieldValue{str{}}}};
}

void Session::multithread_worker(
        std::map<str, TimeTable> &raw_result,
        std::optional<std::tuple<str, std::exception> > &error_result, std::mutex &raw_result_lock,
        const Class &klasse, date start, date end, str function_name, const uuid &call_id,
        int max_attempts) {
    max_attempts = max_attempts > 1 ? max_attempts : 1;

    std::optional<std::map<str, TimeTable> > entry = std::nullopt;
    std::optional<std::tuple<str, std::exception> > error_entry = std::nullopt;

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        try {
            log_in(call_id);

            TimeTable table = timetable_extended(klasse, start, end);

            entry = {{klasse.name, table}};
            error_entry = std::nullopt;

            break;
        } catch (const std::exception &e) {
            my_logger.log_warning(std::format("Attempt {} failed in {}: {}", attempt + 1, function_name, e.what()));

            if (attempt == max_attempts - 1) {
                entry = std::nullopt;
                error_entry = {
                        std::format("{}: []", Config::LanguageConfig::unexpected_error, Logger::current_time()), e};
            } else {
                std::this_thread::sleep_for(std::chrono::duration<double>(0.5 * (attempt + 1)));
            }
        }
    }

    {
        // Enter CR
        std::lock_guard lock(raw_result_lock);

        if (entry.has_value()) {
            for (const auto &[key, value]: *entry) {
                raw_result.insert_or_assign(key, value);
            }
        } else if (error_entry.has_value()) {
            error_result = *error_entry;
        }

        // Leave CR
    }
}

std::variant<std::tuple<str, std::exception>, std::map<str, TimeTable> > Session::multithreading_result(
        float sleep_time, int max_threads, date start, date end, const str &function_name,
        bool logging, const uuid &call_id, bool log_out_afterwards, int max_attempts) {
    std::map<str, TimeTable> raw_result;
    std::optional<std::tuple<str, std::exception> > error_result;
    std::mutex raw_result_lock;

    std::vector<Class> viable_klassen;
    // std::vector<std::thread> threads;

    for (const auto &klasse: all_klassen()) {
        if (klasse.name.length() == 2 && !klasse.name.starts_with("M")) {
            viable_klassen.push_back(klasse);
        }
    }

    max_threads = std::max(max_threads, 1);

    const size_t total_batch_count =
            (viable_klassen.size() + static_cast<size_t>(max_threads) - 1) /
            static_cast<size_t>(max_threads);

    for (size_t batch_start = 0, current_batch_count = 0;
         batch_start < viable_klassen.size();
         batch_start += static_cast<size_t>(max_threads)) {
        const size_t batch_end = std::min(
                batch_start + static_cast<size_t>(max_threads),
                viable_klassen.size()
                );

        std::vector<std::thread> batch_threads;
        batch_threads.reserve(batch_end - batch_start);

        for (size_t i = batch_start; i < batch_end; ++i) {
            batch_threads.emplace_back(
                    &Session::multithread_worker,
                    this,
                    std::ref(raw_result),
                    std::ref(error_result),
                    std::ref(raw_result_lock),
                    viable_klassen[i],
                    start,
                    end,
                    function_name,
                    call_id,
                    max_attempts
                    );
        }

        for (auto &thread: batch_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        ++current_batch_count;

        if (logging) {
            const str percent = total_batch_count == 0
                                    ? "100.0"
                                    : std::format(
                                            "{:.1f}",
                                            100.0 * static_cast<double>(current_batch_count) /
                                            static_cast<double>(total_batch_count)
                                            );

            const size_t filled_length =
                    total_batch_count == 0 ? 50 : 50 * current_batch_count / total_batch_count;

            str bar;
            bar.reserve(filled_length * 3 + (50 - filled_length));

            for (size_t i = 0; i < filled_length; ++i) {
                bar += "█";
            }

            for (size_t i = filled_length; i < 50; ++i) {
                bar += "-";
            }

            std::cout << "\rProgress |" << bar << "| " << percent
                    << "% complete (Batch #" << current_batch_count
                    << " / " << total_batch_count << ")" << std::flush;

            if (current_batch_count == total_batch_count) {
                std::cout << std::endl;
            }
        }

        if (batch_end < viable_klassen.size()) {
            std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time));
        }
    }

    if (log_out_afterwards) {
        log_out(call_id);
    }

    if (error_result.has_value()) {
        return *error_result;
    }

    return raw_result;
}

