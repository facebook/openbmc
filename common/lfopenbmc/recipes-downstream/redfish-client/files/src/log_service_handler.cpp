#include <redfish_client/core/log_service_handler.hpp>
#include <redfish_client/core/log_entry_mapper_registry.hpp>


#include <phosphor-logging/commit.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/State/CPER/event.hpp>

#include <chrono>
#include <exception>
#include <filesystem>
#include <format>
#include <optional>
#include <regex>
#include <sstream>

PHOSPHOR_LOG2_USING;

namespace redfish_client::core
{

auto LogServiceHandler::runOnce() -> sdbusplus::async::task<>
{
    std::string logEntryJson;

    try
    {
        auto response = co_await httpHandle->get(ctx);
        if (response.code != 200)
        {
            throw std::runtime_error(
                std::format("Http response error code: {}", response.code));
        }
        logEntryJson = response.body;
    }
    catch (const std::exception& exn)
    {
        info("Exception while querying url {EXC}", "EXC", exn.what());
        debug("Exception while querying url: {URL}", "URL", url.c_str());
        co_return;
    };

    try
    {
        auto logEntryCollection =
            redfish_binding::LogEntryCollection::parseLogEntryCollection(
                logEntryJson);
        co_await commit(logEntryCollection);
    }
    catch (const std::exception& exn)
    {
        info("Exception while parsing url response {EXC}", "EXC", exn.what());
        debug("Exception while parsing url response: {URL}", "URL",
              url.c_str());
    };
}

auto LogServiceHandler::commit(
    redfish_binding::LogEntryCollection::LogEntryCollection& collection)
    -> sdbusplus::async::task<>
{
    std::optional<size_t> historicalThresholdSeconds;
    if (!committedEntries.isLoaded() && !committedEntries.load())
    {
        historicalThresholdSeconds = skipHistoricalEntriesThresholdSeconds;
    }
    auto& maybeMembers = collection.getMembers();
    if (maybeMembers.hasValue())
    {
        for (auto& member : maybeMembers.value())
        {
            commit(member, historicalThresholdSeconds);
            // Suspend this task so it is rescheduled, giving pending
            // work (d-bus queries etc) a chance to execute
            co_await sdbusplus::async::execution::schedule(
                get_scheduler(ctx));
        }
    }
}

void LogServiceHandler::commit(redfish_binding::LogEntry::LogEntry& entry,
                               std::optional<size_t> historicalThresholdSeconds)
{
    namespace CPER = sdbusplus::error::xyz::openbmc_project::state::CPER;

    if (!entry.getId().hasValue())
    {
        return;
    }
    const auto& entryId = entry.getId().value();
    if (!entry.getCreated().hasValue())
    {
        return;
    }
    const auto& timestamp = entry.getCreated().value();
    bool shouldSkip = false;
    if (historicalThresholdSeconds.has_value())
    {
        std::istringstream ss(timestamp);
        std::chrono::system_clock::time_point tp;
        // Parse ISO 8601 timestamp (e.g., "2025-01-01T12:00:00+00:00")
        // %F=date, %T=time, %Ez=colon-separated UTC offset
        ss >> std::chrono::parse("%FT%T%Ez", tp);
        if (ss.fail())
        {
            warning(
                "Failed to parse timestamp for log entry {ENTRY_ID}: {TIMESTAMP}",
                "ENTRY_ID", entryId, "TIMESTAMP", timestamp);
            shouldSkip = true;
        }
        else if (std::chrono::system_clock::now() - tp >
                 std::chrono::seconds(*historicalThresholdSeconds))
        {
            shouldSkip = true;
        }
    }
    if (!committedEntries.update(entryId, timestamp) || shouldSkip)
    {
        return;
    }
    try
    {
        auto& mapper = LogEntryMapperRegistry::instance().resolve(entry);
        mapper.map(entry);
    }
    catch (const std::exception& e)
    {
        error(
            "Could not commit event log entry for entry id {ENTRY_ID}: {ERROR}",
            "ENTRY_ID", entryId, "ERROR", e);
        return;
    }
}

std::string LogServiceHandler::getPersistPath(const std::string& url,
                                              const std::string& persistDir)
{
    return persistDir.empty()
               ? ""
               : std::filesystem::path(persistDir) /
                     std::filesystem::path(std::regex_replace(
                         url, std::regex("[^a-zA-Z0-9]"), "_"));
}

} // namespace redfish_client::core
