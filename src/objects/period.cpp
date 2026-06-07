#include "period.hpp"

#include "config.hpp"
#include "utils/all.hpp"

Period::Period(
        std::optional<str> raw_period_code,
        datetime start,
        datetime end, std::vector<Subject> subjects,
        std::vector<Class> klassen, std::vector<Room> rooms, std::vector<Room> original_rooms,
        std::vector<Teacher> teachers, std::vector<Teacher> original_teachers, str student_group,
        str activity_type, str bk_remark, str bk_text, str flags, int ls_number, str ls_text, str subst_text,
        str period_type, int period_id
        ) :
    raw_period_code(std::move(raw_period_code)), start(start), end(end),
    subjects(std::move(subjects)),
    klassen(std::move(klassen)), rooms(std::move(rooms)),
    original_rooms(std::move(original_rooms)),
    teachers(std::move(teachers)),
    original_teachers(std::move(original_teachers)),
    student_group(std::move(student_group)),
    activity_type(std::move(activity_type)),
    bk_remark(std::move(bk_remark)),
    bk_text(std::move(bk_text)), flags(std::move(flags)),
    ls_number(ls_number), ls_text(std::move(ls_text)),
    subst_text(std::move(subst_text)), period_type(std::move(period_type)),
    period_id(period_id) {
}

Period::~Period() = default;

