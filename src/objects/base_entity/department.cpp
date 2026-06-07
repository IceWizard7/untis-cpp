#include "department.hpp"

Department::Department(str n, str ln, const int i) :
    Base_Entity(std::move(n), std::move(ln), i) {
}

Department::~Department() = default;

[[nodiscard]] str Department::to_string() const {
    return Base_Entity::to_string("Department");
}
