#include <redfish_client/core/log_service_handler.hpp>
#include <redfish_client/core/log_entry_mapper_registry.hpp>


#include <phosphor-logging/commit.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/State/CPER/event.hpp>

#include <exception>
#include <filesystem>
#include <format>
#include <optional>
#include <regex>

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
    auto& maybeMembers = collection.getMembers();
    if (maybeMembers.hasValue())
    {
        for (auto& member : maybeMembers.value())
        {
            commit(member);
            // Suspend this task so it is rescheduled, giving pending
            // work (d-bus queries etc) a chance to execute
            co_await sdbusplus::async::execution::schedule(
                get_scheduler(ctx));
        }
    }
}

void LogServiceHandler::commit(redfish_binding::LogEntry::LogEntry& entry)
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
    if (!committedEntries.update(entryId, timestamp))
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
