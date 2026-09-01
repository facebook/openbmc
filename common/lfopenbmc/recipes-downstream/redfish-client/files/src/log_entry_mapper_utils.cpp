#include <redfish_client/core/log_entry_mapper_utils.hpp>
#include "redfish-binding/LogEntry_EventSeverity.hpp"

#include <phosphor-logging/lg2.hpp>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace redfish_client::core {

namespace
{

using EventSeverity = redfish_binding::LogEntry::EventSeverity;

} // anonymous namespace

std::optional<std::string> extractSeverity(
    redfish_binding::LogEntry::LogEntry& entry)
{
    auto& maybeSeverity = entry.getSeverity();
    if (!maybeSeverity.hasValue())
    {
        return std::nullopt;
    }

    switch (maybeSeverity.value())
    {
        case EventSeverity::OK:
            return "OK";
        case EventSeverity::Warning:
            return "Warning";
        case EventSeverity::Critical:
            return "Critical";
        default:
            return std::nullopt;
    }
}

std::string extractOrigin(redfish_binding::LogEntry::LogEntry& entry)
{
    auto& maybeLinks = entry.getLinks();
    if (maybeLinks.hasValue())
    {
        auto& origin = maybeLinks.value().getOriginOfCondition();
        if (origin.hasValue() && origin.value().getOdataId().hasValue())
        {
            return origin.value().getOdataId().value();
        }
    }
    return "Unknown Source";
}

std::optional<OemData> extractOemData(
    redfish_binding::LogEntry::LogEntry& entry)
{
    auto& maybeOem = entry.getOem();
    if (!maybeOem.hasValue())
    {
        return std::nullopt;
    }

    try
    {
        nlohmann::json oemJson = maybeOem.value();

        // Oem block must be a non-empty object
        if (!oemJson.is_object() || oemJson.empty())
        {
            lg2::warning("Oem section is empty or not an object");
            return std::nullopt;
        }

        // First key is the vendor name (e.g. "AMD")
        auto it = oemJson.begin();
        const std::string vendor = it.key();
        const nlohmann::json& vendorData = it.value();

        return OemData{
            .vendor = vendor,
            .jsonData = vendorData.dump(),
        };
    }
    catch (const nlohmann::json::exception& e)
    {
        lg2::warning("Failed to serialize Oem section: {ERROR}",
                     "ERROR", e.what());
        return std::nullopt;
    }
}

std::string formatFailureData(
    redfish_binding::LogEntry::LogEntry& entry,
    size_t maxSize)
{
    nlohmann::json failureData;

    auto setIfPresent = [&failureData]<typename T>(const char* key,
                                                   T& maybeValue) {
        if (maybeValue.hasValue())
        {
            failureData[key] = maybeValue.value();
        }
    };

    setIfPresent("Created", entry.getCreated());
    setIfPresent("Message", entry.getMessage());
    setIfPresent("Resolution", entry.getResolution());

    auto severityStr = extractSeverity(entry);
    if (severityStr.has_value())
    {
        failureData["Severity"] = severityStr.value();
    }

    std::string result = failureData.dump();

    // Ensure failureData fits within the size budget. The whole BMC event
    // must be under 4k, so we cap this field at maxSize (default 1.5k).
    if (result.size() > maxSize)
    {
        // Drop optional fields in order of decreasing verbosity to fit.
        if (failureData.contains("Resolution"))
        {
            failureData.erase("Resolution");
            result = failureData.dump();
        }
        if (result.size() > maxSize && failureData.contains("Message"))
        {
            // Truncate the message to fit within budget.
            std::string msg = failureData["Message"].get<std::string>();
            size_t overhead = result.size() - msg.size();
            if (maxSize > overhead + 3)
            {
                msg.resize(maxSize - overhead - 3);
                msg += "...";
                failureData["Message"] = msg;
            }
            else
            {
                failureData.erase("Message");
            }
            result = failureData.dump();
        }
    }

    return result;
}

} // namespace redfish_client::core
