#include "class.hpp"

Class::Class(str n, str ln, const int i) : Base_Entity(std::move(n), std::move(ln), i) {}
Class::~Class() = default;

[[nodiscard]] str Class::to_string() const { return Base_Entity::to_string("Class"); }
