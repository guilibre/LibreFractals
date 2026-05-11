#include "codegen.hpp"

#include <algorithm>
#include <random>
#include <sstream>

namespace Codegen {

namespace {

auto pick_branch(const Parser::Rule &rule, std::mt19937 &rng)
    -> const Parser::Branch & {
    const auto &branches = rule.branches;

    bool all_zero = std::ranges::all_of(
        branches, [](const auto &b) -> auto { return b.probability == 0.F; });

    if (all_zero) {
        std::uniform_int_distribution<size_t> dist(0, branches.size() - 1);
        return branches[dist(rng)];
    }

    std::vector<float> weights;
    weights.reserve(branches.size());
    for (const auto &b : branches) weights.push_back(b.probability);

    std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
    return branches[dist(rng)];
}

auto find_rule(const Parser::Program &program, char name)
    -> const Parser::Rule * {
    for (const auto &rule : program.rules)
        if (rule.variable == name) return &rule;
    return nullptr;
}

auto clone_symbols(const std::vector<Parser::Symbol> &src)
    -> std::vector<Parser::Symbol>;

auto clone_symbol(const Parser::Symbol &sym) -> Parser::Symbol {
    return std::visit(
        [](const auto &s) -> Parser::Symbol {
            using T = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<
                              T, std::unique_ptr<Parser::SymbolBranch>>) {
                return std::make_unique<Parser::SymbolBranch>(
                    Parser::SymbolBranch{clone_symbols(s->symbols)});
            } else {
                return s;
            }
        },
        sym);
}

auto clone_symbols(const std::vector<Parser::Symbol> &src)
    -> std::vector<Parser::Symbol> {
    std::vector<Parser::Symbol> out;
    out.reserve(src.size());
    for (const auto &s : src) out.push_back(clone_symbol(s));
    return out;
}

auto emit_symbols(const std::vector<Parser::Symbol> &symbols,
                  const Parser::Program &program, std::vector<TurtleCmd> &out)
    -> void;

auto emit_symbol(const Parser::Symbol &sym, const Parser::Program &program,
                 std::vector<TurtleCmd> &out) -> void {
    std::visit(
        [&](const auto &s) -> auto {
            using T = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<T, Parser::SymbolF>)
                out.push_back({TurtleCmdType::FORWARD, s.distance});
            else if constexpr (std::is_same_v<T, Parser::SymbolB>)
                out.push_back({TurtleCmdType::GAP, s.distance});
            else if constexpr (std::is_same_v<T, Parser::SymbolRotate>)
                out.push_back({TurtleCmdType::ROTATE, s.angle});
            else if constexpr (std::is_same_v<T, Parser::SymbolScale>)
                out.push_back({TurtleCmdType::SCALE, s.factor});
            else if constexpr (std::is_same_v<T, Parser::SymbolStroke>)
                out.push_back({TurtleCmdType::STROKE_WIDTH, s.factor});
            else if constexpr (std::is_same_v<T, Parser::SymbolH>)
                out.push_back({TurtleCmdType::HUE, s.delta});
            else if constexpr (std::is_same_v<T, Parser::SymbolS>)
                out.push_back({TurtleCmdType::SATURATION, s.delta});
            else if constexpr (std::is_same_v<T, Parser::SymbolV>)
                out.push_back({TurtleCmdType::VALUE, s.delta});
            else if constexpr (std::is_same_v<T, Parser::SymbolA>)
                out.push_back({TurtleCmdType::ALPHA, s.delta});
            else if constexpr (std::is_same_v<
                                   T, std::unique_ptr<Parser::SymbolBranch>>) {
                out.push_back({.type = TurtleCmdType::PUSH, .value = 0.F});
                emit_symbols(s->symbols, program, out);
                out.push_back({.type = TurtleCmdType::POP, .value = 0.F});
            } else if constexpr (std::is_same_v<T, Parser::SymbolVar>) {
                for (const auto &alias : program.aliases) {
                    if (alias.variable == s.name) {
                        emit_symbols(alias.symbols, program, out);
                        return;
                    }
                }
                // sem alias: descartado silenciosamente
            }
        },
        sym);
}

