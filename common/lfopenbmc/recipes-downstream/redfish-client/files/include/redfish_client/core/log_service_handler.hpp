#pragma once

#include <redfish_client/core/async_http_client.hpp>
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

class LogServiceHandler : private sdbusplus::async::context_ref
{
  public:
    LogServiceHandler() = delete;

    explicit LogServiceHandler(sdbusplus::async::context& ctx,
                               const std::string& url,
                               const std::string& persistDir = "") :
        sdbusplus::async::context_ref(ctx), url(url),
        committedEntries(getPersistPath(url, persistDir)),
        httpHandle(std::make_unique<AsyncHttpHandle>(url)) {};

    auto runOnce() -> sdbusplus::async::task<>;

    void commit(
        redfish_binding::LogEntryCollection::LogEntryCollection& collection);

  private:
    std::string url;
    PersistMap committedEntries;
    std::unique_ptr<AsyncHttpHandle> httpHandle;

    void commit(redfish_binding::LogEntry::LogEntry& entry);

    static std::string getPersistPath(const std::string& url,
                                      const std::string& persistDir);
};

} // namespace redfish_client::core