[[nodiscard]] str Period::period_code_class(const Class &klassen_object) const {
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

str Period::period_code_room(const Room &room_object) const {
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
    if (Vector_Utils::contains_value(original_rooms, room_object) and
        !Vector_Utils::contains_value(rooms, room_object)) {
        return "missed";
    }
    if (!Vector_Utils::contains_value(original_rooms, room_object) &&
        Vector_Utils::contains_value(rooms, room_object)) {
        return "extra";
    }
    return "regular";
}

str Period::period_code_teacher(const Teacher &teacher_object) const {
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

    if (Vector_Utils::contains_value(original_teachers, teacher_object) &&
        Vector_Utils::contains_value(teachers, teacher_object)) {
        return "regular";
    }

    if (Vector_Utils::contains_value(original_teachers, teacher_object) &&
        Vector_Utils::contains_value(teachers, teacher_object)) {
        return "missed";
    }

    if (!Vector_Utils::contains_value(original_teachers, teacher_object) &&
        Vector_Utils::contains_value(teachers, teacher_object)) {
        return "extra";
    }
    return "regular";
}

[[nodiscard]] std::pair<str, std::pair<bool, bool> >
Period::get_period_code(const std::variant<Class, Room, Teacher> &featuring_object) const {
    // "regular" / "missed" / "extra"
    str period_code = "regular";
    std::pair<bool, bool> rows_changed{false, false};

    bool klasse_changed = false;
    bool room_changed = false;
    bool teacher_changed = false;

    for (const auto &k: klassen) {
        if (period_code_class(k) != "regular") {
            klasse_changed = true;
            break;
        }
    }

    for (const auto &r: Vector_Utils::concat_ranges(rooms, original_rooms)) {
        if (period_code_room(r) != "regular") {
            klasse_changed = true;
            break;
        }
    }

    for (const auto &t: Vector_Utils::concat_ranges(teachers, original_teachers)) {
        if (period_code_teacher(t) != "regular") {
            klasse_changed = true;
            break;
        }
    }

    if (const Class *c_ptr = std::get_if<Class>(&featuring_object)) {
        period_code = period_code_class(*c_ptr);
        rows_changed = {teacher_changed, room_changed};
    } else if (const Room *r_ptr = std::get_if<Room>(&featuring_object)) {
        period_code = period_code_room(*r_ptr);
        rows_changed = {teacher_changed, klasse_changed};
    } else if (const Teacher *t_ptr = std::get_if<Teacher>(&featuring_object)) {
        period_code = period_code_teacher(*t_ptr);
        rows_changed = {room_changed, klasse_changed};
    }

    return {period_code, rows_changed};
}

[[nodiscard]] str Period::subjects_str() const {
    str subject_str = Str_Utils::join(", ", subjects, &Subject::name);

    if (subject_str.empty()) {
        return Config::HTMLStyleConfig::unknown_element_symbol;
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
        return Config::HTMLStyleConfig::unknown_element_symbol;
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
        return Config::HTMLStyleConfig::unknown_element_symbol;
    }

    return teacher_str;
}

[[nodiscard]] str Period::klassen_str() const {
    std::map<char, std::unordered_set<str> > klassen_level_to_letter;

    for (const auto &klasse: klassen) {
        char klassen_level = klasse.name.at(0);
        str klassen_letter = klasse.name.substr(1);

        klassen_level_to_letter[klassen_level].insert(klassen_letter);
    }

    std::vector<str> parts;

    for (const auto &[klassen_level, letters]: klassen_level_to_letter) {
        str part;
        part += klassen_level;

        std::vector<str> sorted_letters(letters.begin(), letters.end());
        std::ranges::sort(sorted_letters);

        for (const auto &letter: sorted_letters) {
            part += letter;
        }

        parts.push_back(std::move(part));
    }

    str klassen_str;

    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0)
            klassen_str += ", ";
        klassen_str += parts[i];
    }

    if (klassen_str.empty()) {
        return Config::HTMLStyleConfig::unknown_element_symbol;
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

std::tuple<str, str, str, datetime, datetime>
Period::formatted_list(const std::variant<Class, Room, Teacher> &featuring_object, const bool regular_plan) const {
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

str Period::formatted_string(const std::variant<Class, Room, Teacher> &featuring_object,
                             const bool regular_plan) const {
    auto [row1, row2, row3, p_start, p_end] = formatted_list(featuring_object, regular_plan);

    return row1 + " " + row2 + " " + row3;
}

str Period::formatted_string_with_date_part(const std::variant<Class, Room, Teacher> &featuring_object,
                                            const bool regular_plan) const {
    auto [row1, row2, row3, p_start, p_end] = formatted_list(featuring_object, regular_plan);
    return row1 + " " + row2 + " " + row3 + ": " + Date_Utils::datetime_to_str(p_start, "%Y-%m-%d %H:%M:%S") + " - " +
           Date_Utils::datetime_to_str(p_end, "%Y-%m-%d %H:%M:%S");
}

std::tuple<int, day_time, day_time, std::vector<Subject>, std::vector<Class>, std::vector<Room>, std::vector<Teacher> >
Period::regular_plan_identifier() const {
    unsigned int weekday =
            std::chrono::weekday{std::chrono::sys_days{std::chrono::floor<std::chrono::days>(start)}}.c_encoding();
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
    return std::format("Period(start={}, end={}, subjects={}, klassen={}, rooms={}, original_rooms={}, teachers={}, "
                       "original_teachers={})",
                       Date_Utils::datetime_to_str(start, "%Y-%m-%d %H:%M:%S"),
                       Date_Utils::datetime_to_str(end, "%Y-%m-%d %H:%M:%S"), Vector_Utils::vector_to_string(subjects),
                       Vector_Utils::vector_to_string(klassen), Vector_Utils::vector_to_string(rooms),
                       Vector_Utils::vector_to_string(original_rooms), Vector_Utils::vector_to_string(teachers),
                       Vector_Utils::vector_to_string(original_teachers));
}

bool Period::operator==(const Period &other) const {
    return start == other.start && end == other.end && subjects == other.subjects &&
           raw_period_code == other.raw_period_code && klassen == other.klassen && rooms == other.rooms &&
           original_rooms == other.original_rooms && teachers == other.teachers &&
           original_teachers == other.original_teachers && student_group == other.student_group &&
           activity_type == other.activity_type && bk_remark == other.bk_remark && bk_text == other.bk_text &&
           flags == other.flags && ls_number == other.ls_number && ls_text == other.ls_text &&
           subst_text == other.subst_text && period_type == other.period_type && period_id == other.period_id;
}
