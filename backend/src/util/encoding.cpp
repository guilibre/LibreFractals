#include "encoding.hpp"

#include <array>
#include <zlib.h>

namespace Encoding {

auto base64url_decode(std::string_view b64url) -> std::vector<uint8_t> {
    auto b64 = std::string(b64url);
    for (auto &c : b64) {
        if (c == '-')
            c = '+';
        else if (c == '_')
            c = '/';
    }
    while ((b64.size() % 4) != 0U) b64 += '=';

    auto val = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };

    std::vector<uint8_t> out;
    out.reserve(b64.size() * 3 / 4);
    for (size_t i = 0; i + 3 < b64.size(); i += 4) {
        int v0 = val(b64[i]);
        int v1 = val(b64[i + 1]);
        int v2 = val(b64[i + 2]);
        int v3 = val(b64[i + 3]);
        if (v0 < 0 || v1 < 0) break;
        out.push_back(static_cast<uint8_t>((v0 << 2) | (v1 >> 4)));
        if (v2 >= 0)
            out.push_back(static_cast<uint8_t>(((v1 & 0xF) << 4) | (v2 >> 2)));
        if (v3 >= 0)
            out.push_back(static_cast<uint8_t>(((v2 & 0x3) << 6) | v3));
    }
    return out;
}

auto inflate_raw(std::vector<uint8_t> compressed) -> std::string {
    z_stream zs{};
    if (inflateInit2(&zs, -15) != Z_OK) return "";
    zs.next_in = reinterpret_cast<Bytef *>(compressed.data());
    zs.avail_in = static_cast<uInt>(compressed.size());
    std::string out;
    std::array<char, 4096> buf{};
    int ret = Z_OK;
    while (ret == Z_OK) {
        zs.next_out = reinterpret_cast<Bytef *>(buf.data());
        zs.avail_out = buf.size();
        ret = inflate(&zs, Z_NO_FLUSH);
        out.append(buf.data(), buf.size() - zs.avail_out);
    }
    inflateEnd(&zs);
    return (ret == Z_STREAM_END) ? out : "";
}

} // namespace Encoding
