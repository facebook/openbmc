#include <redfish_client/core/async_http_client.hpp>

#include <unistd.h>

#include <sdbusplus/async/fdio.hpp>

#include <format>

namespace redfish_client::core
{

namespace
{
size_t fdRead(char* buffer, size_t size, size_t nitems, void* arg)
{
    int fd = *static_cast<int*>(arg);
    auto count = read(fd, buffer, size * nitems);
    return count < 0 ? CURL_READFUNC_ABORT : count;
}

int fdSeek(void* arg, curl_off_t offset, int origin)
{
    int fd = *static_cast<int*>(arg);
    return lseek(fd, offset, origin) == -1 ? CURL_SEEKFUNC_FAIL
                                           : CURL_SEEKFUNC_OK;
}

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

class Mime
{
  public:
    Mime(CURL* easyHandle,
         const std::vector<HttpMultipartBodyPart>& multipart) :
        easyHandle(easyHandle), mime(curl_mime_init(easyHandle))
    {
        if (!mime)
        {
            throw std::runtime_error("curl_mime_init failed");
        }
        for (const auto& p : multipart)
        {
            auto part = curl_mime_addpart(mime);
            if (!part)
            {
                throw std::runtime_error("curl_mime_addpart failed");
            }
            if (p.name.has_value())
            {
                if (auto res = curl_mime_name(part, p.name->c_str());
                    res != CURLE_OK)
                {
                    throw std::runtime_error(std::format(
                        "curl_mime_name failed: {}", curl_easy_strerror(res)));
                }
            }
            if (p.type.has_value())
            {
                if (auto res = curl_mime_type(part, p.type->c_str());
                    res != CURLE_OK)
                {
                    throw std::runtime_error(std::format(
                        "curl_mime_type failed: {}", curl_easy_strerror(res)));
                }
            }
            if (p.data.has_value())
            {
                if (auto res = curl_mime_data(part, p.data->c_str(),
                                              CURL_ZERO_TERMINATED);
                    res != CURLE_OK)
                {
                    throw std::runtime_error(std::format(
                        "curl_mime_data failed: {}", curl_easy_strerror(res)));
                }
            }
            if (p.fd)
            {
                auto size = lseek(*p.fd, 0, SEEK_END);
                if (size < 0)
                {
                    throw std::runtime_error(std::format(
                        "failed to determine file size from fd: {}", *p.fd));
                }
                if (lseek(*p.fd, 0, SEEK_SET) != 0)
                {
                    throw std::runtime_error(std::format(
                        "failed to reset file offset to 0 for fd: {}", *p.fd));
                }
                if (auto res = curl_mime_data_cb(part, size, fdRead, fdSeek,
                                                 nullptr, p.fd);
                    res != CURLE_OK)
                {
                    throw std::runtime_error(
                        std::format("curl_mime_data_cb failed: {}",
                                    curl_easy_strerror(res)));
                }
            }
        }
        if (auto res = curl_easy_setopt(easyHandle, CURLOPT_MIMEPOST, mime);
            res != CURLE_OK)
        {
            throw std::runtime_error(
                std::format("curl_easy_setopt CURLOPT_MIMEPOST failed: {}",
                            curl_easy_strerror(res)));
        }
    }

    ~Mime()
    {
        curl_easy_setopt(easyHandle, CURLOPT_MIMEPOST, nullptr);
        curl_mime_free(mime);
    }

  private:
    CURL* easyHandle;
    curl_mime* mime;
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
    co_return co_await perform(ctx);
}

sdbusplus::async::task<std::expected<AsyncHttpResponse, std::string>>
    AsyncHttpHandle::tryGet(sdbusplus::async::context& ctx)
{
    try
    {
        co_return co_await get(ctx);
    }
    catch (const std::exception& exn)
    {
        co_return std::unexpected(exn.what());
    }
}

sdbusplus::async::task<AsyncHttpResponse> AsyncHttpHandle::post(
    sdbusplus::async::context& ctx,
    const std::vector<HttpMultipartBodyPart>& multipart)
{
    HandleLock lock(easyHandle, multiHandle, mutex);
    Mime mime{easyHandle, multipart};
    co_return co_await perform(ctx);
}

sdbusplus::async::task<std::expected<AsyncHttpResponse, std::string>>
    AsyncHttpHandle::tryPost(
        sdbusplus::async::context& ctx,
        const std::vector<HttpMultipartBodyPart>& multipart)
{
    try
    {
        co_return co_await post(ctx, multipart);
    }
    catch (const std::exception& exn)
    {
        co_return std::unexpected(exn.what());
    }
}

sdbusplus::async::task<AsyncHttpResponse> AsyncHttpHandle::perform(
    sdbusplus::async::context& ctx)
{
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

} // namespace redfish_client::core
