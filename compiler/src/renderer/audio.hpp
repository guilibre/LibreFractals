#pragma once

#include "midi.hpp"
#include <string>
#include <vector>

namespace Audio {

auto to_bytes(const std::vector<Midi::NoteEvent> &notes)
    -> std::vector<uint8_t>;

auto to_wav(const std::vector<Midi::NoteEvent> &notes, const std::string &path)
    -> void;

} // namespace Audio
