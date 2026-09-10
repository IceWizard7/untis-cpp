#include "calendar.hpp"

#include <algorithm>
#include <stdexcept>
#include "utils/date_utils.hpp"

std::vector<Calendar::Placement> Calendar::layout(const std::vector<CalendarEvent> &events,
                                                 date first_day, int n_days) {
    if (!first_day.ok() || n_days < 1 || n_days > 7) {
        throw std::invalid_argument("Calendar requires a valid first_day and n_days between 1 and 7");
    }
    for (const auto &event : events) {
        if (event.end <= event.start) {
            throw std::invalid_argument("Calendar event end must be after start");
        }
    }
    std::vector<Placement> result;
    for (int day = 0; day < n_days; ++day) {
        const auto midnight = Date_Utils::combine(Date_Utils::add_days(first_day, day), day_time{0});
        const auto next_midnight = midnight + std::chrono::days{1};
        std::vector<Placement> entries;
        for (size_t i = 0; i < events.size(); ++i) {
            const auto &event = events[i];
            if (event.start < next_midnight && midnight < event.end) {
                entries.push_back({i, day, std::max(event.start, midnight) - midnight,
                                   std::min(event.end, next_midnight) - midnight, 0, 1});
            }
        }
        std::stable_sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
            if (a.start != b.start) return a.start < b.start;
            return a.end > b.end;
        });
        for (size_t begin = 0; begin < entries.size();) {
            size_t end = begin;
            auto group_end = entries[begin].end;
            std::vector<day_time> column_ends;
            do {
                auto &entry = entries[end];
                auto free_column = std::find_if(column_ends.begin(), column_ends.end(),
                                               [&](auto t) { return t <= entry.start; });
                entry.column = static_cast<size_t>(free_column - column_ends.begin());
                if (free_column == column_ends.end()) column_ends.push_back(entry.end);
                else *free_column = entry.end;
                group_end = std::max(group_end, entry.end);
                ++end;
            } while (end < entries.size() && entries[end].start < group_end);
            for (size_t i = begin; i < end; ++i) entries[i].columns = column_ends.size();
            begin = end;
        }
        result.insert(result.end(), entries.begin(), entries.end());
    }
    return result;
}
