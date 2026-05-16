#include "genealogy.hpp"

#include <algorithm>
#include <ranges>

namespace Math::Genealogy {

auto edit_distance(std::string_view a, std::string_view b) -> int {
    const size_t m = a.size();
    const size_t n = b.size();
    std::vector<int> prev(n + 1);
    std::vector<int> curr(n + 1);
    for (size_t j = 0; j <= n; ++j) prev[j] = static_cast<int>(j);
    for (size_t i = 1; i <= m; ++i) {
        curr[0] = static_cast<int>(i);
        for (size_t j = 1; j <= n; ++j) {
            if (a[i - 1] == b[j - 1])
                curr[j] = prev[j - 1];
            else
                curr[j] = 1 + std::min({prev[j], curr[j - 1], prev[j - 1]});
        }
        std::swap(prev, curr);
    }
    return prev[n];
}

auto topo_similarity(std::string_view a, std::string_view b) -> double {
    if (a.empty() && b.empty()) return 1.0;
    const size_t max_len = std::max(a.size(), b.size());
    return 1.0 - (static_cast<double>(edit_distance(a, b)) /
                  static_cast<double>(max_len));
}

auto classify(double score) -> std::optional<Relation> {
    if (score >= SCORE_DUPLICATE) return Relation::Duplicate;
    if (score >= SCORE_DERIVED) return Relation::Derived;
    if (score >= SCORE_SIMILAR) return Relation::Similar;
    return std::nullopt;
}

auto relation_name(Relation r) -> std::string {
    switch (r) {
    case Relation::Duplicate:
        return "duplicate";
    case Relation::Derived:
        return "derived";
    case Relation::Similar:
        return "similar";
    }
    return "";
}

void Graph::link(
    const std::string &new_hash, const std::string &new_topo,
    const std::vector<std::pair<std::string, std::string>> &existing) {
    if (new_topo.empty()) return;
    for (const auto &[hash, topo] : existing) {
        if (hash == new_hash || topo.empty()) continue;
        double score = topo_similarity(new_topo, topo);
        auto rel = classify(score);
        if (!rel) continue;
        edges_.push_back({
            .from_hash = new_hash,
            .to_hash = hash,
            .score = score,
            .relation = *rel,
        });
    }
}

auto Graph::relatives(const std::string &hash) const -> std::vector<Edge> {
    std::vector<Edge> result;
    for (const auto &edge : edges_)
        if (edge.from_hash == hash || edge.to_hash == hash)
            result.push_back(edge);

    std::ranges::sort(result, std::greater{}, &Edge::score);
    return result;
}

void Graph::remove_expired(const std::vector<std::string> &expired_hashes) {
    std::erase_if(edges_, [&](const Edge &e) {
        return std::ranges::any_of(expired_hashes, [&](const auto &h) -> auto {
            return e.from_hash == h || e.to_hash == h;
        });
    });
}

auto graph() -> Graph & {
    static Graph instance;
    return instance;
}

} // namespace Math::Genealogy
