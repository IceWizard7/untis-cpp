#include "objects.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <ranges>
#include <semaphore>
#include <unordered_set>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <bitset>

#include "config.hpp"
#include "exceptions.hpp"

day_time Date_Utils::datetime_to_time(const datetime date_time) {
    return std::chrono::seconds{date_time - std::chrono::floor<std::chrono::days>(date_time)};
}

date Date_Utils::datetime_to_date(const datetime date_time) {
    return date{
        std::chrono::floor<std::chrono::days>(date_time)
    };
}

str Date_Utils::datetime_to_str(const datetime& tp, const str& format) {
    const auto t = std::chrono::system_clock::to_time_t(tp);

    const std::tm tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, format.c_str());

    return oss.str();
}

str Date_Utils::date_to_str(const date& d, const str& format) {
    // Convert year_month_day -> sys_days -> time_point
    const std::chrono::sys_days sd{d};

    const std::time_t t = std::chrono::system_clock::to_time_t(sd);

    const std::tm tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, format.c_str());

    return oss.str();
}

str Date_Utils::daytime_to_str(const day_time& t, const str& format) {
    std::tm tm{};
    tm.tm_hour = static_cast<int>(std::chrono::duration_cast<std::chrono::hours>(t).count());
    tm.tm_min  = static_cast<int>(std::chrono::duration_cast<std::chrono::minutes>(t).count() % 60);
    tm.tm_sec  = static_cast<int>(t.count() % 60);

    std::ostringstream oss;
    oss << std::put_time(&tm, format.c_str());

    return oss.str();
}

date Date_Utils::add_days(const date& d, const int days) {
    return date{std::chrono::sys_days(d) + std::chrono::days(days)};
}

date Date_Utils::add_weeks(const date& d, const int weeks) {
    return date{std::chrono::sys_days{d} + std::chrono::weeks{weeks}};
}

date Date_Utils::get_today() {
    return std::chrono::year_month_day{
        std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())
    };
}

date Date_Utils::str_to_date(const str& s, const str& format) {
    std::tm tm{};
    std::istringstream ss(s);

    ss >> std::get_time(&tm, format.c_str());

    if (ss.fail()) {
        throw std::runtime_error("Date parse failed");
    }

    return std::chrono::year{tm.tm_year + 1900}
    / std::chrono::month{static_cast<unsigned>(tm.tm_mon + 1)}
    / std::chrono::day{static_cast<unsigned>(tm.tm_mday)};
}

day_time Date_Utils::str_to_daytime(const str& s, const str& format) {
    std::tm tm{};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, format.c_str());

    return std::chrono::hours{tm.tm_hour}
    + std::chrono::minutes{tm.tm_min}
    + std::chrono::seconds{tm.tm_sec};
}

day_time Date_Utils::str_to_time(str t){
    if (t.length() < 4) {
        t = str(4 - t.length(), '0') + t;
    }

    const int hour = std::stoi(t.substr(0, 2));
    const int minute = std::stoi(t.substr(2, 2));

    return std::chrono::seconds{
        std::chrono::hours{hour} + std::chrono::minutes{minute}
    };
}

datetime Date_Utils::combine(const date& d, const day_time& t) {
    std::chrono::sys_days days{d};

    return std::chrono::time_point_cast<std::chrono::seconds>(
        days + t
    );
}

int Date_Utils::weekday(const date& d) {
    std::chrono::weekday wd{
        std::chrono::sys_days{d}
    };

    // Monday == 0; Sunday == 6
    return (wd.c_encoding() + 6) % 7;
}

size_t Date_Utils::operator()(const date& d) const {
    return std::hash<int>{}(static_cast<int>(d.year()))
         ^ std::hash<unsigned>{}(static_cast<unsigned>(d.month()))
         ^ std::hash<unsigned>{}(static_cast<unsigned>(d.day()));
}

size_t Date_Utils::operator()(const day_time& d) const {
    return std::hash<long>{}(d.count());
}

size_t Date_Utils::operator()(const datetime& dt) const {
    return std::hash<long>{}(dt.time_since_epoch().count());
}


template <typename R1, typename R2>
std::vector<typename R1::value_type> Vector_Utils::concat_ranges(const R1& a, const R2& b) {
    std::vector<typename R1::value_type> out;
    out.reserve(a.size() + b.size());

    out.insert(out.end(), a.begin(), a.end());
    out.insert(out.end(), b.begin(), b.end());

    return out;
}

template <HasToString T>
str Vector_Utils::vector_to_string(const std::vector<T>& vec) {
    str result = "[";
    bool first = true;

    for (const auto& x : vec) {
        if (!first) result += ", ";
        result += x.to_string();
        first = false;
    }

    result += "]";
    return result;
}

template <typename T>
bool Vector_Utils::contains_value(const std::vector<T>& vec, const T& val) {
    return std::ranges::find(vec, val) != vec.end();
}

template<typename Container, typename Projection>
str Str_Utils::join(const str& sep, const Container& items, Projection proj) {
    str result;
    bool first = true;
    for (const auto& item : items) {
        if (!first) result += sep;
        if constexpr (std::is_member_pointer_v<Projection>) {
            result += item.*proj;
        } else {
            result += proj(item);
        }
        first = false;
    }
    return result;
}

template<typename Container>
str Str_Utils::join(const str& sep, const Container& items) {
    str result;
    bool first = true;
    for (const auto& item : items) {
        if (!first) result += sep;
        result += item;
        first = false;
    }
    return result;
}

template<typename Container>
str Str_Utils::join(const Container& items) {
    return join("", items);
}

Base_Entity::Base_Entity(str n, str ln, const int id)
: name(std::move(n)), long_name(std::move(ln)), entity_id(id) {}

Base_Entity::~Base_Entity() = default;

[[nodiscard]] str Base_Entity::to_string(const str& class_name) const {
    return class_name + "(name=" + name
        + ", long_name=" + long_name
        + ", entity_id=" + std::to_string(entity_id)
        + ")";
}

[[nodiscard]] str Base_Entity::to_string() const {
    return to_string("Base_Entity");
}

bool Base_Entity::operator==(const Base_Entity& other) const {
    return name == other.name &&
        long_name == other.long_name &&
        entity_id == other.entity_id;
}

std::ostream& Base_Entity::print(std::ostream& os) const {
    os << to_string();
    return os;
}


Subject::Subject(str n, str ln, const int i) : Base_Entity(std::move(n), std::move(ln), i) {}
Subject::~Subject() = default;

std::tuple<int, int, int> Subject::color() {
    const std::tuple<str, str, int> key = std::make_tuple(name, long_name, entity_id);

    const auto& map = my_config.timetable_mapping_config.subject_to_color;
    if (const auto it = map.find(key); it != map.end()) {
        return it->second;
    }

    return my_config.timetable_mapping_config.default_subject_color;
}

[[nodiscard]] str Subject::to_string() const {
    return Base_Entity::to_string("Subject");
}

Class::Class(str n, str ln, const int i) : Base_Entity(std::move(n), std::move(ln), i) {}
Class::~Class() = default;

[[nodiscard]] str Class::to_string() const {
    return Base_Entity::to_string("Class");
}

Room::Room(str n, str ln, const int i) : Base_Entity(std::move(n), std::move(ln), i) {}
Room::~Room() = default;

[[nodiscard]] str Room::to_string() const {
    return Base_Entity::to_string("Room");
}

Teacher::Teacher(str n, str ln, const int i): Base_Entity(std::move(n), std::move(ln), i) {}
Teacher::~Teacher() = default;

[[nodiscard]] std::variant<std::unordered_set<str>, str> Teacher::subjects() const {
    const auto* map = &my_config.timetable_mapping_config.teacher_mapping;

    if (const auto it = map->find(entity_id); it != map->end()) {
        return std::get<2>(it->second);
    }

    return {std::to_string(entity_id)};
}

[[nodiscard]] str Teacher::get_name(const int raw_teacher_id) {
    const auto* map = &my_config.timetable_mapping_config.teacher_mapping;

    if (const auto it = map->find(raw_teacher_id); it != map->end()) {
        return std::get<1>(it->second);
    }

    return {std::to_string(raw_teacher_id)};
}

[[nodiscard]] str Teacher::get_long_name(const int raw_teacher_id) {
    const auto* map = &my_config.timetable_mapping_config.teacher_mapping;

    if (const auto it = map->find(raw_teacher_id); it != map->end()) {
        return std::get<0>(it->second);
    }

    return {std::to_string(raw_teacher_id)};
}

[[nodiscard]] Teacher Teacher::from_teacher_id(int raw_teacher_id) {
    return {
        get_name(raw_teacher_id),
        get_long_name(raw_teacher_id),
        raw_teacher_id
    };
}

[[nodiscard]] Teacher Teacher::from_teacher_name(const str& raw_teacher_name) {
    // Default: 0 -> unknown teacher
    int raw_teacher_id = 0;

    for (const auto& [tid, value] : my_config.timetable_mapping_config.teacher_mapping) {
        if (std::get<1>(value) == raw_teacher_name) {
            raw_teacher_id = tid;
            break;
        }
    }

    return {
        get_name(raw_teacher_id),
        get_long_name(raw_teacher_id),
        raw_teacher_id,
    };
}

