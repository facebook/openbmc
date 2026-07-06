#include <redfish_client/core/sensor.hpp>

#include <cassert>
#include <cmath>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace redfish_client::core
{

// Source:
// https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Sensor/Value.interface.yaml

std::optional<SensorValueIntf::Unit> Sensor::toMaybeUnit(
    const std::string& unitsStr)
{
    // The strings used here and the mapping to SensorValueIntf::Unit were
    // obtained experimentally and should be extended as needed.
    using Unit = SensorValueIntf::Unit;
    static const std::unordered_map<std::string_view, Unit> units = {
        {"%", Unit::Percent}, {"Cel", Unit::DegreesC}, {"J", Unit::Joules},
        {"Pa", Unit::Pascals}, {"V", Unit::Volts},     {"W", Unit::Watts},
        {"A", Unit::Amperes},
    };

    if (auto it = units.find(unitsStr); it != units.end())
    {
        return it->second;
    }
    return std::nullopt;
}

namespace
{

std::optional<double> toMaybeDouble(redfish_binding::Property<double>& property)
{
    if (property.hasValue())
    {
        return property.value();
    }
    return std::nullopt;
}

} // namespace

std::unique_ptr<Sensor> Sensor::create(
    sdbusplus::async::context& ctx, const std::string& objectPath,
    const std::string& associationPath,
    redfish_binding::Sensor::Sensor& parsed)
{
    // A sensor without a unit cannot be meaningfully represented on the bus, so
    // it is skipped entirely (no D-Bus object is ever created).
    if (!parsed.getReadingUnits().hasValue())
    {
        return nullptr;
    }
    std::string unit = parsed.getReadingUnits().value();

    double reading = toMaybeDouble(parsed.getReading())
                         .value_or(std::numeric_limits<double>::quiet_NaN());

    static const std::unordered_map<std::string, std::pair<double, double>>
        unitDefaults = {{"V", {0.0, 255.0}},      {"A", {0.0, 255.0}},
                        {"W", {0.0, 3000.0}},     {"J", {0.0, 100000.0}},
                        {"Cel", {-128.0, 127.0}}, {"Pa", {30000.0, 120000.0}},
                        {"%", {0.0, 100.0}}};

    auto maybeMin = toMaybeDouble(parsed.getReadingRangeMin());
    auto maybeMax = toMaybeDouble(parsed.getReadingRangeMax());

    std::optional<double> minValue;
    std::optional<double> maxValue;
    if (auto it = unitDefaults.find(unit); it != unitDefaults.end())
    {
        minValue = maybeMin.value_or(it->second.first);
        maxValue = maybeMax.value_or(it->second.second);
    }
    else
    {
        minValue = maybeMin;
        maxValue = maybeMax;
    }

    return std::unique_ptr<Sensor>(new Sensor(
        ctx, objectPath, associationPath, reading, unit, minValue, maxValue));
}

Sensor::Sensor(sdbusplus::async::context& ctx, const std::string& objectPath,
               const std::string& associationPath, double reading,
               const std::string& unit, std::optional<double> minValue,
               std::optional<double> maxValue) :
    SensorInterfaces(ctx, objectPath.c_str())
{
    // Populate every property without signalling the bus; the object is
    // announced atomically with emit_added() below, so consumers only ever see
    // a fully-initialized sensor.
    constexpr bool emitSignal = false;

    if (!associationPath.empty())
    {
        std::vector<std::tuple<std::string, std::string, std::string>>
            associations;
        associations.emplace_back("chassis", "all_sensors", associationPath);
        this->associations<emitSignal>(associations);
    }

    auto maybeUnit = toMaybeUnit(unit);
    // Initialize the unit to something even when unrecognized, otherwise we see
    // errors on some OS versions.
    this->unit<emitSignal>(maybeUnit.value_or(SensorValueIntf::Unit::Amperes));

    if (minValue.has_value())
    {
        this->min_value<emitSignal>(minValue.value());
    }
    if (maxValue.has_value())
    {
        this->max_value<emitSignal>(maxValue.value());
    }

    this->value<emitSignal>(reading);
    if (!std::isnan(reading))
    {
        lastNotifiedValue = reading;
    }

    Definitions::emit_added();
    Value::emit_added();
}

auto Sensor::updateValue(double current) -> sdbusplus::async::task<>
{
    // Uncomment this to debug. If we leave this uncommented in the code
    // then phosphor logging always prints these in TTY mode and that makes
    // manual testing a bit difficult.
    //
    // Note:
    // https://github.com/openbmc/phosphor-logging/blob/master/docs/structured-logging.md
    // "The lg2 APIs detect if the application is running on a TTY and
    // additionally log to the TTY."
    //
    // debug("Updating metric {VALUE}", "VALUE", current);
    std::lock_guard<std::mutex> guard(this->lock);

    if (shouldSkipSignal(current))
    {
        this->value<false>(current);
    }
    else
    {
        lastNotifiedValue = current;
        this->value<true>(current);
    }
    co_return;
}

auto Sensor::shouldSkipSignal(double current) -> bool
{
    if (std::isnan(lastNotifiedValue) && std::isnan(current))
    {
        return true;
    }
    if (std::isnan(lastNotifiedValue) && !(std::isnan(current)))
    {
        return false;
    }
    if ((!std::isnan(lastNotifiedValue)) && std::isnan(current))
    {
        return false;
    }
    assert(!std::isnan(lastNotifiedValue));
    assert(!std::isnan(current));
    if (std::abs(lastNotifiedValue) < 1e-6)
    {
        // avoid division by zero
        return true;
    }
    auto changed =
        std::abs((current - lastNotifiedValue) / lastNotifiedValue * 100.0);
    if (changed >= 1.0)
    {
        return false;
    }
    return true;
}

} // namespace redfish_client::core
