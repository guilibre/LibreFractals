#include "parser/codegen.hpp"
#include "parser/parser.hpp"
#include "renderer/audio.hpp"
#include "renderer/midi.hpp"
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
        << "  -o, --output <path>  Write turtle IR to file instead of stdout\n"
        << "  --svg <path>         Write SVG to file\n"
        << "  --midi <path>        Write MIDI to file\n"
        << "  --wav <path>         Write WAV to file\n"
        << "  -h, --help           Show this help message\n";
}

} // namespace

auto main(int argc, char *argv[]) -> int {
    bool from_file = false;
    std::string input;
    std::string svg_path;
    std::string midi_path;
    std::string wav_path;
    std::string ir_path;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            usage(argv[0]);
            return 0;
        }

        if (arg == "-f" || arg == "--file") {
            from_file = true;
        } else if (arg == "--svg") {
            if (++i >= argc) {
                std::cerr << "Error: --svg requires a path argument\n";
                return 1;
            }
            svg_path = argv[i];
        } else if (arg == "--midi") {
            if (++i >= argc) {
                std::cerr << "Error: --midi requires a path argument\n";
                return 1;
            }
            midi_path = argv[i];
        } else if (arg == "--wav") {
            if (++i >= argc) {
                std::cerr << "Error: --wav requires a path argument\n";
                return 1;
            }
            wav_path = argv[i];
        } else if (arg == "-o" || arg == "--output") {
            if (++i >= argc) {
                std::cerr << "Error: " << arg << " requires a path argument\n";
                return 1;
            }
            ir_path = argv[i];
        } else if (arg.starts_with('-')) {
            std::cerr << "Unknown option: " << arg << "\n";
            usage(argv[0]);
            return 1;
        } else {
            if (!input.empty()) {
                std::cerr << "Error: multiple inputs provided\n";
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

    if (!svg_path.empty()) {
        std::ofstream out(svg_path);
        if (!out) {
            std::cerr << "Error: cannot open output file: " << svg_path << "\n";
            return 1;
        }
        out << Image::to_svg(cmds);
    }

    if (!midi_path.empty() || !wav_path.empty()) {
        const float min_dur = parse_result.program.min_duration.value_or(0.1F);
        const float gliss_frac =
            parse_result.program.glissando_frac.value_or(1.F);
        auto notes = Midi::compile(cmds, min_dur, gliss_frac);
        if (!midi_path.empty()) Midi::write(notes, midi_path);
        if (!wav_path.empty()) Audio::to_wav(notes, wav_path);
    }

    if (svg_path.empty() && midi_path.empty() && wav_path.empty()) {
        std::string ir = Codegen::to_string(cmds);
        if (!ir_path.empty()) {
            std::ofstream out(ir_path);
            if (!out) {
                std::cerr << "Error: cannot open output file: " << ir_path
                          << "\n";
                return 1;
            }
            out << ir;
        } else {
            std::cout << ir;
        }
    }

    return 0;
}

#ifdef __EMSCRIPTEN__
#include "lsp/completions.hpp"
#include "lsp/diagnostics.hpp"
#include "lsp/hover.hpp"
#include <emscripten/bind.h>

static auto base64_encode(const std::vector<uint8_t> &data) -> std::string {
    static constexpr std::string_view table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    for (size_t i = 0; i < data.size(); i += 3) {
        const uint32_t b0 = data[i];
        const uint32_t b1 = (i + 1 < data.size()) ? data[i + 1] : 0;
        const uint32_t b2 = (i + 2 < data.size()) ? data[i + 2] : 0;
        const uint32_t triple = (b0 << 16) | (b1 << 8) | b2;
        out.push_back(table[(triple >> 18) & 0x3F]);
        out.push_back(table[(triple >> 12) & 0x3F]);
        out.push_back((i + 1 < data.size()) ? table[(triple >> 6) & 0x3F]
                                            : '=');
        out.push_back((i + 2 < data.size()) ? table[triple & 0x3F] : '=');
    }
    return out;
}

static auto compile_to_wav(const std::string &source) -> std::string {
    auto tokens = Tokenizer::tokenize(source);
    auto result = Parser::parse(tokens);
    if (result.program.axiom.empty()) return "";
    auto cmds = Codegen::expand(result.program);
    const float min_dur = result.program.min_duration.value_or(0.1F);
    const float gliss_frac = result.program.glissando_frac.value_or(1.F);
    auto notes = Midi::compile(cmds, min_dur, gliss_frac);
    return base64_encode(Audio::to_bytes(notes));
}

static auto compile_to_svg(const std::string &source) -> std::string {
    auto tokens = Tokenizer::tokenize(source);
    auto result = Parser::parse(tokens);
    if (result.program.axiom.empty()) return "";
    auto cmds = Codegen::expand(result.program);
    return Image::to_svg(cmds);
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
    emscripten::function("compile_to_wav", &compile_to_wav);
    emscripten::function("compile_to_svg", &compile_to_svg);
    emscripten::function("compile_to_turtle", &compile_to_turtle);
    emscripten::function("get_diagnostics", &lsp_get_diagnostics);
    emscripten::function("get_hover", &lsp_get_hover);
    emscripten::function("get_completions", &lsp_get_completions);
}
#endif
