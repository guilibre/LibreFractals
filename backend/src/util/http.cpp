#include "http.hpp"

namespace Http {

auto add_cors(drogon::HttpResponsePtr resp) -> drogon::HttpResponsePtr {
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
    return resp;
}

} // namespace Http
