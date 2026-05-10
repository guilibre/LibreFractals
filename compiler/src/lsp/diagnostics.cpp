#include "diagnostics.hpp"

#include "json.hpp"
#include "parser/parser.hpp"
#include "parser/tokenizer.hpp"

namespace Lsp {

auto get_diagnostics(const std::string &source) -> std::string {
    auto tokens = Tokenizer::tokenize(source);
    auto result = Parser::parse(tokens);

    std::string out = "[";
    bool first = true;

    auto emit = [&](size_t line, size_t col_start, size_t col_end,
                    const char *msg) {
        if (!first) out += ',';
        first = false;
        out += "{\"message\":" + json_str(msg);
        out += ",\"from\":" + json_pos(line, col_start);
        out += ",\"to\":" + json_pos(line, col_end);
        out += R"(,"severity":"error"})";
    };

    for (const auto &entry : tokens) {
        if (std::holds_alternative<Tokenizer::Error>(entry)) {
            const auto &e = std::get<Tokenizer::Error>(entry);
            emit(e.line, e.column_start, e.column_end, "Unexpected character");
        }
    }

    for (const auto &e : result.errors) {
        emit(e.line, e.column_start, e.column_end, "Syntax error");
    }

    out += ']';
    return out;
}

} // namespace Lsp
