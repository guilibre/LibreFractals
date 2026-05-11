#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace Tokenizer {

enum TokenType : uint8_t {
    END_OF_FILE,
    F,
    B,
    H,
    S,
    V,
    A,
    VARIABLE,
    NUMBER,
    ARROW,
    BAR,
    TILDE,
    SEMICOLON,
    PLUS,
    MINUS,
    ASTERISK,
    PARENTHESIS_OPEN,
    PARENTHESIS_CLOSE,
    BRACKET_OPEN,
    BRACKET_CLOSE,
    AT,
    BANG,
    QUESTION,
    PERCENT,
    EQUALS,
    DOLLAR,
};

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
    size_t length;
};

struct Error {
    size_t line;
    size_t column_start;
    size_t column_end;
};

using TokenResult = std::variant<Token, Error>;

auto tokenize(const std::string &input) -> std::vector<TokenResult>;

auto print(const std::vector<TokenResult> &tokens) -> std::string;

} // namespace Tokenizer
