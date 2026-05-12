#include "midi.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stack>
#include <vector>

namespace Midi {

namespace {

constexpr float TICKS_PER_SEC = static_cast<float>(TICKS_PER_BEAT * BPM) / 60.F;

struct State {
    float freq = 440.F;
    float amplitude = 0.5F;
    float tempo_scale = 1.F;
    uint32_t midi_tick = 0;
};

auto find_min_duration(const std::vector<Codegen::TurtleCmd> &cmds) -> float {
    auto min_d = std::numeric_limits<float>::max();
    auto tempo_scale = 1.F;
    std::stack<float> scale_stack;

    for (const auto &cmd : cmds) {
        switch (cmd.type) {
        case Codegen::TurtleCmdType::FORWARD:
        case Codegen::TurtleCmdType::GAP:
            min_d = std::min(min_d, cmd.value * tempo_scale);
            break;
        case Codegen::TurtleCmdType::SCALE:
            tempo_scale *= cmd.value;
            break;
        case Codegen::TurtleCmdType::PUSH:
            scale_stack.push(tempo_scale);
            break;
        case Codegen::TurtleCmdType::POP:
            if (!scale_stack.empty()) {
                tempo_scale = scale_stack.top();
                scale_stack.pop();
            }
            break;
        default:
            break;
        }
    }

    return (min_d == std::numeric_limits<float>::max()) ? 1.F : min_d;
}

auto freq_to_pitch(float freq) -> uint8_t {
    const auto pitch = std::clamp(
        static_cast<int>(std::round(69.F + (12.F * std::log2(freq / 440.F)))),
        0, 127);
    return static_cast<uint8_t>(pitch);
}

auto amplitude_to_velocity(float amp) -> uint8_t {
    const auto v = std::clamp(static_cast<int>(amp * 127.F), 1, 127);
    return static_cast<uint8_t>(v);
}

auto append_u32be(std::vector<uint8_t> &buf, uint32_t v) -> void {
    buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
}

auto append_u16be(std::vector<uint8_t> &buf, uint16_t v) -> void {
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
}

auto append_vlq(std::vector<uint8_t> &buf, uint32_t v) -> void {
    std::array<uint8_t, 4> bytes{};
    int n = 0;
    bytes[static_cast<size_t>(n++)] = v & 0x7F;
    v >>= 7;
    while (v > 0) {
        bytes[static_cast<size_t>(n++)] = (v & 0x7F) | 0x80;
        v >>= 7;
    }
    for (int i = n - 1; i >= 0; --i)
        buf.push_back(bytes[static_cast<size_t>(i)]);
}

auto pitch_bend_value(float freq, uint8_t pitch) -> uint16_t {
    const float ref =
        440.F * std::pow(2.F, (static_cast<float>(pitch) - 69.F) / 12.F);
    const float cents_offset = 1200.F * std::log2(freq / ref);
    const float semitones = cents_offset / 100.F;
    const int raw =
        static_cast<int>(std::round(8192.F + (semitones / 48.F * 8191.F)));
    return static_cast<uint16_t>(std::clamp(raw, 0, 16383));
}

auto emit_cc(std::vector<uint8_t> &track, uint32_t &prev_tick, uint32_t tick,
             uint8_t ch, uint8_t cc, uint8_t val) -> void {
    append_vlq(track, tick - prev_tick);
    prev_tick = tick;
    track.push_back(static_cast<uint8_t>(0xB0 | ch));
    track.push_back(cc);
    track.push_back(val);
}

auto emit_rpn(std::vector<uint8_t> &track, uint32_t &prev_tick, uint32_t tick,
              uint8_t ch, uint8_t rpn_msb, uint8_t rpn_lsb, uint8_t data_msb,
              uint8_t data_lsb) -> void {
    emit_cc(track, prev_tick, tick, ch, 101, rpn_msb);
    emit_cc(track, prev_tick, tick, ch, 100, rpn_lsb);
    emit_cc(track, prev_tick, tick, ch, 6, data_msb);
    emit_cc(track, prev_tick, tick, ch, 38, data_lsb);
    emit_cc(track, prev_tick, tick, ch, 101, 127);
    emit_cc(track, prev_tick, tick, ch, 100, 127);
}

} // namespace

auto compile(const std::vector<Codegen::TurtleCmd> &cmds, float min_duration_s,
             float glissando_frac) -> std::vector<NoteEvent> {
    const auto min_d = find_min_duration(cmds);
    const auto tempo_base = min_duration_s / min_d;

    constexpr auto MAX_TICKS = static_cast<uint32_t>(60.F * TICKS_PER_SEC);

    struct ConnState {
        int last_note_idx = -1;
        bool pending_glissando = false;
        bool pending_fade = false;
        bool broken = false;
    };

    std::vector<NoteEvent> notes;
    State state;
    std::stack<State> stack;
    ConnState conn;
    std::stack<ConnState> conn_stack;

    for (const auto &cmd : cmds) {
        if (state.midi_tick >= MAX_TICKS) continue;

        switch (cmd.type) {
        case Codegen::TurtleCmdType::FORWARD: {
            const bool connected_to_prev =
                conn.last_note_idx >= 0 && !conn.broken;
            if (connected_to_prev) {
                auto &prev = notes[static_cast<size_t>(conn.last_note_idx)];
                prev.chain_end = false;
                if (conn.pending_glissando) prev.freq_end = state.freq;
                if (conn.pending_fade)
                    prev.velocity_end = amplitude_to_velocity(state.amplitude);
            }
            const auto dur_s = cmd.value * tempo_base * state.tempo_scale;
            const auto ticks = std::max<uint32_t>(
                static_cast<uint32_t>(dur_s * TICKS_PER_SEC), 1);
            const auto vel = amplitude_to_velocity(state.amplitude);
            notes.push_back({
                .tick_on = state.midi_tick,
                .tick_off = std::min(state.midi_tick + ticks, MAX_TICKS),
                .freq = state.freq,
                .freq_end = state.freq,
                .pitch = freq_to_pitch(state.freq),
                .velocity = vel,
                .velocity_end = vel,
                .chain_start = !connected_to_prev,
                .chain_end = true,
                .glissando_frac = glissando_frac,
            });
            conn.last_note_idx = static_cast<int>(notes.size()) - 1;
            conn.pending_glissando = false;
            conn.pending_fade = false;
            conn.broken = false;
            state.midi_tick += ticks;
            break;
        }
        case Codegen::TurtleCmdType::GAP: {
            const auto dur_s = cmd.value * tempo_base * state.tempo_scale;
            state.midi_tick += static_cast<uint32_t>(dur_s * TICKS_PER_SEC);
            conn.broken = true;
            break;
        }
        case Codegen::TurtleCmdType::ROTATE:
            state.freq *= std::pow(2.F, cmd.value / 360.F);
            if (!conn.broken) conn.pending_glissando = true;
            break;
        case Codegen::TurtleCmdType::SCALE:
            state.tempo_scale *= cmd.value;
            break;
        case Codegen::TurtleCmdType::STROKE_WIDTH:
            state.amplitude = std::clamp(state.amplitude * cmd.value, 0.F, 1.F);
            if (!conn.broken) conn.pending_fade = true;
            break;
        case Codegen::TurtleCmdType::PUSH:
            stack.push(state);
            conn_stack.push(conn);
            conn = ConnState{};
            break;
        case Codegen::TurtleCmdType::POP:
            if (!stack.empty()) {
                state = stack.top();
                stack.pop();
            }
            if (!conn_stack.empty()) {
                conn = conn_stack.top();
                conn_stack.pop();
                conn.broken = true;
            }
            break;

        case Codegen::TurtleCmdType::HUE:
        case Codegen::TurtleCmdType::SATURATION:
        case Codegen::TurtleCmdType::VALUE:
        case Codegen::TurtleCmdType::ALPHA:
            break;
        }
    }

    return notes;
}

auto write(const std::vector<NoteEvent> &notes, const std::string &path)
    -> void {
    struct RawEvent {
        uint32_t tick;
        uint8_t type;
        uint8_t ch;
        uint8_t pitch;
        uint8_t velocity;
        uint16_t bend;
    };

    constexpr int MPE_CHANNELS = 15;
    constexpr uint8_t FIRST_CH = 1;

    struct Voice {
        uint32_t tick_off = 0;
        bool active = false;
    };
    std::array<Voice, MPE_CHANNELS> voices{};

    auto alloc_channel = [&](uint32_t tick_on) -> uint8_t {
        for (int i = 0; i < MPE_CHANNELS; ++i) {
            if (!voices[i].active || voices[i].tick_off <= tick_on) {
                voices[i].active = true;
                return static_cast<uint8_t>(FIRST_CH + i);
            }
        }
        int best = 0;
        for (int i = 1; i < MPE_CHANNELS; ++i) {
            if (voices[i].tick_off < voices[best].tick_off) best = i;
        }
        voices[best].active = true;
        return static_cast<uint8_t>(FIRST_CH + best);
    };

    auto sorted_notes = notes;
    std::ranges::stable_sort(sorted_notes,
                             [](const NoteEvent &a, const NoteEvent &b) {
                                 return a.tick_on < b.tick_on;
                             });

    struct GlissEvent {
        uint32_t tick;
        uint8_t ch;
        uint16_t bend;
    };
    struct FadeEvent {
        uint32_t tick;
        uint8_t ch;
        uint8_t expr;
    };

    std::vector<RawEvent> events;
    std::vector<GlissEvent> gliss_events;
    std::vector<FadeEvent> fade_events;
    events.reserve(sorted_notes.size() * 2);

    constexpr int GLISS_STEPS = 8;

    for (const auto &n : sorted_notes) {
        const uint8_t ch = alloc_channel(n.tick_on);
        voices[ch - FIRST_CH].tick_off = n.tick_off;
        events.push_back({n.tick_on, 0, ch, n.pitch, n.velocity,
                          pitch_bend_value(n.freq, n.pitch)});
        events.push_back({n.tick_off, 1, ch, n.pitch, 0, 0});

        const uint32_t dur = n.tick_off - n.tick_on;

        if (n.freq_end != n.freq) {
            const float sweep_dur = static_cast<float>(dur) * n.glissando_frac;
            for (int s = 1; s <= GLISS_STEPS; ++s) {
                const float t =
                    static_cast<float>(s) / static_cast<float>(GLISS_STEPS);
                const float freq = n.freq * std::pow(n.freq_end / n.freq, t);
                const uint32_t tick =
                    n.tick_on + static_cast<uint32_t>(t * sweep_dur);
                gliss_events.push_back(
                    {tick, ch, pitch_bend_value(freq, n.pitch)});
            }
        }

        if (n.velocity_end != n.velocity) {
            for (int s = 1; s <= GLISS_STEPS; ++s) {
                const float t =
                    static_cast<float>(s) / static_cast<float>(GLISS_STEPS);
                const float expr_f = static_cast<float>(n.velocity) +
                                     ((static_cast<float>(n.velocity_end) -
                                       static_cast<float>(n.velocity)) *
                                      t);
                const uint8_t expr = static_cast<uint8_t>(
                    std::clamp(static_cast<int>(expr_f), 0, 127));
                const uint32_t tick =
                    n.tick_on +
                    static_cast<uint32_t>(t * static_cast<float>(dur));
                fade_events.push_back({tick, ch, expr});
            }
        }
    }

    std::ranges::stable_sort(events, [](const RawEvent &a, const RawEvent &b) {
        return a.tick < b.tick;
    });
    std::ranges::stable_sort(gliss_events,
                             [](const GlissEvent &a, const GlissEvent &b) {
                                 return a.tick < b.tick;
                             });
    std::ranges::stable_sort(
        fade_events,
        [](const FadeEvent &a, const FadeEvent &b) { return a.tick < b.tick; });

    struct MergedEvent {
        uint32_t tick;
        uint8_t type;
        uint8_t ch;
        uint8_t pitch;
        uint8_t velocity;
        uint16_t bend;
        uint8_t expr;
    };

    std::vector<MergedEvent> merged;
    merged.reserve(events.size() + gliss_events.size() + fade_events.size());
    for (const auto &e : events)
        merged.push_back(
            {e.tick, e.type, e.ch, e.pitch, e.velocity, e.bend, 0});
    for (const auto &e : gliss_events)
        merged.push_back({e.tick, 2, e.ch, 0, 0, e.bend, 0});
    for (const auto &e : fade_events)
        merged.push_back({e.tick, 3, e.ch, 0, 0, 0, e.expr});
    std::ranges::stable_sort(merged,
                             [](const MergedEvent &a, const MergedEvent &b) {
                                 return a.tick < b.tick;
                             });

    std::vector<uint8_t> track;
    uint32_t prev_tick = 0;

    emit_rpn(track, prev_tick, 0, 0, 0, 6, MPE_CHANNELS, 0);
    for (uint8_t i = 0; i < MPE_CHANNELS; ++i)
        emit_rpn(track, prev_tick, 0, static_cast<uint8_t>(FIRST_CH + i), 0, 0,
                 48, 0);

    for (const auto &ev : merged) {
        if (ev.type == 0) {
            const uint8_t lsb = ev.bend & 0x7F;
            const uint8_t msb = (ev.bend >> 7) & 0x7F;
            append_vlq(track, ev.tick - prev_tick);
            prev_tick = ev.tick;
            track.push_back(static_cast<uint8_t>(0xE0 | ev.ch));
            track.push_back(lsb);
            track.push_back(msb);
            append_vlq(track, 0);
            track.push_back(static_cast<uint8_t>(0x90 | ev.ch));
            track.push_back(ev.pitch);
            track.push_back(ev.velocity);
        } else if (ev.type == 1) {
            append_vlq(track, ev.tick - prev_tick);
            prev_tick = ev.tick;
            track.push_back(static_cast<uint8_t>(0x80 | ev.ch));
            track.push_back(ev.pitch);
            track.push_back(0);
        } else if (ev.type == 2) {
            const uint8_t lsb = ev.bend & 0x7F;
            const uint8_t msb = (ev.bend >> 7) & 0x7F;
            append_vlq(track, ev.tick - prev_tick);
            prev_tick = ev.tick;
            track.push_back(static_cast<uint8_t>(0xE0 | ev.ch));
            track.push_back(lsb);
            track.push_back(msb);
        } else {
            emit_cc(track, prev_tick, ev.tick, ev.ch, 11, ev.expr);
        }
    }

    append_vlq(track, 0);
    track.push_back(0xFF);
    track.push_back(0x2F);
    track.push_back(0x00);

    std::vector<uint8_t> file;
    file.insert(file.end(), {'M', 'T', 'h', 'd'});
    append_u32be(file, 6);
    append_u16be(file, 0);
    append_u16be(file, 1);
    append_u16be(file, TICKS_PER_BEAT);
    file.insert(file.end(), {'M', 'T', 'r', 'k'});
    append_u32be(file, static_cast<uint32_t>(track.size()));
    file.insert(file.end(), track.begin(), track.end());

    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char *>(file.data()),
              static_cast<std::streamsize>(file.size()));
}

} // namespace Midi
