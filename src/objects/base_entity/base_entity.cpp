#include "base_entity.hpp"

Base_Entity::Base_Entity(str n, str ln, const int id) :
    name(std::move(n)), long_name(std::move(ln)), entity_id(id) {
}

Base_Entity::~Base_Entity() = default;

[[nodiscard]] str Base_Entity::to_string(const str &class_name) const {
    return class_name + "(name=" + name + ", long_name=" + long_name + ", entity_id=" + std::to_string(entity_id) + ")";
}

[[nodiscard]] str Base_Entity::to_string() const {
    return to_string("Base_Entity");
}

bool Base_Entity::operator==(const Base_Entity &other) const {
    return name == other.name && long_name == other.long_name && entity_id == other.entity_id;
}

std::ostream &Base_Entity::print(std::ostream &os) const {
    os << to_string();
    return os;
}
