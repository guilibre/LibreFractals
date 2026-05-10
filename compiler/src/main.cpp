#include "parser/codegen.hpp"
#include "parser/parser.hpp"
#include "renderer/svg.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

auto usage(const char *prog) -> void {
    std::cerr
        << "Usage: " << prog << " [options] <input>\n"
        << "\n"
        << "  <input>          Fractal expression string or path to a "
           ".frac file\n"
        << "\n"
        << "Options:\n"
        << "  -f, --file           Treat <input> as a file path\n"
        << "  -o, --output <path>  Write output to file instead of stdout\n"
        << "  --svg                Output SVG instead of turtle IR\n"
        << "  -h, --help           Show this help message\n";
}

} // namespace

auto main(int argc, char *argv[]) -> int {
    bool from_file = false;
    bool output_svg = false;
    std::string input;
    std::string output_path;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            usage(argv[0]);
            return 0;
        }

        if (arg == "-f" || arg == "--file") {
            from_file = true;
        } else if (arg == "--svg") {
            output_svg = true;
        } else if (arg == "-o" || arg == "--output") {
            if (++i >= argc) {
                std::cerr << "Error: " << arg << " requires a path argument\n";
                usage(argv[0]);
                return 1;
            }
            output_path = argv[i];
        } else if (arg.starts_with('-')) {
            std::cerr << "Unknown option: " << arg << "\n";
            usage(argv[0]);
            return 1;
        } else {
            if (!input.empty()) {
                std::cerr << "Error: multiple inputs provided\n";
                usage(argv[0]);
                return 1;
            }
            input = arg;
        }
    }

    if (input.empty()) {
        usage(argv[0]);
        return 1;
    }

    std::string source;
    if (from_file) {
        std::ifstream file(input);
        if (!file) {
            std::cerr << "Error: cannot open file: " << input << "\n";
            return 1;
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        source = ss.str();
    } else {
        source = input;
    }

    auto tokens = Tokenizer::tokenize(source);
    auto parse_result = Parser::parse(tokens);

    if (!parse_result.errors.empty()) {
        for (const auto &err : parse_result.errors)
            std::cerr << "parse error at " << err.line + 1 << ':'
                      << err.column_start + 1 << '\n';
    }

    if (parse_result.program.axiom.empty()) {
        std::cerr << "Error: no axiom defined (use @ <symbols>;)\n";
        return 1;
    }

    auto cmds = Codegen::expand(parse_result.program);
    std::string result =
        output_svg ? Renderer::to_svg(cmds) : Codegen::to_string(cmds);

    if (!output_path.empty()) {
        std::ofstream out(output_path);
        if (!out) {
            std::cerr << "Error: cannot open output file: " << output_path
                      << "\n";
            return 1;
        }
        out << result;
    } else {
        std::cout << result;
    }

    return 0;
}

#ifdef __EMSCRIPTEN__
#include "lsp/completions.hpp"
#include "lsp/diagnostics.hpp"
#include "lsp/hover.hpp"
#include <emscripten/bind.h>

static auto compile_to_svg(const std::string &source) -> std::string {
    auto tokens = Tokenizer::tokenize(source);
    auto result = Parser::parse(tokens);
    if (result.program.axiom.empty()) return "";
    auto cmds = Codegen::expand(result.program);
    return Renderer::to_svg(cmds);
}

static auto compile_to_turtle(const std::string &source) -> std::string {
    auto tokens = Tokenizer::tokenize(source);
    auto result = Parser::parse(tokens);
    if (result.program.axiom.empty()) return "ERROR: no axiom";
    std::string info =
        "steps=" + std::to_string(result.program.steps) +
        " rules=" + std::to_string(result.program.rules.size()) +
        " axiom_syms=" + std::to_string(result.program.axiom.size()) +
        " errors=" + std::to_string(result.errors.size()) + "\n";
    auto cmds = Codegen::expand(result.program);
    return info + Codegen::to_string(cmds);
}

static auto lsp_get_diagnostics(const std::string &source) -> std::string {
    return Lsp::get_diagnostics(source);
}

static auto lsp_get_hover(const std::string &source, int line, int col)
    -> std::string {
    return Lsp::get_hover(source, static_cast<size_t>(line),
                          static_cast<size_t>(col));
}

static auto lsp_get_completions(const std::string &source, int line, int col)
    -> std::string {
    return Lsp::get_completions(source, static_cast<size_t>(line),
                                static_cast<size_t>(col));
}

EMSCRIPTEN_BINDINGS(librefractals) {
    emscripten::function("compile_to_svg", &compile_to_svg);
    emscripten::function("compile_to_turtle", &compile_to_turtle);
    emscripten::function("get_diagnostics", &lsp_get_diagnostics);
    emscripten::function("get_hover", &lsp_get_hover);
    emscripten::function("get_completions", &lsp_get_completions);
}
#endif
