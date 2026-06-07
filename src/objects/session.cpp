#include "session.hpp"

#include <cpr/cpr.h>
#include <iostream>
#include <random>

#include "exceptions.hpp"

Session::Session(str session_name, bool use_cache, std::optional<str> cache_file, std::optional<Logger> logger,
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
    if (jsessionid) {
        cookies = cpr::Cookies{{"JSESSIONID", *jsessionid}};
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

int Session::format_date(const date &d) { return stoi(Date_Utils::date_to_str(d, "%Y%m%d")); }

date Session::parse_date(const int d) { return Date_Utils::str_to_date(std::to_string(d), "%Y%m%d"); }

day_time Session::parse_time(const int t) { return Date_Utils::str_to_time(std::to_string(t)); }

std::map<str, str> Session::create_date_param(const date start, const date end, const json &kwargs) {
    if (start > end) {
        throw std::runtime_error("Start date cannot be later than end date.");
    }
    json params = {{"startDate", format_date(start)}, {"endDate", format_date(end)}};
    params.update(kwargs);
    return params;
}

void Session::log_in(uuid unique_id) {
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

        jsessionid = result["sessionId"];
        person_type = result["personType"];
        person_id = result["personId"];
        klasse_id = result["klasseId"];

        logger.log_info(std::format("Logged in ({})!", session_name));
    }
    active_session_uuids.insert(unique_id);
}

void Session::log_out(const uuid &unique_id) {
    active_session_uuids.erase(unique_id);
    if (active_session_uuids.empty() && jsessionid.has_value()) {
        try {
            rpc_request("logout", {});
        } catch (const std::exception &) {
        }
        jsessionid.reset();

        logger.log_info(std::format("Logged out ({})!", session_name));
    }
}

std::vector<Class> Session::all_klassen() {
    std::vector<Class> all_kl;
    for (const auto &k: rpc_request("getKlassen", {})) {
        all_kl.emplace_back(k.value("name", ""), k.value("longName", ""), k.value("id", 0));
    }
    return all_kl;
}

std::vector<Room> Session::all_rooms() {
    std::vector<Room> all_ro;
    for (const auto &r: rpc_request("getRooms", {})) {
        all_ro.emplace_back(r.value("name", ""), r.value("longName", ""), r.value("id", 0));
    }
    return all_ro;
}

std::vector<Subject> Session::all_subjects() {
    std::vector<Subject> all_su;
    for (const auto &s: rpc_request("getSubjects", {})) {
        all_su.emplace_back(s.value("name", ""), s.value("longName", ""), s.value("id", 0));
    }
    return all_su;
}

std::vector<Department> Session::all_departments() {
    std::vector<Department> all_de;
    for (const auto &d: rpc_request("getDepartments", {})) {
        all_de.emplace_back(d.value("name", ""), d.value("longName", ""), d.value("id", 0));
    }
    return all_de;
}

std::vector<Holiday> Session::all_holidays() {
    std::vector<Holiday> all_ho;

    using FieldValue = std::variant<str, int>;
    using RawObj = std::unordered_map<str, FieldValue>;

    for (const auto &h: rpc_request("getHolidays", {})) {
        all_ho.emplace_back(RawObj{{"name", FieldValue{h.value("name", "")}},
                                   {"longName", FieldValue{h.value("longName", "")}},
                                   {"id", FieldValue{h.value("id", 0)}},
                                   {"startDate", FieldValue{h.value("startDate", 0)}},
                                   {"endDate", FieldValue{h.value("startDate", 0)}}});
    }

    return all_ho;
}

std::vector<SchoolYear> Session::all_schoolyears() {
    std::vector<SchoolYear> all_sy;

    using FieldValue = std::variant<str, int>;
    using RawObj = std::unordered_map<str, FieldValue>;

    for (const auto &s: rpc_request("getSchoolyears", {})) {
        all_sy.emplace_back(RawObj{{"name", FieldValue{s.value("name", "")}},
                                   {"longName", FieldValue{s.value("longName", "")}},
                                   {"id", FieldValue{s.value("id", 0)}},
                                   {"startDate", FieldValue{s.value("startDate", 0)}},
                                   {"endDate", FieldValue{s.value("startDate", 0)}}});
    }

    return all_sy;
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
}

std::optional<Class> Session::get_klasse_by_name(const str &name) {
    for (const auto &k: all_klassen()) {
        if (k.name == name) {
            return k;
        }
    }
    return std::nullopt;
}

std::optional<Room> Session::get_room_by_name(const str &name) {
    for (const auto &r: all_rooms()) {
        if (r.name == name) {
            return r;
        }
    }
    return std::nullopt;
}

std::optional<Teacher> Session::get_teacher_by_name(const str &name) {
    try {
        return Teacher::from_teacher_name(name);
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

std::optional<Teacher> Session::get_teacher_by_long_name(const str &name) {
    try {
        return Teacher::from_teacher_long_name(name);
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

TimeTable Session::timetable_extended(const std::variant<Class, Room, Teacher> &element, const date &start,
                                      const date &end) {
    std::map<str, int> element_type_table = {
            {"klasse", 1}, {"teacher", 2}, {"subject", 3}, {"room", 4}, {"student", 5}};
    int element_type;
    int entity_id;

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

    json raw_result;

    try {
        raw_result = rpc_request("getTimetable", {{"options", options}});
    } catch (const std::exception &e) {
        std::cout << "Error in getTimetable" << std::endl;
        std::cout << e.what();
        return TimeTable({});
    }

    std::map<int, json> all_su = {};
    std::map<int, json> all_kl = {};
    std::map<int, json> all_ro = {};

    for (const auto &s: rpc_request("getSubjects", {})) {
        all_su[s["id"]] = s;
    }

    for (const auto &k: rpc_request("getKlassen", {})) {
        all_kl[k["id"]] = k;
    }

    for (const auto &r: rpc_request("getRooms", {})) {
        all_ro[r["id"]] = r;
    }

    std::vector<Period> periods;

    for (const auto &raw_p: raw_result) {
        try {
            datetime start_dt = Date_Utils::combine(parse_date(raw_p["date"]), parse_time(raw_p["startTime"]));
            datetime end_dt = Date_Utils::combine(parse_date(raw_p["date"]), parse_time(raw_p["endTime"]));

            std::vector<Subject> subjects;
            std::vector<Class> klassen;
            std::vector<Room> rooms;
            std::vector<Room> original_rooms;
            std::vector<Teacher> teachers;
            std::vector<Teacher> original_teachers;

            for (const auto &s: raw_p["su"]) {
                auto master_su = all_su.contains(s["id"]) ? all_su.at(s["id"]) : json{};

                subjects.emplace_back(master_su["name"], master_su["longName"], s["id"]);
            }

            for (const auto &k: raw_p["kl"]) {
                auto master_kl = all_kl.contains(k["id"]) ? all_kl.at(k["id"]) : json{};

                klassen.emplace_back(master_kl["name"], master_kl["longName"], k["id"]);
            }

            for (const auto &r: raw_p["ro"]) {
                int rid = 0;
                if (r.contains("id")) {
                    rid = r["id"];
                }

                auto master_ro = all_ro.contains(rid) ? all_ro.at(rid) : json{};

                rooms.emplace_back(master_ro["name"], master_ro["longName"], rid);
            }

            for (const auto &r: raw_p["ro"]) {
                if (!r.contains("orgid")) {
                    continue;
                }
                int org_id = r["orgid"];

                auto master_ro = all_ro.contains(org_id) ? all_ro.at(org_id) : json{};

                original_rooms.emplace_back(master_ro["name"], master_ro["longName"], org_id);
            }

            for (const auto &t: raw_p["te"]) {
                if (t.contains("id")) {
                    teachers.emplace_back(Teacher::from_teacher_id(t["id"]));
                }
                if (t.contains("orgid")) {
                    teachers.emplace_back(Teacher::from_teacher_id(t["orgid"]));
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

json Session::teachers() { return rpc_request("getTeachers", {}); }

json Session::statusdata() { return rpc_request("getStatusData", {}); }

int Session::last_import_time() { return rpc_request("getLatestImportTime", {}); }

json Session::substitutions(const date &start, const date &end, const int department_id) {
    const json params = create_date_param(start, end, {{"departmentId", department_id}});
    return rpc_request("getSubstitutions", params);
}

[[nodiscard]] std::vector<str> Session::timegrid_units() {
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

                lesson_time_ranges.push_back(std::format("{} - {}", convert_time(start_time), convert_time(end_time)));
            }
        }
    }

    return lesson_time_ranges;
}

json Session::students() { return rpc_request("getStudents", {}); }

json Session::exam_types() { return rpc_request("getExamTypes", {}); }

json Session::exams(const date &start, const date &end, const int exam_type_id) {
    const json params = create_date_param(start, end, {{"examTypeId", exam_type_id}});
    return rpc_request("getExams", params);
}

json Session::timetable_with_absences(const date &start, const date &end) {
    const json params = {{"options", create_date_param(start, end, {})}};
    return rpc_request("getTimetableWithAbsences", params);
}

json Session::class_reg_events(const date &start, const date &end) {
    const json params = create_date_param(start, end, {});
    return rpc_request("getClassregEvents", params);
}

json Session::class_reg_event_for_id(const date &start, const date &end, const json &type_and_id) {
    std::map<str, int> element_type_table = {
            {"klasse", 1}, {"teacher", 2}, {"subject", 3}, {"room", 4}, {"student", 5}};
    const auto it = type_and_id.begin();

    const str &element_type = it.key();
    const int element_id = it.value().get<int>();

    const json &params =
            create_date_param(start, end, {{"id", element_id}, {"type", element_type_table[element_type]}});
    return rpc_request("getClassregEvents", params);
}

json Session::class_reg_categories() { return rpc_request("getClassregCategories", {}); }

json Session::class_reg_category_groups() { return rpc_request("getClassregCategoryGroups", {}); }

[[nodiscard]] TimeTable Session::my_timetable(const date &start, const date &end) {
    // TODO: IMPLEMENT HERE
    if (!person_id.has_value() || !person_type.has_value()) {
        throw std::runtime_error("Person ID or Type not available. Are you logged in?");
    }
    json options = {{"startDate", format_date(start)},
                    {"endDate", format_date(end)},
                    {"element", {{"id", person_id}, {"type", person_type}}},
                    {"showBooking", true},
                    {"showInfo", true},
                    {"showSubstText", true},
                    {"showLsText", true},
                    {"showLsNumber", true},
                    {"showStudentgroup", true}};

    json raw_result;

    try {
        raw_result = rpc_request("getTimetable", {{"options", options}});
    } catch (const std::exception &e) {
        std::cout << "Error in getTimetable" << std::endl;
        std::cout << e.what();
        return TimeTable({});
    }

    std::map<int, json> all_su = {};
    std::map<int, json> all_kl = {};
    std::map<int, json> all_ro = {};

    for (const auto &s: rpc_request("getSubjects", {})) {
        all_su[s["id"]] = s;
    }

    for (const auto &k: rpc_request("getKlassen", {})) {
        all_kl[k["id"]] = k;
    }

    for (const auto &r: rpc_request("getRooms", {})) {
        all_ro[r["id"]] = r;
    }

    std::vector<Period> periods;

    for (const auto &raw_p: raw_result) {
        try {
            datetime start_dt = Date_Utils::combine(parse_date(raw_p["date"]), parse_time(raw_p["startTime"]));
            datetime end_dt = Date_Utils::combine(parse_date(raw_p["date"]), parse_time(raw_p["endTime"]));

            std::vector<Subject> subjects;
            std::vector<Class> klassen;
            std::vector<Room> rooms;
            std::vector<Room> original_rooms;
            std::vector<Teacher> teachers;
            std::vector<Teacher> original_teachers;

            for (const auto &s: raw_p["su"]) {
                auto master_su = all_su.contains(s["id"]) ? all_su.at(s["id"]) : json{};

                subjects.emplace_back(master_su["name"], master_su["longName"], s["id"]);
            }

            for (const auto &k: raw_p["kl"]) {
                auto master_kl = all_kl.contains(k["id"]) ? all_kl.at(k["id"]) : json{};

                klassen.emplace_back(master_kl["name"], master_kl["longName"], k["id"]);
            }

            for (const auto &r: raw_p["ro"]) {
                int rid = 0;
                if (r.contains("id")) {
                    rid = r["id"];
                }

                auto master_ro = all_ro.contains(rid) ? all_ro.at(rid) : json{};

                rooms.emplace_back(master_ro["name"], master_ro["longName"], rid);
            }

            for (const auto &r: raw_p["ro"]) {
                if (!r.contains("orgid")) {
                    continue;
                }
                int org_id = r["orgid"];

                auto master_ro = all_ro.contains(org_id) ? all_ro.at(org_id) : json{};

                original_rooms.emplace_back(master_ro["name"], master_ro["longName"], org_id);
            }

            for (const auto &t: raw_p["te"]) {
                if (t.contains("id")) {
                    teachers.emplace_back(Teacher::from_teacher_id(t["id"]));
                }
                if (t.contains("orgid")) {
                    teachers.emplace_back(Teacher::from_teacher_id(t["orgid"]));
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

json Session::_search(const str &surname, const str &fore_name, const int dob, const int what) {
    const json &params = {{"sn", surname}, {"fn", fore_name}, {"dob", dob}, {"type", what}};
    return rpc_request("getPersonId", params);
}

std::map<str, std::variant<str, int, json>> Session::get_student(const str &surname, const str &fore_name,
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

std::map<str, std::variant<str, int, json>> Session::get_teacher_from_search(const str &surname, const str &fore_name,
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

void Session::multithread_worker(std::map<str, std::variant<str, std::exception, std::map<str, TimeTable>>> &raw_result,
                                 std::mutex &raw_result_lock, const Class &klasse, str raw_date, date start, date end,
                                 str function_name, uuid call_id, int max_attempts) {
    max_attempts = max_attempts > 1 ? max_attempts : 1;

    std::optional<std::map<str, std::map<str, TimeTable>>> entry = std::nullopt;
    std::optional<std::map<str, std::variant<str, std::exception, std::map<str, TimeTable>>>> error_entry =
            std::nullopt;

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        try {
            log_in(call_id);
            TimeTable table = timetable_extended(klasse, start, end);
            entry = {{klasse.name, {{"table", table}}}};
            error_entry = std::nullopt;
        } catch (const std::exception &e) {
            my_logger.log_warning(std::format("Attempt {} failed in {}: {}", attempt + 1, function_name, e.what()));

            if (attempt == max_attempts - 1) {
                entry = std::nullopt;
                error_entry = {{"error", std::format("{}: []", Config::LanguageConfig::unexpected_error,
                                                     my_logger.current_time())},
                               {"exception", e}};
            } else {
                std::this_thread::sleep_for(std::chrono::duration<double>(0.5 * (attempt + 1)));
                continue;
            }
        }
    }

    {
        // Enter CR
        std::lock_guard<std::mutex> lock(raw_result_lock);

        if (entry.has_value()) {
            for (const auto &[key, value]: *entry) {
                raw_result[key] = value;
            }
        } else if (error_entry.has_value()) {
            for (const auto &[key, value]: *error_entry) {
                raw_result[key] = value;
            }
        }

        // Leave CR
    }
}

std::map<str, std::variant<str, std::exception, std::map<str, TimeTable>>>
Session::multithreading_result(float sleep_time, int max_threads, str raw_date, date start, date end, str function_name,
                               bool logging, uuid call_id, bool log_out_afterwards, int max_attempts) {
    std::map<str, std::variant<str, std::exception, std::map<str, TimeTable>>> raw_result;
    std::mutex raw_result_lock;
    std::vector<Class> klassen_list = all_klassen();
    std::vector<Class> viable_klassen;
    std::vector<std::thread> threads;

    for (const auto &klasse: klassen_list) {
        if (klasse.name.length() == 2 && !klasse.name.starts_with("M")) {
            viable_klassen.push_back(klasse);
        }
    }

    if (logging) {
        size_t current_batch_count = 0;
        size_t total_batch_count = (viable_klassen.size() + max_threads - 1) / max_threads;

        for (size_t i = 0; i < viable_klassen.size(); i += max_threads) {
            // TODO: fix-up needed with += max_threads or not? probs this is fine.
            size_t batch_end = std::min(i + max_threads, viable_klassen.size());

            for (size_t j = i; j < batch_end; j++) {
                const Class &klasse = viable_klassen[j];

                threads.push_back(std::thread(&Session::multithread_worker, this, std::ref(raw_result),
                                              std::ref(raw_result_lock), klasse, raw_date, start, end, function_name,
                                              call_id, max_attempts));
            }

            ++current_batch_count;

            for (size_t j = i; j < batch_end; j++) {
                auto &thread = threads.at(j);
                if (thread.joinable()) {
                    thread.join();
                }
            }

            const str percent =
                    std::format("{:.1f}", 100.0 * static_cast<double>(current_batch_count) / total_batch_count);
            const int filled_length = 50 * current_batch_count / total_batch_count;
            str bar;
            bar.reserve(filled_length * 3); // █ is 3 bytes in UTF-8
            bar.reserve(50 - filled_length); // - is 1 byte in UTF-8
            for (int j = 0; j < filled_length; ++j) {
                bar += "█";
            }
            for (int j = 0; j < 50 - filled_length; ++j) {
                bar += "-";
            }

            std::cout << "\rProgress |" << bar << "| " << percent << "% complete (Batch #" << current_batch_count
                      << " / " << total_batch_count << ")" << std::flush;

            if (current_batch_count == total_batch_count) {
                std::cout << std::endl;
            }

            if (i + max_threads < threads.size()) {
                std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time));
            }
        }

        if (log_out_afterwards) {
            log_out(call_id);
        }

        return raw_result;
    }

    for (int i = 0; i < viable_klassen.size(); ++i) {
        if ((i + 1) % max_threads == 0) {
            std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time));
        }

        const Class &klasse = viable_klassen[i];

        threads.push_back(std::thread(&Session::multithread_worker, this, std::ref(raw_result),
                                      std::ref(raw_result_lock), klasse, raw_date, start, end, function_name, call_id,
                                      max_attempts));
    }

    for (auto &thread: threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    if (log_out_afterwards) {
        log_out(call_id);
    }

    return raw_result;
}
