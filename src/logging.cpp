#include "logging.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

std::string Logger::current_time() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    const std::tm* tm = std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(tm, "%d.%m.%Y %H:%M:%S");
    return oss.str();
}

std::string Logger::to_string_any(const std::string& msg) {
    return msg;
}

Logger::Logger() {
    log_messages[LogLevels::DEBUG] = {};
    log_messages[LogLevels::INFO] = {};
    log_messages[LogLevels::WARNING] = {};
    log_messages[LogLevels::ERROR] = {};

    only_log_levels = {
        LogLevels::INFO,
        LogLevels::WARNING,
        LogLevels::ERROR
    };
}

Logger::~Logger() = default;

void Logger::log_debug(const std::string& message) {
    _log(message, LogLevels::DEBUG);
}

void Logger::log_info(const std::string& message) {
    _log(message, LogLevels::INFO);
}

void Logger::log_warning(const std::string& message) {
    _log(message, LogLevels::WARNING);
}

void Logger::log_error(const std::string& message) {
    _log(message, LogLevels::ERROR);
}

void Logger::_log(const std::string& message, LogLevels level) {
    log_messages[level].push_back(message);

    const std::string time = current_time();

    const char* color;

    switch (level) {
        case LogLevels::DEBUG:
            color = Color::GREEN;
            break;
        case LogLevels::INFO:
            color = Color::CYAN;
            break;
        case LogLevels::WARNING:
            color = Color::YELLOW;
            break;
        case LogLevels::ERROR:
            color = Color::RED;
            break;
        default:
            color = Color::RESET;
            break;
    }

    std::cout << color
        << "[" << static_cast<int>(level) << "] " << time << ": "
        << message
        << Color::RESET << std::endl;

}

void Logger::log_levels(const std::vector<LogLevels>& levels) {
    only_log_levels = levels;
}
