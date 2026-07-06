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

auto Sensor::toMaybeMetadata(const std::string& unitsStr)
    -> std::optional<Metadata>
{
    using Unit = SensorValueIntf::Unit;
    static const std::unordered_map<std::string_view, Metadata> table = {
        {"%", {Unit::Percent, 0.0, 100.0}},
        {"Cel", {Unit::DegreesC, -128.0, 127.0}},
        {"J", {Unit::Joules, 0.0, 100000.0}},
        {"Pa", {Unit::Pascals, 30000.0, 120000.0}},
        {"V", {Unit::Volts, 0.0, 255.0}},
        {"W", {Unit::Watts, 0.0, 3000.0}},
        {"A", {Unit::Amperes, 0.0, 255.0}},
    };

    if (auto it = table.find(unitsStr); it != table.end())
    {
        return it->second;
    }
    return std::nullopt;
}

std::optional<SensorValueIntf::Unit> Sensor::toMaybeUnit(
    const std::string& unitsStr)
{
    return toMaybeMetadata(unitsStr).transform([](const Metadata& metadata) {
        return metadata.unit;
    });
}

std::unique_ptr<Sensor> Sensor::create(
    sdbusplus::async::context& ctx, const std::string& ns,
    const std::string& id, const std::string& associationPath,
    redfish_binding::Sensor::Sensor& parsed)
{
    // Without a unit there is nothing meaningful to put on the bus.
    if (!parsed.getReadingUnits().hasValue())
    {
        return nullptr;
    }
    std::string unit = parsed.getReadingUnits().value();

    // An unmappable unit is skipped rather than published under a guessed one.
    auto maybeMetadata = toMaybeMetadata(unit);
    if (!maybeMetadata.has_value())
    {
        return nullptr;
    }
    Metadata metadata = maybeMetadata.value();

    if (auto& minProp = parsed.getReadingRangeMin(); minProp.hasValue())
    {
        metadata.minValue = minProp.value();
    }
    if (auto& maxProp = parsed.getReadingRangeMax(); maxProp.hasValue())
    {
        metadata.maxValue = maxProp.value();
    }

    auto objectPath = std::string(rootPath) + "/" + ns + "/" + id;
    return std::unique_ptr<Sensor>(new Sensor(
        ctx, objectPath, associationPath,
        toMaybeDouble(parsed.getReading())
            .value_or(std::numeric_limits<double>::quiet_NaN()),
        metadata));
}

Sensor::Sensor(sdbusplus::async::context& ctx, const std::string& objectPath,
               const std::string& associationPath, double reading,
               const Metadata& metadata) :
    SensorInterfaces(ctx, objectPath.c_str())
{
    // Populate everything before announcing: emit_added() publishes the object
    // as a unit, so a half-built sensor is never visible to consumers.
    constexpr bool emitSignal = false;

    if (!associationPath.empty())
    {
        std::vector<std::tuple<std::string, std::string, std::string>>
            associations;
        associations.emplace_back("chassis", "all_sensors", associationPath);
        this->associations<emitSignal>(associations);
    }

    this->unit<emitSignal>(metadata.unit);
    this->min_value<emitSignal>(metadata.minValue);
    this->max_value<emitSignal>(metadata.maxValue);

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
    // Skip unless the reading moved by at least 1%; sub-percent jitter is not
    // worth waking every bus consumer. A reading appearing or disappearing
    // (a NaN transition) always signals.
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
        // Baseline is ~0, so a percentage change is undefined; treat as no
        // change.
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
