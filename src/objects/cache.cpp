#include "cache.hpp"

#include <fstream>

std::optional<double> Cache::cache_file_last_changed() const {
    if (!cache_file_path.has_value()) {
        return std::nullopt;
    }
    try {
        const auto now = std::chrono::system_clock::now();

        const auto ftime = std::filesystem::last_write_time(cache_file_path.value());

        // convert file time -> system_clock time_point
        const auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());

        return std::chrono::duration<double>(now - sctp).count();
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

std::optional<json> Cache::get_json(const str &key) const {
    const auto it = cache.find(key);

    if (it == cache.end()) {
        return std::nullopt;
    }

    return it->second;
}

void Cache::set_json(const str &key, json value) {
    cache[key] = std::move(value);
}

void Cache::clear_cache() {
    cache.clear();
}

void Cache::read_cache_from_file() {
    if (!cache_file_path) {
        return;
    }

    std::ifstream in(*cache_file_path);

    if (!in) {
        return;
    }

    try {
        json j;
        in >> j;

        if (!j.is_object()) {
            cache.clear();
            return;
        }

        cache = j.get<decltype(cache)>();
    } catch (const std::exception &) {
        cache.clear();
    }
}

void Cache::write_cache_to_file() const {
    if (!cache_file_path) {
        return;
    }

    std::ofstream out(*cache_file_path);

    if (!out) {
        return;
    }

    const json j(cache);

    out << j.dump(4);
}
