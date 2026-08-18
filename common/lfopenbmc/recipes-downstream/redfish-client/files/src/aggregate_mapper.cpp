#include "redfish-binding/LogEntry_EventSeverity.hpp"

#include <phosphor-logging/commit.hpp>
#include <phosphor-logging/lg2.hpp>
#include <redfish_client/core/aggregate_mapper.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/Logging/event.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/event.hpp>
#include <xyz/openbmc_project/Sensor/Value/common.hpp>
#include <xyz/openbmc_project/Sensor/event.hpp>
#include <xyz/openbmc_project/Software/Update/event.hpp>
#include <xyz/openbmc_project/State/BMC/event.hpp>
#include <xyz/openbmc_project/State/CPER/event.hpp>
#include <xyz/openbmc_project/State/Cable/event.hpp>
#include <xyz/openbmc_project/State/Fan/event.hpp>
#include <xyz/openbmc_project/State/Filter/event.hpp>
#include <xyz/openbmc_project/State/Leak/Detector/event.hpp>
#include <xyz/openbmc_project/State/Leak/DetectorGroup/event.hpp>
#include <xyz/openbmc_project/State/LockOut/event.hpp>
#include <xyz/openbmc_project/State/Power/event.hpp>
#include <xyz/openbmc_project/State/Pump/event.hpp>
#include <xyz/openbmc_project/State/SMC/event.hpp>
#include <xyz/openbmc_project/State/Thermal/event.hpp>
#include <xyz/openbmc_project/State/Valve/event.hpp>