[[nodiscard]] Teacher Teacher::from_teacher_long_name(const str &raw_teacher_long_name) {
    // Default: 0 -> unknown teacher
    int raw_teacher_id = 0;

    for (const auto& [tid, value] : my_config.timetable_mapping_config.teacher_mapping) {
        if (std::get<0>(value) == raw_teacher_long_name) {
            raw_teacher_id = tid;
            break;
        }
    }

    return {
        get_name(raw_teacher_id),
        get_long_name(raw_teacher_id),
        raw_teacher_id
    };
}

[[nodiscard]] str Teacher::to_string() const {
    return Base_Entity::to_string("Teacher");
}

Department::Department(str n, str ln, const int i) : Base_Entity(std::move(n), std::move(ln), i) {}
Department::~Department() = default;

[[nodiscard]] str Department::to_string() const {
    return Base_Entity::to_string("Department");
}

Base_Date_Entity::Base_Date_Entity(str n, str ln, int id, date start_date, date end_date)
: name(std::move(n)), long_name(std::move(ln)), entity_id(id), start_date(start_date), end_date(end_date) {}

Base_Date_Entity::~Base_Date_Entity() = default;

std::optional<date> Base_Date_Entity::parse_date(const std::optional<int>& value) {
    if (!value.has_value()) {
        return std::nullopt;
    }

    const str s = std::to_string(*value);
    if (s.size() != 8) {
        // Invalid format
        return std::nullopt;
    }

    const int year       = std::stoi(s.substr(0, 4));
    const unsigned month = std::stoi(s.substr(4, 2));
    const unsigned day   = std::stoi(s.substr(6, 2));

    date ymd{
        std::chrono::year{year},
        std::chrono::month{static_cast<unsigned>(month)},
        std::chrono::day{static_cast<unsigned>(day)}
    };

    if (!ymd.ok()) {
        return std::nullopt;
    }

    return ymd;
}

Base_Date_Entity::Base_Date_Entity(const std::unordered_map<str, std::variant<str, int>>& raw_obj)
    : name(std::get<str>(raw_obj.at("name"))),
    long_name(std::get<str>(raw_obj.at("longName"))),
    entity_id(std::get<int>(raw_obj.at("id"))),
    start_date(parse_date(std::get<int>(raw_obj.at("startDate")))),
    end_date(parse_date(std::get<int>(raw_obj.at("endDate")))) {}

Period::Period(
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
)
: raw_period_code(std::move(raw_period_code)),
  start(start),
  end(end),
  subjects(std::move(subjects)),
  klassen(std::move(klassen)),
  rooms(std::move(rooms)),
  original_rooms(std::move(original_rooms)),
  teachers(std::move(teachers)),
  original_teachers(std::move(original_teachers)),
  student_group(std::move(student_group)),
  activity_type(std::move(activity_type)),
  bk_remark(std::move(bk_remark)),
  bk_text(std::move(bk_text)),
  flags(std::move(flags)),
  ls_number(ls_number),
  ls_text(std::move(ls_text)),
  subst_text(std::move(subst_text)),
  period_type(std::move(period_type)),
  period_id(period_id)
{}

Period::~Period() = default;

[[nodiscard]] str Period::period_code_class(const Class& klassen_object) const {
    // Base it on the raw_period_code

    if (!raw_period_code.has_value() || raw_period_code == "regular") {
        return "regular";
    }
    if (raw_period_code == "cancelled") {
        return "missed";
    }
    if (raw_period_code == "irregular") {
        return "extra";
    }

    return "regular";
}

str Period::period_code_room(const Room& room_object) const {
    // Regular: period.original_rooms = []
    // Missed: room_object in period.original_rooms and room_object not in period.rooms
    // Extra: room_object not in period.original_rooms and room_object in period.rooms

    if (raw_period_code == "cancelled") {
        return "missed";
    }
    if (original_rooms.empty() && Vector_Utils::contains_value(rooms, room_object)) {
        if (raw_period_code == "irregular") {
            return "extra";
        }
        return "regular";
    }
    if (Vector_Utils::contains_value(original_rooms, room_object) && Vector_Utils::contains_value(rooms, room_object)) {
        return "regular";
    }
    if (Vector_Utils::contains_value(original_rooms, room_object) and !Vector_Utils::contains_value(rooms, room_object)) {
        return "missed";
    }
    if (!Vector_Utils::contains_value(original_rooms, room_object) && Vector_Utils::contains_value(rooms, room_object)) {
        return "extra";
    }
    return "regular";
}

str Period::period_code_teacher(const Teacher& teacher_object) const {
    // Regular: period.original_teachers = []
    // Missed: teacher_object in period.original_teachers and teacher_object not in period.teachers
    // Extra: teacher_object not in period.original_teachers and teacher_object in period.teachers

    if (raw_period_code == "cancelled") {
        return "missed";
    }

    if (original_teachers.empty() && Vector_Utils::contains_value(teachers, teacher_object)) {
        if (raw_period_code == "irregular") {
            return "extra";
        }
        return "regular";
    }

    if (Vector_Utils::contains_value(original_teachers, teacher_object) && Vector_Utils::contains_value(teachers, teacher_object)) {
        return "regular";
    }

    if (Vector_Utils::contains_value(original_teachers, teacher_object) && Vector_Utils::contains_value(teachers, teacher_object)) {
        return "missed";
    }

    if (!Vector_Utils::contains_value(original_teachers, teacher_object) && Vector_Utils::contains_value(teachers, teacher_object)) {
        return "extra";
    }
    return "regular";
}

[[nodiscard]] std::pair<str, std::pair<bool, bool>> Period::get_period_code(const std::variant<Class, Room, Teacher>& featuring_object) const {
    // "regular" / "missed" / "extra"
    str period_code = "regular";
    std::pair<bool, bool> rows_changed{false, false};

    bool klasse_changed = false;
    bool room_changed = false;
    bool teacher_changed = false;

    for (const auto& k : klassen) {
        if (period_code_class(k) != "regular") {
            klasse_changed = true;
            break;
        }
    }

    for (const auto& r : Vector_Utils::concat_ranges(rooms, original_rooms)) {
        if (period_code_room(r) != "regular") {
            klasse_changed = true;
            break;
        }
    }

    for (const auto& t : Vector_Utils::concat_ranges(teachers, original_teachers)) {
        if (period_code_teacher(t) != "regular") {
            klasse_changed = true;
            break;
        }
    }

    if (const Class* c_ptr = std::get_if<Class>(&featuring_object)) {
        period_code = period_code_class(*c_ptr);
        rows_changed = {teacher_changed, room_changed};
    } else if (const Room* r_ptr = std::get_if<Room>(&featuring_object)) {
        period_code = period_code_room(*r_ptr);
        rows_changed = {teacher_changed, klasse_changed};
    } else if (const Teacher* t_ptr = std::get_if<Teacher>(&featuring_object)) {
        period_code = period_code_teacher(*t_ptr);
        rows_changed = {room_changed, klasse_changed};
    }

    return {period_code, rows_changed};
}

[[nodiscard]] str Period::subjects_str() const {
    str subject_str = Str_Utils::join(", ", subjects, &Subject::name);

    if (subject_str.empty()) {
        return my_config.html_style_config.unknown_element_symbol;
    }

    return subject_str;
}

[[nodiscard]] str Period::room_str(const bool regular_plan) const {
    str room_str = Str_Utils::join(", ", rooms, &Room::name);
    const str original_room_str = Str_Utils::join(", ", original_rooms, &Room::name);

    if (regular_plan && !original_room_str.empty()) {
        room_str = original_room_str;
    }

    if (room_str.empty()) {
        return my_config.html_style_config.unknown_element_symbol;
    }

    return room_str;
}

[[nodiscard]] str Period::teacher_str(const bool regular_plan) const {
    str teacher_str = Str_Utils::join(", ", teachers, &Teacher::name);
    const str original_teacher_str = Str_Utils::join(", ", original_teachers, &Teacher::name);

    if (regular_plan && !original_teacher_str.empty()) {
        teacher_str = original_teacher_str;
    }

    if (teacher_str.empty()) {
        return my_config.html_style_config.unknown_element_symbol;
    }

    return teacher_str;
}

[[nodiscard]] str Period::klassen_str() const {
    std::map<char, std::unordered_set<str>> klassen_level_to_letter;

    for (const auto& klasse : klassen) {
        char klassen_level = klasse.name.at(0);
        str klassen_letter = klasse.name.substr(1);

        klassen_level_to_letter[klassen_level].insert(klassen_letter);
    }

    std::vector<str> parts;

    for (const auto& [klassen_level, letters] : klassen_level_to_letter) {
        str part;
        part += klassen_level;

        std::vector<str> sorted_letters(letters.begin(), letters.end());
        std::ranges::sort(sorted_letters);

        for (const auto& letter : sorted_letters) {
            part += letter;
        }

        parts.push_back(std::move(part));
    }

    str klassen_str;

    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) klassen_str += ", ";
        klassen_str += parts[i];
    }

    if (klassen_str.empty()) {
        return my_config.html_style_config.unknown_element_symbol;
    }

    return klassen_str;
}

std::tuple<str, str, str, datetime, datetime> Period::formatted_list_class(const bool regular_plan) const {
    return std::make_tuple(subjects_str(), teacher_str(regular_plan), room_str(regular_plan), start, end);
}

std::tuple<str, str, str, datetime, datetime> Period::formatted_list_room(const bool regular_plan) const {
    return std::make_tuple(subjects_str(), teacher_str(regular_plan), klassen_str(), start, end);
}

