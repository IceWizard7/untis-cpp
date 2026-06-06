#pragma once

#include "config.hpp"

#include <map>
#include <string>
#include <vector>

enum class LogLevels : int {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

namespace Color {
    constexpr const char* GREEN  = "\033[32m";
    constexpr const char* CYAN   = "\033[36m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* RED    = "\033[31m";
    constexpr const char* RESET  = "\033[0m";
};

class Logger {
    std::map<LogLevels, std::vector<str>> log_messages;
    std::vector<LogLevels> only_log_levels;

    static str to_string_any(const str& msg);

public:
    Logger();
    ~Logger();

    static str current_time();

    void log_debug(const str& message);
    void log_info(const str& message);
    void log_warning(const str& message);
    void log_error(const str& message);
    void _log(const str& message, LogLevels level);
    void log_levels(const std::vector<LogLevels>& levels);
};
