#pragma once

#include "config.hpp"

#include <map>
#include <string>
#include <vector>

enum class LogLevels : int { DEBUG = 0, INFO = 1, WARNING = 2, ERROR = 3 };

namespace Color {
    constexpr auto *BLACK = "\033[30m";
    constexpr auto *RED = "\033[31m";
    constexpr auto *GREEN = "\033[32m";
    constexpr auto *YELLOW = "\033[33m";
    constexpr auto *BLUE = "\033[34m";
    constexpr auto *MAGENTA = "\033[35m";
    constexpr auto *CYAN = "\033[36m";
    constexpr auto *WHITE = "\033[37m";

    constexpr auto *BRIGHT_BLACK = "\033[90m";
    constexpr auto *BRIGHT_RED = "\033[91m";
    constexpr auto *BRIGHT_GREEN = "\033[92m";
    constexpr auto *BRIGHT_YELLOW = "\033[93m";
    constexpr auto *BRIGHT_BLUE = "\033[94m";
    constexpr auto *BRIGHT_MAGENTA = "\033[95m";
    constexpr auto *BRIGHT_CYAN = "\033[96m";
    constexpr auto *BRIGHT_WHITE = "\033[97m";

    constexpr auto *RESET = "\033[0m";
}; // namespace Color

class Logger {
    std::map<LogLevels, std::vector<str>> log_messages;
    std::vector<LogLevels> only_log_levels;

    void log(const str &message, LogLevels level);

public:
    Logger();
    ~Logger();

    static str current_time();

    template<Stringable T>
    void log_debug(const T &message) {
        log(message, LogLevels::DEBUG);
    }

    template<Stringable T>
    void log_info(const T &message) {
        log(message, LogLevels::INFO);
    }

    template<Stringable T>
    void log_warning(const T &message) {
        log(message, LogLevels::WARNING);
    }

    template<Stringable T>
    void log_error(const T &message) {
        log(message, LogLevels::ERROR);
    }

    void log_levels(const std::vector<LogLevels> &levels);
};
