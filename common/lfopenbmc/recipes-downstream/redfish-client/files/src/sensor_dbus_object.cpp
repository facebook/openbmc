#include <redfish_client/core/sensor_dbus_object.hpp>

namespace redfish_client::core
{


SensorDbusObject::SensorDbusObject(sdbusplus::async::context& ctx, const char* metricPath,
                 const SensorMapper& mapper,
                 const std::string& associationPath) :
    ctx(ctx), metricPath(metricPath), mapper(mapper),
    associationPath(associationPath)
{}

// Source:
// https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Sensor/Value.interface.yaml

std::optional<ValueIntf::Unit> toMaybeIntfUnits(const std::string& unitsStr)
{
    // The strings used in this function and the mapping to items in the
    // ValueIntf::Unit enumeration were obtained experimentally and should be
    // extended as needed.

    if (unitsStr == "%")
    {
        return ValueIntf::Unit::Percent;
    }

    if (unitsStr == "Cel")
    {
        return ValueIntf::Unit::DegreesC;
    }

    if (unitsStr == "J")
    {
        return ValueIntf::Unit::Joules;
    }

    if (unitsStr == "Pa")
    {
        return ValueIntf::Unit::Pascals;
    }

    if (unitsStr == "V")
    {
        return ValueIntf::Unit::Volts;
    }

    if (unitsStr == "W")
    {
        return ValueIntf::Unit::Watts;
    }

    return std::nullopt;
}

using PathIntf = ValueIntf::namespace_path;

// Source:
// https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Sensor/Value.interface.yaml

const char* getActualMetricNamespace(const char* logicalNameParam)
{
    // This function is an identify function for the time being, but
    // having a lookup table infrastructure makes it possible for us to
    // change the values supported in the interface without worrying about
    // the values actually emitted by the redfish server.

    std::string logicalName(logicalNameParam);
    if (logicalName == "airflow")
    {
        return PathIntf::airflow;
    }
    if (logicalName == "altitude")
    {
        return PathIntf::altitude;
    }
    if (logicalName == "current")
    {
        return PathIntf::current;
    }
    if (logicalName == "energy")
    {
        return PathIntf::energy;
    }
    if (logicalName == "fan_tach")
    {
        return PathIntf::fan_tach;
    }
    if (logicalName == "humidity")
    {
        return PathIntf::humidity;
    }
    if (logicalName == "liquidflow")
    {
        return PathIntf::liquidflow;
    }
    if (logicalName == "power")
    {
        return PathIntf::power;
    }
    if (logicalName == "pressure")
    {
        return PathIntf::pressure;
    }
    if (logicalName == "temperature")
    {
        return PathIntf::temperature;
    }
    if (logicalName == "utilization")
    {
        return PathIntf::utilization;
    }
    if (logicalName == "voltage")
    {
        return PathIntf::voltage;
    }
    throw std::invalid_argument(logicalName.c_str());
}

const char* getSensorRootPath()
{
    return PathIntf::value;
}

auto SensorDbusObject::update(Sensor sensor) -> sdbusplus::async::task<>
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
    // debug("Updating metric {NAME}", "NAME", metricPath.c_str());
    std::lock_guard<std::mutex> guard(this->lock);
    bool addedThisTime = false;

    if (object == nullptr)
    {
        // Don't signal the bus while the object is getting created.
        constexpr bool emitSignal = false;
        object =
            std::make_unique<ServerObjectIntf>(ctx, metricPath.c_str());

        if (!associationPath.empty())
        {
            std::vector<std::tuple<std::string, std::string, std::string>>
                associations;
            associations.emplace_back("chassis", "all_sensors",
                                      associationPath);
            object->associations<emitSignal>(associations);
        }

        auto maybeUnit = toMaybeIntfUnits(sensor.getSensorUnitText());
        if (maybeUnit.has_value())
        {
            object->unit<emitSignal>(maybeUnit.value());
        }
        else
        {
            // Initialize it to something, otherwise we see errors on some
            // OS versions.
            object->unit<emitSignal>(ValueIntf::Unit::Amperes);
        }

        object->value<emitSignal>(std::numeric_limits<double>::quiet_NaN());
        assert(std::isnan(lastNotifiedValue));
        addedThisTime = true;
    }

    if (!minValueNotified)
    {
        auto minValue = sensor.getMinValue();
        if (minValue.has_value())
        {
            object->min_value<false>(minValue.value());
            minValueNotified = true;
        }
    }
    if (!maxValueNotified)
    {
        auto maxValue = sensor.getMaxValue();
        if (maxValue.has_value())
        {
            object->max_value<false>(maxValue.value());
            maxValueNotified = true;
        }
    }

    double current = sensor.getReading();

    const bool emitSignal = !shouldSkipSignal(current);
    if (emitSignal)
    {
        lastNotifiedValue = current;
    }
    if (emitSignal && !addedThisTime)
    {
        object->value<true>(current);
    }
    else
    {
        object->value<false>(current);
    }
    if (addedThisTime)
    {
        object->emit_added();
    }
    co_return;
}

auto SensorDbusObject::shouldSkipSignal(double current) -> bool
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
