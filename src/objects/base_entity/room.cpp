#include "room.hpp"

Room::Room(str n, str ln, const int i) :
    Base_Entity(std::move(n), std::move(ln), i) {
}

Room::~Room() = default;

[[nodiscard]] str Room::to_string() const {
    return Base_Entity::to_string("Room");
}
