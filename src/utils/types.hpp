#pragma once

#include <chrono>
#include <nlohmann/json.hpp>

using date = std::chrono::year_month_day;
using day_time = std::chrono::seconds;
using datetime = std::chrono::sys_time<std::chrono::seconds>;
using str = std::string;
using uuid = str;
using json = nlohmann::json;

template<typename T>
concept HasToString = requires(const T &x) {
    { x.to_string() } -> std::convertible_to<str>;
};

template<typename T>
concept Stringable = std::convertible_to<T, str> || HasToString<T>;
