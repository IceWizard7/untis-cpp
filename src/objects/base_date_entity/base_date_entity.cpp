#include "base_date_entity.hpp"

Base_Date_Entity::Base_Date_Entity(str n, str ln, int id, date start_date, date end_date) :
    name(std::move(n)), long_name(std::move(ln)), entity_id(id), start_date(start_date), end_date(end_date) {
}

Base_Date_Entity::~Base_Date_Entity() = default;

std::optional<date> Base_Date_Entity::parse_date(const std::optional<int> &value) {
    if (!value.has_value()) {
        return std::nullopt;
    }

    const str s = std::to_string(*value);
    if (s.size() != 8) {
        // Invalid format
        return std::nullopt;
    }

    const int year = std::stoi(s.substr(0, 4));
    const unsigned month = std::stoi(s.substr(4, 2));
    const unsigned day = std::stoi(s.substr(6, 2));

    date ymd{std::chrono::year{year}, std::chrono::month{static_cast<unsigned>(month)},
             std::chrono::day{static_cast<unsigned>(day)}};

    if (!ymd.ok()) {
        return std::nullopt;
    }

    return ymd;
}

Base_Date_Entity::Base_Date_Entity(
        const std::unordered_map<str, std::variant<str, int> > &raw_obj) :
    name(std::get<str>(raw_obj.at("name"))),
    long_name(std::get<str>(raw_obj.at("longName"))),
    entity_id(std::get<int>(raw_obj.at("id"))),
    start_date(parse_date(std::get<int>(raw_obj.at("startDate")))),
    end_date(parse_date(std::get<int>(raw_obj.at("endDate")))) {
}
