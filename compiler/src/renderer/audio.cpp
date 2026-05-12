#include "audio.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <numbers>
#include <vector>

namespace Audio {

namespace {

constexpr int SAMPLE_RATE = 48000;

constexpr float TICKS_PER_SEC =
    static_cast<float>(Midi::TICKS_PER_BEAT * Midi::BPM) / 60.F;

auto asr(int i, int n) -> float {
    if (n <= 1) return 1.F;
    const int ramp = std::max(2, n / 10);
    const auto d = static_cast<float>(ramp - 1);
    if (i < ramp) return static_cast<float>(i) / d;
    if (i >= n - ramp) return static_cast<float>(n - 1 - i) / d;
    return 1.F;
}

auto append_u32le(std::vector<uint8_t> &buf, uint32_t v) -> void {
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

auto append_u16le(std::vector<uint8_t> &buf, uint16_t v) -> void {
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

} // namespace

auto to_bytes(const std::vector<Midi::NoteEvent> &notes)
    -> std::vector<uint8_t> {
    if (notes.empty()) return {};

    uint32_t max_tick = 0;
    for (const auto &n : notes) max_tick = std::max(max_tick, n.tick_off);

    const int total_samples =
        static_cast<int>(static_cast<float>(max_tick) / TICKS_PER_SEC *
                         static_cast<float>(SAMPLE_RATE)) +
        1;

    std::vector<float> buf(static_cast<size_t>(total_samples), 0.F);

    size_t ni = 0;
    while (ni < notes.size()) {
        size_t chain_end = ni;
        while (!notes[chain_end].chain_end) ++chain_end;

        float phase = 0.F;

        for (size_t k = ni; k <= chain_end; ++k) {
            const auto &note = notes[k];
            const bool is_first = (k == ni);
            const bool is_last = (k == chain_end);

            const int start_sample = static_cast<int>(
                std::round(static_cast<float>(note.tick_on) / TICKS_PER_SEC *
                           static_cast<float>(SAMPLE_RATE)));
            if (start_sample >= total_samples) continue;

            const int end_sample =
                std::min(static_cast<int>(std::round(
                             static_cast<float>(note.tick_off) / TICKS_PER_SEC *
                             static_cast<float>(SAMPLE_RATE))),
                         total_samples);
            const int note_samples = end_sample - start_sample;

            const int ramp = std::max(2, note_samples / 10);
            const auto ramp_d = static_cast<float>(ramp - 1);

            const float vel_start = static_cast<float>(note.velocity) / 127.F;
            const float vel_end = static_cast<float>(note.velocity_end) / 127.F;
            const bool glissando = (note.freq_end != note.freq);
            const bool fade = (note.velocity_end != note.velocity);

            for (int i = start_sample; i < end_sample; ++i) {
                const int local = i - start_sample;

                float env = 1.F;
                if (is_first && is_last) {
                    env = asr(local, note_samples);
                } else if (is_first) {
                    env = (local < ramp) ? static_cast<float>(local) / ramp_d
                                         : 1.F;
                } else if (is_last) {
                    env = (local >= note_samples - ramp)
                              ? static_cast<float>(note_samples - 1 - local) /
                                    ramp_d
                              : 1.F;
                } else {
                    env = 1.F;
                }

                const float t_norm =
                    (note_samples > 1)
                        ? static_cast<float>(local) /
                              static_cast<float>(note_samples - 1)
                        : 0.F;

                const float gliss_t =
                    (note.glissando_frac < 1.F)
                        ? std::min(t_norm / note.glissando_frac, 1.F)
                        : t_norm;
                const float freq =
                    glissando ? (note.freq *
                                 std::pow(note.freq_end / note.freq, gliss_t))
                              : note.freq;

                const float vel =
                    fade ? (vel_start + ((vel_end - vel_start) * t_norm))
                         : vel_start;

                phase += 2.F * std::numbers::pi_v<float> * freq /
                         static_cast<float>(SAMPLE_RATE);
                phase = std::fmod(phase, 2.F * std::numbers::pi_v<float>);
                buf[static_cast<size_t>(i)] += env * vel * std::sin(phase);
            }
        }

        ni = chain_end + 1;
    }

    float peak = 0.F;
    for (const float s : buf) peak = std::max(peak, std::abs(s));
    if (peak > 0.F) {
        const float scale = 0.5F / peak;
        for (float &s : buf) s *= scale;
    }

    std::vector<int16_t> pcm;
    pcm.reserve(buf.size());
    for (const float s : buf)
        pcm.push_back(static_cast<int16_t>(
            std::clamp(static_cast<int>(s * 32'767.F), -32'768, 32'767)));

    const uint32_t data_size = static_cast<uint32_t>(pcm.size()) * 2;
    const uint32_t riff_size = 36 + data_size;

    std::vector<uint8_t> out;
    out.reserve(44 + data_size);
    out.insert(out.end(), {'R', 'I', 'F', 'F'});
    append_u32le(out, riff_size);
    out.insert(out.end(), {'W', 'A', 'V', 'E'});
    out.insert(out.end(), {'f', 'm', 't', ' '});
    append_u32le(out, 16);
    append_u16le(out, 1);
    append_u16le(out, 1);
    append_u32le(out, static_cast<uint32_t>(SAMPLE_RATE));
    append_u32le(out, static_cast<uint32_t>(SAMPLE_RATE) * 2);
    append_u16le(out, 2);
    append_u16le(out, 16);
    out.insert(out.end(), {'d', 'a', 't', 'a'});
    append_u32le(out, data_size);

    const auto *pcm_bytes = reinterpret_cast<const uint8_t *>(pcm.data());
    out.insert(out.end(), pcm_bytes, pcm_bytes + (pcm.size() * 2));

    return out;
}

auto to_wav(const std::vector<Midi::NoteEvent> &notes, const std::string &path)
    -> void {
    auto bytes = to_bytes(notes);
    if (bytes.empty()) return;
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char *>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
}

} // namespace Audio
