#include "helper.hpp"
#include "http_client.hpp"

#include <gtest/gtest.h>

namespace redfish_client_daemon
{

TEST(HttpClientTests, TestClient)
{
    redfish_client_daemon::HttpClient::globalInit();
    auto client = std::make_unique<redfish_client_daemon::HttpClient>(1);

    static constexpr int kIters = 10;
    std::string responseBody = "Hello, World!";
    std::unordered_map<std::string, std::string> responseHeaders;
    responseHeaders["h0"] = "v0";
    responseHeaders["h1"] = "v1";
    responseHeaders["h2"] = "v2";
    SimpleTestHttpServer server(responseBody, responseHeaders);
    unsigned short port = server.getPort();
    std::string url = "http://localhost:" + std::to_string(port) + "/";

    // Run it a few times to detect flakiness with higher probability.
    for (int j = 0; j < kIters; ++j)
    {
        // Test the response received by the client.
        auto response = client->get(url.c_str());
        EXPECT_EQ(200, response.responseCode);
        EXPECT_EQ(3, response.headers.size());
        EXPECT_STREQ("v0", response.headers["h0"].c_str());
        EXPECT_STREQ("v1", response.headers["h1"].c_str());
        EXPECT_STREQ("v2", response.headers["h2"].c_str());

        response.body.push_back(0);
        const char* bodyCstr = response.body.data();
        EXPECT_STREQ(responseBody.c_str(), bodyCstr);
    }
    server.stopListening();
    auto requestsCopy = server.getReceivedRequests();
    for (const auto& request : requestsCopy)
    {
        // Make a local copy of the headers so that we can drop the
        // const for verification.
        auto headers = request.headers;
        EXPECT_STREQ("GET", request.method.c_str());
        EXPECT_STREQ("/", request.path.c_str());
        EXPECT_TRUE(strstr(request.version.c_str(), "HTTP/1.1") != nullptr);
        EXPECT_TRUE(strstr(headers["Accept"].c_str(), "*/*") != nullptr);
        EXPECT_TRUE(strstr(headers["User-Agent"].c_str(), "curl") != nullptr);
    }

    // Destroy the client before global deinit.
    client = nullptr;
    redfish_client_daemon::HttpClient::globalDeinit();
}

} // namespace redfish_client_daemon
