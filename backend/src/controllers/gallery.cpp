#include "gallery.hpp"

#include "../math/genealogy.hpp"
#include "../store/gallery.hpp"
#include "../util/http.hpp"
#include "../util/json.hpp"

#include <charconv>
#include <json/json.h>

namespace Controller::Gallery {

void post(const drogon::HttpRequestPtr &req,
          std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
    const auto json = req->getJsonObject();
    if (!json || !(*json)["hash"].isString()) {
        std::move(cb)(Json::json_error(drogon::k400BadRequest, "missing hash"));
        return;
    }

    const auto hash = (*json)["hash"].asString();
    const auto name = (*json).get("name", "").asString();
    if (!(*json)["svg_bytes"].isInt()) {
        std::move(cb)(
            Json::json_error(drogon::k400BadRequest, "missing svg_bytes"));
        return;
    }
    const auto svg_bytes = (*json)["svg_bytes"].asInt();
    if (svg_bytes > 524288) {
        std::move(cb)(Json::json_error(drogon::k400BadRequest,
                                       "svg_bytes exceeds 512KB limit"));
        return;
    }

    if (!(*json)["topo"].isString()) {
        std::move(cb)(Json::json_error(drogon::k400BadRequest, "missing topo"));
        return;
    }
    auto topo = (*json)["topo"].asString();
    if (topo.size() > Math::Genealogy::TOPO_MAX_LEN)
        topo.resize(Math::Genealogy::TOPO_MAX_LEN);

    Store::Gallery::add(hash, name, svg_bytes, topo);

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

    const auto result = Store::Gallery::page(page_num, limit);

    Json::Value root;
    root["entries"] = Json::arrayValue;
    for (const auto &entry : result.entries) {
        Json::Value e;
        e["hash"] = entry.hash;
        e["name"] = entry.name;
        e["svg_bytes"] = entry.svg_bytes;
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

void get_relatives(const drogon::HttpRequestPtr & /*req*/,
                   std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                   const std::string &hash) {
    const auto edges = Math::Genealogy::graph().relatives(hash);

    Json::Value root;
    root["relatives"] = Json::arrayValue;
    for (const auto &edge : edges) {
        Json::Value e;
        e["hash"] = (edge.from_hash == hash) ? edge.to_hash : edge.from_hash;
        e["similarity"] = edge.score;
        e["relation"] = Math::Genealogy::relation_name(edge.relation);
        root["relatives"].append(e);
    }

    Json::FastWriter writer;
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp->setBody(writer.write(root));
    std::move(cb)(Http::add_cors(resp));
}

} // namespace Controller::Gallery
