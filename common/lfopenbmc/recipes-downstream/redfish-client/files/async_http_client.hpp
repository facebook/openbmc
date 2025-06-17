#pragma once

#include <curl/curl.h>

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/task.hpp>

#include <memory>
#include <string>

namespace redfish_client_daemon
{

struct AsyncHttpResponse
{
    long code;
    std::string body;

    static size_t write(char* ptr, size_t size, size_t nmemb, void* userdata);
};

class AsyncHttpHandle
{
  public:
    static constexpr auto kDefaultTimeoutSec = 5;

    AsyncHttpHandle() = delete;
    AsyncHttpHandle(const AsyncHttpHandle&) = delete;
    AsyncHttpHandle(AsyncHttpHandle&&) = delete;
    AsyncHttpHandle& operator=(const AsyncHttpHandle&) = delete;
    AsyncHttpHandle& operator=(AsyncHttpHandle&&) = delete;

    explicit AsyncHttpHandle(const std::string& url);

    ~AsyncHttpHandle();

    sdbusplus::async::task<AsyncHttpResponse> get(
        sdbusplus::async::context& ctx);

  private:
    CURL* easyHandle;
    CURLM* multiHandle;
    std::mutex mutex;
};

} // namespace redfish_client_daemon
