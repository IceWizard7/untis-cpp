#include "subject.hpp"

#include "config.hpp"

Subject::Subject(str n, str ln, const int i) : Base_Entity(std::move(n), std::move(ln), i) {}
Subject::~Subject() = default;

std::tuple<int, int, int> Subject::color() {
    const std::tuple<str, str, int> key = std::make_tuple(name, long_name, entity_id);

    const auto &map = Config::TimeTableMappingConfig::subject_to_color;
    if (const auto it = map.find(key); it != map.end()) {
        return it->second;
    }

    return Config::TimeTableMappingConfig::default_subject_color;
}

[[nodiscard]] str Subject::to_string() const { return Base_Entity::to_string("Subject"); }