std::tuple<str, str, str, datetime, datetime> Period::formatted_list_teacher(const bool regular_plan) const {
    return std::make_tuple(subjects_str(), room_str(regular_plan), klassen_str(), start, end);
}

std::tuple<str, str, str, datetime, datetime> Period::formatted_list(const std::variant<Class, Room, Teacher>& featuring_object, const bool regular_plan) const {
    if (std::holds_alternative<Class>(featuring_object)) {
        return formatted_list_class(regular_plan);
    }
    if (std::holds_alternative<Room>(featuring_object)) {
        return formatted_list_room(regular_plan);
    }
    if (std::holds_alternative<Teacher>(featuring_object)) {
        return formatted_list_teacher(regular_plan);
    }
    return std::make_tuple("", "", "", start, end);
}

str Period::formatted_string(const std::variant<Class, Room, Teacher>& featuring_object, const bool regular_plan) const {
    auto [row1, row2, row3, p_start, p_end] = formatted_list(featuring_object, regular_plan);

    return row1 + " " + row2 + " " + row3;
}

str Period::formatted_string_with_date_part(const std::variant<Class, Room, Teacher>& featuring_object, const bool regular_plan) const {
    auto [row1, row2, row3, p_start, p_end] = formatted_list(featuring_object, regular_plan);
    return row1 + " " + row2 + " " + row3 + ": " + Date_Utils::datetime_to_str(p_start, "%Y-%m-%d %H:%M:%S") + " - " + Date_Utils::datetime_to_str(p_end, "%Y-%m-%d %H:%M:%S");
}

std::tuple<int, day_time, day_time, std::vector<Subject>, std::vector<Class>, std::vector<Room>, std::vector<Teacher>> Period::regular_plan_identifier() const {
    unsigned int weekday = std::chrono::weekday{std::chrono::sys_days{std::chrono::floor<std::chrono::days>(start)}}.c_encoding();
    auto start_time = Date_Utils::datetime_to_time(start);
    auto end_time = Date_Utils::datetime_to_time(end);

    std::vector<Room> regular_rooms = rooms;
    std::vector<Teacher> regular_teachers = teachers;

    if (!original_rooms.empty()) {
        regular_rooms = original_rooms;
    }

    if (!original_teachers.empty()) {
        regular_teachers = original_teachers;
    }

    return std::make_tuple(weekday, start_time, end_time, subjects, klassen, regular_rooms, regular_teachers);
}

[[nodiscard]] str Period::to_string() const {
    return std::format(
        "Period(start={}, end={}, subjects={}, klassen={}, rooms={}, original_rooms={}, teachers={}, original_teachers={})",
        Date_Utils::datetime_to_str(start, "%Y-%m-%d %H:%M:%S"),
        Date_Utils::datetime_to_str(end,   "%Y-%m-%d %H:%M:%S"),
        Vector_Utils::vector_to_string(subjects),
        Vector_Utils::vector_to_string(klassen),
        Vector_Utils::vector_to_string(rooms),
        Vector_Utils::vector_to_string(original_rooms),
        Vector_Utils::vector_to_string(teachers),
        Vector_Utils::vector_to_string(original_teachers)
    );
}

bool Period::operator==(const Period& other) const {
    return start == other.start &&
        end == other.end &&
        subjects == other.subjects &&
        raw_period_code == other.raw_period_code &&
        klassen == other.klassen &&
        rooms == other.rooms &&
        original_rooms == other.original_rooms &&
        teachers == other.teachers &&
        original_teachers == other.original_teachers &&
        student_group == other.student_group &&
        activity_type == other.activity_type &&
        bk_remark == other.bk_remark &&
        bk_text == other.bk_text &&
        flags == other.flags &&
        ls_number == other.ls_number &&
        ls_text == other.ls_text &&
        subst_text == other.subst_text &&
        period_type == other.period_type &&
        period_id == other.period_id;
}

TimeTable::TimeTable(std::vector<Period> periods_) : periods(std::move(periods_)) {}

TimeTable::~TimeTable() = default;

[[nodiscard]] TimeTable TimeTable::copy_by_date_range(const date start_date, const date end_date) const {
    std::vector<Period> new_periods;

    for (const auto& p : periods) {
        auto this_start = Date_Utils::datetime_to_date(p.start);
        auto this_end   = Date_Utils::datetime_to_date(p.end);

        if (start_date <= this_start && this_start <= end_date &&
            start_date <= this_end && this_end <= end_date) {
                new_periods.push_back(p);
        }
    }
    return TimeTable(std::move(new_periods));
}

/// Keep any period that stores that subject in the period (modify TimeTable in place)
void TimeTable::filter_hours_by_subject(const Subject& subject) {
    std::erase_if(periods, [&](const Period& p) {
        return !Vector_Utils::contains_value(p.subjects, subject);
    });
}

/// Keep any period that stores that class in the period (modify TimeTable in place)
void TimeTable::filter_hours_by_class(const Class& klasse) {
    std::erase_if(periods, [&](const Period& p) {
        return !Vector_Utils::contains_value(p.klassen, klasse);
    });
}

/// Keep any period that stores that room in the period (modify TimeTable in place)
void TimeTable::filter_hours_by_room(const Room& room) {
    std::erase_if(periods, [&](const Period& p) {
        return !Vector_Utils::contains_value(Vector_Utils::concat_ranges(p.rooms, p.original_rooms), room);
    });
}

/// Keep any period that stores that teacher in the period (modify TimeTable in place)
void TimeTable::filter_hours_by_teacher(const Teacher& teacher) {
    std::erase_if(periods, [&](const Period& p) {
        return !Vector_Utils::contains_value(Vector_Utils::concat_ranges(p.teachers, p.original_teachers), teacher);
    });
}

