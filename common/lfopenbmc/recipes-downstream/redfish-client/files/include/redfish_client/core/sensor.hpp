#pragma once

#include <redfish_client/core/config.hpp>

#include <redfish-binding/Sensor_Sensor.hpp>

#include <sdbusplus/async.hpp>
#include <xyz/openbmc_project/Association/Definitions/aserver.hpp>
#include <xyz/openbmc_project/Sensor/Value/aserver.hpp>
#include <xyz/openbmc_project/Sensor/Value/common.hpp>

#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace redfish_client::core
{

// The D-Bus sensor Value interface: the target of the Redfish -> D-Bus
// translation performed by Sensor.
using SensorValueIntf = sdbusplus::common::xyz::openbmc_project::sensor::Value;

// Root object path under which all sensor D-Bus objects are created
// (e.g. "/xyz/openbmc_project/sensors").
inline constexpr auto sensorRootPath = SensorValueIntf::namespace_path::value;

class Sensor;

// The set of D-Bus interfaces a sensor object exposes: the sensor Value
// (value/unit/min/max) and the Association.Definitions (link to its chassis).
using SensorInterfaces = sdbusplus::async::server_t<
    Sensor,
    sdbusplus::aserver::xyz::openbmc_project::association::Definitions,
    sdbusplus::aserver::xyz::openbmc_project::sensor::Value>;

// Sensor is the D-Bus representation of a single fetched Redfish sensor. It is
// the live bus object: constructing it registers the object on the bus and
// publishes the first reading. A Sensor is only created once a sensor has been
// read successfully and has a unit, so sensors that never produce a valid
// reading never appear on the bus.
//
// The unit and value range are fixed at construction (Redfish sensors do not
// change their unit); only the value is mutable afterwards via updateValue().
class Sensor : public SensorInterfaces
{
  public:
    Sensor() = delete;
    Sensor(const Sensor&) = delete;
    Sensor(Sensor&&) = delete;
    Sensor& operator=(const Sensor&) = delete;
    Sensor& operator=(Sensor&&) = delete;
    ~Sensor() = default;

    // Translate a parsed Redfish sensor and, if it is valid, create and publish
    // the D-Bus object for it. Returns nullptr when the sensor has no
    // ReadingUnits, in which case nothing is placed on the bus. Per-unit default
    // value ranges are applied when the Redfish sensor omits them.
    static std::unique_ptr<Sensor> create(
        sdbusplus::async::context& ctx, const std::string& objectPath,
        const std::string& associationPath,
        redfish_binding::Sensor::Sensor& parsed);

    // Publish a new value, suppressing redundant PropertiesChanged signals.
    auto updateValue(double current) -> sdbusplus::async::task<>;

    // Map a Redfish ReadingUnits string (e.g. "Cel", "W", "%") to the D-Bus
    // sensor Value unit enum. Returns nullopt for unrecognized units. Static so
    // other units (e.g. the threshold mapper) can reuse the same mapping.
    static std::optional<SensorValueIntf::Unit> toMaybeUnit(
        const std::string& unitsStr);

  private:
    Sensor(sdbusplus::async::context& ctx, const std::string& objectPath,
           const std::string& associationPath, double reading,
           const std::string& unit, std::optional<double> minValue,
           std::optional<double> maxValue);

    auto shouldSkipSignal(double current) -> bool;

    std::mutex lock;
    double lastNotifiedValue = std::numeric_limits<double>::quiet_NaN();
};

} // namespace redfish_client::core
