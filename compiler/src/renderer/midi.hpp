#pragma once

#include "parser/codegen.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace Midi {

constexpr int TICKS_PER_BEAT = 480;
constexpr int BPM = 120;

struct NoteEvent {
    uint32_t tick_on{};
    uint32_t tick_off{};
    float freq{};
    float freq_end{};
    uint8_t pitch{};
    uint8_t velocity{};
    uint8_t velocity_end{};
    bool chain_start = true;
    bool chain_end = true;
    float glissando_frac = 1.F;
};

auto compile(const std::vector<Codegen::TurtleCmd> &cmds,
             float min_duration_s = 0.1F, float glissando_frac = 1.F)
    -> std::vector<NoteEvent>;

auto write(const std::vector<NoteEvent> &notes, const std::string &path)
    -> void;

} // namespace Midi
