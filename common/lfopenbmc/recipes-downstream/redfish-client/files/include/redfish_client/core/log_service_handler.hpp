#pragma once

#include <redfish_client/core/async_http_client.hpp>
#include <redfish_client/core/config.hpp>
#include <redfish_client/core/persist_map.hpp>
#include "redfish-binding/LogEntryCollection_LogEntryCollection.hpp"
#include "redfish-binding/LogEntry_EventSeverity.hpp"
#include "redfish-binding/LogEntry_LogEntry.hpp"

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/message.hpp>

#include <memory>
#include <string>

namespace redfish_client::core
{

class LogServiceHandler : private sdbusplus::async::context_ref,
                          private sdbusplus::async::details::context_friend
{
  public:
    LogServiceHandler() = delete;

    explicit LogServiceHandler(
        sdbusplus::async::context& ctx, const std::string& url,
        std::optional<size_t> skipHistoricalEntriesThresholdSeconds,
        const std::string& persistDir = "") :
        sdbusplus::async::context_ref(ctx), url(url),
        skipHistoricalEntriesThresholdSeconds(
            skipHistoricalEntriesThresholdSeconds),
        committedEntries(getPersistPath(url, persistDir)),
        httpHandle(std::make_unique<AsyncHttpHandle>(url)) {};

    // Spawn a loop that polls every configured log-service URL on the
    // configured interval until the context stops.
    static auto run(sdbusplus::async::context& ctx, const std::string& host,
                    const LogServiceConfig& config,
                    const std::string& persistDir)
        -> sdbusplus::async::task<void>;

    // Poll this URL once: fetch the log entry collection and commit any new
    // entries.
    auto load() -> sdbusplus::async::task<>;

    auto commit(
        redfish_binding::LogEntryCollection::LogEntryCollection& collection)
        -> sdbusplus::async::task<>;

  private:
    std::string url;
    std::optional<size_t> skipHistoricalEntriesThresholdSeconds;
    PersistMap committedEntries;
    std::unique_ptr<AsyncHttpHandle> httpHandle;

    void commit(redfish_binding::LogEntry::LogEntry& entry,
                std::optional<size_t> historicalThresholdSeconds);

    static std::string getPersistPath(const std::string& url,
                                      const std::string& persistDir);
};

} // namespace redfish_client::core