// Keep any period that personal attends (modify TimeTable in place)
void TimeTable::filter_hours_by_personal(const str& name) {
    std::unordered_set<str> personal_teachers;
    std::unordered_set<str> personal_subjects;

    const auto it = my_config.timetable_mapping_config.personal_timetable_entries.find(name);

    if (it != my_config.timetable_mapping_config.personal_timetable_entries.end()) {
        const auto& [teachers, subjects] = it->second;
        personal_teachers = teachers;
        personal_subjects = subjects;
    } else {
        personal_teachers = {};
        personal_subjects = {};
    }

    std::erase_if(periods, [&](const Period& p) {
        std::vector<str> teacher_names;
        teacher_names.reserve(p.teachers.size() + p.original_teachers.size());

        for (const auto& t : p.teachers)
            teacher_names.push_back(t.name);

        for (const auto& t : p.original_teachers)
            teacher_names.push_back(t.name);

        const bool teacher_match = std::ranges::any_of(teacher_names,
                                                       [&](const str& t_name) {
                                                           return personal_teachers.contains(t_name);
                                                       });

        const bool subject_match = std::ranges::any_of(p.subjects,
                                                       [&](const Subject& s) {
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
    for (char &c : base_string) {
        if (c == '.') c = ',';
    }

    // Also adapt whole number (shouldn't contain '.', but consistent)
    for (char &c : base_string_whole_number) {
        if (c == '.') c = ',';
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

[[nodiscard]] std::tuple<int, int, int, int> TimeTable::get_statistics(
    date start_date,
    date end_date,
    const std::variant<Class, Room, Teacher>& featuring_object,
    const std::vector<Class>& filtered_class_objects,
    bool filter_classes,
    const std::vector<Room>& filtered_room_objects,
    bool filter_rooms,
    const std::vector<Teacher>& filtered_teacher_objects,
    bool filter_teachers
) const {
    std::map<datetime, bool> raw_hours_taught;
    std::map<datetime, bool> raw_hours_missed;
    std::map<datetime, bool> raw_hours_extra;
    std::map<datetime, bool> raw_hours_special_cases;

    for (const auto& p : periods) {
        if (filter_classes) {
            bool found = false;

            for (const auto& k : p.klassen) {
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

            for (const auto& r : Vector_Utils::concat_ranges(p.rooms, p.original_rooms)) {
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

            for (const auto& t : Vector_Utils::concat_ranges(p.teachers, p.original_teachers)) {
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
    for (const auto& [dt, value] : raw_hours_special_cases) {
        if (auto d = Date_Utils::datetime_to_date(dt); value && start_date <= d && d <= end_date) {
            dates_special_cases.push_back(d);
        }
    }

    int hours_special_cases = static_cast<int>(dates_special_cases.size());

    int hours_taught = 0;
    int hours_missed = 0;
    int hours_extra = 0;


    for (const auto& [dt, value] : raw_hours_taught) {
        if (auto d = Date_Utils::datetime_to_date(dt); value &&
                                                       start_date <= d && d <= end_date &&
                                                       !Vector_Utils::contains_value(dates_special_cases, d)) {
            ++hours_taught;
        }
    }

    for (const auto& [dt, value] : raw_hours_missed) {
        auto d = Date_Utils::datetime_to_date(dt);

        if (value &&
            start_date <= d && d <= end_date &&
            !Vector_Utils::contains_value(dates_special_cases, d)) {
            ++hours_missed;
        }
    }

    for (const auto& [dt, value] : raw_hours_extra) {
        auto d = Date_Utils::datetime_to_date(dt);

        if (value &&
            start_date <= d && d <= end_date &&
            !Vector_Utils::contains_value(dates_special_cases, d)) {
            ++hours_extra;
        }
    }

    return std::make_tuple(hours_special_cases, hours_taught, hours_missed, hours_extra);
}

[[nodiscard]] std::vector<str> TimeTable::get_separate_hours(
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
) const {
    auto [hours_special_cases, hours_taught, hours_missed, hours_extra] = get_statistics(start_date, end_date, featuring_object, filtered_class_objects, filter_classes, filtered_room_objects,
        filter_rooms, filtered_teacher_objects, filter_teachers);

    int hours_regular = hours_taught + hours_missed;

    str hours_taught_percent = "0,00%";
    str hours_taught_extra_percent = "0,00%";
    str hours_extra_per_regular = "0,00%";

    if (hours_regular > 0) {
        hours_taught_percent = format_value(static_cast<float>(hours_taught) / static_cast<float>(hours_regular) * 100.0f, true, false);
        hours_taught_extra_percent = format_value(static_cast<float>(hours_taught + hours_extra) / static_cast<float>(hours_regular) * 100.0f, true, false);
        hours_extra_per_regular = format_value(static_cast<float>(hours_extra) / static_cast<float>(hours_regular) * 100.0f, true, false);
    }

    if ((hours_taught + hours_missed + hours_extra + hours_special_cases + hours_regular) == 0) {
        if (filter_unused_objects) {
            return {};
        }
    }

    str taught_all_percent = "0,00%";
    if (total_periods > 0) {
        taught_all_percent = format_value(static_cast<float>(hours_taught + hours_extra) / static_cast<float>(total_periods) * 100.0f, true, false);
    }

    return {
        std::to_string(hours_taught),
        std::to_string(hours_missed),
        std::to_string(hours_extra),
        std::to_string(hours_special_cases),
        taught_all_percent,
        hours_taught_percent,
        hours_taught_extra_percent,
        hours_extra_per_regular
    };
}

std::tuple<str, str> TimeTable::get_table_name(
    const std::variant<Class, Room, Teacher>& featuring_object,
    const date start_date,
    const date end_date
) {
    if (const Class* c_ptr = std::get_if<Class>(&featuring_object)) {
        return std::make_tuple(
            my_config.language_config.class_timetable + " " + c_ptr->name,
            "(" + Date_Utils::date_to_str(start_date, "%d.%m.%Y") + " - " + Date_Utils::date_to_str(end_date, "%d.%m.%Y") + ")"
        );
    }
    if (const Room* r_ptr = std::get_if<Room>(&featuring_object)) {
        return std::make_tuple(
            my_config.language_config.room_timetable + " " + r_ptr->name,
            "(" + Date_Utils::date_to_str(start_date, "%d.%m.%Y") + " - " + Date_Utils::date_to_str(end_date, "%d.%m.%Y") + ")"
        );
    }
    if (const Teacher* t_ptr = std::get_if<Teacher>(&featuring_object)) {
        return std::make_tuple(
            my_config.language_config.teacher_timetable + " " + t_ptr->name + " (" + t_ptr->long_name + ")",
            "(" + Date_Utils::date_to_str(start_date, "%d.%m.%Y") + " - " + Date_Utils::date_to_str(end_date, "%d.%m.%Y") + ")"
        );
    }
    return std::make_tuple("", "");
}

[[nodiscard]] std::vector<Period> TimeTable::unsorted_table() const {
    return periods;
}

[[nodiscard]] std::vector<std::pair<day_time, std::vector<std::pair<date, std::vector<Period>>>>> TimeTable::to_table() const {
    if (periods.empty()) return {};

    std::unordered_set<day_time, Date_Utils> times;
    std::unordered_set<date, Date_Utils> dates;
    std::unordered_set<datetime, Date_Utils> date_times;
    for (const auto& p : periods) {
        auto a = Date_Utils::datetime_to_time(p.start);
        times.insert(a);
        dates.insert(Date_Utils::datetime_to_date(p.start));
        date_times.insert(p.start);
    }

    std::map<day_time, std::map<date, std::vector<Period>>> time_table;
    for (const auto& t : times) {
        for (const auto& d : dates) {
            time_table[t][d] = {};
        }
    }

    for (const auto& p : periods) {
        for (const auto& dt : date_times) {
            if (p.start <= dt && dt <= p.end) {
                time_table[Date_Utils::datetime_to_time(dt)][Date_Utils::datetime_to_date(dt)].push_back(p);
            }
        }
    }

    std::vector<std::pair<day_time, std::vector<std::pair<date, std::vector<Period>>>>> result;

    for (const auto& [t, date_map] : time_table) {
        std::vector<std::pair<date, std::vector<Period>>> date_vec(date_map.begin(), date_map.end());
        result.emplace_back(t, std::move(date_vec));
    }

    return result;
}

[[nodiscard]] std::vector<str> TimeTable::to_class_cancelled_hours() const {
    std::map<std::pair<datetime, datetime>, std::vector<std::vector<str>>> cancelled_hours_one_time;
    for (const auto& [time, row] : to_table()) {
        for (const auto &cell: row | std::views::values) {
            for (const auto& p : cell) {
                // TODO: The python has a try-catch here
                str long_subject_name = Str_Utils::join(", ", p.subjects, &Subject::long_name);
                str room_name = Str_Utils::join(", ", p.rooms, &Room::name);
                str original_room_name = Str_Utils::join(", ", p.original_rooms, &Room::name);
                str teachers = Str_Utils::join(", ", p.teachers, &Teacher::name);
                str original_teachers = Str_Utils::join(", ", p.original_teachers, &Teacher::name);

                if (teachers.empty()) {
                    teachers = "[" + my_config.language_config.unknown_element_extended_text + "]";
                }

                if (original_teachers.empty()) {
                    original_teachers = teachers;
                }

                if (long_subject_name.empty()) {
                    long_subject_name = my_config.language_config.some_hour;
                }

                if (room_name.empty()) {
                    room_name = my_config.language_config.unknown_element_extended_text;
                }

                if (original_room_name.empty()) {
                    original_room_name = room_name;
                }

                std::pair<datetime, datetime> times = std::make_pair(p.start, p.end);
                auto [p_info, p_code] = std::make_pair(
                    std::format("{} ({}, {})", long_subject_name, room_name, teachers),
                    p.raw_period_code
                );

                str cancelled_info = std::format("{} ({}, {}) {}: {}",
                    long_subject_name,
                    room_name,
                    teachers,
                    my_config.language_config.is_cancelled,
                    Date_Utils::datetime_to_str(p.start, "%Y-%m-%d %H:%M:%S")
                );
                str irregular_info = std::format("{} ({}, {}) -> {} ({}, {}): {}",
                    long_subject_name,
                    original_room_name,
                    original_teachers,
                    long_subject_name,
                    room_name,
                    teachers,
                    Date_Utils::datetime_to_str(p.start, "%Y-%m-%d %H:%M:%S")
                );

                auto add_info = [&](const str& info) {
                    const std::vector<str> entry = {
                        p_info,
                        p_code.value_or(""),
                        info
                    };
                    cancelled_hours_one_time[times].push_back(entry);
                };

                if (auto value = p.raw_period_code; value.has_value() && (*value == "cancelled" || *value == "irregular")) {
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
                special_hours.push_back(std::format("{} {}: {}", lesson_1_string, my_config.language_config.is_irregular, Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S")));
            } else {
                special_hours.push_back(lesson_1_special_string);
            }
        } else if (cancelled_hours_one_time[dates].size() == 2) {
            // 2 Hours / date
            if (lesson_1_code == "cancelled" && lesson_2_code == "irregular") {
                special_hours.push_back(
                    std::format("{} {}: {}: {}", my_config.language_config.instead, lesson_1_string, lesson_2_string,  Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S"))
                );
            } else if (lesson_2_code == "cancelled" && lesson_1_code == "irregular") {
                special_hours.push_back(
                    std::format("{} {}: {}: {}", my_config.language_config.instead, lesson_2_string, lesson_1_string, Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S"))
                );
            } else if (lesson_1_code == "cancelled" && lesson_2_code == "cancelled") {
                special_hours.push_back(
                    std::format("{} & {} {}: {}", lesson_1_string, lesson_2_string, my_config.language_config.are_cancelled, Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S"))
                );
            } else {
                special_hours.push_back(
                    std::format("{} & {} {}: {}", lesson_1_string, lesson_2_string, my_config.language_config.are_irregular, Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S"))
                );
            }
        } else {
            // > 2 Hours / date
            bool all_cancelled = std::ranges::all_of(
                cancelled_hours_one_time[dates],
                [](const std::vector<str>& entry) {
                    return entry[1] != "irregular";
                }
            );

            if (all_cancelled) {
                special_hours.push_back(
                    std::format("{}: {}", my_config.language_config.multiple_lessons_cancelled, Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S"))
                );
            } else {
                special_hours.push_back(
                    std::format("{}: {}", my_config.language_config.multiple_lessons_irregular, Date_Utils::datetime_to_str(dates.first, "%Y-%m-%d %H:%M:%S"))
                );
            }
        }

    }

    return special_hours;
}

[[nodiscard]] std::tuple<std::vector<str>, std::vector<str>, std::map<str, std::map<str, std::vector<Period>>>> TimeTable::html_setup(
    const int user_id,
    const bool website,
    const std::tuple<str, str> &table_name,
    const std::optional<date> &start_date,
    const std::optional<date> &end_date
) const {
    std::map<str, std::map<str, std::vector<Period>>> final_hours;

    for (const auto& weekday : my_config.language_config.weekday_name_mapping | std::views::keys) {
        if (weekday == "Saturday" || weekday == "Sunday") {
            continue;
        }

        auto it = std::ranges::find(my_config.language_config.weekday_name_mapping,
                            weekday, &std::pair<str, str>::first);
        const str& mapped_weekday = it->second;

        for (const auto& p : periods) {
            if (Date_Utils::datetime_to_str(p.start, "%A") != weekday) {
                continue;
            }

            // Actual start & end time, ex. 08:40 & 09:35, or irregular times: 00:00 & 23:59 (in one lesson)
            day_time  period_start_time = Date_Utils::datetime_to_time(p.start);
            day_time period_end_time    = Date_Utils::datetime_to_time(p.end);

            for (const auto& time_range : my_config.html_style_config.lesson_time_ranges) {
                const auto sep = time_range.find(" - ");
                const day_time i_start_time = Date_Utils::str_to_daytime(time_range.substr(0, sep), my_config.html_style_config.lesson_time_ranges_format);
                const day_time i_end_time   = Date_Utils::str_to_daytime(time_range.substr(sep + 3), my_config.html_style_config.lesson_time_ranges_format);

                if (period_start_time <= i_start_time && i_start_time <= period_end_time) {
                    if (period_start_time <= i_end_time && i_end_time <= period_end_time) {
                        const str time_key = std::format("{} - {}",
                            Date_Utils::daytime_to_str(i_start_time, my_config.html_style_config.lesson_time_ranges_format),
                            Date_Utils::daytime_to_str(i_end_time,   my_config.html_style_config.lesson_time_ranges_format)
                        );

                        final_hours[mapped_weekday][time_key].push_back(p);
                    }
                }
            }
        }
    }

    std::vector<str> weekdays;

    for (const auto& weekday : my_config.language_config.weekday_name_mapping | std::views::keys) {
        if (weekday == "Saturday" || weekday == "Sunday") {
            continue;
        }
        
        auto it = std::ranges::find(my_config.language_config.weekday_name_mapping,
                            weekday, &std::pair<str, str>::first);
        weekdays.push_back(it->second);
    }

    auto bits4_to_signed_int = [](const str& bits) -> int {
        int n = std::stoi(bits, nullptr, 2);
        if (bits[0] == '1')
            n -= (1 << 4);
        return n;
    };

    // RGB changes:
    // Interpret as 4 bit signed int [-8; +7]
    // For better reading with rgb colour picker, *2 -> [-16; +14]

    str water_mark_bits = std::bitset<64>(user_id).to_string();
    std::tuple<int, int, int> base_rgb_value = my_config.html_style_config.table_header_base_rgb;
    std::vector<std::tuple<int, int, int>> water_mark_rgb_value;

    // Handle the first 5 full RGB triplets
    for (int num_header = 0; num_header < 5; num_header++) {
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
            my_config.html_style_config.timetable_html_header,
            "<p>",
            std::format("<a href=\"?date=0\"><button>{}</button></a>", my_config.language_config.back),
            "<br>",
            std::format("<a href=\"?date={}\"><button>{}</button></a>", Date_Utils::date_to_str(Date_Utils::add_weeks(start_date.value(), -1), "%d-%m-%Y"), my_config.language_config.last_week),
            std::format("{} {}", std::get<0>(table_name), std::get<1>(table_name)),
            std::format("<a href=\"?date={}\"><button>{}</button></a>", Date_Utils::date_to_str(Date_Utils::add_weeks(start_date.value(), 1), "%d-%m-%Y"), my_config.language_config.next_week),
            "</p>",
            R"(<table border="1" cellspacing="0" cellpadding="5">)",
            std::format("<tr><th style=\"background-color: rgb({},{},{});\">{}</th>",
                std::get<0>(my_config.html_style_config.table_header_base_rgb),
                std::get<1>(my_config.html_style_config.table_header_base_rgb),
                std::get<2>(my_config.html_style_config.table_header_base_rgb),
                my_config.language_config.time
            )
        };

        for (const auto& day : weekdays) {
            html.push_back(std::format("<th style=\"background-color: rgb({}, {}, {});\">{}</th>",
                std::get<0>(my_config.html_style_config.table_header_base_rgb),
                std::get<1>(my_config.html_style_config.table_header_base_rgb),
                std::get<2>(my_config.html_style_config.table_header_base_rgb),
                day.substr(0, 2)
            ));
        }
        html.emplace_back("</tr>");
    } else {
        html = {
            my_config.html_style_config.timetable_html_header,
            std::format("<p>{} {}</p>", std::get<0>(table_name), std::get<1>(table_name)),
            R"(<table border="1" cellspacing="0" cellpadding="5">)",
            std::format("<tr><th style=\"background-color: rgb({}, {}, {});\">{}</th>",
                std::get<0>(my_config.html_style_config.table_header_base_rgb),
                std::get<1>(my_config.html_style_config.table_header_base_rgb),
                std::get<2>(my_config.html_style_config.table_header_base_rgb),
                my_config.language_config.time
            )
        };

        for (int count = 0; count < weekdays.size(); count++) {
            const auto& day = weekdays[count];
            html.push_back(std::format("<th style=\"background-color: rgb({}, {}, {});\">{}</th>",
                std::get<0>(water_mark_rgb_value.at(count + 1)),
                std::get<1>(water_mark_rgb_value.at(count + 1)),
                std::get<2>(water_mark_rgb_value.at(count + 1)),
                day.substr(0, 2)
            ));
        }
        html.emplace_back("</tr>");
    }

    return std::make_tuple(html, weekdays, final_hours);
}

bool TimeTable::html_line_too_long(const std::vector<std::tuple<str, str, str, datetime, datetime>>& distinct_lessons_list_formatted) {
    unsigned long max_html_chars_per_row = 10;

    std::vector<unsigned long> chars_rows = {0, 0, 0};

    // Maximum number of characters per row (3 total)
    for (const auto& lesson : distinct_lessons_list_formatted) {
        chars_rows.at(0) += std::get<0>(lesson).length();
        chars_rows[1] += std::get<1>(lesson).length() + 2; // Brackets ()
        chars_rows[2] += std::get<2>(lesson).length() + 2; // Brackets []
    }

    // Do not use num lessons; len('NWP [PAM] (RBU1)') > len('E [SU] [R6B]')

    return std::ranges::any_of(chars_rows, [max_html_chars_per_row](const unsigned long num_chars) {
        return num_chars > max_html_chars_per_row;
    });
}

void TimeTable::html_add_lesson_time_range(std::vector<str>& html, const int lesson_count_index, const str& lesson_time_range) {
    const auto sep = lesson_time_range.find(" - ");
    const str start = lesson_time_range.substr(0, sep);
    const str end   = lesson_time_range.substr(sep + 3);


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
        </td>)", lesson_count_index + 1, start, end));
}

[[nodiscard]] str TimeTable::to_html(
    const std::variant<Class, Room, Teacher>& featuring_object,
    const int user_id,
    const bool website,
    const std::tuple<str, str> &table_name,
    const std::optional<date> start_date,
    const std::optional<date> end_date
) const {
    auto [html, weekdays, final_hours] = html_setup(user_id, website, table_name, start_date, end_date);

    for (int count = 0; count <  my_config.html_style_config.lesson_time_ranges.size(); count++) {
        const auto& time_range = my_config.html_style_config.lesson_time_ranges[count];
        html_add_lesson_time_range(html, count, time_range);

        for (const auto& day : weekdays) {
            std::vector<Period> lessons;
            if (final_hours.contains(day) && final_hours.at(day).contains(time_range)) {
                lessons = final_hours.at(day).at(time_range);
            }

            if (lessons.empty()) {
                html.emplace_back("<td></td>");
                continue;
            }

            std::vector<std::tuple<str, str, str, datetime, datetime>> distinct_lessons_list_formatted;

            for (const auto& lesson : lessons) {
                auto list_formatted = lesson.formatted_list(featuring_object, false);
                if (!Vector_Utils::contains_value(distinct_lessons_list_formatted, list_formatted)) {
                    distinct_lessons_list_formatted.push_back(std::move(list_formatted));
                }
            }

            std::vector<str> formatted_lessons;
            std::unordered_set<str> seen_lesson_strings;
            unsigned long total_character = 0;

            for (const auto& lesson : lessons) {
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
                    (rows_changed.first  && lesson_code == "regular") ? " style=\"color: #F9A825;\"" : "",
                    std::get<1>(list_formatted),
                    (rows_changed.second && lesson_code == "regular") ? " style=\"color: #F9A825;\"" : "",
                    std::get<2>(list_formatted)
                );

                // Render lesson
                if (lessons.size() == 1) {
                    formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:5px; vertical-align: top; margin-left:5px; {}\">{}</span>", text_color, formatted_lesson));
                } else {
                    if (seen_lesson_strings.contains(string_formatted)) {
                        continue;
                    }
                    seen_lesson_strings.insert(string_formatted);

                    if (website) {
                        if (distinct_lessons_list_formatted.size() < 5) {
                            formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:1px; vertical-align: top; margin-left:1px; {}\">{}</span>", text_color, formatted_lesson));
                        } else {
                            formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:5px; vertical-align: top; margin-left:5px; {}\">{}</span>", text_color, std::get<0>(list_formatted)));
                        }
                    } else {
                        if (!html_line_too_long(distinct_lessons_list_formatted)) {
                            formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:1px; vertical-align: top; margin-left:1px; {}\">{}</span>", text_color, formatted_lesson));
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

                            formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:5px; vertical-align: top; margin-left:5px; {}\">{}</span>", short_subject_name_text_color, std::get<0>(list_formatted)));
                        }
                    }
                }
            }

            // Stripe colour (based on first eligible subject / fallback to first subject)
            std::tuple<int, int, int> rgba_value = my_config.timetable_mapping_config.default_subject_color;

            // Collect eligible subjects (None or irregular)
            std::vector<Subject> eligible_subjects;

            for (const auto& lesson : lessons) {
                str p_code = lesson.get_period_code(featuring_object).first;
                if (p_code != "regular" && p_code != "extra") {
                    continue;
                }
                if (lesson.subjects.empty()) {
                    continue;
                }
                Subject su = lesson.subjects.at(0);
                std::cout << "Added eligible subject " << su.to_string() << std::endl;
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
                std::get<0>(rgba_value),
                std::get<1>(rgba_value),
                std::get<2>(rgba_value),
                Str_Utils::join(formatted_lessons)
            ));
        }
        html.emplace_back("</tr>");
    }

    html.emplace_back("</table>");

    html.emplace_back(my_config.html_style_config.timetable_html_footer);

    str html_content = Str_Utils::join("\n", html);

    return html_content;
}

[[nodiscard]] str TimeTable::to_untis_html(
    const std::variant<Class, Room, Teacher>& featuring_object,
    const int user_id,
    const std::tuple<str, str>& table_name,
    const date start_date,
    const date end_date
) const {
    return to_html(featuring_object, user_id, false, table_name, start_date, end_date);
}

[[nodiscard]] str TimeTable::to_website_html(
    const std::variant<Class, Room, Teacher>& featuring_object,
    const date start_date,
    const date end_date
) const {
    std::tuple<str, str> table_name = get_table_name(featuring_object, start_date, end_date);
    return to_html(featuring_object, 0, true, table_name, start_date, end_date);
}

[[nodiscard]] str TimeTable::to_personal_html(
    const std::variant<Class, Room, Teacher>& featuring_object,
    date target_date,
    const str& person_name
) const {
    str english_weekday = Date_Utils::date_to_str(target_date, "%A");
    
    auto it = std::ranges::find(my_config.language_config.weekday_name_mapping,
                            english_weekday, &std::pair<str, str>::first);
    const str& native_weekday = it->second;

    std::map<str, std::map<str, std::vector<Period>>> final_hours;
    final_hours[native_weekday] = {};

    for (const auto& period : periods) {
        if (Date_Utils::datetime_to_str(period.start, "%A") != english_weekday) {
            continue;
        }

        // Actual start & end time, ex. 08:40 & 09:35, or irregular times: 00:00 & 23:59 (in one lesson)
        day_time period_start_time = Date_Utils::datetime_to_time(period.start);
        day_time period_end_time = Date_Utils::datetime_to_time(period.end);

        for (const auto& time_range : my_config.html_style_config.lesson_time_ranges) {
            const auto sep = time_range.find(" - ");
            const day_time i_start_time = Date_Utils::str_to_daytime(time_range.substr(0, sep), my_config.html_style_config.lesson_time_ranges_format);
            const day_time i_end_time   = Date_Utils::str_to_daytime(time_range.substr(sep + 3), my_config.html_style_config.lesson_time_ranges_format);

            if (period_start_time <= i_start_time && i_start_time <= period_end_time) {
                if (period_start_time <= i_end_time && i_end_time <= period_end_time) {
                    const str time_key = std::format("{} - {}",
                        Date_Utils::daytime_to_str(i_start_time, my_config.html_style_config.lesson_time_ranges_format),
                        Date_Utils::daytime_to_str(i_end_time,   my_config.html_style_config.lesson_time_ranges_format)
                    );

                    final_hours[native_weekday][time_key].push_back(period);
                }
            }
        }
    }

    std::tuple<int, int, int> rgb_value = my_config.html_style_config.table_header_base_rgb;

    if (target_date == Date_Utils::get_today()) {
        rgb_value = my_config.html_style_config.today_personal_rgb_value;
    }

    std::vector<str> html = {
        my_config.html_style_config.personal_timetable_html_header,
        "<p>",

        std::format("<a href=\"?date={}\"><button>{}</button></a>",
            Date_Utils::date_to_str(Date_Utils::add_days(target_date, -1), "%d-%m-%Y"),
            my_config.language_config.yesterday),

        std::format("{} {} ({})",
            my_config.language_config.personal_timetable,
            person_name,
            Date_Utils::date_to_str(target_date, "%d.%m.%Y")),

        std::format("<a href=\"?date={}\"><button>{}</button></a>",
            Date_Utils::date_to_str(Date_Utils::add_days(target_date, 1), "%d-%m-%Y"),
            my_config.language_config.tomorrow),

        "</p>",
        "<p>",

        std::format("<a href=\"?date={}\"><button>{}</button></a>",
            Date_Utils::date_to_str(Date_Utils::get_today(), "%d-%m-%Y"),
            my_config.language_config.today),

        "</p>",
        R"(<table border="1" cellspacing="0" cellpadding="5">)",

        std::format("<tr><th style=\"background-color: rgb({},{},{});\">{}</th>",
            std::get<0>(my_config.html_style_config.table_header_base_rgb),
            std::get<1>(my_config.html_style_config.table_header_base_rgb),
            std::get<2>(my_config.html_style_config.table_header_base_rgb),
            my_config.language_config.time),

        std::format("<th style=\"background-color: rgb({},{},{});\">{}</th>",
            std::get<0>(rgb_value),
            std::get<1>(rgb_value),
            std::get<2>(rgb_value),
            native_weekday.substr(0, 2)),

        "<tr>"
    };

    for (int count = 0; count < my_config.html_style_config.lesson_time_ranges.size(); count++) {
        const auto& time_range = my_config.html_style_config.lesson_time_ranges[count];
        html_add_lesson_time_range(html, count, time_range);
        std::vector<Period> lessons;
        if (final_hours.contains(native_weekday) && final_hours.at(native_weekday).contains(time_range)) {
            lessons = final_hours.at(native_weekday).at(time_range);
        }

        if (lessons.empty()) {
            html.emplace_back("<td></td>");
            continue;
        }

        std::vector<std::tuple<str, str, str, datetime, datetime>> distinct_lessons_list_formatted;

        for (const auto& lesson : lessons) {
            auto list_formatted = lesson.formatted_list(featuring_object, false);
            if (!Vector_Utils::contains_value(distinct_lessons_list_formatted, list_formatted)) {
                distinct_lessons_list_formatted.push_back(std::move(list_formatted));
            }
        }

        std::vector<str> formatted_lessons;
        std::unordered_set<str> seen_lesson_strings;

        for (const auto& lesson : lessons) {
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
                (rows_changed.first  && lesson_code == "regular") ? " style=\"color: #F9A825;\"" : "",
                std::get<1>(list_formatted),
                (rows_changed.second && lesson_code == "regular") ? " style=\"color: #F9A825;\"" : "",
                std::get<2>(list_formatted)
            );

            if (lessons.size() == 1) {
                formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:5px; vertical-align: top; margin-left:5px; {}\">{}</span>", text_color, formatted_lesson));
            } else {
                if (seen_lesson_strings.contains(string_formatted)) {
                    continue;
                }
                seen_lesson_strings.insert(string_formatted);

                if (distinct_lessons_list_formatted.size() < 5) {
                    formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:1px; vertical-align: top; margin-left:1px; {}\">{}</span>", text_color, formatted_lesson));
                } else {
                    formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:5px; vertical-align: top; margin-left:5px; {}\">{}</span>", text_color, std::get<0>(list_formatted)));
                }
            }
        }

        // Stripe colour (based on first eligible subject / fallback to first subject)
        std::tuple<int, int, int> rgba_value = my_config.timetable_mapping_config.default_subject_color;

        // Collect eligible subjects (None or irregular)
        std::vector<Subject> eligible_subjects;

        for (const auto& lesson : lessons) {
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
            std::get<0>(rgba_value),
            std::get<1>(rgba_value),
            std::get<2>(rgba_value),
            Str_Utils::join(formatted_lessons)
        ));
        html.emplace_back("</tr>");
    }

    html.emplace_back("</table>");

    html.emplace_back(my_config.html_style_config.timetable_html_footer);

    str html_content = Str_Utils::join("\n", html);

    return html_content;
}

