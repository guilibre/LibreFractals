#pragma once

#include "parser.hpp"
#include <string>

namespace Codegen {

enum class TurtleCmdType : uint8_t {
    FORWARD,
    GAP,
    ROTATE,
    SCALE,
    STROKE_WIDTH,
    PUSH,
    POP,
    HUE,
    SATURATION,
    VALUE,
    ALPHA,
};

struct TurtleCmd {
    TurtleCmdType type;
    float value;
};

auto expand(const Parser::Program &program) -> std::vector<TurtleCmd>;

auto to_string(const std::vector<TurtleCmd> &cmds) -> std::string;

} // namespace Codegen
