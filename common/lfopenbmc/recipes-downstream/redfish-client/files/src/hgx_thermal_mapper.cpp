#include <redfish_client/core/hgx_thermal_mapper.hpp>
#include <redfish_client/core/log_entry_mapper_utils.hpp>
#include <phosphor-logging/lg2.hpp>
#include <phosphor-logging/commit.hpp>

#include <xyz/openbmc_project/State/Thermal/event.hpp>
#include "redfish-binding/LogEntry_EventSeverity.hpp"

#include <algorithm>
#include <vector>

namespace redfish_client::core {

namespace {

using EventSeverity = redfish_binding::LogEntry::EventSeverity;

namespace ThermalError = sdbusplus::error::xyz::openbmc_project::state::Thermal;

/**
 * Define the key strings to be identified according to the specification.
 */
const std::vector<std::string> thermalKeywords = {
    "THERM_WARN_INT",
    "TEMP_ALERT_INT",
    "ThrottleReason",
    "HWThermalSlowdown",
    "SWPowerCap",
    "HWPowerBrakeSlowdown",
    "SyncBoost",
    "GPUThermalOvertTreshold"
};

/**
 * Extract the source from Links/OriginOfCondition; return Unknown if not present.
 */
std::string extractThermalSource(redfish_binding::LogEntry::LogEntry& entry) {
    auto& maybeLinks = entry.getLinks();
    if (maybeLinks.hasValue()) {
        auto& origin = maybeLinks.value().getOriginOfCondition();
        if (origin.hasValue() && origin.value().getOdataId().hasValue()) {
            return origin.value().getOdataId().value();
        }
    }
    return "Unknown_Thermal_Source";
}

} // anonymous namespace

bool HgxThermalMapper::canHandle(redfish_binding::LogEntry::LogEntry& entry) const {
    auto& maybeMessageId = entry.getMessageId();
    if (!maybeMessageId.hasValue()) return false;

    const std::string& msgId = maybeMessageId.value();

    // Check whether the MessageId complies with the specification.
    bool isValidMsgId = (msgId.find("ResourceErrorsDetected") != std::string::npos ||
                         msgId.find("ResourceStatusChangedWarning") != std::string::npos);

    if (!isValidMsgId) return false;

    // Check whether the MessageArgs contain thermal event keywords.
    auto& maybeArgs = entry.getMessageArgs();
    if (maybeArgs.hasValue()) {
        for (const auto& arg : maybeArgs.value()) {
            for (const auto& keyword : thermalKeywords) {
                if (arg.find(keyword) != std::string::npos) {
                    return true;
                }
            }
        }
    }

    return false;
}

void HgxThermalMapper::map(redfish_binding::LogEntry::LogEntry& entry) {
    std::string source = extractThermalSource(entry);

    std::string failureData = formatFailureData(entry);

    // Determine the mapped BMC event based on the Severity.
    auto& maybeSeverity = entry.getSeverity();
    bool isCritical = (maybeSeverity.hasValue() &&
                       maybeSeverity.value() == EventSeverity::Critical);

    lg2::info("Thermal event mapped from Redfish, source: {SRC}, critical: {CRIT}",
              "SRC", source, "CRIT", isCritical);

    if (isCritical) {
        lg2::commit(ThermalError::DeviceOverOperatingTemperatureFault(
            "DEVICE", source,
            "FAILURE_DATA", failureData
        ));
    } else {
        lg2::commit(ThermalError::DeviceOverOperatingTemperature(
            "DEVICE", source,
            "FAILURE_DATA", failureData
        ));
    }
}

} // namespace redfish_client::core