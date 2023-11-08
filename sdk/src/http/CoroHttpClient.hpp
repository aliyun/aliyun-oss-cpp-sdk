/*
 * Copyright 2009-2023 Alibaba Cloud All rights reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string_view>
#include <ylt/coro_http/coro_http_client.hpp>
#include "../utils/LogUtils.h"

namespace AlibabaCloud::OSS {
inline const char* CORO_TAG = "CurlHttpClient";

class CoroHttpClient : public HttpClient {
public:
  CoroHttpClient() = default;
  virtual std::shared_ptr<HttpResponse>
  makeRequest(const std::shared_ptr<HttpRequest> &request) override {
    return async_simple::coro::syncAwait(makeRequestCoro(request));
  }

  async_simple::coro::Lazy<std::shared_ptr<HttpResponse>>
  makeRequestCoro(const std::shared_ptr<HttpRequest> &request) override {
    coro_http::coro_http_client client{};

    auto url = request->url().toString();
    auto method = request->method();

    coro_http::uri_t u;
    u.parse_from(url.data());

    std::unordered_map<std::string, std::string> headers;
    auto &req_headers = request->Headers();
    for (auto &[k, v] : req_headers) {
      headers.emplace(k, v);
    }

    headers.emplace("Host", u.get_host());

    auto body = request->Body();
    bool has_content = (body != nullptr);
    std::string content;

    if (has_content) {
        body->seekg(0, std::ios::end);
        int size = body->tellg();
        content.resize(size);
        body->seekg(0, std::ios::end);
        body->seekg(0, std::ios::beg);
        body->read(content.data(), size);
    }

    coro_http::resp_data result;
    switch (method) {
    case Http::Method::Head:
      result = co_await client.async_head(url, std::move(headers));
      break;
    case Http::Method::Delete:
      result = co_await client.async_delete(url, std::move(content), coro_http::req_content_type::none, std::move(headers));
      break;
    case Http::Method::Get:
      result = co_await client.async_get(url, std::move(headers));
      break;
    case Http::Method::Put:
    {
      auto filename = request->MultipartFilename();
      if(filename.empty()){
        result = co_await client.async_upload_chunked(
          url, coro_http::http_method::PUT, body, coro_http::req_content_type::none,
          std::move(headers));
      }else {
        result = co_await client.async_upload_chunked(
          url, coro_http::http_method::PUT, filename, coro_http::req_content_type::none,
          std::move(headers));
      }
    }
      break;
    case Http::Method::Post:
      if (has_content) {
        body->seekg(0, std::ios::end);
        int size = body->tellg();
        content.resize(size);
        body->seekg(0, std::ios::end);
        body->seekg(0, std::ios::beg);
        body->read(content.data(), size);
      }

      result = co_await client.async_post(url, std::move(content),
          coro_http::req_content_type::none,
          std::move(headers));
      break;
    case Http::Method::Connect:
      result = co_await client.async_http_connect(url, std::move(headers));
      break;
    case Http::Method::Options:
      result = co_await client.async_options(url, std::move(headers));
      break;
    case Http::Method::Patch:
      result = co_await client.async_patch(url, std::move(headers));
      break;
    case Http::Method::Trace:
      result = co_await client.async_trace(url, std::move(headers));
      break;
    default:
      OSS_LOG(LogLevel::LogError, CORO_TAG, "request(%p) method not implement", request.get());
      result.status = 501;
      break;
    }

    OSS_LOG(LogLevel::LogDebug, CORO_TAG, "request(%p) response status: %d, body: %s", request.get(), result.status, result.resp_body);

    auto response = std::make_shared<HttpResponse>(request);
    response->setStatusCode(result.status);
    if (result.net_err) {
      response->setStatusMsg(result.net_err.message().data());
    }
    if (!result.resp_body.empty()) {
      auto ss = std::make_shared<std::stringstream>();
      ss->write(result.resp_body.data(), result.resp_body.size());
      response->addBody(ss);
    }

    co_return response;
  }
};
} // namespace AlibabaCloud::OSS