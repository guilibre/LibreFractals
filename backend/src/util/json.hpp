#pragma once

#include "drogon/HttpResponse.h"

namespace Json {

auto json_error(drogon::HttpStatusCode status, std::string_view msg)
    -> drogon::HttpResponsePtr;

}
