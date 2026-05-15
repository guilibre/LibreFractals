#pragma once

#include "drogon/HttpResponse.h"

namespace Controller::Gallery {

void post(const drogon::HttpRequestPtr &req,
          std::function<void(const drogon::HttpResponsePtr &)> &&cb);

void get(const drogon::HttpRequestPtr &req,
         std::function<void(const drogon::HttpResponsePtr &)> &&cb);

} // namespace Controller::Gallery
