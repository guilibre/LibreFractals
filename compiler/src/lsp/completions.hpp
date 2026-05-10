#pragma once

#include <cstddef>
#include <string>

namespace Lsp {

auto get_completions(const std::string &source, size_t line, size_t col)
    -> std::string;

} // namespace Lsp
