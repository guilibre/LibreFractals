#include "gallery.hpp"

#include <deque>
#include <mutex>
#include <ranges>
#include <shared_mutex>

namespace Store::Gallery {

namespace {

inline constexpr size_t MAX_ENTRIES = 100;
inline constexpr auto TTL = std::chrono::hours(24);

auto store() -> std::deque<Entry> & {
    static std::deque<Entry> instance;
    return instance;
}

auto store_mutex() -> std::shared_mutex & {
    static std::shared_mutex instance;
    return instance;
}

} // namespace

void add(std::string hash, std::string name, int compile_ms) {
    auto now = std::chrono::system_clock::now();
    std::unique_lock lock(store_mutex());
    while (!store().empty() && now - store().front().created_at > TTL)
        store().pop_front();
    while (store().size() >= MAX_ENTRIES) store().pop_front();
    store().push_back({std::move(hash), std::move(name), compile_ms, now});
}

auto page(int page_num, int limit) -> Page {
    auto now = std::chrono::system_clock::now();
    std::shared_lock lock(store_mutex());

    Page result;
    int skip = page_num * limit;
    int shown = 0;

    for (auto &entry : std::ranges::reverse_view(store())) {
        if (now - entry.created_at > TTL) continue;
        result.total++;
        if (skip-- > 0) continue;
        if (shown >= limit) continue;
        result.entries.push_back(entry);
        shown++;
    }

    return result;
}

} // namespace Store::Gallery
