#pragma once

#include "http_client.hpp"
#include "persist_map.hpp"
#include "redfish-binding/LogEntryCollection_LogEntryCollection.hpp"
#include "redfish-binding/LogEntry_EventSeverity.hpp"
#include "redfish-binding/LogEntry_LogEntry.hpp"

#include <sdbusplus/message.hpp>

#include <memory>
#include <string>

namespace redfish_client_daemon
{

class LogServiceHandler
{
  public:
    LogServiceHandler() = delete;

    explicit LogServiceHandler(const std::string& url,
                               const std::string& persistDir = "") :
        url(url), committedEntries(getPersistPath(url, persistDir)) {};

    void runOnce();

    void commit(
        redfish_binding::LogEntryCollection::LogEntryCollection& collection);

  private:
    std::string url;
    PersistMap committedEntries;
    std::unique_ptr<HttpClient> httpClient = std::make_unique<HttpClient>(1);

    void commit(redfish_binding::LogEntry::LogEntry& entry);

    static std::string getPersistPath(const std::string& url,
                                      const std::string& persistDir);
};

} // namespace redfish_client_daemon
