#include "async_http_client.hpp"
#include "helper.hpp"

#include <sdbusplus/async/execution.hpp>

#include <gtest/gtest.h>

namespace redfish_client_daemon
{

sdbusplus::async::task<> sendRequests(
    sdbusplus::async::context& ctx, const std::string& url,
    std::vector<AsyncHttpResponse>& responses, int iteration)
{
    AsyncHttpHandle handle{url};
    for (int i = 0; i < iteration; i++)
    {
        responses.push_back(co_await handle.get(ctx));
    }
}

TEST(AsyncHttpClientTests, TestClient)
{
    std::string responseBody = "Hello, World!";
    std::unordered_map<std::string, std::string> responseHeaders;
    SimpleTestHttpServer server(responseBody, responseHeaders);
    unsigned short port = server.getPort();
    std::string url = "http://localhost:" + std::to_string(port) + "/";
    sdbusplus::async::context ctx;
    std::vector<AsyncHttpResponse> responses;
    constexpr int iteration = 10;
    ctx.spawn(
        sendRequests(ctx, url, responses, iteration) |
        sdbusplus::async::execution::then([&ctx]() { ctx.request_stop(); }));
    ctx.run();
    for (int i = 0; i < iteration; i++)
    {
        EXPECT_EQ(responses[i].code, 200);
        EXPECT_EQ(responses[i].body, responseBody);
    }
}

} // namespace redfish_client_daemon