#include <charconv>
#include <cstdint>
#include <map>
#include <memory>
#include <source_location>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace redfish_client::core
{

namespace
{

using EventSeverity = redfish_binding::LogEntry::EventSeverity;
using MsgArgs = std::vector<std::string>;
using ValueUnit = sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit;
using FieldNames = std::vector<std::string_view>;
using EventPtr = std::unique_ptr<sdbusplus::exception::generated_event_base>;
using MakeFn = EventPtr (*)(const MsgArgs&, const FieldNames&);

struct EventSpec
{
    FieldNames fields;
    MakeFn make;
};

namespace LoggingEvt = sdbusplus::event::xyz::openbmc_project::Logging;
namespace SensorErr = sdbusplus::error::xyz::openbmc_project::Sensor;
namespace SensorEvt = sdbusplus::event::xyz::openbmc_project::Sensor;
namespace ThreshErr = sdbusplus::error::xyz::openbmc_project::sensor::Threshold;
namespace ThreshEvt = sdbusplus::event::xyz::openbmc_project::sensor::Threshold;
namespace BmcEvt = sdbusplus::event::xyz::openbmc_project::state::BMC;
namespace CperErr = sdbusplus::error::xyz::openbmc_project::state::CPER;
namespace CableErr = sdbusplus::error::xyz::openbmc_project::state::Cable;
namespace CableEvt = sdbusplus::event::xyz::openbmc_project::state::Cable;
namespace FanErr = sdbusplus::error::xyz::openbmc_project::state::Fan;
namespace FanEvt = sdbusplus::event::xyz::openbmc_project::state::Fan;
namespace FilterErr = sdbusplus::error::xyz::openbmc_project::state::Filter;
namespace FilterEvt = sdbusplus::event::xyz::openbmc_project::state::Filter;
namespace LeakDetErr =
    sdbusplus::error::xyz::openbmc_project::state::leak::Detector;
namespace LeakDetEvt =
    sdbusplus::event::xyz::openbmc_project::state::leak::Detector;
namespace LeakGrpErr =
    sdbusplus::error::xyz::openbmc_project::state::leak::DetectorGroup;
namespace LeakGrpEvt =
    sdbusplus::event::xyz::openbmc_project::state::leak::DetectorGroup;
namespace LockOutEvt = sdbusplus::event::xyz::openbmc_project::state::LockOut;
namespace PowerErr = sdbusplus::error::xyz::openbmc_project::state::Power;
namespace PowerEvt = sdbusplus::event::xyz::openbmc_project::state::Power;
namespace PumpErr = sdbusplus::error::xyz::openbmc_project::state::Pump;
namespace PumpEvt = sdbusplus::event::xyz::openbmc_project::state::Pump;
namespace SmcErr = sdbusplus::error::xyz::openbmc_project::state::SMC;
namespace SmcEvt = sdbusplus::event::xyz::openbmc_project::state::SMC;
namespace ThermalErr = sdbusplus::error::xyz::openbmc_project::state::Thermal;
namespace ThermalEvt = sdbusplus::event::xyz::openbmc_project::state::Thermal;
namespace ValveEvt = sdbusplus::event::xyz::openbmc_project::state::Valve;
namespace UpdateErr = sdbusplus::error::xyz::openbmc_project::software::Update;
namespace UpdateEvt = sdbusplus::event::xyz::openbmc_project::software::Update;
using CommonBmc = sdbusplus::common::xyz::openbmc_project::state::BMC;
using CommonHost = sdbusplus::common::xyz::openbmc_project::state::Host;

std::string argStr(const MsgArgs& args, size_t i)
{
    return i < args.size() ? args[i] : std::string{};
}

sdbusplus::object_path argPath(const MsgArgs& args, size_t i)
{
    return sdbusplus::object_path(argStr(args, i));
}

double argNum(const MsgArgs& args, size_t i)
{
    double value = 0.0;
    std::string text = argStr(args, i);
    std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}

ValueUnit argUnit(const MsgArgs& args, size_t i)
{
    static const std::unordered_map<std::string_view, ValueUnit> units = {
        {"Amperes", ValueUnit::Amperes},
        {"AmpereHours", ValueUnit::AmpereHours},
        {"CFM", ValueUnit::CFM},
        {"DegreesC", ValueUnit::DegreesC},
        {"Hertz", ValueUnit::Hertz},
        {"Joules", ValueUnit::Joules},
        {"LPM", ValueUnit::LPM},
        {"Meters", ValueUnit::Meters},
        {"Pascals", ValueUnit::Pascals},
        {"Percent", ValueUnit::Percent},
        {"PercentRH", ValueUnit::PercentRH},
        {"Radians", ValueUnit::Radians},
        {"RPMS", ValueUnit::RPMS},
        {"Volts", ValueUnit::Volts},
        {"Watts", ValueUnit::Watts},
    };

    std::string unit = argStr(args, i);
    if (auto it = units.find(unit); it != units.end())
    {
        return it->second;
    }
    return ValueUnit::DegreesC;
}

int parseSyslogSeverity(redfish_binding::LogEntry::LogEntry& entry)
{
    auto severity = entry.getSeverity().hasValue() ? entry.getSeverity().value()
                                                   : EventSeverity::Critical;

    switch (severity)
    {
        case EventSeverity::OK:
            return LOG_INFO;
        case EventSeverity::Warning:
            return LOG_WARNING;
        case EventSeverity::Critical:
            return LOG_CRIT;
        default:
            return LOG_INFO;
    }
}

std::pair<std::string_view, std::string_view> parseMessageIdFormat(
    std::string_view messageId)
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
    std::string_view messageKey =
        (lastDot != std::string_view::npos && (lastDot + 1) < messageId.size())
            ? messageId.substr(lastDot + 1)
            : messageId;

    return {registryPrefix, messageKey};
}

class AggregatedEvent : public sdbusplus::exception::generated_event_base
{
  public:
    AggregatedEvent(int severity, std::string message,
                    nlohmann::json eventData) :
        eventSeverity(severity),
        eventMessage(message.empty() ? kEventName : std::move(message)),
        eventData(std::move(eventData))
    {}

    const char* name() const noexcept override
    {
        return eventMessage.c_str();
    }

    const char* description() const noexcept override
    {
        return "Aggregated event from Satellite BMC";
    }

    int get_errno() const noexcept override
    {
        return EIO;
    }

    int severity() const noexcept override
    {
        return eventSeverity;
    }

    nlohmann::json to_json() const override
    {
        nlohmann::json props = {};
        props["AMC_REDFISH_EVENT"] = eventData.dump();
        return nlohmann::json{{eventMessage, std::move(props)}};
    }

  private:
    int eventSeverity;
    std::string eventMessage;
    nlohmann::json eventData;
    static constexpr const char* kEventName =
        "com.meta.RedfishClient.AggregatedEvent";
};

template <typename E>
concept ThresholdReading = requires(E e) {
    e.readingValue;
    e.units;
};
template <typename E>
concept ThresholdReadingWithLimit =
    ThresholdReading<E> && requires(E e) { e.thresholdValue; };

template <typename Event>
EventPtr makeEvent(const MsgArgs& args, const FieldNames& fields)
{
    if constexpr (std::is_same_v<Event, LoggingEvt::Cleared>)
    {
        uint64_t count = 0;
        std::string text = argStr(args, 0);
        std::from_chars(text.data(), text.data() + text.size(), count);
        return EventPtr(new Event("NUMBER_OF_LOGS", count));
    }
    else if constexpr (std::is_same_v<Event, UpdateEvt::ResetRequired>)
    {
        auto transition = sdbusplus::message::convert_from_string<
            CommonHost::Transition>(argStr(args, 1));
        if (!transition)
        {
            return nullptr;
        }
        return EventPtr(new Event(
            "TARGET_NAME", argPath(args, 0), "RESET_TYPE", *transition));
    }
    else if constexpr (std::is_same_v<Event, BmcEvt::RebootCause>)
    {
        auto cause = sdbusplus::message::convert_from_string<
            CommonBmc::RebootCause>(argStr(args, 0));
        if (!cause)
        {
            return nullptr;
        }
        return EventPtr(new Event(
            "CAUSE", *cause, "BOOT_DEVICE", argStr(args, 1)));
    }
    else if constexpr (std::is_same_v<Event, BmcEvt::StateChanged>)
    {
        auto state = sdbusplus::message::convert_from_string<
            CommonBmc::BMCState>(argStr(args, 0));
        if (!state)
        {
            return nullptr;
        }
        return EventPtr(new Event("STATE", *state));
    }
    else if constexpr (ThresholdReadingWithLimit<Event>)
    {
        return EventPtr(new Event(
            "SENSOR_NAME", argPath(args, 0), "READING_VALUE", argNum(args, 1),
            "UNITS", argUnit(args, 2), "THRESHOLD_VALUE", argNum(args, 3)));
    }
    else if constexpr (ThresholdReading<Event>)
    {
        return EventPtr(new Event(
            "SENSOR_NAME", argPath(args, 0), "READING_VALUE", argNum(args, 1),
            "UNITS", argUnit(args, 2)));
    }
    else
    {
        nlohmann::json body;
        for (size_t i = 0; i < fields.size(); ++i)
        {
            body[std::string(fields[i])] = argStr(args, i);
        }
        return std::make_unique<Event>(
            nlohmann::json{{Event::errName, std::move(body)}},
            std::source_location::current());
    }
}

const std::map<std::pair<std::string_view, std::string_view>, EventSpec>&
    eventHandlers()
{
    // clang-format off
    static const std::map<std::pair<std::string_view, std::string_view>, EventSpec>
        handlers = {
            {{"OpenBMC_Logging", "Cleared"}, {{}, &makeEvent<LoggingEvt::Cleared>}},
            {{"Update", "VerificationFailed"}, {{"IMAGE_IDENTIFIER", "TARGET_NAME"}, &makeEvent<UpdateErr::VerificationFailed>}},
            {{"Update", "TargetDetermined"}, {{"TARGET_NAME", "IMAGE_IDENTIFIER"}, &makeEvent<UpdateEvt::TargetDetermined>}},
            {{"Update", "UpdateSuccessful"}, {{"TARGET_NAME", "IMAGE_IDENTIFIER"}, &makeEvent<UpdateEvt::UpdateSuccessful>}},
            {{"Base", "ResetRequired"}, {{}, &makeEvent<UpdateEvt::ResetRequired>}},
            {{"SensorEvent", "InvalidSensorReading"}, {{"SENSOR_NAME"}, &makeEvent<SensorErr::InvalidSensorReading>}},
            {{"SensorEvent", "SensorFailure"}, {{"SENSOR_NAME"}, &makeEvent<SensorErr::SensorFailure>}},
            {{"SensorEvent", "SensorRestored"}, {{"SENSOR_NAME"}, &makeEvent<SensorEvt::SensorRestored>}},
            {{"SensorEvent", "ReadingAboveLowerCriticalThreshold"}, {{}, &makeEvent<ThreshErr::ReadingAboveLowerCriticalThreshold>}},
            {{"SensorEvent", "ReadingAboveLowerFatalThreshold"}, {{}, &makeEvent<ThreshErr::ReadingAboveLowerHardShutdownThreshold>}},
            {{"SensorEvent", "ReadingAboveLowerCautionThreshold"}, {{}, &makeEvent<ThreshErr::ReadingBelowLowerWarningThreshold>}},
            {{"SensorEvent", "ReadingAboveUpperCriticalThreshold"}, {{}, &makeEvent<ThreshErr::ReadingAboveUpperCriticalThreshold>}},
            {{"SensorEvent", "ReadingAboveUpperFatalThreshold"}, {{}, &makeEvent<ThreshErr::ReadingAboveUpperHardShutdownThreshold>}},
            {{"SensorEvent", "ReadingAboveUpperCautionThreshold"}, {{}, &makeEvent<ThreshErr::ReadingAboveUpperWarningThreshold>}},
            {{"SensorEvent", "ReadingBelowLowerCriticalThreshold"}, {{}, &makeEvent<ThreshErr::ReadingBelowLowerCriticalThreshold>}},
            {{"SensorEvent", "ReadingBelowLowerFatalThreshold"}, {{}, &makeEvent<ThreshErr::ReadingBelowLowerHardShutdownThreshold>}},
            {{"SensorEvent", "ReadingBelowLowerCautionThreshold"}, {{}, &makeEvent<ThreshErr::ReadingBelowLowerWarningThreshold>}},
            {{"SensorEvent", "ReadingBelowUpperCriticalThreshold"}, {{}, &makeEvent<ThreshErr::ReadingBelowUpperCriticalThreshold>}},
            {{"SensorEvent", "ReadingBelowFatalThreshold"}, {{}, &makeEvent<ThreshErr::ReadingBelowUpperHardShutdownThreshold>}},
            {{"SensorEvent", "ReadingCritical"}, {{}, &makeEvent<ThreshErr::ReadingCritical>}},
            {{"SensorEvent", "ReadingNoLongerCritical"}, {{}, &makeEvent<ThreshErr::ReadingNoLongerCritical>}},
            {{"SensorEvent", "ReadingWarning"}, {{}, &makeEvent<ThreshErr::ReadingWarning>}},
            {{"SensorEvent", "SensorReadingNormalRange"}, {{}, &makeEvent<ThreshEvt::SensorReadingNormalRange>}},
            {{"OpenBMC_StateBMC", "RebootCause"}, {{}, &makeEvent<BmcEvt::RebootCause>}},
            {{"OpenBMC_StateBMC", "StateChanged"}, {{}, &makeEvent<BmcEvt::StateChanged>}},
            {{"OpenBMC_StateCPER", "GenericCPERFault"}, {{"SOURCE", "CPER"}, &makeEvent<CperErr::GenericCPERFault>}},
            {{"OpenBMC_StateCPER", "GenericCPERWarning"}, {{"SOURCE", "CPER"}, &makeEvent<CperErr::GenericCPERWarning>}},
            {{"OpenBMC_StateCable", "CableConnected"}, {{"PORT_ID"}, &makeEvent<CableEvt::CableConnected>}},
            {{"OpenBMC_StateCable", "CableDisconnected"}, {{"PORT_ID"}, &makeEvent<CableErr::CableDisconnected>}},
            {{"Environmental", "FanFailed"}, {{"FAN_NAME"}, &makeEvent<FanErr::FanFailed>}},
            {{"Environmental", "FanRestored"}, {{"FAN_NAME"}, &makeEvent<FanEvt::FanRestored>}},
            {{"Environmental", "FilterRequiresService"}, {{"FILTER_NAME"}, &makeEvent<FilterErr::FilterRequiresService>}},
            {{"Environmental", "FilterRestored"}, {{"FILTER_NAME"}, &makeEvent<FilterEvt::FilterRestored>}},
            {{"Environmental", "LeakDetectedCritical"}, {{"DETECTOR_NAME"}, &makeEvent<LeakDetErr::LeakDetectedCritical>}},
            {{"Environmental", "LeakDetectedNormal"}, {{"DETECTOR_NAME"}, &makeEvent<LeakDetEvt::LeakDetectedNormal>}},
            {{"Environmental", "LeakDetectedWarning"}, {{"DETECTOR_NAME"}, &makeEvent<LeakDetErr::LeakDetectedWarning>}},
            {{"Environmental", "PumpFailed"}, {{"PUMP_NAME"}, &makeEvent<PumpErr::PumpFailed>}},
            {{"Environmental", "PumpRestored"}, {{"PUMP_NAME"}, &makeEvent<PumpEvt::PumpRestored>}},
            {{"OpenBMC_StateLeakDetectorGroup", "DetectorGroupCritical"}, {{"DETECTOR_GROUP_NAME"}, &makeEvent<LeakGrpErr::DetectorGroupCritical>}},
            {{"OpenBMC_StateLeakDetectorGroup", "DetectorGroupNormal"}, {{"DETECTOR_GROUP_NAME"}, &makeEvent<LeakGrpEvt::DetectorGroupNormal>}},
            {{"OpenBMC_StateLeakDetectorGroup", "DetectorGroupWarning"}, {{"DETECTOR_GROUP_NAME"}, &makeEvent<LeakGrpErr::DetectorGroupWarning>}},
            {{"OpenBMC_StateLockOut", "LockOutDisabled"}, {{"IDENTIFIER"}, &makeEvent<LockOutEvt::LockOutDisabled>}},
            {{"OpenBMC_StateLockOut", "LockOutEnabled"}, {{"IDENTIFIER"}, &makeEvent<LockOutEvt::LockOutEnabled>}},
            {{"OpenBMC_StatePower", "PowerRailFault"}, {{"POWER_RAIL", "FAILURE_DATA"}, &makeEvent<PowerErr::PowerRailFault>}},
            {{"OpenBMC_StatePower", "PowerRailFaultRecovered"}, {{"POWER_RAIL"}, &makeEvent<PowerEvt::PowerRailFaultRecovered>}},
            {{"OpenBMC_StatePower", "VoltageRegulatorFault"}, {{"VOLTAGE_REGULATOR", "FAILURE_DATA"}, &makeEvent<PowerErr::VoltageRegulatorFault>}},
            {{"OpenBMC_StatePower", "VoltageRegulatorFaultRecovered"}, {{"VOLTAGE_REGULATOR"}, &makeEvent<PowerEvt::VoltageRegulatorFaultRecovered>}},
            {{"OpenBMC_StateSMC", "SMCFailed"}, {{"IDENTIFIER", "FAILURE_TYPE"}, &makeEvent<SmcErr::SMCFailed>}},
            {{"OpenBMC_StateSMC", "SMCRestored"}, {{"IDENTIFIER"}, &makeEvent<SmcEvt::SMCRestored>}},
            {{"OpenBMC_StateThermal", "DeviceOperatingNormalTemperature"}, {{"DEVICE"}, &makeEvent<ThermalEvt::DeviceOperatingNormalTemperature>}},
            {{"OpenBMC_StateThermal", "DeviceOverOperatingTemperature"}, {{"DEVICE", "FAILURE_DATA"}, &makeEvent<ThermalErr::DeviceOverOperatingTemperature>}},
            {{"OpenBMC_StateThermal", "DeviceOverOperatingTemperatureFault"}, {{"DEVICE", "FAILURE_DATA"}, &makeEvent<ThermalErr::DeviceOverOperatingTemperatureFault>}},
            {{"OpenBMC_StateValve", "ValveClose"}, {{"VALVE_NAME"}, &makeEvent<ValveEvt::ValveClose>}},
            {{"OpenBMC_StateValve", "ValveOpen"}, {{"VALVE_NAME"}, &makeEvent<ValveEvt::ValveOpen>}},
        };
    // clang-format on
    return handlers;
}

} // anonymous namespace

