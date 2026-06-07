#pragma once

#include <optional>
#include <unordered_map>

#include "utils/types.hpp"

class Base_Date_Entity {
public:
    str name;
    str long_name;
    int entity_id;
    std::optional<date> start_date;
    std::optional<date> end_date;

    Base_Date_Entity(str n, str ln, int id, date start_date, date end_date);
    ~Base_Date_Entity();

    static std::optional<date> parse_date(const std::optional<int> &value);

    explicit Base_Date_Entity(const std::unordered_map<str, std::variant<str, int>> &raw_obj);
};
