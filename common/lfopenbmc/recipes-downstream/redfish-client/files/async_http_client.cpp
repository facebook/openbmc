#include "async_http_client.hpp"

#include <sdbusplus/async/fdio.hpp>

#include <format>

namespace redfish_client_daemon
{

namespace
{
class GlobalInit
{
  public:
    GlobalInit()
    {
        if (auto res = curl_global_init(CURL_GLOBAL_ALL); res != CURLE_OK)
        {
            throw std::runtime_error(std::format("curl_global_init failed: {}",
                                                 curl_easy_strerror(res)));
        }
    }
    ~GlobalInit()
    {
        curl_global_cleanup();
    }
};

class HandleLock
{
  public:
    HandleLock(CURL* easyHandle, CURLM* multiHandle, std::mutex& mutex) :
        easyHandle(easyHandle), multiHandle(multiHandle),
        lock(mutex, std::defer_lock)
    {
        if (!lock.try_lock())
        {
            throw std::runtime_error("handle is in use and cannot be shared");
        }
        if (auto res = curl_multi_add_handle(multiHandle, easyHandle);
            res != CURLM_OK)
        {
            throw std::runtime_error(std::format(
                "curl_multi_add_handle failed: {}", curl_multi_strerror(res)));
        }
    }

    ~HandleLock()
    {
        curl_multi_remove_handle(multiHandle, easyHandle);
        lock.unlock();
    }

  private:
    CURL* easyHandle;
    CURLM* multiHandle;
    std::unique_lock<std::mutex> lock;
};
} // namespace

size_t AsyncHttpResponse::write(char* ptr, size_t size, size_t nmemb,
                                void* userdata)
{
    auto response = static_cast<AsyncHttpResponse*>(userdata);
    response->body.append(ptr, nmemb);
    return nmemb;
}

AsyncHttpHandle::AsyncHttpHandle(const std::string& url) :
    easyHandle(curl_easy_init()), multiHandle(curl_multi_init())
{
    static const GlobalInit init{};
    if (!easyHandle)
    {
        throw std::runtime_error("curl_easy_init failed");
    }
    if (!multiHandle)
    {
        throw std::runtime_error("curl_multi_init failed");
    }
    if (auto res = curl_easy_setopt(easyHandle, CURLOPT_URL, url.c_str());
        res != CURLE_OK)
    {
        throw std::runtime_error(
            std::format("curl_easy_setopt CURLOPT_URL failed: {}",
                        curl_easy_strerror(res)));
    }
    if (auto res = curl_easy_setopt(easyHandle, CURLOPT_WRITEFUNCTION,
                                    AsyncHttpResponse::write);
        res != CURLE_OK)
    {
        throw std::runtime_error(
            std::format("curl_easy_setopt CURLOPT_WRITEFUNCTION failed: {}",
                        curl_easy_strerror(res)));
    }
    if (auto res =
            curl_easy_setopt(easyHandle, CURLOPT_TIMEOUT, kDefaultTimeoutSec);
        res != CURLE_OK)
    {
        throw std::runtime_error(
            std::format("curl_easy_setopt CURLOPT_TIMEOUT failed: {}",
                        curl_easy_strerror(res)));
    }
}

AsyncHttpHandle::~AsyncHttpHandle()
{
    curl_multi_cleanup(multiHandle);
    curl_easy_cleanup(easyHandle);
}

sdbusplus::async::task<AsyncHttpResponse> AsyncHttpHandle::get(
    sdbusplus::async::context& ctx)
{
    HandleLock lock(easyHandle, multiHandle, mutex);
    AsyncHttpResponse response;
    if (auto res = curl_easy_setopt(easyHandle, CURLOPT_WRITEDATA, &response);
        res != CURLE_OK)
    {
        throw std::runtime_error(
            std::format("curl_easy_setopt CURLOPT_WRITEDATA failed: {}",
                        curl_easy_strerror(res)));
    }
    constexpr auto fdSize = 1;
    curl_waitfd fds[fdSize];
    unsigned int fdCount = 0;
    int stillRunning = 0;
    do
    {
        if (auto res = curl_multi_perform(multiHandle, &stillRunning);
            res != CURLM_OK)
        {
            throw std::runtime_error(std::format(
                "curl_multi_perform failed: {}", curl_multi_strerror(res)));
        }
        if (stillRunning == 0)
        {
            break;
        }
        if (auto res = curl_multi_waitfds(multiHandle, fds, fdSize, &fdCount);
            res != CURLM_OK)
        {
            throw std::runtime_error(std::format(
                "curl_multi_waitfds failed: {}", curl_multi_strerror(res)));
        }
        if (fdCount == 0 || (fds[0].events & CURL_WAIT_POLLIN) == 0)
        {
            continue;
        }
        sdbusplus::async::fdio fdioInstance{ctx, fds[0].fd};
        co_await fdioInstance.next();
    } while (stillRunning > 0);
    int msgq = 0;
    auto msg = curl_multi_info_read(multiHandle, &msgq);
    if (msg == nullptr)
    {
        throw std::runtime_error("curl_multi_info_read failed");
    }

    if (msg->data.result != CURLE_OK)
    {
        throw std::runtime_error(
            std::format("curl_multi_info_read failed: {}",
                        curl_easy_strerror(msg->data.result)));
    }
    if (auto res = curl_easy_getinfo(easyHandle, CURLINFO_RESPONSE_CODE,
                                     &response.code);
        res != CURLE_OK)
    {
        throw std::runtime_error(
            std::format("curl_easy_getinfo CURLINFO_RESPONSE_CODE failed: {}",
                        curl_easy_strerror(res)));
    }
    co_return response;
}

} // namespace redfish_client_daemon
