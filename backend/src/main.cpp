#include "routes/gallery.hpp"

#include <drogon/drogon.h>

auto main() -> int {
    auto &app = drogon::app();
    app.setLogLevel(trantor::Logger::kInfo).setThreadNum(1);
    app.registerHandler(
        "/health",
        [](const drogon::HttpRequestPtr &,
           std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
            std::move(cb)(drogon::HttpResponse::newHttpResponse());
        });
    Routes::Gallery::register_all(app);
    app.addListener("0.0.0.0", 8080).run();
}