[[nodiscard]] str TimeTable::to_regular_html(
    const std::variant<Class, Room, Teacher>& featuring_object,
    int user_id,
    const std::tuple<str, str>& table_name
) const {
    auto [html, weekdays, final_hours] = html_setup(user_id, false, table_name, std::nullopt, std::nullopt);

    bool any_two_week_lesson = false;

    for (int count = 0; count < my_config.html_style_config.lesson_time_ranges.size(); count++) {
        const auto& time_range = my_config.html_style_config.lesson_time_ranges[count];
        html_add_lesson_time_range(html, count, time_range);

        for (const auto& day : weekdays) {
            std::vector<Period> raw_lessons;
            if (final_hours.contains(day) && final_hours.at(day).contains(time_range)) {
                raw_lessons = final_hours.at(day).at(time_range);
            }

            std::vector<Period> filtered_lessons;
            for (const auto& lesson : raw_lessons) {
                if (lesson.get_period_code(featuring_object).first != "extra") {
                    filtered_lessons.push_back(lesson);
                }
            }

            if (filtered_lessons.empty()) {
                html.emplace_back("<td></td>");
                continue;
            }

            std::vector<std::tuple<str, str, str, datetime, datetime>> distinct_lessons_list_formatted;
            std::unordered_set<str> distinct_lessons_string;

            for (const auto& lesson : filtered_lessons) {
                auto list_formatted = lesson.formatted_list(featuring_object, false);

                str formatted_lesson = std::format("<span>{}</span><br><span>[{}]</span><br><span>({})</span>",
                    std::get<0>(list_formatted),
                    std::get<1>(list_formatted),
                    std::get<2>(list_formatted)
                );

                if (!distinct_lessons_string.contains(formatted_lesson)) {
                    distinct_lessons_list_formatted.push_back(std::move(list_formatted));
                    distinct_lessons_string.insert(std::move(formatted_lesson));
                }
            }

            std::vector<str> formatted_lessons;
            std::unordered_set<str> seen_formatted_lessons;
            unsigned long total_character = 0;

            for (const auto& lesson : filtered_lessons) {
                auto [lesson_code, _] = lesson.get_period_code(featuring_object);

                auto list_formatted = lesson.formatted_list(featuring_object, false);

                if (lesson_code == "extra") {
                    continue;
                }

                str formatted_lesson = std::format("<span>{}</span><br><span>[{}]</span><br><span>({})</span>",
                    std::get<0>(list_formatted),
                    std::get<1>(list_formatted),
                    std::get<2>(list_formatted)
                );

                str this_lesson_bi_weekly;

                // Bi-weekly check
                if (count_appearances(lesson) == 1) {
                    this_lesson_bi_weekly = std::format("{}<br>", my_config.language_config.two_week_abbreviation);
                    any_two_week_lesson = true;
                }

                // Render lesson
                if (filtered_lessons.size() == 1) {
                    // 1 Lesson / block (-> bi-weekly, but irrelevant)
                    formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:5px; vertical-align: top; margin-left:5px;\">{}{}</span>", this_lesson_bi_weekly, formatted_lesson));
                } else {
                    // Could still contain bi-weekly lessons; If multiple lessons are in the same block

                    if (seen_formatted_lessons.contains(formatted_lesson)) {
                        continue;
                    }
                    seen_formatted_lessons.insert(formatted_lesson);

                    if (!html_line_too_long(distinct_lessons_list_formatted)) {
                        formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:1px; vertical-align: top; margin-left:1px;\">{}{}</span>", this_lesson_bi_weekly, formatted_lesson));
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

                        formatted_lessons.push_back(std::format("<span style=\"display:inline-block; margin-right:5px; vertical-align: top; margin-left:5px;\">{}{}</span>", this_lesson_bi_weekly, std::get<0>(list_formatted)));
                    }
                }
            }

            // Stripe colour (based on first eligible subject / fallback to first subject)
            std::tuple<int, int, int> rgba_value = my_config.timetable_mapping_config.default_subject_color;

            // Collect eligible subjects (None or irregular)
            std::vector<Subject> eligible_subjects;

            for (const auto& lesson : filtered_lessons) {
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
                std::get<0>(rgba_value),
                std::get<1>(rgba_value),
                std::get<2>(rgba_value),
                Str_Utils::join(formatted_lessons)
            ));
        }
        html.emplace_back("</tr>");
    }

    html.emplace_back("</table>");

    if (any_two_week_lesson) {
        html.emplace_back(my_config.html_style_config.timetable_html_footer_two_week);
    }

    html.emplace_back(my_config.html_style_config.timetable_html_footer);

    str html_content = Str_Utils::join("\n", html);

    return html_content;
}

