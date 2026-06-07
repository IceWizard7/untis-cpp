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

template<typename T>
std::optional<T> Cache::get_from_cache(const str &key) {
    const auto it = cache.find(key);

    if (it == cache.end())
        return std::nullopt;

    try {
        return it->second.get<T>();
    } catch (...) {
        return std::nullopt;
    }
}

template<typename T>
void Cache::update_cache(const str &key, T value) {
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

    json j;
    in >> j;

    cache = j.get<decltype(cache)>();
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
