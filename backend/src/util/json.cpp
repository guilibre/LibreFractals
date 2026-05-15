#include "json.hpp"
#include "http.hpp"

namespace Json {

auto json_error(drogon::HttpStatusCode status, std::string_view msg)
    -> drogon::HttpResponsePtr {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(status);
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp->setBody(R"({"error":")" + std::string(msg) + R"("})");
    return Http::add_cors(resp);
}

} // namespace Json
