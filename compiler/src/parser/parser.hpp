#pragma once

#include "parser/tokenizer.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace Parser {

struct SymbolF {
    float distance;
};

struct SymbolB {
    float distance;
};

struct SymbolH {
    float delta;
};

struct SymbolS {
    float delta;
};

struct SymbolV {
    float delta;
};

struct SymbolA {
    float delta;
};

struct SymbolScale {
    float factor;
};

struct SymbolRotate {
    float angle;
};

struct SymbolStroke {
    float factor;
};

struct SymbolVar {
    char name;
};

struct SymbolBranch;

using Symbol = std::variant<SymbolF, SymbolB, SymbolH, SymbolS, SymbolV,
                            SymbolA, SymbolScale, SymbolRotate, SymbolStroke,
                            SymbolVar, std::unique_ptr<SymbolBranch>>;

struct SymbolBranch {
    std::vector<Symbol> symbols;
};

struct Branch {
    float probability;
    std::vector<Symbol> symbols;
};

struct Rule {
    char variable;
    std::vector<Branch> branches;
};

struct AliasDecl {
    char variable;
    Symbol symbol;
};

struct Program {
    std::vector<Rule> rules;
    std::vector<AliasDecl> aliases;
    std::vector<Symbol> axiom;
    int steps = 1;
    std::optional<uint32_t> seed;
};

struct ParseResult {
    Program program;
    std::vector<Tokenizer::Error> errors;
};

auto parse(const std::vector<Tokenizer::TokenResult> &tokens) -> ParseResult;

auto print(const ParseResult &result) -> std::string;

} // namespace Parser
