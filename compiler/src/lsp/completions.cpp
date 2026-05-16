#include "completions.hpp"

#include "json.hpp"
#include "parser/tokenizer.hpp"
#include <array>
#include <set>

namespace Lsp {

namespace {

enum class Context : uint8_t { TopLevel, SymbolList, NumberExpected };

struct CompletionItem {
    const char *label;
    const char *detail;
    const char *kind;
};

constexpr std::array<CompletionItem, 11> k_builtins = {{
    {
        .label = "F",
        .detail = "Move forward drawing a line",
        .kind = "builtin",
    },
    {
        .label = "B",
        .detail = "Move forward without drawing a line",
        .kind = "builtin",
    },
    {
        .label = "H",
        .detail = "Shift hue",
        .kind = "builtin",
    },
    {
        .label = "S",
        .detail = "Shift saturation",
        .kind = "builtin",
    },
    {
        .label = "V",
        .detail = "Shift value (brightness)",
        .kind = "builtin",
    },
    {
        .label = "A",
        .detail = "Shift alpha (opacity)",
        .kind = "builtin",
    },
    {
        .label = "*(n)",
        .detail = "Scale stroke length by n",
        .kind = "builtin",
    },
    {
        .label = "+(n)",
        .detail = "Rotate clockwise by n degrees",
        .kind = "builtin",
    },
    {
        .label = "-(n)",
        .detail = "Rotate counter-clockwise by n degrees",
        .kind = "builtin",
    },
    {
        .label = "%(n)",
        .detail = "Multiply stroke width by n",
        .kind = "builtin",
    },
    {
        .label = "[...]",
        .detail = "Branch: push/pop turtle state",
        .kind = "builtin",
    },
}};

constexpr std::array<CompletionItem, 5> k_keywords = {{
    {
        .label = "@",
        .detail = "Axiom declaration: @ symbols;",
        .kind = "keyword",
    },
    {
        .label = "!",
        .detail = "Steps declaration: ! n;",
        .kind = "keyword",
    },
    {
        .label = "?",
        .detail = "Random seed declaration: ? n;",
        .kind = "keyword",
    },
    {
        .label = "$",
        .detail = "Min duration declaration: $ n;",
        .kind = "keyword",
    },
    {
        .label = "&",
        .detail = "Glissando fraction declaration: & n;",
        .kind = "keyword",
    },
}};

auto item_json(const CompletionItem &item) -> std::string {
    return "{\"label\":" + json_str(item.label) +
           ",\"detail\":" + json_str(item.detail) +
           ",\"kind\":" + json_str(item.kind) + "}";
}

} // namespace

auto get_completions(const std::string &source, size_t line, size_t col)
    -> std::string {
    auto tokens = Tokenizer::tokenize(source);

    std::set<char> rule_vars;
    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (!std::holds_alternative<Tokenizer::Token>(tokens[i])) continue;
        if (!std::holds_alternative<Tokenizer::Token>(tokens[i + 1])) continue;
        const auto &a = std::get<Tokenizer::Token>(tokens[i]);
        const auto &b = std::get<Tokenizer::Token>(tokens[i + 1]);
        if (a.type == Tokenizer::VARIABLE && b.type == Tokenizer::ARROW)
            rule_vars.insert(a.value[0]);
    }

    Context ctx = Context::TopLevel;
    for (const auto &entry : tokens) {
        if (!std::holds_alternative<Tokenizer::Token>(entry)) continue;
        const auto &tok = std::get<Tokenizer::Token>(entry);
        if (tok.type == Tokenizer::END_OF_FILE) break;
        if (tok.line > line || (tok.line == line && tok.column > col)) break;

        switch (tok.type) {
        case Tokenizer::SEMICOLON:
            ctx = Context::TopLevel;
            break;
        case Tokenizer::AT:
            ctx = Context::SymbolList;
            break;
        case Tokenizer::BANG:
        case Tokenizer::QUESTION:
        case Tokenizer::DOLLAR:
        case Tokenizer::AMPERSAND:
            ctx = Context::NumberExpected;
            break;
        case Tokenizer::EQUALS:
        case Tokenizer::ARROW:
        case Tokenizer::BAR:
            ctx = Context::SymbolList;
            break;
        default:
            break;
        }
    }

    if (ctx == Context::NumberExpected) return "[]";

    std::string out = "[";
    bool first = true;

    auto push = [&](const std::string &item_json) -> void {
        if (!first) out += ',';
        first = false;
        out += item_json;
    };

    if (ctx == Context::SymbolList) {
        for (const auto &item : k_builtins) push(item_json(item));
        for (char v : rule_vars) {
            CompletionItem item{
                .label = nullptr,
                .detail = "User-defined rewrite symbol",
                .kind = "variable",
            };
            std::string label(1, v);
            push("{\"label\":" + json_str(label) +
                 ",\"detail\":" + json_str(item.detail) +
                 ",\"kind\":" + json_str(item.kind) + "}");
        }
    } else {
        for (const auto &item : k_keywords) push(item_json(item));
        for (char v : rule_vars) {
            std::string label(1, v);
            push("{\"label\":" + json_str(label) +
                 R"(,"detail":"User-defined rule","kind":"variable"})");
        }
    }

    out += ']';
    return out;
}

} // namespace Lsp
