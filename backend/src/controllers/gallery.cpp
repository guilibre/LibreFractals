#include "gallery.hpp"

#include "../store/gallery.hpp"
#include "../util/http.hpp"
#include "../util/json.hpp"

#include <charconv>
#include <json/json.h>

namespace Controller::Gallery {

void post(const drogon::HttpRequestPtr &req,
          std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
    auto json = req->getJsonObject();
    if (!json || !(*json)["hash"].isString()) {
        std::move(cb)(Json::json_error(drogon::k400BadRequest, "missing hash"));
        return;
    }

    const std::string hash = (*json)["hash"].asString();
    const std::string name = (*json).get("name", "").asString();
    if (!(*json)["compile_ms"].isInt()) {
        std::move(cb)(Json::json_error(drogon::k400BadRequest, "missing compile_ms"));
        return;
    }
    const int compile_ms = (*json)["compile_ms"].asInt();
    if (compile_ms > 1000) {
        std::move(cb)(Json::json_error(drogon::k400BadRequest, "compile_ms exceeds 1s limit"));
        return;
    }

    Store::Gallery::add(hash, name, compile_ms);

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp->setBody(R"({"ok":true})");
    std::move(cb)(Http::add_cors(resp));
}

void get(const drogon::HttpRequestPtr &req,
         std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
    int page_num = 0;
    int limit = 20;
    if (auto p = req->getParameter("page"); !p.empty())
        std::from_chars(p.data(), p.data() + p.size(), page_num);
    if (auto l = req->getParameter("limit"); !l.empty()) {
        std::from_chars(l.data(), l.data() + l.size(), limit);
        limit = std::min(limit, 50);
    }
    page_num = std::max(page_num, 0);
    if (limit <= 0) limit = 20;

    auto result = Store::Gallery::page(page_num, limit);

    Json::Value root;
    root["entries"] = Json::arrayValue;
    for (auto &entry : result.entries) {
        Json::Value e;
        e["hash"] = entry.hash;
        e["name"] = entry.name;
        e["compile_ms"] = entry.compile_ms;
        e["created_at"] =
            Json::Int64(std::chrono::duration_cast<std::chrono::seconds>(
                            entry.created_at.time_since_epoch())
                            .count());
        root["entries"].append(e);
    }
    root["total"] = result.total;

    Json::FastWriter writer;
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp->setBody(writer.write(root));
    std::move(cb)(Http::add_cors(resp));
}

} // namespace Controller::Gallery
