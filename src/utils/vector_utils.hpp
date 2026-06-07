#pragma once

#include <vector>

#include "types.hpp"

namespace Vector_Utils {
    template<typename R1, typename R2>
    std::vector<typename R1::value_type> concat_ranges(const R1 &a, const R2 &b) {
        std::vector<typename R1::value_type> out;
        out.reserve(a.size() + b.size());

        out.insert(out.end(), a.begin(), a.end());
        out.insert(out.end(), b.begin(), b.end());

        return out;
    }

    template<Stringable T>
    str vector_to_string(const std::vector<T> &vec) {
        str result = "[";
        bool first = true;

        for (const auto &x: vec) {
            if (!first)
                result += ", ";
            result += x.to_string();
            first = false;
        }

        result += "]";
        return result;
    }

    template<typename T>
    bool contains_value(const std::vector<T> &vec, const T &val) {
        return std::ranges::find(vec, val) != vec.end();
    }
}; // namespace Vector_Utils
