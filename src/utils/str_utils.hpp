#pragma once

#include "types.hpp"

namespace Str_Utils {
    template<typename Container, typename Projection>
    str join(const str &sep, const Container &items, Projection proj) {
        str result;
        bool first = true;
        for (const auto &item: items) {
            if (!first)
                result += sep;
            if constexpr (std::is_member_pointer_v<Projection>) {
                result += item.*proj;
            } else {
                result += proj(item);
            }
            first = false;
        }
        return result;
    }

    template<typename Container>
    str join(const str &sep, const Container &items) {
        str result;
        bool first = true;
        for (const auto &item: items) {
            if (!first)
                result += sep;
            result += item;
            first = false;
        }
        return result;
    }

    template<typename Container>
    str join(const Container &items) {
        return join("", items);
    }
}; // namespace Str_Utils
