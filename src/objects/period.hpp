#pragma once

#include <optional>

#include "base_entity/class.hpp"
#include "base_entity/room.hpp"
#include "base_entity/subject.hpp"
#include "base_entity/teacher.hpp"
#include "utils/types.hpp"

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

    Period(std::optional<str> raw_period_code, datetime start, datetime end, std::vector<Subject> subjects,
           std::vector<Class> klassen, std::vector<Room> rooms, std::vector<Room> original_rooms,
           std::vector<Teacher> teachers, std::vector<Teacher> original_teachers, str student_group, str activity_type,
           str bk_remark, str bk_text, str flags, int ls_number, str ls_text, str subst_text, str period_type,
           int period_id);

    ~Period();

    [[nodiscard]] str period_code_class(const Class &klassen_object) const;

    [[nodiscard]] str period_code_room(const Room &room_object) const;

    [[nodiscard]] str period_code_teacher(const Teacher &teacher_object) const;

    [[nodiscard]] std::pair<str, std::pair<bool, bool> >
    get_period_code(const std::variant<Class, Room, Teacher> &featuring_object) const;

    [[nodiscard]] str subjects_str() const;

    [[nodiscard]] str room_str(bool regular_plan) const;

    [[nodiscard]] str teacher_str(bool regular_plan) const;

    [[nodiscard]] str klassen_str() const;

    [[nodiscard]] std::tuple<str, str, str, datetime, datetime> formatted_list_class(bool regular_plan) const;

    [[nodiscard]] std::tuple<str, str, str, datetime, datetime> formatted_list_room(bool regular_plan) const;

    [[nodiscard]] std::tuple<str, str, str, datetime, datetime> formatted_list_teacher(bool regular_plan) const;

    [[nodiscard]] std::tuple<str, str, str, datetime, datetime>
    formatted_list(const std::variant<Class, Room, Teacher> &featuring_object, bool regular_plan) const;

    [[nodiscard]] str formatted_string(const std::variant<Class, Room, Teacher> &featuring_object,
                                       bool regular_plan) const;

    [[nodiscard]] str formatted_string_with_date_part(const std::variant<Class, Room, Teacher> &featuring_object,
                                                      bool regular_plan) const;

    [[nodiscard]] std::tuple<int, day_time, day_time, std::vector<Subject>, std::vector<Class>, std::vector<Room>,
                             std::vector<Teacher> >
    regular_plan_identifier() const;

    [[nodiscard]] str to_string() const;

    bool operator==(const Period &other) const;
};
