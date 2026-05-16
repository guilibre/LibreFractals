#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Math::Genealogy {

inline constexpr double SCORE_DUPLICATE = 1.0;
inline constexpr double SCORE_DERIVED = 0.85;
inline constexpr double SCORE_SIMILAR = 0.70;
inline constexpr size_t TOPO_MAX_LEN = 512;

enum class Relation : uint8_t { Duplicate, Derived, Similar };

struct Edge {
    std::string from_hash;
    std::string to_hash;
    double score;
    Relation relation;
};

auto edit_distance(std::string_view a, std::string_view b) -> int;
auto topo_similarity(std::string_view a, std::string_view b) -> double;
auto classify(double score) -> std::optional<Relation>;
auto relation_name(Relation r) -> std::string;

class Graph {
    std::vector<Edge> edges_;

  public:
    void link(const std::string &new_hash, const std::string &new_topo,
              const std::vector<std::pair<std::string, std::string>> &existing);

    [[nodiscard]] auto relatives(const std::string &hash) const
        -> std::vector<Edge>;

    void remove_expired(const std::vector<std::string> &expired_hashes);
};

auto graph() -> Graph &;

} // namespace Math::Genealogy
