#pragma once

#include "base_date_entity.hpp"

class Holiday : public Base_Date_Entity {
public:
    explicit Holiday(const std::unordered_map<str, std::variant<str, int>> &raw_obj) : Base_Date_Entity(raw_obj) {}
};
