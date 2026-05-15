#pragma once

#include <drogon/drogon.h>

namespace Http {

auto add_cors(drogon::HttpResponsePtr resp) -> drogon::HttpResponsePtr;

} // namespace Http
