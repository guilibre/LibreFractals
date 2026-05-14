#include <drogon/drogon.h>

auto main() -> int {
    drogon::app()
        .setLogLevel(trantor::Logger::kInfo)
        .setThreadNum(1)
        .registerHandler(
            "/health",
            [](const drogon::HttpRequestPtr &,
               std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
                std::move(cb)(drogon::HttpResponse::newHttpResponse());
            })
        .addListener("0.0.0.0", 8080)
        .run();
}
