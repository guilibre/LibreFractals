#pragma once

#include "drogon/HttpResponse.h"

namespace Controller::Gallery {

void post(const drogon::HttpRequestPtr &req,
          std::function<void(const drogon::HttpResponsePtr &)> &&cb);

void get(const drogon::HttpRequestPtr &req,
         std::function<void(const drogon::HttpResponsePtr &)> &&cb);

void get_relatives(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                   const std::string &hash);

} // namespace Controller::Gallery
