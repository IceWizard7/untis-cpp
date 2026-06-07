#pragma once

#include <unordered_map>

#include "utils/types.hpp"

class Base_Entity {
public:
    str name;
    str long_name;
    int entity_id;

    Base_Entity(str n, str ln, int id);

    virtual ~Base_Entity();

    [[nodiscard]] str to_string(const str &class_name) const;

    [[nodiscard]] virtual str to_string() const;

    bool operator==(const Base_Entity &other) const;

    std::ostream &print(std::ostream &os) const;
};