auto emit_symbols(const std::vector<Parser::Symbol> &symbols,
                  const Parser::Program &program, std::vector<TurtleCmd> &out)
    -> void {
    for (const auto &sym : symbols) emit_symbol(sym, program, out);
}

auto expand_symbols(const std::vector<Parser::Symbol> &syms,
                    const Parser::Program &program, std::mt19937 &rng)
    -> std::vector<Parser::Symbol>;

auto expand_symbol(const Parser::Symbol &sym, const Parser::Program &program,
                   std::mt19937 &rng) -> std::vector<Parser::Symbol> {
    if (const auto *var = std::get_if<Parser::SymbolVar>(&sym)) {
        const auto *rule = find_rule(program, var->name);
        if (rule != nullptr) {
            const auto &branch = pick_branch(*rule, rng);
            return clone_symbols(branch.symbols);
        }
        std::vector<Parser::Symbol> v;
        v.push_back(clone_symbol(sym));
        return v;
    }
    if (const auto *br =
            std::get_if<std::unique_ptr<Parser::SymbolBranch>>(&sym)) {
        auto inner = expand_symbols((*br)->symbols, program, rng);
        auto new_branch = std::make_unique<Parser::SymbolBranch>(
            Parser::SymbolBranch{std::move(inner)});
        std::vector<Parser::Symbol> v;
        v.emplace_back(std::move(new_branch));
        return v;
    }
    std::vector<Parser::Symbol> v;
    v.push_back(clone_symbol(sym));
    return v;
}

auto expand_symbols(const std::vector<Parser::Symbol> &syms,
                    const Parser::Program &program, std::mt19937 &rng)
    -> std::vector<Parser::Symbol> {
    std::vector<Parser::Symbol> out;
    for (const auto &sym : syms) {
        auto expanded = expand_symbol(sym, program, rng);
        for (auto &s : expanded) out.push_back(std::move(s));
    }
    return out;
}

} // namespace

auto expand(const Parser::Program &program) -> std::vector<TurtleCmd> {
    std::mt19937 rng{
        program.seed.has_value() ? *program.seed : std::random_device{}(),
    };
    std::vector<Parser::Symbol> current = clone_symbols(program.axiom);

    for (int step = 0; step < program.steps; ++step)
        current = expand_symbols(current, program, rng);

    std::vector<TurtleCmd> cmds;
    emit_symbols(current, program, cmds);
    return cmds;
}

auto to_string(const std::vector<TurtleCmd> &cmds) -> std::string {
    std::ostringstream ss;
    for (const auto &cmd : cmds) {
        switch (cmd.type) {
        case TurtleCmdType::FORWARD:
            ss << "FORWARD " << cmd.value << '\n';
            break;
        case TurtleCmdType::GAP:
            ss << "GAP " << cmd.value << '\n';
            break;
        case TurtleCmdType::ROTATE:
            ss << "ROTATE " << cmd.value << '\n';
            break;
        case TurtleCmdType::SCALE:
            ss << "SCALE " << cmd.value << '\n';
            break;
        case TurtleCmdType::STROKE_WIDTH:
            ss << "STROKE_WIDTH " << cmd.value << '\n';
            break;
        case TurtleCmdType::PUSH:
            ss << "PUSH\n";
            break;
        case TurtleCmdType::POP:
            ss << "POP\n";
            break;
        case TurtleCmdType::HUE:
            ss << "HUE " << cmd.value << '\n';
            break;
        case TurtleCmdType::SATURATION:
            ss << "SATURATION " << cmd.value << '\n';
            break;
        case TurtleCmdType::VALUE:
            ss << "VALUE " << cmd.value << '\n';
            break;
        case TurtleCmdType::ALPHA:
            ss << "ALPHA " << cmd.value << '\n';
            break;
        }
    }
    return ss.str();
}

} // namespace Codegen
