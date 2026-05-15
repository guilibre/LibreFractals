#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Encoding {

auto base64url_decode(std::string_view b64url) -> std::vector<uint8_t>;
auto inflate_raw(std::vector<uint8_t> compressed) -> std::string;

} // namespace Encoding
