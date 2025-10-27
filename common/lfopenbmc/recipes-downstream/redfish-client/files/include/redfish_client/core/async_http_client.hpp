#pragma once

#include <curl/curl.h>

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/task.hpp>

#include <expected>
#include <memory>
#include <string>

namespace redfish_client::core
{

struct AsyncHttpResponse
{
    long code;
    std::string body;

    static size_t write(char* ptr, size_t size, size_t nmemb, void* userdata);
};

struct HttpMultipartBodyPart
{
    std::optional<std::string> name;
    std::optional<std::string> type;
    std::optional<std::string> data;
    int* fd{nullptr};
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

    sdbusplus::async::task<std::expected<AsyncHttpResponse, std::string>>
        tryGet(sdbusplus::async::context& ctx);

    sdbusplus::async::task<AsyncHttpResponse> post(
        sdbusplus::async::context& ctx,
        const std::vector<HttpMultipartBodyPart>& multipart);

    sdbusplus::async::task<std::expected<AsyncHttpResponse, std::string>>
        tryPost(sdbusplus::async::context& ctx,
                const std::vector<HttpMultipartBodyPart>& multipart);

  private:
    CURL* easyHandle;
    CURLM* multiHandle;
    std::mutex mutex;

    sdbusplus::async::task<AsyncHttpResponse> perform(
        sdbusplus::async::context& ctx);
};

} // namespace redfish_client::core
