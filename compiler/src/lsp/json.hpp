#pragma once

#include <string>
#include <string_view>

namespace Lsp {

inline auto json_str(std::string_view s) -> std::string {
    std::string out = "\"";
    for (char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:   out += c;
        }
    }
    return out + "\"";
}

inline auto json_num(size_t n) -> std::string { return std::to_string(n); }

inline auto json_pos(size_t line, size_t col) -> std::string {
    return "{\"line\":" + json_num(line) + ",\"col\":" + json_num(col) + "}";
}

} // namespace Lsp
