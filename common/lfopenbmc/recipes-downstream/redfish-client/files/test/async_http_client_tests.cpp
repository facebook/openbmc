#include <redfish_client/core/async_http_client.hpp>
#include "helper.hpp"

#include <sdbusplus/async/execution.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish_client::core
{

sdbusplus::async::task<> sendGetRequests(
    sdbusplus::async::context& ctx, const std::string& url,
    std::vector<AsyncHttpResponse>& responses, int iteration)
{
    AsyncHttpHandle handle{url};
    for (int i = 0; i < iteration; i++)
    {
        responses.push_back(co_await handle.get(ctx));
    }
}

sdbusplus::async::task<> sendPostRequests(
    sdbusplus::async::context& ctx, const std::string& url,
    const std::vector<HttpMultipartBodyPart>& multipart,
    std::vector<AsyncHttpResponse>& responses, int iteration)
{
    AsyncHttpHandle handle{url};
    for (int i = 0; i < iteration; i++)
    {
        responses.push_back(co_await handle.post(ctx, multipart));
    }
}

TEST(AsyncHttpClientTests, HttpGet)
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
        sendGetRequests(ctx, url, responses, iteration) |
        sdbusplus::async::execution::then([&ctx]() { ctx.request_stop(); }));
    ctx.run();
    for (int i = 0; i < iteration; i++)
    {
        EXPECT_EQ(responses[i].code, 200);
        EXPECT_EQ(responses[i].body, responseBody);
    }
}

TEST(AsyncHttpClientTests, HttpPost)
{
    std::string responseBody = "Hello, World!";
    std::unordered_map<std::string, std::string> responseHeaders;
    SimpleTestHttpServer server(responseBody, responseHeaders);
    unsigned short port = server.getPort();
    std::string url = "http://localhost:" + std::to_string(port) + "/";
    sdbusplus::async::context ctx;
    std::vector<AsyncHttpResponse> responses;
    constexpr int iteration = 10;
    std::FILE* tmpf = std::tmpfile();
    std::fputs("Temp file content", tmpf);
    std::rewind(tmpf);
    std::vector<HttpMultipartBodyPart> multipart;
    auto fd = fileno(tmpf);
    multipart.push_back({
        .name = "UploadFile",
        .type = "application/octet-stream",
        .fd = &fd,
    });
    multipart.push_back({
        .name = "Params",
        .type = "application/json",
        .data = "{\"key1\":\"value1\"}",
    });
    ctx.spawn(
        sendPostRequests(ctx, url, multipart, responses, iteration) |
        sdbusplus::async::execution::then([&ctx]() { ctx.request_stop(); }));
    ctx.run();
    for (int i = 0; i < iteration; i++)
    {
        EXPECT_EQ(responses[i].code, 200);
        EXPECT_EQ(responses[i].body, responseBody);
        const auto request = server.getReceivedRequests()[i];

        EXPECT_THAT(
            request.headers,
            testing::Contains(testing::Pair(
                "Content-Type",
                testing::StartsWith("multipart/form-data; boundary="))));
        EXPECT_THAT(
            request.body,
            testing::HasSubstr(
                "Content-Disposition: form-data; name=\"UploadFile\"\r\nContent-Type: application/octet-stream\r\n\r\nTemp file content\r\n"));
        EXPECT_THAT(
            request.body,
            testing::HasSubstr(
                "Content-Disposition: form-data; name=\"Params\"\r\nContent-Type: application/json\r\n\r\n{\"key1\":\"value1\"}\r\n"));
    }
}

} // namespace redfish_client::core
