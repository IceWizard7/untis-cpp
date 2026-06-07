#pragma once

#include <unordered_set>
#include "base_entity.hpp"

class Teacher final : public Base_Entity {
public:
    Teacher(str n, str ln, int i);
    ~Teacher() override;

    [[nodiscard]] std::variant<std::unordered_set<str>, str> subjects() const;

    static str get_name(int raw_teacher_id);

    static str get_long_name(int raw_teacher_id);

    static Teacher from_teacher_id(int raw_teacher_id);

    static Teacher from_teacher_name(const str &raw_teacher_name);

    static Teacher from_teacher_long_name(const str &raw_teacher_long_name);

    [[nodiscard]] str to_string() const override;
};
