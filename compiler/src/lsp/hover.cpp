#include "hover.hpp"

#include "json.hpp"
#include "parser/tokenizer.hpp"

namespace Lsp {

namespace {

auto token_description(const Tokenizer::Token &tok) -> const char * {
    switch (tok.type) {
    case Tokenizer::F:
        return "**F(n)** — Move forward drawing a line of length `n`";
    case Tokenizer::B:
        return "**B(n)** — Move backward drawing a line of length `n`";
    case Tokenizer::H:
        return "**H(n)** — Shift hue by `n` degrees (0–360)";
    case Tokenizer::S:
        return "**S(n)** — Shift saturation by `n` (-1 to 1)";
    case Tokenizer::V:
        return "**V(n)** — Shift value (brightness) by `n` (-1 to 1)";
    case Tokenizer::A:
        return "**A(n)** — Shift alpha (opacity) by `n` (-1 to 1)";
    case Tokenizer::ARROW:
        return "**`->`** — Rewrite rule arrow: `variable -> symbols;`";
    case Tokenizer::AT:
        return "**`@`** — Axiom declaration: `@ symbols;`";
    case Tokenizer::BANG:
        return "**`!`** — Steps declaration: `! n;`";
    case Tokenizer::QUESTION:
        return "**`?`** — Random seed declaration: `? n;`";
    case Tokenizer::ASTERISK:
        return "**`*(n)`** — Scale stroke length by factor `n`";
    case Tokenizer::PLUS:
        return "**`+(n)`** — Rotate clockwise by `n` degrees";
    case Tokenizer::MINUS:
        return "**`-(n)`** — Rotate counter-clockwise by `n` degrees";
    case Tokenizer::PERCENT:
        return "**`%(n)`** — Multiply stroke width by factor `n`";
    case Tokenizer::BRACKET_OPEN:
        return "**`[`** — Begin branch: push turtle state onto stack";
    case Tokenizer::BRACKET_CLOSE:
        return "**`]`** — End branch: pop turtle state from stack";
    case Tokenizer::BAR:
        return "**`|`** — Probabilistic branch separator";
    case Tokenizer::DOLLAR:
        return "**`$`** — Min duration declaration: `$ n;` (seconds)";
    case Tokenizer::AMPERSAND:
        return "**`&`** — Glissando fraction declaration: `& n;` (0 to 1)";
    case Tokenizer::EQUALS:
        return "**`=`** — Alias assignment: `variable = symbols;`";
    case Tokenizer::TILDE:
        return "**`~`** — Probability separator: `p~ symbols`";
    case Tokenizer::SEMICOLON:
        return "**`;`** — Statement terminator";
    case Tokenizer::NUMBER:
        return "**Number** — Numeric literal";
    case Tokenizer::VARIABLE:
    default:
        return nullptr;
    }
}

} // namespace

auto get_hover(const std::string &source, size_t line, size_t col)
    -> std::string {
    auto tokens = Tokenizer::tokenize(source);

    for (const auto &entry : tokens) {
        if (!std::holds_alternative<Tokenizer::Token>(entry)) continue;
        const auto &tok = std::get<Tokenizer::Token>(entry);
        if (tok.type == Tokenizer::END_OF_FILE) break;

        if (tok.line != line) continue;
        if (col < tok.column || col >= tok.column + tok.length) continue;

        if (tok.type == Tokenizer::VARIABLE) {
            std::string desc = "**Variable `" + tok.value +
                               "`** — User-defined rewrite symbol";
            return "{\"contents\":" + json_str(desc) + "}";
        }

        const char *desc = token_description(tok);
        if (desc == nullptr) return "null";
        return "{\"contents\":" + json_str(desc) + "}";
    }

    return "null";
}

} // namespace Lsp
