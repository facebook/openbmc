#include <sdbusplus/async.hpp>

#include <redfish_client/core/config.hpp>
#include <redfish_client/core/sensor.hpp>
#include <redfish_client/core/server_object_intf.hpp>

#include <string>
#include <limits>

namespace redfish_client::core
{

// The following helpers are exposed for unit testing purposes.
// This is the interface to use for sensor values before emitting the data.
using ValueIntf = sdbusplus::common::xyz::openbmc_project::sensor::Value;
using PathIntf = ValueIntf::namespace_path;

std::optional<ValueIntf::Unit> toMaybeIntfUnits(const std::string& unitsStr);

const char* getActualMetricNamespace(const char* logicalNameParam);

const char* getSensorRootPath();

struct SensorDbusObject
{
    SensorDbusObject() = delete;
    SensorDbusObject(const SensorDbusObject&) = delete;
    SensorDbusObject(SensorDbusObject&&) = delete;

    SensorDbusObject(sdbusplus::async::context& ctx, const char* metricPath,
                     const SensorMapper& mapper,
                     const std::string& associationPath);

    auto update(Sensor sensor) -> sdbusplus::async::task<>;

    auto shouldSkipSignal(double current) -> bool;

    sdbusplus::async::context& ctx;
    std::mutex lock;
    std::string metricPath;
    std::unique_ptr<ServerObjectIntf> object;
    double lastNotifiedValue = std::numeric_limits<double>::quiet_NaN();
    bool minValueNotified = false;
    bool maxValueNotified = false;
    SensorMapper mapper;
    const std::string& associationPath;
};

} // namespace redfish_client::core