void TimeTable::render_one_image_by_html(
    std::counting_semaphore<MAX_CONCURRENCY_WEBSITE_CAPTURE>& sem,
    std::map<str, std::vector<uint8_t>>& results,
    const std::tuple<str, str>& table_name,
    const str& html_content
) {
    sem.acquire();
    const str filename = Renderer::sanitize_filename(
        std::get<0>(table_name) + " " + std::get<1>(table_name) + ".png"
    );
    results[filename] = Renderer::base64_decode(my_config.renderer.generate_base64_image(html_content));
    sem.release();
}

std::vector<uint8_t> TimeTable::capture_image_by_html(
    const int concurrency_website_capture,
    const std::tuple<str, str>& table_name,
    const str& html_content
) {
    std::counting_semaphore<MAX_CONCURRENCY_WEBSITE_CAPTURE> sem(concurrency_website_capture);

    std::map<str, std::vector<uint8_t>> results;
    render_one_image_by_html(sem, results, table_name, html_content);
    return results.begin()->second;
}

// Returns map of sanitized filename -> PNG bytes
std::map<str, std::vector<uint8_t>> TimeTable::capture_all_images(
    const int concurrency_website_capture,
    const std::vector<std::pair<std::tuple<str, str>, str>>& pages
) {
    std::counting_semaphore<MAX_CONCURRENCY_WEBSITE_CAPTURE> sem(concurrency_website_capture);
    std::map<str, std::vector<uint8_t>> results;
    std::mutex results_mutex;
    std::vector<std::thread> threads;
    threads.reserve(pages.size());

    for (const auto& [table_name, html_content] : pages) {
        threads.emplace_back([&, table_name, html_content]() {
            sem.acquire();
            const str filename = Renderer::sanitize_filename(
                std::get<0>(table_name) + " " + std::get<1>(table_name) + ".png"
            );
            auto image = Renderer::base64_decode(
                my_config.renderer.generate_base64_image(html_content)
            );
            sem.release();

            std::lock_guard<std::mutex> lock(results_mutex);
            results[filename] = std::move(image);
        });
    }

    for (auto& t : threads)
        t.join();

    return results;
}

