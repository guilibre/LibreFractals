#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace Store::Gallery {

struct Entry {
    std::string hash;
    std::string name;
    int svg_bytes;
    std::string topo;
    std::chrono::system_clock::time_point created_at;
};

struct Page {
    std::vector<Entry> entries;
    int total;
};

void add(const std::string &hash, const std::string &name, int svg_bytes,
         const std::string &topo);
auto page(int page_num, int limit) -> Page;

} // namespace Store::Gallery
