#pragma once

#include "parser/codegen.hpp"
#include <string>
#include <vector>

namespace Renderer {

struct SvgOptions {
    float base_stroke_width = 1.F;
    float padding_ratio = 0.02F;
};

auto to_svg(const std::vector<Codegen::TurtleCmd> &cmds,
            const SvgOptions &opts = {}) -> std::string;

} // namespace Renderer
