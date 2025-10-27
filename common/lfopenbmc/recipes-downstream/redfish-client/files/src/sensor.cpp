#include <redfish_client/core/sensor.hpp>

#include <redfish-binding/Sensor_Sensor.hpp>

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

std::optional<Sensor> Sensor::parseSensor(const std::string& sensorJson)
{
    auto parsed = redfish_binding::Sensor::parseSensor(sensorJson);

    auto maybeReading = toMaybeDouble(parsed.getReading());
    if (!maybeReading.has_value())
    {
        maybeReading = std::numeric_limits<double>::quiet_NaN();
    }

    if (!parsed.getReadingUnits().hasValue())
    {
        return std::nullopt;
    }

    Sensor sensor;
    sensor.reading = maybeReading.value();
    sensor.sensorUnit = parsed.getReadingUnits().value();
    sensor.minValue = toMaybeDouble(parsed.getReadingRangeMin());
    sensor.maxValue = toMaybeDouble(parsed.getReadingRangeMax());
    return sensor;
}

double Sensor::getReading() const
{
    return reading;
}

const std::string& Sensor::getSensorUnitText() const
{
    return sensorUnit;
}

std::optional<double> Sensor::getMinValue() const
{
    return minValue;
}

std::optional<double> Sensor::getMaxValue() const
{
    return maxValue;
}

Sensor Sensor::createTestSensor(double reading, const char* sensorUnit,
                               std::optional<double> minValue,
                               std::optional<double> maxValue)
{
    Sensor rv;
    rv.reading = reading;
    rv.sensorUnit = sensorUnit;
    rv.minValue = minValue;
    rv.maxValue = maxValue;
    return rv;
}

} // namespace redfish_client::core