std::vector<uint8_t> TimeTable::table_to_image(
    int concurrency_website_capture,
    const std::variant<Class, Room, Teacher>& featuring_object,
    int user_id,
    date start_date,
    date end_date
) const {
    std::tuple<str, str> table_name = get_table_name(featuring_object, start_date, end_date);
    str html_content = to_untis_html(featuring_object, user_id, table_name, start_date, end_date);

    return capture_image_by_html(concurrency_website_capture, table_name, html_content);
}

unsigned long TimeTable::count_appearances(const Period& period_to_count) const {
    unsigned long appearances = 0;
    std::vector<Period> distinct_periods;

    for (const auto& period : periods) {
        // If it's an **exact match**, the date hour etc. match -> continue
        if (Vector_Utils::contains_value(distinct_periods, period)) {
            continue;
        }
        distinct_periods.push_back(period);

        // If the regular identifier is the same, appearances += 1
        if (period_to_count.regular_plan_identifier() == period.regular_plan_identifier()) {
            appearances++;
        }
    }

    return appearances;
}

size_t TimeTable::size() const {
    return periods.size();
}

bool TimeTable::operator==(const TimeTable& other) const {
    return periods == other.periods;
}

TimeTable TimeTable::operator+(const TimeTable& other) const {
    std::vector<Period> combined;
    combined.reserve(periods.size() + other.periods.size());
    combined.insert(combined.end(), periods.begin(), periods.end());
    combined.insert(combined.end(), other.periods.begin(), other.periods.end());
    return TimeTable(std::move(combined));
}

str TimeTable::to_string() const {
    std::vector<str> period_strings;
    period_strings.reserve(periods.size());

    for (const auto& period : periods) {
        period_strings.push_back(period.to_string());
    }

    return Str_Utils::join("\n", period_strings);
}

std::optional<double> Cache::cache_file_last_changed(const std::optional<str>& file_path) const {
    if (!file_path.has_value()) {
        return std::nullopt;
    }
    try {
        const auto now = std::chrono::system_clock::now();

        const auto ftime = std::filesystem::last_write_time(file_path.value());

        // convert file time -> system_clock time_point
        const auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now()
            + std::chrono::system_clock::now()
        );

        return std::chrono::duration<double>(now - sctp).count();
    } catch (std::exception& e) {
        return std::nullopt;
    }
}

template<typename T>
std::optional<T> Cache::get_from_cache(const str& key) {
    auto it = cache.find(key);

    if (it == cache.end())
        return std::nullopt;

    try {
        return it->second.get<T>();
    }
    catch (...) {
        return std::nullopt;
    }
}

template<typename T>
void Cache::update_cache(const str& key, T value) {
    cache[key] = std::move(value);
}

void Cache::clear_cache() {
    cache.clear();
}

void Cache::read_cache_from_file() {
    if (!cache_file_path) {
        return;
    }

    std::ifstream in(*cache_file_path);

    if (!in) {
        return;
    }

    nlohmann::json j;
    in >> j;

    cache = j.get<decltype(cache)>();
}

void Cache::write_cache_to_file() const {
    if (!cache_file_path) {
        return;
    }

    std::ofstream out(*cache_file_path);

    if (!out) {
        return;
    }

    nlohmann::json j(cache);

    out << j.dump(4);
}

Session::Session(
    str session_name,
    bool use_cache,
    std::optional<str> cache_file,
    std::optional<Logger> logger,
    str username,
    str password,
    str server,
    str school,
    str client
)
    : username(std::move(username)),
      password(std::move(password)),
      school(std::move(school)),
      client(std::move(client)),
      session_name(std::move(session_name)),
      use_cache(use_cache),
      logger(logger.value_or(Logger{})),
      cache(std::move(cache_file))
{
    if (!server.starts_with("http")) {
        server = std::format("https://{}", server);
    }

    while (!server.empty() && server.back() == '/') {
        server.pop_back();
    }

    this->server = std::move(server);
}

