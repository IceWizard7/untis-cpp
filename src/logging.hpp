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
    std::map<LogLevels, std::vector<std::string>> log_messages;
    std::vector<LogLevels> only_log_levels;

    static std::string current_time();
    static std::string to_string_any(const std::string& msg);

public:
    Logger();
    ~Logger();

    void log_debug(const std::string& message);
    void log_info(const std::string& message);
    void log_warning(const std::string& message);
    void log_error(const std::string& message);
    void _log(const std::string& message, LogLevels level);
    void log_levels(const std::vector<LogLevels>& levels);
};
