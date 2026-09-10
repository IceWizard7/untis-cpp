#include "date_utils.hpp"

#include <iomanip>
#include <ctime>
#include <sstream>
#include <stdexcept>

namespace Date_Utils {
    namespace {
        std::tm calendar_tm(const date &d) {
            if (!d.ok()) throw std::invalid_argument("Invalid calendar date");
            std::tm tm{};
            tm.tm_year = static_cast<int>(d.year()) - 1900;
            tm.tm_mon = static_cast<unsigned>(d.month()) - 1;
            tm.tm_mday = static_cast<unsigned>(d.day());
            tm.tm_wday = std::chrono::weekday{std::chrono::sys_days{d}}.c_encoding();
            tm.tm_yday = static_cast<int>((std::chrono::sys_days{d} - std::chrono::sys_days{d.year()/1/1}).count());
            return tm;
        }
    }
    day_time datetime_to_time(const datetime date_time) {
        return std::chrono::seconds{date_time - std::chrono::floor<std::chrono::days>(date_time)};
    }

    date datetime_to_date(const datetime date_time) {
        return date{std::chrono::floor<std::chrono::days>(date_time)};
    }

    str datetime_to_str(const datetime &tp, const str &format) {
        auto tm = calendar_tm(datetime_to_date(tp));
        const auto time = std::chrono::hh_mm_ss{datetime_to_time(tp)};
        tm.tm_hour = static_cast<int>(time.hours().count());
        tm.tm_min = static_cast<int>(time.minutes().count());
        tm.tm_sec = static_cast<int>(time.seconds().count());

        std::ostringstream oss;
        oss << std::put_time(&tm, format.c_str());

        return oss.str();
    }

    str date_to_str(const date &d, const str &format) {
        const auto tm = calendar_tm(d);

        std::ostringstream oss;
        oss << std::put_time(&tm, format.c_str());

        return oss.str();
    }

    str daytime_to_str(const day_time &t, const str &format) {
        std::tm tm{};
        tm.tm_hour = static_cast<int>(std::chrono::duration_cast<std::chrono::hours>(t).count());
        tm.tm_min = static_cast<int>(std::chrono::duration_cast<std::chrono::minutes>(t).count() % 60);
        tm.tm_sec = static_cast<int>(t.count() % 60);

        std::ostringstream oss;
        oss << std::put_time(&tm, format.c_str());

        return oss.str();
    }

    date add_days(const date &d, const int days) {
        return date{std::chrono::sys_days(d) + std::chrono::days(days)};
    }

    date add_weeks(const date &d, const int weeks) {
        return date{std::chrono::sys_days{d} + std::chrono::weeks{weeks}};
    }

    date get_today() {
        const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        return std::chrono::year{local.tm_year + 1900} /
               std::chrono::month{static_cast<unsigned>(local.tm_mon + 1)} /
               std::chrono::day{static_cast<unsigned>(local.tm_mday)};
    }

    date str_to_date(const str &s, const str &format) {
        std::tm tm{};
        std::istringstream ss(s);

        ss >> std::get_time(&tm, format.c_str());

        if (ss.fail()) {
            throw std::runtime_error("Date parse failed");
        }

        return std::chrono::year{tm.tm_year + 1900} / std::chrono::month{static_cast<unsigned>(tm.tm_mon + 1)} /
               std::chrono::day{static_cast<unsigned>(tm.tm_mday)};
    }

    day_time str_to_daytime(const str &s, const str &format) {
        std::tm tm{};
        std::istringstream ss(s);
        ss >> std::get_time(&tm, format.c_str());

        return std::chrono::hours{tm.tm_hour} + std::chrono::minutes{tm.tm_min} + std::chrono::seconds{tm.tm_sec};
    }

    datetime str_to_datetime(const str &s, const str &format) {
        std::tm tm{};
        std::istringstream ss(s);

        ss >> std::get_time(&tm, format.c_str());

        if (ss.fail()) {
            throw std::runtime_error("Datetime parse failed");
        }

        date d = std::chrono::year{tm.tm_year + 1900} / std::chrono::month{static_cast<unsigned>(tm.tm_mon + 1)} /
                 std::chrono::day{static_cast<unsigned>(tm.tm_mday)};

        if (!d.ok()) {
            throw std::runtime_error("Invalid datetime");
        }

        const day_time t =
                std::chrono::hours{tm.tm_hour} + std::chrono::minutes{tm.tm_min} + std::chrono::seconds{tm.tm_sec};

        return std::chrono::sys_days{d} + t;
    }

    day_time str_to_time(str t) {
        if (t.length() < 4) {
            t = str(4 - t.length(), '0') + t;
        }

        const int hour = std::stoi(t.substr(0, 2));
        const int minute = std::stoi(t.substr(2, 2));

        return std::chrono::seconds{std::chrono::hours{hour} + std::chrono::minutes{minute}};
    }

    datetime combine(const date &d, const day_time &t) {
        const std::chrono::sys_days days{d};

        return std::chrono::time_point_cast<std::chrono::seconds>(days + t);
    }

    int weekday(const date &d) {
        const std::chrono::weekday wd{std::chrono::sys_days{d}};

        // Monday == 0; Sunday == 6
        return (wd.c_encoding() + 6) % 7;
    }

    size_t Date_Hash::operator()(const date &d) const {
        return std::hash<int>{}(static_cast<int>(d.year())) ^ std::hash<unsigned>{}(static_cast<unsigned>(d.month())) ^
               std::hash<unsigned>{}(static_cast<unsigned>(d.day()));
    }

    size_t Date_Hash::operator()(const day_time &d) const {
        return std::hash<long>{}(d.count());
    }

    size_t Date_Hash::operator()(const datetime &dt) const {
        return std::hash<long>{}(dt.time_since_epoch().count());
    }
}; // namespace Date_Utils
