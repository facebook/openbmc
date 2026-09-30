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

using SensorValueIntf = sdbusplus::common::xyz::openbmc_project::sensor::Value;

class Sensor;

// A sensor also implements Association.Definitions, not just Value, so it can be
// linked back to the chassis that owns it.
using SensorInterfaces = sdbusplus::async::server_t<
    Sensor,
    sdbusplus::aserver::xyz::openbmc_project::association::Definitions,
    sdbusplus::aserver::xyz::openbmc_project::sensor::Value>;

// The live D-Bus object for one Redfish sensor. Created lazily, only once a
// reading with a known unit has arrived, so sensors that never yield a valid
// reading stay off the bus. Unit and range are fixed at construction because
// Redfish never changes them mid-life; only the value moves afterwards.
class Sensor : public SensorInterfaces
{
  public:
    Sensor() = delete;
    Sensor(const Sensor&) = delete;
    Sensor(Sensor&&) = delete;
    Sensor& operator=(const Sensor&) = delete;
    Sensor& operator=(Sensor&&) = delete;
    ~Sensor() = default;

    // Sourced from the interface definition so it stays in step with the
    // namespace the objects actually live under.
    static constexpr auto rootPath = SensorValueIntf::namespace_path::value;

    // The immutable half of a sensor: the unit and the range its value spans,
    // both fixed once the object is published.
    struct Metadata
    {
        SensorValueIntf::Unit unit;
        double minValue;
        double maxValue;
    };

    // A sensor whose unit is absent or unrecognized cannot be represented
    // faithfully, so it is skipped (nullptr) rather than published under a
    // guessed unit. The object path is composed as rootPath/ns/id.
    static std::unique_ptr<Sensor> create(
        sdbusplus::async::context& ctx, const std::string& ns,
        const std::string& id, const std::string& associationPath,
        redfish_binding::Sensor::Sensor& parsed);

    // Stays silent when the value barely moved, so continuous polling does not
    // flood the bus with PropertiesChanged signals.
    auto updateValue(double current) -> sdbusplus::async::task<>;

    // Redfish reports units as free-form strings, so only a known set maps onto
    // D-Bus; an unknown unit yields nullopt. The range is a fallback default for
    // sensors that report none of their own.
    static std::optional<Metadata> toMaybeMetadata(const std::string& unitsStr);

    // The unit alone, for when the range is not needed.
    static std::optional<SensorValueIntf::Unit> toMaybeUnit(
        const std::string& unitsStr);

  private:
    Sensor(sdbusplus::async::context& ctx, const std::string& objectPath,
           const std::string& associationPath, double reading,
           const Metadata& metadata);

    auto shouldSkipSignal(double current) -> bool;

    std::mutex lock;
    double lastNotifiedValue = std::numeric_limits<double>::quiet_NaN();
};

} // namespace redfish_client::core
