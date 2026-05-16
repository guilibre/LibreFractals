#include "gallery.hpp"

#include "../math/genealogy.hpp"

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

void add(const std::string &hash, const std::string &name, int svg_bytes,
         const std::string &topo) {
    const auto now = std::chrono::system_clock::now();
    std::unique_lock lock(store_mutex());

    std::vector<std::string> expired_hashes;
    while (!store().empty() && now - store().front().created_at > TTL) {
        expired_hashes.push_back(store().front().hash);
        store().pop_front();
    }
    while (store().size() >= MAX_ENTRIES) {
        expired_hashes.push_back(store().front().hash);
        store().pop_front();
    }
    if (!expired_hashes.empty())
        Math::Genealogy::graph().remove_expired(expired_hashes);

    std::vector<std::pair<std::string, std::string>> existing;
    existing.reserve(store().size());
    for (const auto &e : store()) {
        if (e.hash == hash) return;
        existing.emplace_back(e.hash, e.topo);
    }

    store().push_back({
        .hash = hash,
        .name = name,
        .svg_bytes = svg_bytes,
        .topo = topo,
        .created_at = now,
    });

    if (!existing.empty()) Math::Genealogy::graph().link(hash, topo, existing);
}

auto page(int page_num, int limit) -> Page {
    const auto now = std::chrono::system_clock::now();
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
