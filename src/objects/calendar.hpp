#pragma once

#include <vector>
#include "utils/types.hpp"

// Local wall-clock values in one display timezone, like Period::start/end.
// Intervals are half-open [start, end); no school-hour rounding is performed.
struct CalendarEvent {
    str title;
    datetime start;
    datetime end;
    str location;
    str description;
};

namespace Calendar {
    struct Placement {
        size_t event_index;
        int day_index;
        day_time start;
        day_time end;
        size_t column;
        size_t columns;
    };

    // Split at midnight, clip to the requested dates, and allocate columns to
    // each connected overlap group. Touching endpoints do not overlap.
    std::vector<Placement> layout(const std::vector<CalendarEvent> &events, date first_day, int n_days);
}
