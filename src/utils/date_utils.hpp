#pragma once

#include "types.hpp"

namespace Date_Utils {
    day_time datetime_to_time(datetime date_time);

    date datetime_to_date(datetime date_time);

    str datetime_to_str(const datetime &tp, const str &format);

    str date_to_str(const date &d, const str &format);

    date str_to_date(const str &s, const str &format);

    day_time str_to_daytime(const str &s, const str &format);

    datetime str_to_datetime(const str &s, const str &format);

    day_time str_to_time(str t);

    datetime combine(const date &d, const day_time &t);

    int weekday(const date &d);

    str daytime_to_str(const day_time &t, const str &format);

    date add_days(const date &d, int days);

    date add_weeks(const date &d, int weeks);

    date get_today();

    struct Date_Hash {
        size_t operator()(const date &d) const;
        size_t operator()(const day_time &d) const;
        size_t operator()(const datetime &dt) const;
    };
}; // namespace Date_Utils
