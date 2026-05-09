#include "svg.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <numbers>
#include <sstream>
#include <utility>

namespace Renderer {

namespace {

struct TurtleState {
    float x = 0.F;
    float y = 0.F;
    float angle = -90.F;
    float scale = 1.F;
    float stroke_width = 1.F;
    int depth = 0;
    float hue = 0.F;
    float saturation = 0.F;
    float value = 0.F;
    float alpha = 1.F;
};

struct BBox {
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = -std::numeric_limits<float>::max();
    float max_y = -std::numeric_limits<float>::max();

    auto update(float x, float y) -> void {
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
    }

    auto valid() const -> bool { return min_x <= max_x; }
};

auto hsva_to_hex(float h, float s, float v, float a) -> std::string {
    h = std::fmod(h, 360.F);
    if (h < 0.F) h += 360.F;
    float c = v * s;
    float x = c * (1.F - std::fabs(std::fmod(h / 60.F, 2.F) - 1.F));
    float m = v - c;
    float r = 0.F;
    float g = 0.F;
    float b = 0.F;
    if (h < 60.F) {
        r = c;
        g = x;
    } else if (h < 120.F) {
        r = x;
        g = c;
    } else if (h < 180.F) {
        g = c;
        b = x;
    } else if (h < 240.F) {
        g = x;
        b = c;
    } else if (h < 300.F) {
        r = x;
        b = c;
    } else {
        r = c;
        b = x;
    }
    auto u = [](float f) -> int {
        return static_cast<int>(std::lround(f * 255.F));
    };
    return std::format("#{:02x}{:02x}{:02x}{:02x}", u(r + m), u(g + m),
                       u(b + m), u(a));
}

using Cmds = std::vector<Codegen::TurtleCmd>;

template <typename Visitor>
auto simulate(const Cmds &cmds, TurtleState init, Visitor &&visit) -> void {
    TurtleState cur = init;
    std::vector<TurtleState> stack;

    for (const auto &cmd : cmds) {
        switch (cmd.type) {
        case Codegen::TurtleCmdType::FORWARD: {
            float rad = cur.angle * std::numbers::pi_v<float> / 180.F;
            float dx = cur.scale * cmd.value * std::cos(rad);
            float dy = cur.scale * cmd.value * std::sin(rad);
            float x2 = cur.x + dx;
            float y2 = cur.y + dy;
            std::forward<Visitor>(visit)(cur.x, cur.y, x2, y2, cur);
            cur.x = x2;
            cur.y = y2;
            break;
        }
        case Codegen::TurtleCmdType::GAP: {
            float rad = cur.angle * std::numbers::pi_v<float> / 180.F;
            cur.x += cur.scale * cmd.value * std::cos(rad);
            cur.y += cur.scale * cmd.value * std::sin(rad);
            break;
        }
        case Codegen::TurtleCmdType::ROTATE:
            cur.angle += cmd.value;
            break;
        case Codegen::TurtleCmdType::SCALE:
            cur.scale *= cmd.value;
            break;
        case Codegen::TurtleCmdType::STROKE_WIDTH:
            cur.stroke_width *= cmd.value;
            break;
        case Codegen::TurtleCmdType::PUSH:
            stack.push_back(cur);
            cur.depth++;
            break;
        case Codegen::TurtleCmdType::POP:
            if (!stack.empty()) {
                cur = stack.back();
                stack.pop_back();
            }
            break;
        case Codegen::TurtleCmdType::HUE:
            cur.hue = std::fmod(cur.hue + cmd.value, 360.F);
            if (cur.hue < 0.F) cur.hue += 360.F;
            break;
        case Codegen::TurtleCmdType::SATURATION:
            cur.saturation =
                std::clamp(cur.saturation + (cmd.value / 100.F), 0.F, 1.F);
            break;
        case Codegen::TurtleCmdType::VALUE:
            cur.value = std::clamp(cur.value + (cmd.value / 100.F), 0.F, 1.F);
            break;
        case Codegen::TurtleCmdType::ALPHA:
            cur.alpha = std::clamp(cur.alpha + (cmd.value / 100.F), 0.F, 1.F);
            break;
        }
    }
}

} // namespace

auto to_svg(const std::vector<Codegen::TurtleCmd> &cmds, const SvgOptions &opts)
    -> std::string {
    TurtleState init;

    BBox bbox;
    simulate(cmds, init,
             [&](float x1, float y1, float x2, float y2,
                 const TurtleState &) -> void {
                 bbox.update(x1, y1);
                 bbox.update(x2, y2);
             });

    if (!bbox.valid()) {
        return R"(<?xml version="1.0" encoding="UTF-8"?>)"
               "\n<svg xmlns=\"http://www.w3.org/2000/svg\"/>\n";
    }

    float content_w = bbox.max_x - bbox.min_x;
    float content_h = bbox.max_y - bbox.min_y;
    init.stroke_width =
        std::max(content_w, content_h) * opts.base_stroke_width * 0.005F;
    float pad = std::max(std::max(content_w, content_h) * opts.padding_ratio,
                         init.stroke_width / 2.F);
    float vx = bbox.min_x - pad;
    float vy = bbox.min_y - pad;
    float vw = content_w + (2.F * pad);
    float vh = content_h + (2.F * pad);

    std::ostringstream ss;
    ss << R"(<?xml version="1.0" encoding="UTF-8"?>)" << '\n';
    ss << "<svg xmlns=\"http://www.w3.org/2000/svg\""
       << " viewBox=\"" << vx << ' ' << vy << ' ' << vw << ' ' << vh << "\""
       << R"( width="100%" height="100%")"
       << ">\n";

    simulate(cmds, init,
             [&](float x1, float y1, float x2, float y2,
                 const TurtleState &st) -> void {
                 ss << "  <line"
                    << " x1=\"" << x1 << "\""
                    << " y1=\"" << y1 << "\""
                    << " x2=\"" << x2 << "\""
                    << " y2=\"" << y2 << "\""
                    << " stroke=\""
                    << hsva_to_hex(st.hue, st.saturation, st.value, st.alpha)
                    << "\""
                    << " stroke-width=\"" << st.stroke_width << "\""
                    << " stroke-linecap=\"round\""
                    << "/>\n";
             });

    ss << "</svg>\n";
    return ss.str();
}

} // namespace Renderer
