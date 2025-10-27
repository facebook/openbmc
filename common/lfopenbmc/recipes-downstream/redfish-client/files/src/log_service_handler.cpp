#include <redfish_client/core/log_service_handler.hpp>

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

using EventSeverity = redfish_binding::LogEntry::EventSeverity;

namespace
{

template <typename T>
T makeCPER(const auto& source, const auto& cper)
{
    return T("SOURCE", sdbusplus::message::object_path(source), "CPER",
             cper.toJson().dump());
}

std::optional<std::string> cperOemNvidiaHandler(const nlohmann::json& entryJson)
{
    try
    {
        const auto& cperOemNvidia = entryJson.at("CPER").at("Oem").at("Nvidia");

        if (cperOemNvidia.contains("Pcie"))
        {
            const auto& pcie = cperOemNvidia["Pcie"];
            if (pcie.contains("DeviceID"))
            {
                const auto& deviceId = pcie["DeviceID"];
                if (deviceId.contains("PrimaryOrDeviceBusNumber") &&
                    deviceId.contains("DeviceNumber") &&
                    deviceId.contains("FunctionNumber"))
                {
                    return std::format(
                        "pcie-b{}-d{}-f{}",
                        deviceId["PrimaryOrDeviceBusNumber"].get<int>(),
                        deviceId["DeviceNumber"].get<int>(),
                        deviceId["FunctionNumber"].get<int>());
                }
            }
        }
    }
    catch (...)
    {
        // silent catch, just return null at end of function.
    }

    return std::nullopt;
}

class UnhandledException : public sdbusplus::exception::generated_event_base
{
  public:
    UnhandledException(EventSeverity eventSeverity,
                       const nlohmann::json& eventData) :
        eventSeverity(eventSeverity), eventData(eventData)
    {}

    const char* name() const noexcept override
    {
        return kExceptionName;
    }

    const char* description() const noexcept override
    {
        return "Unhandled Exception from Satellite MC";
    }

    int get_errno() const noexcept override
    {
        return EIO;
    }

    nlohmann::json to_json() const override
    {
        nlohmann::json j = {};
        j["REDFISH_EVENT"] = eventData.dump();

        return nlohmann::json{{kExceptionName, std::move(j)}};
    }

    int severity() const noexcept override
    {
        switch (eventSeverity)
        {
            case EventSeverity::OK:
                return LOG_INFO;
            case EventSeverity::Warning:
                return LOG_WARNING;
            case EventSeverity::Critical:
                return LOG_CRIT;
            default:
                return LOG_ERR;
        }
    }

  private:
    EventSeverity eventSeverity;
    nlohmann::json eventData;
    static constexpr const char* kExceptionName =
        "com.meta.RedfishClient.UnexpectedException";
};
} // anonymous namespace

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
        commit(logEntryCollection);
    }
    catch (const std::exception& exn)
    {
        info("Exception while parsing url response {EXC}", "EXC", exn.what());
        debug("Exception while parsing url response: {URL}", "URL",
              url.c_str());
    };
}

void LogServiceHandler::commit(
    redfish_binding::LogEntryCollection::LogEntryCollection& collection)
{
    auto& maybeMembers = collection.getMembers();
    if (maybeMembers.hasValue())
    {
        for (auto& member : maybeMembers.value())
        {
            commit(member);
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
        auto eventSeverity = entry.getSeverity().hasValue()
                                 ? entry.getSeverity().value()
                                 : EventSeverity::Critical;
        auto& maybeCPER = entry.getCPER();
        auto& maybeLinks = entry.getLinks();

        // If the event has a CPER section, it must be a CPER.  Translate
        // it into the GenericCPER* exceptions.
        if (maybeCPER.hasValue())
        {
            auto source = [&]() -> std::string {
                auto unknownSource = "Unknown Source";

                if (maybeLinks.hasValue())
                {
                    auto& origin = maybeLinks.value().getOriginOfCondition();
                    if (origin.hasValue() &&
                        origin.value().getOdataId().hasValue())
                    {
                        return origin.value().getOdataId().value();
                    }
                }

                auto findSource = cperOemNvidiaHandler(entry.toJson());
                if (findSource)
                {
                    return *findSource;
                }

                return unknownSource;
            }();

            if (eventSeverity == EventSeverity::Critical)
            {
                lg2::commit(makeCPER<CPER::GenericCPERFault>(
                    source, maybeCPER.value()));
            }
            else
            {
                lg2::commit(makeCPER<CPER::GenericCPERWarning>(
                    source, maybeCPER.value()));
            }
            return;
        }

        lg2::commit(UnhandledException(eventSeverity, entry.toJson()));
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
