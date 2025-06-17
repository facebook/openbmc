#include "http_client.hpp"

#include <vector>

static void throwUnless(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

#define THROW_UNLESS(condition) throwUnless(condition, #condition " failed")

namespace redfish_client_daemon
{

void HttpClient::globalInit()
{
    CURLcode rc = curl_global_init(CURL_GLOBAL_ALL);
    THROW_UNLESS(CURLE_OK == rc);
}

void HttpClient::globalDeinit()
{
    curl_global_cleanup();
}

HttpClient::HttpClient(size_t poolSize) :
    running(true), nextId(0), poolSize(poolSize)
{
    eventLoopThread = std::thread([this] { eventLoop(); });
}

HttpClient::~HttpClient()
{
    {
        std::lock_guard<std::mutex> guard(this->lock);
        running = false;
    }
    condVar.notify_all();
    eventLoopThread.join();
}

HttpResponse HttpClient::get(const char* url)
{
    auto wrappedRequest = std::make_shared<WrappedRequest>();

    {
        std::lock_guard<std::mutex> guard(this->lock);
        wrappedRequest->id = nextId++;
        wrappedRequest->url = std::string(url);
        requestQueue.push(wrappedRequest);
    }
    condVar.notify_all();

    // Wait for the response.
    HttpResponse rv;
    {
        std::unique_lock<std::mutex> ulock(this->lock);
        int64_t requestId = wrappedRequest->id;
        condVar.wait(ulock, [this, requestId] {
            return (completedRequests.find(requestId) !=
                    completedRequests.end());
        });
        THROW_UNLESS(running);
        THROW_UNLESS(CURLE_OK == wrappedRequest->errorCode);
        rv = std::move(wrappedRequest->response);
        completedRequests.erase(requestId);
    }

    return rv;
}

static size_t writeData(char* buffer, size_t size, size_t nmemb, void* userp)
{
    THROW_UNLESS(1 == size);

    HttpResponse* response = (HttpResponse*)userp;
    response->body.append(buffer, nmemb);
    return nmemb;
}

struct EasyHandlePool
{
    struct WrappedEasyHandle
    {
        CURL* easyHandle;
        bool inUse;
    };

    std::unordered_map<CURL*, bool> easyHandlesInUseMap;

    explicit EasyHandlePool(size_t poolSize)
    {
        for (size_t i = 0; i < poolSize; i++)
        {
            CURL* easyHandle = curl_easy_init();
            THROW_UNLESS(easyHandle != nullptr);
            easyHandlesInUseMap[easyHandle] = false;
        }
    }

    ~EasyHandlePool()
    {
        for (auto& [easyHandle, inUse] : easyHandlesInUseMap)
        {
            THROW_UNLESS(!inUse);
            curl_easy_cleanup(easyHandle);
        }
    }

    CURL* tryGetEasyHandleFromPool()
    {
        CURL* rv = nullptr;
        for (auto& [easyHandle, inUse] : easyHandlesInUseMap)
        {
            if (!inUse)
            {
                rv = easyHandle;
                break;
            }
        }

        if (rv)
        {
            easyHandlesInUseMap[rv] = true;
            curl_easy_reset(rv);
        }

        return rv;
    }

    void returnEasyHandleToPool(CURL* doneHandle)
    {
        THROW_UNLESS(easyHandlesInUseMap.find(doneHandle) !=
                     easyHandlesInUseMap.end());
        THROW_UNLESS(easyHandlesInUseMap[doneHandle]);
        easyHandlesInUseMap[doneHandle] = false;
    }
};

void HttpClient::eventLoop()
{
    CURLMcode mc;
    CURLcode rc;

    std::unordered_map<CURL*, std::shared_ptr<WrappedRequest>>
        handleToWrappedRequestMap;

    CURLM* multiHandle = curl_multi_init();
    THROW_UNLESS(multiHandle != nullptr);

    auto handlePool = std::make_unique<EasyHandlePool>(poolSize);

    struct curl_slist* defaultHeaderList = nullptr;
    defaultHeaderList =
        curl_slist_append(defaultHeaderList, "User-Agent: curl");
    THROW_UNLESS(defaultHeaderList != nullptr);

    while (true)
    {
        std::shared_ptr<WrappedRequest> nextRequest;
        std::vector<int64_t> aggregatedResponseReceivedIds;
        CURL* easyHandle = nullptr;

        {
            std::lock_guard<std::mutex> guard(this->lock);
            if (!running)
            {
                condVar.notify_all();
                break;
            }
        }

        // Get a request from the queue
        {
            std::unique_lock<std::mutex> ulock(this->lock);
            // Wait for a request to be available if necessary.
            if (handleToWrappedRequestMap.empty() && requestQueue.empty())
            {
                condVar.wait(ulock, [this] { return true; });
            }
            if (!running)
            {
                condVar.notify_all();
                break;
            }
            if (!requestQueue.empty())
            {
                easyHandle = handlePool->tryGetEasyHandleFromPool();
                if (easyHandle == nullptr)
                {
                    // No handles available in the pool, something must be
                    // running right now.
                    THROW_UNLESS(!handleToWrappedRequestMap.empty());
                }
                else
                {
                    nextRequest = requestQueue.front();
                    requestQueue.pop();
                }
            }
        }

        if (nextRequest)
        {
            THROW_UNLESS(easyHandle != nullptr);

            rc = curl_easy_setopt(easyHandle, CURLOPT_URL,
                                  nextRequest->url.c_str());
            THROW_UNLESS(CURLE_OK == rc);

            rc = curl_easy_setopt(easyHandle, CURLOPT_WRITEFUNCTION, writeData);
            THROW_UNLESS(CURLE_OK == rc);

            HttpResponse* responsePtr = &nextRequest->response;
            rc = curl_easy_setopt(easyHandle, CURLOPT_WRITEDATA, responsePtr);
            THROW_UNLESS(CURLE_OK == rc);

            rc = curl_easy_setopt(easyHandle, CURLOPT_HTTPHEADER,
                                  defaultHeaderList);
            THROW_UNLESS(CURLE_OK == rc);

            // Add the easy handle to the multi handle
            mc = curl_multi_add_handle(multiHandle, easyHandle);
            THROW_UNLESS(CURLM_OK == mc);

            handleToWrappedRequestMap[easyHandle] = nextRequest;
        }

        // Perform one round of wait & check.
        {
            int stillRunning = 0;
            CURLMcode mc = curl_multi_perform(multiHandle, &stillRunning);
            THROW_UNLESS(CURLM_OK == mc);

            // Wait for activity on the multi handle
            if (stillRunning)
            {
                mc = curl_multi_poll(multiHandle, nullptr, 0, 10, nullptr);
                THROW_UNLESS(CURLM_OK == mc);
            }
        }

        // Pull the results.
        while (true)
        {
            int msgsLeft = 0;
            CURLMsg* msg = curl_multi_info_read(multiHandle, &msgsLeft);
            if (msg == nullptr)
            {
                break;
            }
            if (msg->msg != CURLMSG_DONE)
            {
                continue;
            }

            CURL* doneHandle = msg->easy_handle;
            std::shared_ptr<WrappedRequest> wrappedRequest =
                handleToWrappedRequestMap[doneHandle];
            handleToWrappedRequestMap.erase(doneHandle);

            wrappedRequest->errorCode = msg->data.result;
            rc = curl_easy_getinfo(doneHandle, CURLINFO_RESPONSE_CODE,
                                   &wrappedRequest->response.responseCode);
            if (CURLE_OK == rc)
            {
                struct curl_header* prev = nullptr;
                struct curl_header* h = nullptr;
                while ((h = curl_easy_nextheader(doneHandle, CURLH_HEADER, 0,
                                                 prev)))
                {
                    wrappedRequest->response.headers[h->name] = h->value;
                    prev = h;
                }
            }

            aggregatedResponseReceivedIds.push_back(wrappedRequest->id);

            mc = curl_multi_remove_handle(multiHandle, doneHandle);
            THROW_UNLESS(CURLM_OK == mc);

            handlePool->returnEasyHandleToPool(doneHandle);
        }

        if (!aggregatedResponseReceivedIds.empty())
        {
            std::lock_guard<std::mutex> guard(this->lock);
            for (const auto& request_id : aggregatedResponseReceivedIds)
            {
                completedRequests.insert(request_id);
            }
        }

        condVar.notify_all();
    }

    curl_slist_free_all(defaultHeaderList);

    // Destroy the handles before the multi handle.
    handlePool = nullptr;

    mc = curl_multi_cleanup(multiHandle);
    THROW_UNLESS(CURLM_OK == mc);
}

} // namespace redfish_client_daemon
