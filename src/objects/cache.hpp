#pragma once

#include <filesystem>
#include <optional>
#include <unordered_map>

#include "utils/types.hpp"

class Cache {
    struct TimeTableCacheKey {
        date start_date;
        date end_date;
        str method_name;
    };

    struct ElementCacheKey {
        str method_name;
    };

    struct ElementByNameCacheKey {
        str method_name;
        str object_name;
    };

    struct TimeGridCacheKey {
        str method_name;
    };

private:
    std::unordered_map<str, json> cache;
    std::optional<std::filesystem::path> cache_file_path;

public:
    explicit Cache(std::optional<std::filesystem::path> cache_file) :
        cache_file_path(std::move(cache_file)) {
    }

    [[nodiscard]] std::optional<double> cache_file_last_changed() const;

    [[nodiscard]] std::optional<json> get_json(const str &key) const;

    void set_json(const str &key, json value);

    void clear_cache();

    void read_cache_from_file();

    void write_cache_to_file() const;
};
