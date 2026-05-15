#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace Store::Gallery {

struct Entry {
    std::string hash;
    std::string name;
    int compile_ms;
    std::chrono::system_clock::time_point created_at;
};

struct Page {
    std::vector<Entry> entries;
    int total;
};

void add(std::string hash, std::string name, int compile_ms);
auto page(int page_num, int limit) -> Page;

} // namespace Store::Gallery
