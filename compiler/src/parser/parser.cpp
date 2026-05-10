#include "parser.hpp"

#include <sstream>

namespace Parser {

namespace {

struct ParserState {
    const std::vector<Tokenizer::TokenResult> &tokens;
    size_t pos = 0;

    auto peek() -> const Tokenizer::Token * {
        if (pos < tokens.size() &&
            std::holds_alternative<Tokenizer::Token>(tokens[pos]))
            return &std::get<Tokenizer::Token>(tokens[pos]);
        return nullptr;
    }

    auto advance() -> const auto * {
        const auto *tok = peek();
        if (tok != nullptr) ++pos;
        return tok;
    }

    auto expect(Tokenizer::TokenType type) -> const Tokenizer::Token * {
        const auto *tok = peek();
        if (tok != nullptr && tok->type == type) {
            ++pos;
            return tok;
        }
        return nullptr;
    }

    auto current_error() -> Tokenizer::Error {
        const auto *tok = peek();
        if (tok != nullptr)
            return {
                .line = tok->line,
                .column_start = tok->column,
                .column_end = tok->column,
            };
        return {
            .line = 0,
            .column_start = 0,
            .column_end = 0,
        };
    }
};

using SymbolResult = std::variant<Symbol, Tokenizer::Error, std::monostate>;
using SymbolsResult = std::variant<std::vector<Symbol>, Tokenizer::Error>;

auto parse_float(const std::string &s) -> float {
    return std::strtof(s.c_str(), nullptr);
}
auto parse_symbols(ParserState &state) -> SymbolsResult;

auto parse_symbol(ParserState &state) -> SymbolResult {
    const auto *tok = state.peek();
    if (tok == nullptr) return std::monostate{};

    // F(n)
    if (tok->type == Tokenizer::F) {
        state.advance();
        if (state.expect(Tokenizer::PARENTHESIS_OPEN) == nullptr)
            return state.current_error();
        const auto *num = state.expect(Tokenizer::NUMBER);
        if (num == nullptr) return state.current_error();
        if (state.expect(Tokenizer::PARENTHESIS_CLOSE) == nullptr)
            return state.current_error();
        return Symbol{SymbolF{parse_float(num->value)}};
    }

    // B(n)
    if (tok->type == Tokenizer::B) {
        state.advance();
        if (state.expect(Tokenizer::PARENTHESIS_OPEN) == nullptr)
            return state.current_error();
        const auto *num = state.expect(Tokenizer::NUMBER);
        if (num == nullptr) return state.current_error();
        if (state.expect(Tokenizer::PARENTHESIS_CLOSE) == nullptr)
            return state.current_error();
        return Symbol{SymbolB{parse_float(num->value)}};
    }

    // H(n), S(n), V(n), A(n)
    if (tok->type == Tokenizer::H || tok->type == Tokenizer::S ||
        tok->type == Tokenizer::V || tok->type == Tokenizer::A) {
        auto kind = tok->type;
        state.advance();
        if (state.expect(Tokenizer::PARENTHESIS_OPEN) == nullptr)
            return state.current_error();
        const auto *num = state.expect(Tokenizer::NUMBER);
        if (num == nullptr) return state.current_error();
        if (state.expect(Tokenizer::PARENTHESIS_CLOSE) == nullptr)
            return state.current_error();
        float val = parse_float(num->value);
        if (kind == Tokenizer::H) return Symbol{SymbolH{.delta = val}};
        if (kind == Tokenizer::S) return Symbol{SymbolS{.delta = val}};
        if (kind == Tokenizer::V) return Symbol{SymbolV{.delta = val}};
        return Symbol{SymbolA{.delta = val}};
    }

    // *(n)
    if (tok->type == Tokenizer::ASTERISK) {
        state.advance();
        if (state.expect(Tokenizer::PARENTHESIS_OPEN) == nullptr)
            return state.current_error();
        const auto *num = state.expect(Tokenizer::NUMBER);
        if (num == nullptr) return state.current_error();
        if (state.expect(Tokenizer::PARENTHESIS_CLOSE) == nullptr)
            return state.current_error();
        return Symbol{SymbolScale{parse_float(num->value)}};
    }

    // %(n)
    if (tok->type == Tokenizer::PERCENT) {
        state.advance();
        if (state.expect(Tokenizer::PARENTHESIS_OPEN) == nullptr)
            return state.current_error();
        const auto *num = state.expect(Tokenizer::NUMBER);
        if (num == nullptr) return state.current_error();
        if (state.expect(Tokenizer::PARENTHESIS_CLOSE) == nullptr)
            return state.current_error();
        return Symbol{SymbolStroke{parse_float(num->value)}};
    }

    // +(n) or -(n)
    if (tok->type == Tokenizer::PLUS || tok->type == Tokenizer::MINUS) {
        float sign = (tok->type == Tokenizer::PLUS) ? 1.0F : -1.0F;
        state.advance();
        if (state.expect(Tokenizer::PARENTHESIS_OPEN) == nullptr)
            return state.current_error();
        const auto *num = state.expect(Tokenizer::NUMBER);
        if (num == nullptr) return state.current_error();
        if (state.expect(Tokenizer::PARENTHESIS_CLOSE) == nullptr)
            return state.current_error();
        return Symbol{SymbolRotate{sign * parse_float(num->value)}};
    }

    // [...]
    if (tok->type == Tokenizer::BRACKET_OPEN) {
        state.advance();
        auto inner = parse_symbols(state);
        if (std::holds_alternative<Tokenizer::Error>(inner))
            return std::get<Tokenizer::Error>(inner);
        if (state.expect(Tokenizer::BRACKET_CLOSE) == nullptr)
            return state.current_error();
        auto branch = std::make_unique<SymbolBranch>(
            SymbolBranch{std::move(std::get<std::vector<Symbol>>(inner))});
        return Symbol{std::move(branch)};
    }

    // variable reference
    if (tok->type == Tokenizer::VARIABLE) {
        state.advance();
        return Symbol{SymbolVar{tok->value[0]}};
    }

    return std::monostate{};
}

auto parse_symbols(ParserState &state) -> SymbolsResult {
    std::vector<Symbol> symbols;
    while (true) {
        auto result = parse_symbol(state);
        if (std::holds_alternative<Tokenizer::Error>(result))
            return std::get<Tokenizer::Error>(result);
        if (std::holds_alternative<std::monostate>(result)) break;
        symbols.push_back(std::move(std::get<Symbol>(result)));
    }
    return symbols;
}

auto parse_branch(ParserState &state)
    -> std::variant<Branch, Tokenizer::Error, std::monostate> {
    const auto *tok = state.peek();
    if (tok == nullptr || tok->type == Tokenizer::SEMICOLON ||
        tok->type == Tokenizer::END_OF_FILE)
        return std::monostate{};

    float probability = 0.F;

    if (tok->type == Tokenizer::NUMBER) {
        const auto *num = state.advance();
        if (state.expect(Tokenizer::TILDE) == nullptr)
            return Tokenizer::Error{
                .line = num->line,
                .column_start = num->column,
                .column_end = num->column,
            };
        probability = parse_float(num->value);
    }

    auto result = parse_symbols(state);
    if (std::holds_alternative<Tokenizer::Error>(result))
        return std::get<Tokenizer::Error>(result);

    return Branch{
        .probability = probability,
        .symbols = std::move(std::get<std::vector<Symbol>>(result)),
    };
}

auto parse_alias(ParserState &state)
    -> std::variant<AliasDecl, Tokenizer::Error, std::monostate> {
    const auto *var_tok = state.peek();
    if (var_tok == nullptr || var_tok->type != Tokenizer::VARIABLE)
        return std::monostate{};

    size_t saved = state.pos;
    state.pos++;
    const auto *eq = state.peek();
    if (eq == nullptr || eq->type != Tokenizer::EQUALS) {
        state.pos = saved;
        return std::monostate{};
    }
    char name = var_tok->value[0];
    state.pos++;

    auto sym_result = parse_symbol(state);
    if (std::holds_alternative<std::monostate>(sym_result))
        return state.current_error();
    if (std::holds_alternative<Tokenizer::Error>(sym_result))
        return std::get<Tokenizer::Error>(sym_result);

    auto &sym = std::get<Symbol>(sym_result);
    if (std::holds_alternative<SymbolVar>(sym) ||
        std::holds_alternative<std::unique_ptr<SymbolBranch>>(sym))
        return state.current_error();

    if (state.expect(Tokenizer::SEMICOLON) == nullptr)
        return state.current_error();

    return AliasDecl{.variable = name, .symbol = std::move(sym)};
}

auto parse_rule(ParserState &state)
    -> std::variant<Rule, Tokenizer::Error, std::monostate> {
    const auto *var = state.expect(Tokenizer::VARIABLE);
    if (var == nullptr) return std::monostate{};

    if (state.expect(Tokenizer::ARROW) == nullptr)
        return Tokenizer::Error{
            .line = var->line,
            .column_start = var->column,
            .column_end = var->column,
        };

    std::vector<Branch> branches;
    while (true) {
        auto result = parse_branch(state);
        if (std::holds_alternative<Tokenizer::Error>(result))
            return std::get<Tokenizer::Error>(result);
        if (std::holds_alternative<std::monostate>(result)) break;
        branches.push_back(std::move(std::get<Branch>(result)));

        if (state.expect(Tokenizer::BAR) == nullptr) break;
    }

    if (state.expect(Tokenizer::SEMICOLON) == nullptr)
        return state.current_error();

    return Rule{
        .variable = var->value[0],
        .branches = std::move(branches),
    };
}

} // namespace

auto parse(const std::vector<Tokenizer::TokenResult> &tokens) -> ParseResult {
    ParserState state{.tokens = tokens};
    ParseResult out;

    while (true) {
        const auto *tok = state.peek();
        if (tok == nullptr || tok->type == Tokenizer::END_OF_FILE) break;

        // @ symbols ;  — axiom declaration
        if (tok->type == Tokenizer::AT) {
            state.advance();
            auto syms = parse_symbols(state);
            if (std::holds_alternative<Tokenizer::Error>(syms)) {
                out.errors.push_back(std::get<Tokenizer::Error>(syms));
            } else {
                out.program.axiom =
                    std::move(std::get<std::vector<Symbol>>(syms));
            }
            if (state.expect(Tokenizer::SEMICOLON) == nullptr)
                out.errors.push_back(state.current_error());
            continue;
        }

        // ! NUMBER ;  — steps declaration
        if (tok->type == Tokenizer::BANG) {
            state.advance();
            const auto *num = state.expect(Tokenizer::NUMBER);
            if (num == nullptr) {
                out.errors.push_back(state.current_error());
            } else {
                out.program.steps = static_cast<int>(parse_float(num->value));
            }
            if (state.expect(Tokenizer::SEMICOLON) == nullptr)
                out.errors.push_back(state.current_error());
            continue;
        }

        // ? NUMBER ;  — seed declaration
        if (tok->type == Tokenizer::QUESTION) {
            state.advance();
            const auto *num = state.expect(Tokenizer::NUMBER);
            if (num == nullptr) {
                out.errors.push_back(state.current_error());
            } else {
                out.program.seed =
                    static_cast<uint32_t>(parse_float(num->value));
            }
            if (state.expect(Tokenizer::SEMICOLON) == nullptr)
                out.errors.push_back(state.current_error());
            continue;
        }

        {
            auto alias = parse_alias(state);
            if (std::holds_alternative<AliasDecl>(alias)) {
                out.program.aliases.push_back(
                    std::move(std::get<AliasDecl>(alias)));
                continue;
            }
            if (std::holds_alternative<Tokenizer::Error>(alias)) {
                out.errors.push_back(std::get<Tokenizer::Error>(alias));
                while (true) {
                    const auto *t = state.peek();
                    if (t == nullptr || t->type == Tokenizer::END_OF_FILE)
                        break;
                    const bool semi = (t->type == Tokenizer::SEMICOLON);
                    state.advance();
                    if (semi) break;
                }
                continue;
            }
        }

        auto result = parse_rule(state);
        if (std::holds_alternative<Tokenizer::Error>(result)) {
            out.errors.push_back(std::get<Tokenizer::Error>(result));
            while (true) {
                const auto *t = state.peek();
                if (t == nullptr || t->type == Tokenizer::END_OF_FILE) break;
                const bool was_semicolon = (t->type == Tokenizer::SEMICOLON);
                state.advance();
                if (was_semicolon) break;
            }
            continue;
        }
        if (std::holds_alternative<std::monostate>(result)) break;
        out.program.rules.push_back(std::move(std::get<Rule>(result)));
    }

    return out;
}

namespace {

auto print_symbols(std::ostringstream &ss, const std::vector<Symbol> &symbols,
                   int indent) -> void;

auto print_symbol(std::ostringstream &ss, const Symbol &sym, size_t indent)
    -> void {
    std::string pad(indent * 2, ' ');
    std::visit(
        [&](const auto &s) {
            using T = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<T, SymbolF>)
                ss << pad << "F(" << s.distance << ")\n";
            else if constexpr (std::is_same_v<T, SymbolB>)
                ss << pad << "B(" << s.distance << ")\n";
            else if constexpr (std::is_same_v<T, SymbolScale>)
                ss << pad << "*(" << s.factor << ")\n";
            else if constexpr (std::is_same_v<T, SymbolRotate>)
                ss << pad << (s.angle >= 0 ? "+" : "") << "(" << s.angle
                   << ")\n";
            else if constexpr (std::is_same_v<T, SymbolH>)
                ss << pad << "H(" << s.delta << ")\n";
            else if constexpr (std::is_same_v<T, SymbolS>)
                ss << pad << "S(" << s.delta << ")\n";
            else if constexpr (std::is_same_v<T, SymbolV>)
                ss << pad << "V(" << s.delta << ")\n";
            else if constexpr (std::is_same_v<T, SymbolA>)
                ss << pad << "A(" << s.delta << ")\n";
            else if constexpr (std::is_same_v<T, SymbolVar>)
                ss << pad << s.name << "\n";
            else if constexpr (std::is_same_v<T,
                                              std::unique_ptr<SymbolBranch>>) {
                ss << pad << "[\n";
                print_symbols(ss, s->symbols, indent + 1);
                ss << pad << "]\n";
            }
        },
        sym);
}

auto print_symbols(std::ostringstream &ss, const std::vector<Symbol> &symbols,
                   int indent) -> void {
    for (const auto &sym : symbols) print_symbol(ss, sym, indent);
}

} // namespace

auto print(const ParseResult &result) -> std::string {
    std::ostringstream ss;

    for (const auto &rule : result.program.rules) {
        ss << rule.variable << " ->\n";
        for (size_t i = 0; i < rule.branches.size(); ++i) {
            const auto &branch = rule.branches[i];
            if (i > 0) ss << "  |\n";
            if (branch.probability != 0.F)
                ss << "  " << branch.probability << "~\n";
            print_symbols(ss, branch.symbols, 1);
        }
        ss << ";\n";
    }

    for (const auto &err : result.errors)
        ss << "error at " << err.line << ":" << err.column_start << "-"
           << err.column_end << "\n";

    return ss.str();
}

} // namespace Parser
