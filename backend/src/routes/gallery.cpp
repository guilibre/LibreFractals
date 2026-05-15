#include "gallery.hpp"

#include "../controllers/gallery.hpp"
#include "../util/json.hpp"

namespace Routes::Gallery {

void register_all(drogon::HttpAppFramework &app) {
    app.registerHandler(
        "/gallery",
        [](const drogon::HttpRequestPtr &req,
           std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
            if (req->method() == drogon::Options) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods",
                                "GET, POST, OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
                std::move(cb)(resp);
                return;
            }
            if (req->method() == drogon::Post) {
                Controller::Gallery::post(req, std::move(cb));
                return;
            }
            if (req->method() == drogon::Get) {
                Controller::Gallery::get(req, std::move(cb));
                return;
            }
            std::move(cb)(Json::json_error(drogon::k405MethodNotAllowed,
                                           "method not allowed"));
        },
        {drogon::Get, drogon::Post, drogon::Options});
}

} // namespace Routes::Gallery
