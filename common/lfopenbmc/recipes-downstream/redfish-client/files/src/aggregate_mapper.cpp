#include <redfish_client/core/aggregate_mapper.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/message.hpp>
#include "redfish-binding/LogEntry_EventSeverity.hpp"
#include <map>
#include <string>

namespace redfish_client::core
{

namespace
{

using EventSeverity = redfish_binding::LogEntry::EventSeverity;

struct DbusEventMap
{
    std::string dbusEventName;
    std::vector<std::string_view> argName;
};

struct ParsedEventData
{
    std::string message;
    std::string severity;
    std::map<std::string, std::string> additionalData;
};

static const std::map<std::pair<std::string_view, std::string_view>, DbusEventMap> redfishEventMap =
{
    {{"OpenBMC_MetaIPMIUnifiedSEL", "UnifiedSELEvent"}, {"com.meta.IPMI.UnifiedSEL.UnifiedSELEvent", {"SOURCE", "EVENT"}}},
    {{"OpenBMC_Logging", "Cleared"}, {"xyz.openbmc_project.Logging.Cleared", {}}},
    {{"Update", "ApplyFailed"}, {"xyz.openbmc_project.Software.Update.ApplyFailed", {"IMAGE_IDENTIFIER", "TARGET_NAME"}}},
    {{"Update", "TargetDetermined"}, {"xyz.openbmc_project.Software.Update.TargetDetermined", {"TARGET_NAME", "IMAGE_IDENTIFIER"}}},
    {{"Update", "UpdateSuccessful"}, {"xyz.openbmc_project.Software.Update.UpdateSuccessful", {"TARGET_NAME", "IMAGE_IDENTIFIER"}}},
    {{"Update", "VerificationFailed"}, {"xyz.openbmc_project.Software.Update.VerificationFailed", {"IMAGE_IDENTIFIER", "TARGET_NAME"}}},
    {{"Base", "ResetRequired"}, {"xyz.openbmc_project.Software.Update.ResetDevice", {"TARGET_NAME", "RESET_TYPE"}}},
    {{"OpenBMC_SoftwareUpdate", "UpdateInProgress"}, {"xyz.openbmc_project.Software.Update.UpdateInProgress", {"TARGET_NAME", "PROGRESS_PERCENTAGE"}}},
    {{"SensorEvent", "InvalidSensorReading"}, {"xyz.openbmc_project.Sensor.InvalidSensorReading", {"SENSOR_NAME"}}},
    {{"SensorEvent", "SensorFailure"}, {"xyz.openbmc_project.Sensor.SensorFailure", {"SENSOR_NAME"}}},
    {{"SensorEvent", "SensorRestored"}, {"xyz.openbmc_project.Sensor.SensorRestored", {"SENSOR_NAME"}}},
    {{"SensorEvent", "ReadingAboveLowerCriticalThreshold"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingAboveLowerCriticalThreshold", {"SENSOR_NAME", "READING_VALUE", "UNITS", "THRESHOLD_VALUE"}}},
    {{"SensorEvent", "ReadingAboveLowerFatalThreshold"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingAboveLowerHardShutdownThreshold", {"SENSOR_NAME", "READING_VALUE", "UNITS", "THRESHOLD_VALUE"}}},
    {{"SensorEvent", "ReadingAboveLowerCautionThreshold"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingBelowLowerWarningThreshold", {"SENSOR_NAME", "READING_VALUE", "UNITS", "THRESHOLD_VALUE"}}},
    {{"SensorEvent", "ReadingAboveUpperCriticalThreshold"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingAboveUpperCriticalThreshold", {"SENSOR_NAME", "READING_VALUE", "UNITS", "THRESHOLD_VALUE"}}},
    {{"SensorEvent", "ReadingAboveUpperFatalThreshold"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingAboveUpperHardShutdownThreshold", {"SENSOR_NAME", "READING_VALUE", "UNITS", "THRESHOLD_VALUE"}}},
    {{"SensorEvent", "ReadingAboveUpperCautionThreshold"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingAboveUpperWarningThreshold", {"SENSOR_NAME", "READING_VALUE", "UNITS", "THRESHOLD_VALUE"}}},
    {{"SensorEvent", "ReadingBelowLowerCriticalThreshold"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingBelowLowerCriticalThreshold", {"SENSOR_NAME", "READING_VALUE", "UNITS", "THRESHOLD_VALUE"}}},
    {{"SensorEvent", "ReadingBelowLowerFatalThreshold"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingBelowLowerHardShutdownThreshold", {"SENSOR_NAME", "READING_VALUE", "UNITS", "THRESHOLD_VALUE"}}},
    {{"SensorEvent", "ReadingBelowLowerCautionThreshold"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingBelowLowerWarningThreshold", {"SENSOR_NAME", "READING_VALUE", "UNITS", "THRESHOLD_VALUE"}}},
    {{"SensorEvent", "ReadingBelowUpperCriticalThreshold"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingBelowUpperCriticalThreshold", {"SENSOR_NAME", "READING_VALUE", "UNITS", "THRESHOLD_VALUE"}}},
    {{"SensorEvent", "ReadingBelowFatalThreshold"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingBelowUpperHardShutdownThreshold", {"SENSOR_NAME", "READING_VALUE", "UNITS", "THRESHOLD_VALUE"}}},
    {{"SensorEvent", "ReadingCritical"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingCritical", {"SENSOR_NAME", "READING_VALUE", "UNITS"}}},
    {{"SensorEvent", "ReadingNoLongerCritical"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingNoLongerCritical", {"SENSOR_NAME", "READING_VALUE", "UNITS"}}},
    {{"SensorEvent", "ReadingWarning"}, {"xyz.openbmc_project.Sensor.Threshold.ReadingWarning", {"SENSOR_NAME", "READING_VALUE", "UNITS"}}},
    {{"SensorEvent", "SensorReadingNormalRange"}, {"xyz.openbmc_project.Sensor.Threshold.SensorReadingNormalRange", {"SENSOR_NAME", "READING_VALUE", "UNITS"}}},
    {{"OpenBMC_StateBMC", "RebootCause"}, {"xyz.openbmc_project.State.BMC.RebootCause", {"CAUSE"}}},
    {{"OpenBMC_StateBMC", "StateChanged"}, {"xyz.openbmc_project.State.BMC.StateChanged", {"STATE"}}},
    {{"OpenBMC_StateCPER", "GenericCPERFault"}, {"xyz.openbmc_project.State.CPER.GenericCPERFault", {"SOURCE", "CPER"}}},
    {{"OpenBMC_StateCPER", "GenericCPERWarning"}, {"xyz.openbmc_project.State.CPER.GenericCPERWarning", {"SOURCE", "CPER"}}},
    {{"OpenBMC_StateCable", "CableConnected"}, {"xyz.openbmc_project.State.Cable.CableConnected", {"PORT_ID"}}},
    {{"OpenBMC_StateCable", "CableDisconnected"}, {"xyz.openbmc_project.State.Cable.CableDisconnected", {"PORT_ID"}}},
    {{"Environmental", "FanFailed"}, {"xyz.openbmc_project.State.Fan.FanFailed", {"FAN_NAME"}}},
    {{"Environmental", "FanRestored"}, {"xyz.openbmc_project.State.Fan.FanRestored", {"FAN_NAME"}}},
    {{"Environmental", "FilterRequiresService"}, {"xyz.openbmc_project.State.Filter.FilterRequiresService", {"FILTER_NAME"}}},
    {{"Environmental", "FilterRestored"}, {"xyz.openbmc_project.State.Filter.FilterRestored", {"FILTER_NAME"}}},
    {{"Environmental", "LeakDetectedCritical"}, {"xyz.openbmc_project.State.Leak.Detector.LeakDetectedCritical", {"DETECTOR_NAME"}}},
    {{"Environmental", "LeakDetectedNormal"}, {"xyz.openbmc_project.State.Leak.Detector.LeakDetectedNormal", {"DETECTOR_NAME"}}},
    {{"Environmental", "LeakDetectedWarning"}, {"xyz.openbmc_project.State.Leak.Detector.LeakDetectedWarning", {"DETECTOR_NAME"}}},
    {{"Environmental", "PumpFailed"}, {"xyz.openbmc_project.State.Pump.PumpFailed", {"PUMP_NAME"}}},
    {{"Environmental", "PumpRestored"}, {"xyz.openbmc_project.State.Pump.PumpRestored", {"PUMP_NAME"}}},
    {{"OpenBMC_StateLeakDetectorGroup", "DetectorGroupCritical"}, {"xyz.openbmc_project.State.Leak.DetectorGroup.DetectorGroupCritical", {"DETECTOR_GROUP_NAME"}}},
    {{"OpenBMC_StateLeakDetectorGroup", "DetectorGroupNormal"}, {"xyz.openbmc_project.State.Leak.DetectorGroup.DetectorGroupNormal", {"DETECTOR_GROUP_NAME"}}},
    {{"OpenBMC_StateLeakDetectorGroup", "DetectorGroupWarning"}, {"xyz.openbmc_project.State.Leak.DetectorGroup.DetectorGroupWarning", {"DETECTOR_GROUP_NAME"}}},
    {{"OpenBMC_StateLockOut", "LockOutDisabled"}, {"xyz.openbmc_project.State.LockOut.LockOutDisabled", {"IDENTIFIER"}}},
    {{"OpenBMC_StateLockOut", "LockOutEnabled"}, {"xyz.openbmc_project.State.LockOut.LockOutEnabled", {"IDENTIFIER"}}},
    {{"OpenBMC_StatePower", "PowerRailFault"}, {"xyz.openbmc_project.State.Power.PowerRailFault", {"POWER_RAIL"}}},
    {{"OpenBMC_StatePower", "PowerRailFaultRecovered"}, {"xyz.openbmc_project.State.Power.PowerRailFaultRecovered", {"POWER_RAIL"}}},
    {{"OpenBMC_StatePower", "VoltageRegulatorFault"}, {"xyz.openbmc_project.State.Power.VoltageRegulatorFault", {"VOLTAGE_REGULATOR"}}},
    {{"OpenBMC_StatePower", "VoltageRegulatorFaultRecovered"}, {"xyz.openbmc_project.State.Power.VoltageRegulatorFaultRecovered", {"VOLTAGE_REGULATOR"}}},
    {{"OpenBMC_StateSMC", "SMCFailed"}, {"xyz.openbmc_project.State.SMC.SMCFailed", {"IDENTIFIER"}}},
    {{"OpenBMC_StateSMC", "SMCRestored"}, {"xyz.openbmc_project.State.SMC.SMCRestored", {"IDENTIFIER"}}},
    {{"OpenBMC_StateThermal", "DeviceOperatingNormalTemperature"}, {"xyz.openbmc_project.State.Thermal.DeviceOperatingNormalTemperature", {"DEVICE"}}},
    {{"OpenBMC_StateThermal", "DeviceOverOperatingTemperature"}, {"xyz.openbmc_project.State.Thermal.DeviceOverOperatingTemperature", {"DEVICE"}}},
    {{"OpenBMC_StateThermal", "DeviceOverOperatingTemperatureFault"}, {"xyz.openbmc_project.State.Thermal.DeviceOverOperatingTemperatureFault", {"DEVICE"}}},
    {{"OpenBMC_StateValve", "ValveClose"}, {"xyz.openbmc_project.State.Valve.ValveClose", {"VALVE_NAME"}}},
    {{"OpenBMC_StateValve", "ValveOpen"}, {"xyz.openbmc_project.State.Valve.ValveOpen", {"VALVE_NAME"}}}
};

std::pair<std::string_view, std::string_view> parseMessageIdFormat(std::string_view messageId)
{
    if (messageId.empty())
    {
        return {"", ""};
    }

    auto firstDot = messageId.find('.');
    std::string_view registryPrefix = (firstDot != std::string_view::npos)
                                    ? messageId.substr(0, firstDot)
                                    : messageId;

    auto lastDot = messageId.find_last_of('.');
    std::string_view messageKey = (lastDot != std::string_view::npos && (lastDot + 1) < messageId.size())
                                ? messageId.substr(lastDot + 1)
                                : messageId;

    return {registryPrefix, messageKey};
}

std::string_view parseDbusSeverity(redfish_binding::LogEntry::LogEntry& entry)
{
    auto severity = entry.getSeverity().hasValue()
                  ? entry.getSeverity().value()
                  : EventSeverity::Critical;

    switch (severity)
    {
        case EventSeverity::OK:
            return "xyz.openbmc_project.Logging.Entry.Level.Informational";
        case EventSeverity::Warning:
            return "xyz.openbmc_project.Logging.Entry.Level.Warning";
        case EventSeverity::Critical:
            return "xyz.openbmc_project.Logging.Entry.Level.Critical";
        default:
            return "xyz.openbmc_project.Logging.Entry.Level.Informational";
    }
}

template <typename JsonType>
ParsedEventData parseEventData(redfish_binding::LogEntry::LogEntry& entry,
                               const JsonType& entryJson,
                               std::string_view messageId,
                               std::string_view registryPrefix,
                               std::string_view messageKey)
{
    ParsedEventData event;

    event.severity = std::string(parseDbusSeverity(entry));
    event.additionalData.insert_or_assign("RAW", entryJson.dump());

    if (entryJson.contains("Message") && entryJson["Message"].is_string())
    {
        event.message = entryJson["Message"].template get<std::string>();
    }
    else
    {
        event.message = "com.meta.RedfishClient.AggregatedEvent";
    }

    if (!entryJson.contains("MessageArgs") || !entryJson["MessageArgs"].is_array())
    {
        return event;
    }

    auto it = redfishEventMap.find({registryPrefix, messageKey});
    if (it != redfishEventMap.end())
    {
        auto argArray = entryJson["MessageArgs"];
        auto getArg = [&](size_t index) -> std::string
        {
            if (index >= argArray.size()) return "";
            return argArray[index].is_string() ? argArray[index].template get<std::string>() : argArray[index].dump();
        };

        event.message = std::string(it->second.dbusEventName);

        const auto& argName = it->second.argName;
        for (size_t i = 0; i < argName.size() && i < argArray.size(); ++i)
        {
            event.additionalData.insert_or_assign(std::string(argName[i]), getArg(i));
        }
    }

    return event;
}

} // anonymous namespace

AggregateMapper::AggregateMapper() : bus_(sdbusplus::bus::new_default())
{
}

bool AggregateMapper::canHandle(redfish_binding::LogEntry::LogEntry& entry) const
{
    return true;
}

void AggregateMapper::map(redfish_binding::LogEntry::LogEntry& entry)
{
    auto entryJson = entry.toJson();

    std::string_view messageId;
    auto it = entryJson.find("MessageId");
    if (it != entryJson.end() && it->is_string())
    {
        messageId = it->get<std::string_view>();
    }

    auto [registryPrefix, messageKey] = parseMessageIdFormat(messageId);

    ParsedEventData event = parseEventData(entry, entryJson, messageId, registryPrefix, messageKey);

    try
    {
        auto method = bus_.new_method_call(
            "xyz.openbmc_project.Logging",
            "/xyz/openbmc_project/logging",
            "xyz.openbmc_project.Logging.Create",
            "Create"
        );
        method.append(event.message, event.severity, event.additionalData);
        bus_.call_noreply(method);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to map and commit Redfish log to D-Bus (MessageId: {MSG_ID}): {ERROR}", 
                   "MSG_ID", messageId, 
                   "ERROR", e);
    }
}

} // redfish_client::core