AggregateMapper::AggregateMapper() = default;

bool AggregateMapper::canHandle(
    redfish_binding::LogEntry::LogEntry& entry) const
{
    return true;
}

void AggregateMapper::map(redfish_binding::LogEntry::LogEntry& entry)
{
    auto entryJson = entry.toJson();

    std::string_view messageId;
    if (auto idIt = entryJson.find("MessageId");
        idIt != entryJson.end() && idIt->is_string())
    {
        messageId = idIt->get<std::string_view>();
    }

    auto [registryPrefix, messageKey] = parseMessageIdFormat(messageId);

    static const MsgArgs kNoArgs;
    auto& maybeArgs = entry.getMessageArgs();
    const MsgArgs& messageArgs =
        maybeArgs.hasValue() ? maybeArgs.value() : kNoArgs;

    lg2::AdditionalData_t additionalData{{"AMC_REDFISH_EVENT", entryJson.dump()}};

    if (auto it = eventHandlers().find({registryPrefix, messageKey});
        it != eventHandlers().end())
    {
        if (auto event = it->second.make(messageArgs, it->second.fields))
        {
            lg2::commit(std::move(*event), std::nullopt, additionalData);
            return;
        }
    }

    std::string message;
    if (auto msgIt = entryJson.find("Message");
        msgIt != entryJson.end() && msgIt->is_string())
    {
        message = msgIt->get<std::string>();
    }

    lg2::commit(
        AggregatedEvent(parseSyslogSeverity(entry), message, entryJson));
}

} // namespace redfish_client::core
