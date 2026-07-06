#include <redfish_client/core/sensor_threshold_mapper.hpp>
#include <redfish_client/core/log_entry_mapper_utils.hpp>
#include <phosphor-logging/lg2.hpp>
#include <phosphor-logging/commit.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/event.hpp>
#include <redfish_client/core/sensor.hpp>

#include <optional>
#include <string>

PHOSPHOR_LOG2_USING;

namespace redfish_client::core
{

namespace
{

using ValueUnit = SensorValueIntf::Unit;
namespace ThresholdError =
    sdbusplus::error::xyz::openbmc_project::sensor::Threshold;
namespace ThresholdEvent =
    sdbusplus::event::xyz::openbmc_project::sensor::Threshold;

constexpr std::string_view kSensorEventPrefix = "SensorEvent.";

std::string_view getSuffix(const std::string& messageId)
{
    std::string_view id = messageId;
    auto pos = id.find_last_of('.');
    if (pos == std::string_view::npos)
    {
        return id;
    }
    return id.substr(pos + 1);
}

std::string_view unitToNamespace(ValueUnit unit)
{
    switch (unit)
    {
        case ValueUnit::DegreesC:  return "temperature";
        case ValueUnit::Amperes:   return "current";
        case ValueUnit::Volts:     return "voltage";
        case ValueUnit::Watts:     return "power";
        case ValueUnit::Joules:    return "energy";
        case ValueUnit::Pascals:   return "pressure";
        case ValueUnit::Percent:   return "utilization";
        case ValueUnit::PercentRH: return "humidity";
        case ValueUnit::RPMS:      return "fan_tach";
        case ValueUnit::LPM:       return "liquidflow";
        case ValueUnit::Meters:    return "altitude";
        case ValueUnit::CFM:       return "airflow";
        default:                   return "temperature";
    }
}

struct ThresholdArgs
{
    std::string sensorName;
    double readingValue{0.0};
    ValueUnit units{ValueUnit::DegreesC};
    double thresholdValue{0.0};
};

// Build the full OpenBMC sensor path from MessageArgs:
//   [0] SensorName (short, e.g. "HGX_GPU0_Temp")
//   [1] ReadingValue
//   [2] Units (e.g. "Cel")
//   [3] ThresholdValue
//
// Result: /xyz/openbmc_project/sensors/<namespace>/<SensorName>
std::string extractSensorName(redfish_binding::LogEntry::LogEntry& entry,
                              const std::vector<SensorMapper>& mappers)
{
    auto& maybeArgs = entry.getMessageArgs();
    if (maybeArgs.hasValue() && maybeArgs.value().size() >= 3)
    {
        const auto& msgArgs = maybeArgs.value();
        std::string sensorId = msgArgs[0];

        // Try to map the sensor ID using OriginOfCondition or fallback to suffix match
        std::string origin = extractOrigin(entry);
        bool found = false;
        for (const auto& mapper : mappers)
        {
            if (mapper.fromUrl == origin)
            {
                sensorId = mapper.toId;
                found = true;
                break;
            }
        }

        if (!found)
        {
            std::string sensorIdWithSlash = "/" + msgArgs[0];
            for (const auto& mapper : mappers)
            {
                if (mapper.fromUrl.ends_with(sensorIdWithSlash))
                {
                    sensorId = mapper.toId;
                    break;
                }
            }
        }

        auto maybeUnit = Sensor::toMaybeUnit(msgArgs[2]);
        auto unit = maybeUnit.value_or(ValueUnit::DegreesC);
        // unitToNamespace already yields a supported namespace segment.
        auto ns = unitToNamespace(unit);

        return std::string(sensorRootPath) + "/" + std::string(ns) + "/" +
               sensorId;
    }
    return "Unknown Sensor";
}

// SensorEvent 1.0.0 MessageArgs layout:
//   [0] SensorName  [1] ReadingValue  [2] Units  [3] ThresholdValue
ThresholdArgs extractArgs(redfish_binding::LogEntry::LogEntry& entry,
                          const std::vector<SensorMapper>& mappers)
{
    ThresholdArgs args;
    args.sensorName = extractSensorName(entry, mappers);

    auto& maybeArgs = entry.getMessageArgs();
    if (!maybeArgs.hasValue())
    {
        return args;
    }
    const auto& msgArgs = maybeArgs.value();

    if (msgArgs.size() > 1)
    {
        try
        {
            args.readingValue = std::stod(msgArgs[1]);
        }
        catch (...)
        {}
    }
    if (msgArgs.size() > 2)
    {
        args.units = Sensor::toMaybeUnit(msgArgs[2]).value_or(ValueUnit::DegreesC);
    }
    if (msgArgs.size() > 3)
    {
        try
        {
            args.thresholdValue = std::stod(msgArgs[3]);
        }
        catch (...)
        {}
    }
    return args;
}

template <typename T>
T makeThresholdError(const ThresholdArgs& a)
{
    return T("SENSOR_NAME", sdbusplus::object_path(a.sensorName),
             "READING_VALUE", a.readingValue,
             "UNITS", a.units,
             "THRESHOLD_VALUE", a.thresholdValue);
}

template <typename T>
T makeThresholdNormal(const ThresholdArgs& a)
{
    return T("SENSOR_NAME", sdbusplus::object_path(a.sensorName),
             "READING_VALUE", a.readingValue,
             "UNITS", a.units);
}

} // anonymous namespace

bool SensorThresholdMapper::canHandle(
    redfish_binding::LogEntry::LogEntry& entry) const
{
    auto& maybeMessageId = entry.getMessageId();
    if (!maybeMessageId.hasValue())
    {
        return false;
    }
    const auto& msgId = maybeMessageId.value();
    if (!msgId.starts_with(kSensorEventPrefix))
    {
        return false;
    }
    auto suffix = getSuffix(msgId);
    return suffix == "ReadingAboveUpperFatalThreshold"    ||
           suffix == "ReadingAboveUpperCriticalThreshold" ||
           suffix == "ReadingAboveUpperCautionThreshold"  ||
           suffix == "ReadingBelowLowerFatalThreshold"    ||
           suffix == "ReadingBelowLowerCriticalThreshold" ||
           suffix == "ReadingBelowLowerCautionThreshold"  ||
           suffix == "SensorReadingNormalRange";
}

void SensorThresholdMapper::map(redfish_binding::LogEntry::LogEntry& entry)
{
    auto args = extractArgs(entry, mappers);
    auto suffix = getSuffix(entry.getMessageId().value());

    if (suffix == "ReadingAboveUpperFatalThreshold")
    {
        lg2::commit(makeThresholdError<ThresholdError::ReadingAboveUpperHardShutdownThreshold>(args));
    }
    else if (suffix == "ReadingAboveUpperCriticalThreshold")
    {
        lg2::commit(makeThresholdError<ThresholdError::ReadingAboveUpperCriticalThreshold>(args));
    }
    else if (suffix == "ReadingAboveUpperCautionThreshold")
    {
        lg2::commit(makeThresholdError<ThresholdError::ReadingAboveUpperWarningThreshold>(args));
    }
    else if (suffix == "ReadingBelowLowerFatalThreshold")
    {
        lg2::commit(makeThresholdError<ThresholdError::ReadingBelowLowerHardShutdownThreshold>(args));
    }
    else if (suffix == "ReadingBelowLowerCriticalThreshold")
    {
        lg2::commit(makeThresholdError<ThresholdError::ReadingBelowLowerCriticalThreshold>(args));
    }
    else if (suffix == "ReadingBelowLowerCautionThreshold")
    {
        lg2::commit(makeThresholdError<ThresholdError::ReadingBelowLowerWarningThreshold>(args));
    }
    else if (suffix == "SensorReadingNormalRange")
    {
        lg2::commit(makeThresholdNormal<ThresholdEvent::SensorReadingNormalRange>(args));
    }
    else
    {
        warning("SensorThresholdMapper::map: unhandled suffix {SUFFIX}",
                "SUFFIX", std::string(suffix));
    }
}

} // namespace redfish_client::core