#pragma once

#include <curl/curl.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace redfish_client_daemon
{

struct HttpResponse
{
    std::unordered_map<std::string, std::string> headers;
    long responseCode;
    std::string body;
};

class HttpClient
{
  public:
    static void globalInit();
    static void globalDeinit();

    explicit HttpClient(size_t poolSize);
    virtual ~HttpClient();
    HttpClient(const HttpClient&) = delete;
    HttpClient(HttpClient&&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient& operator=(HttpClient&&) = delete;

    virtual HttpResponse get(const char* url);

  private:
    struct WrappedRequest
    {
        int64_t id;
        std::string url;
        HttpResponse response;
        CURLcode errorCode;
    };

    void eventLoop();

    std::thread eventLoopThread;
    std::mutex lock;
    std::condition_variable condVar;
    bool running;
    int64_t nextId;
    size_t poolSize;
    std::queue<std::shared_ptr<WrappedRequest>> requestQueue;
    std::unordered_set<int64_t> completedRequests;
};

} // namespace redfish_client_daemon