uuid Session::get_unique_uuid() {
    return boost::uuids::random_generator()();
}

json Session::rpc_request(
    const str& method,
    const json& params,
    bool retry_on_authentication_error
) {
    json payload = {
        {"id", get_unique_uuid()}, // TODO: WHY???
        {"method", method},
        {"params", params},
        {"jsonrpc", "2.0"}
    };

    str url = std::format("{}/jsonrpc.do?school={}", server, school);

    cpr::Header headers{
        {"Content-Type", "application/json"}
    };

    cpr::Cookies cookies;
    if (jsessionid) {
        cookies = cpr::Cookies{
            {"JSESSIONID", *jsessionid}
        };
    }

    cpr::Response response = cpr::Post(
        cpr::Url{url},
        headers,
        cookies,
        cpr::Body{payload.dump()}
    );

    if (response.status_code >= 400) {
        throw std::runtime_error(
            std::format("HTTP error {}", response.status_code)
        );
    }

    json data = json::parse(response.text);

    if (data.contains("error")) {
        auto error = data["error"];
        str msg = error.value("message", "");

        if (msg == "not authenticated") {
            logger.log_warning(
                std::format(
                    "WebUntis API NotAuthenticatedError ({}): {}",
                    method,
                    error.dump()
                )
            );

            if (retry_on_authentication_error) {
                logger.log_debug(
                    "Logging in & retrying Authentication..."
                );

                auto call_id = get_unique_uuid();

                log_in(call_id);

                auto result =
                    rpc_request(method, params, false);

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

        throw std::runtime_error(
            std::format(
                "WebUntis API Error ({}): {}",
                method,
                error.dump()
            )
        );
    }

    return data["result"];
}

int Session::format_date(const date& d) {
    return stoi(Date_Utils::date_to_str(d, "%Y%m%d"));
}

date Session::parse_date(int d) {
    return Date_Utils::str_to_date(std::to_string(d), "%Y%m%d");
}

day_time Session::parse_time(int t) {
    return Date_Utils::str_to_time(std::to_string(t));
}

std::map<str, str> Session::create_date_param(const date start, const date end, const json& kwargs) {
    if (start > end) {
        throw std::runtime_error("Start date cannot be later than end date.");
    }
    json params = {
        {"startDate", format_date(start)},
        {"endDate", format_date(end)}
    };
    params.update(kwargs);
    return params;
}

void Session::log_in(uuid unique_id) {
    if (active_session_uuids.empty()) {
        json params = {
            {"user", username},
                {"password", password},
                {"client", client}
        };

        // Use raw request for login to catch the session ID
        json payload = {
            {"id", get_unique_uuid()}, // TODO: WHY???
            {"method", "authenticate"},
            {"params", params},
            {"jsonrpc", "2.0"}
        };

        str url = std::format("{}/jsonrpc.do?school={}", server, school);

        cpr::Header headers{
            {"Content-Type", "application/json"}
        };

        cpr::Response response = cpr::Post(
            cpr::Url{url},
            headers,
            cpr::Body{payload.dump()}
        );

        if (response.status_code >= 400) {
            throw std::runtime_error(
                std::format("HTTP error {}", response.status_code)
            );
        }

        json data = json::parse(response.text);

        if (data.contains("error")) {
            auto error = data["error"];
            throw std::runtime_error(
                std::format("Login failed: {}", error.dump())
            );
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

void Session::log_out(const uuid unique_id) {
    active_session_uuids.erase(unique_id);
    if (active_session_uuids.empty() && jsessionid.has_value()) {
        try {
            rpc_request("logout", {});
        } catch (std::exception& e) {}
        jsessionid.reset();

        logger.log_info(std::format("Logged out ({})!", session_name));
    }
}

std::vector<Class> Session::all_klassen() {
    std::vector<Class> all_kl;
    for (const auto& k : rpc_request("getKlassen", {})) {
        all_kl.emplace_back(k.value("name", ""), k.value("longName", ""), k.value("id", 0));
    }
    return all_kl;
}

std::vector<Room> Session::all_rooms() const {

}

std::vector<Subject> Session::all_subjects() const {

}

std::vector<Department> Session::all_departments() const {

}

std::vector<Holiday> Session::all_holidays() const {

}

std::vector<SchoolYear> Session::all_schoolyears() const {

}

SchoolYear Session::return_current_year() const {

}

std::optional<Class> Session::get_klasse_by_name(const str& name) {
    for (const auto& k : all_klassen()) {
        if (k.name == name) {
            return k;
        }
    }
    return std::nullopt;
}

std::optional<Room> Session::get_room_by_name(const str& name) const {

}

std::optional<Teacher> Session::get_teacher_by_name(const str& name) const {

}

std::optional<Teacher> Session::get_teacher_by_long_name(const str& name) const {

}

TimeTable Session::timetable_extended(
    const std::variant<Class, Room, Teacher>& element,
    date start,
    date end
) {
    std::map<str, int> element_type_table = {{"klasse", 1}, {"teacher", 2}, {"subject", 3}, {"room", 4}, {"student", 5}};
    int element_type;
    int entity_id;

    if (const Class* c_ptr = std::get_if<Class>(&element)) {
        element_type = element_type_table["klasse"];
        entity_id = c_ptr->entity_id;
    } else if (const Room* r_ptr = std::get_if<Room>(&element)) {
        element_type = element_type_table["room"];
        entity_id = r_ptr->entity_id;
    } else if (const Teacher* t_ptr = std::get_if<Teacher>(&element)) {
        element_type = element_type_table["teacher"];
        entity_id = t_ptr->entity_id;
    }

    json options = {
        {"startDate", format_date(start)},
        {"endDate", format_date(end)},
        {"element", {{"id", entity_id}, {"type", element_type}}},
        {"showBooking", true},
        {"showInfo", true},
        {"showSubstText", true},
        {"showLsText", true},
        {"showLsNumber", true},
        {"showStudentgroup", true}
    };

    json raw_result;

    try {
        raw_result = rpc_request("getTimetable", {{"options", options}});
    } catch (std::exception& e) {
        std::cout << "Error in getTimetable" << std::endl;
        std::cout << e.what();
        return TimeTable({});
    }

    std::map<int, json> all_su = {};
    std::map<int, json> all_kl = {};
    std::map<int, json> all_ro = {};

    for (const auto& s : rpc_request("getSubjects", {})) {
        all_su[s["id"]] = s;
    }

    for (const auto& k : rpc_request("getKlassen", {})) {
        all_kl[k["id"]] = k;
    }

    for (const auto& r : rpc_request("getRooms", {})) {
        all_ro[r["id"]] = r;
    }

    std::vector<Period> periods;

    for (const auto& raw_p : raw_result) {
        try {
            datetime start_dt = Date_Utils::combine(parse_date(raw_p["date"]), parse_time(raw_p["startTime"]));
            datetime end_dt = Date_Utils::combine(parse_date(raw_p["date"]), parse_time(raw_p["endTime"]));

            std::vector<Subject> subjects;
            std::vector<Class> klassen;
            std::vector<Room> rooms;
            std::vector<Room> original_rooms;
            std::vector<Teacher> teachers;
            std::vector<Teacher> original_teachers;

            for (const auto& s : raw_p["su"]) {
                auto master_su = all_su.contains(s["id"])
                    ? all_su.at(s["id"])
                    : json{};

                subjects.emplace_back(master_su["name"], master_su["longName"], s["id"]);
            }

            for (const auto& k : raw_p["kl"]) {
                auto master_kl = all_kl.contains(k["id"])
                    ? all_kl.at(k["id"])
                    : json{};

                klassen.emplace_back(master_kl["name"], master_kl["longName"], k["id"]);
            }

            for (const auto& r : raw_p["ro"]) {
                int rid = 0;
                if (r.contains("id")) {
                    rid = r["id"];
                }

                auto master_ro = all_ro.contains(rid)
                    ? all_ro.at(rid)
                    : json{};

                rooms.emplace_back(master_ro["name"], master_ro["longName"], rid);
            }

            for (const auto& r : raw_p["ro"]) {
                if (!r.contains("orgid")) {
                    continue;
                }
                int org_id = r["orgid"];

                auto master_ro = all_ro.contains(org_id)
                    ? all_ro.at(org_id)
                    : json{};

                original_rooms.emplace_back(master_ro["name"], master_ro["longName"], org_id);
            }

            for (const auto& t : raw_p["te"]) {
                if (t.contains("id")) {
                    teachers.emplace_back(Teacher::from_teacher_id(t["id"]));
                }
                if (t.contains("orgid")) {
                    teachers.emplace_back(Teacher::from_teacher_id(t["orgid"]));
                }
            }

            periods.emplace_back(
                raw_p.value("code", ""),
                start_dt,
                end_dt,
                subjects,
                klassen,
                rooms,
                original_rooms,
                teachers,
                original_teachers,
                raw_p.value("sg", ""),
                raw_p.value("activityType", ""),
                raw_p.value("bkRemark", ""),
                raw_p.value("bkText", ""),
                raw_p.value("flags", ""),
                raw_p.value("lsnumber", 0),
                raw_p.value("lstext", ""),
                raw_p.value("substText", ""),
                raw_p.value("type", ""),
                raw_p.value("id", 0)
            );
        } catch (std::exception&) {
            continue;
        }
    }
    return TimeTable(periods);
}
