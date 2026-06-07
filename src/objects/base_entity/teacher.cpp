#include "teacher.hpp"

#include "config.hpp"

Teacher::Teacher(str n, str ln, const int i) :
    Base_Entity(std::move(n), std::move(ln), i) {
}

Teacher::~Teacher() = default;

[[nodiscard]] std::variant<std::unordered_set<str>, str> Teacher::subjects() const {
    const auto *map = &Config::TimeTableMappingConfig::teacher_mapping;

    if (const auto it = map->find(entity_id); it != map->end()) {
        return std::get<2>(it->second);
    }

    return {std::to_string(entity_id)};
}

[[nodiscard]] str Teacher::get_name(const int raw_teacher_id) {
    const auto *map = &Config::TimeTableMappingConfig::teacher_mapping;

    if (const auto it = map->find(raw_teacher_id); it != map->end()) {
        return std::get<1>(it->second);
    }

    return {std::to_string(raw_teacher_id)};
}

[[nodiscard]] str Teacher::get_long_name(const int raw_teacher_id) {
    const auto *map = &Config::TimeTableMappingConfig::teacher_mapping;

    if (const auto it = map->find(raw_teacher_id); it != map->end()) {
        return std::get<0>(it->second);
    }

    return {std::to_string(raw_teacher_id)};
}

[[nodiscard]] Teacher Teacher::from_teacher_id(int raw_teacher_id) {
    return {get_name(raw_teacher_id), get_long_name(raw_teacher_id), raw_teacher_id};
}

[[nodiscard]] Teacher Teacher::from_teacher_name(const str &raw_teacher_name) {
    // Default: 0 -> unknown teacher
    int raw_teacher_id = 0;

    for (const auto &[tid, value]: Config::TimeTableMappingConfig::teacher_mapping) {
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

    for (const auto &[tid, value]: Config::TimeTableMappingConfig::teacher_mapping) {
        if (std::get<0>(value) == raw_teacher_long_name) {
            raw_teacher_id = tid;
            break;
        }
    }

    return {get_name(raw_teacher_id), get_long_name(raw_teacher_id), raw_teacher_id};
}

[[nodiscard]] str Teacher::to_string() const {
    return Base_Entity::to_string("Teacher");
}
