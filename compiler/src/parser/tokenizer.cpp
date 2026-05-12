#include "tokenizer.hpp"

namespace Tokenizer {

auto tokenize(const std::string &input) -> std::vector<TokenResult> {
    std::vector<TokenResult> tokens;
    size_t i = 0;
    size_t line = 0;
    size_t column = 0;

    auto advance = [&]() -> void {
        if (i < input.size() && input[i] == '\n') {
            ++line;
            column = 0;
        } else {
            ++column;
        }
        ++i;
    };

    while (i < input.size()) {
        // skip whitespace
        if (std::isspace((unsigned char)input[i]) != 0) {
            advance();
            continue;
        }

        size_t tok_line = line;
        size_t tok_col = column;
        char c = input[i];

        // single-char tokens
        if (c == '+') {
            tokens.emplace_back(Token{
                .type = PLUS,
                .value = "+",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == '*') {
            tokens.emplace_back(Token{
                .type = ASTERISK,
                .value = "*",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == ';') {
            tokens.emplace_back(Token{
                .type = SEMICOLON,
                .value = ";",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == '|') {
            tokens.emplace_back(Token{
                .type = BAR,
                .value = "|",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == '~') {
            tokens.emplace_back(Token{
                .type = TILDE,
                .value = "~",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == '(') {
            tokens.emplace_back(Token{
                .type = PARENTHESIS_OPEN,
                .value = "(",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == ')') {
            tokens.emplace_back(Token{
                .type = PARENTHESIS_CLOSE,
                .value = ")",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == '%') {
            tokens.emplace_back(Token{
                .type = PERCENT,
                .value = "%",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == '=') {
            tokens.emplace_back(Token{
                .type = EQUALS,
                .value = "=",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == '$') {
            tokens.emplace_back(Token{
                .type = DOLLAR,
                .value = "$",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == '&') {
            tokens.emplace_back(Token{
                .type = AMPERSAND,
                .value = "&",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == '@') {
            tokens.emplace_back(Token{
                .type = AT,
                .value = "@",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == '!') {
            tokens.emplace_back(Token{
                .type = BANG,
                .value = "!",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == '?') {
            tokens.emplace_back(Token{
                .type = QUESTION,
                .value = "?",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == '[') {
            tokens.emplace_back(Token{
                .type = BRACKET_OPEN,
                .value = "[",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }
        if (c == ']') {
            tokens.emplace_back(Token{
                .type = BRACKET_CLOSE,
                .value = "]",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }

        // arrow ->
        if (c == '-' && i + 1 < input.size() && input[i + 1] == '>') {
            tokens.emplace_back(Token{
                .type = ARROW,
                .value = "->",
                .line = tok_line,
                .column = tok_col,
                .length = 2,
            });
            advance();
            advance();
            continue;
        }

        // numbers (integers and decimals)
        if (std::isdigit((unsigned char)c) != 0 ||
            (c == '-' && i + 1 < input.size() &&
             std::isdigit((unsigned char)input[i + 1]) != 0)) {
            std::string num;
            if (c == '-') {
                num += c;
                advance();
            }
            while (i < input.size() &&
                   std::isdigit((unsigned char)input[i]) != 0) {
                num += input[i];
                advance();
            }
            if (i < input.size() && input[i] == '.') {
                num += '.';
                advance();
                while (i < input.size() &&
                       std::isdigit((unsigned char)input[i]) != 0) {
                    num += input[i];
                    advance();
                }
            }
            tokens.emplace_back(Token{
                .type = NUMBER,
                .value = num,
                .line = tok_line,
                .column = tok_col,
                .length = num.size(),
            });
            continue;
        }

        if (c == '-') {
            tokens.emplace_back(Token{
                .type = MINUS,
                .value = "-",
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }

        // keywords F, B, H, S, V, A (uppercase)
        if (c == 'F' || c == 'B' || c == 'H' || c == 'S' || c == 'V' ||
            c == 'A') {
            TokenType type = (c == 'F')   ? F
                             : (c == 'B') ? B
                             : (c == 'H') ? H
                             : (c == 'S') ? S
                             : (c == 'V') ? V
                                          : A;
            tokens.emplace_back(Token{
                .type = type,
                .value = std::string(1, c),
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }

        // variable: single lowercase letter
        if (std::islower((unsigned char)c) != 0) {
            tokens.emplace_back(Token{
                .type = VARIABLE,
                .value = std::string(1, c),
                .line = tok_line,
                .column = tok_col,
                .length = 1,
            });
            advance();
            continue;
        }

        // unknown character — register error
        tokens.emplace_back(Error{
            .line = tok_line,
            .column_start = tok_col,
            .column_end = tok_col,
        });
        advance();
    }

    tokens.emplace_back(Token{
        .type = END_OF_FILE,
        .value = "",
        .line = line,
        .column = column,
        .length = 0,
    });
    return tokens;
}

auto print(const std::vector<TokenResult> &tokens) -> std::string {
    std::string result;
    for (const auto &entry : tokens) {
        if (std::holds_alternative<Error>(entry)) {
            const auto &err = std::get<Error>(entry);
            result += "Error(line=" + std::to_string(err.line) +
                      ", column_start=" + std::to_string(err.column_start) +
                      ", column_end=" + std::to_string(err.column_end) + ")\n";
            continue;
        }
        const auto &token = std::get<Token>(entry);
        result += "Token(type=" + std::to_string(token.type) + ", value=\"" +
                  token.value + "\", line=" + std::to_string(token.line) +
                  ", column=" + std::to_string(token.column) +
                  ", length=" + std::to_string(token.length) + ")\n";
    }
    return result;
}

} // namespace Tokenizer
