#pragma once

#include "sensor.hpp"

#include <sdbusplus/async.hpp>
#include <xyz/openbmc_project/Association/Definitions/aserver.hpp>
#include <xyz/openbmc_project/Sensor/Value/aserver.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace redfish_client_daemon
{

/*
E.g. json snippet from daemon config:

  "sensor_configs": {
    "association_path": "/xyz/openbmc_project/inventory/system/board/HMC",
    "sensors": {
      "/machine1_temp": {
        "url": "/redfish/v1/Chassis/machine1/Sensors/machine1_temp",
        "symbolic_namespace": "temperature"
      },
      "/machine1_power": {
        "url": "/redfish/v1/Chassis/HGX_CPU_0/Sensors/machine1_power",
        "symbolic_namespace": "power"
      },
*/

// SensorConfigValue holds the dictionary values corresponding to the
// "sensor_configs" dictionary in the daemon config json.
struct SensorConfigValue
{
    std::string url;
    std::string symbolicNamespace;
};

struct EventLogConfigs
{
    std::vector<std::string> urls;
    size_t intervalMilliseconds;
};

// DaemonConfig holds the parsed representation of the daemon config json.
struct DaemonConfig
{
    static DaemonConfig fromJson(const std::string& configJson);
    std::string serviceName;
    std::string hostPort;
    std::string sensorAssociation;
    // Keys are the dbus paths of the sensors. Values are the SensorConfigValue.

    // E.g. "/machine1_temp" ->
    // /xyz/openbmc_project/sensors/temperature/machine1_temp
    std::unordered_map<std::string, SensorConfigValue> sensorConfigs;

    // Daemon exeuction parameters.
    size_t intervalMilliseconds;
    int retries;
    size_t waitMilliseconds;

    // Event log polling parameters.
    std::optional<EventLogConfigs> eventLogConfigs;
};

void installSignalHandlers();

// The dbus server runs till process receives SIGINT or SIGTERM and does a clean
// shutdown after that.
void runDbusServerTillInterrupted(const DaemonConfig& daemonConfig,
                                  sdbusplus::async::context& ctx,
                                  std::string persistDir = "");

// The following helpers are exposed for unit testing purposes.
// This is the interface to use for sensor values before emitting the data.
using ValueIntf = sdbusplus::common::xyz::openbmc_project::sensor::Value;

const char* getSensorRootPath();

const char* getActualMetricNamespace(const char* logicalNameParam);

std::optional<ValueIntf::Unit> toMaybeIntfUnits(const std::string& unitsStr);

// SensorDbusObject interface
class ISensorDbusObject
{
  public:
    ISensorDbusObject() = default;
    virtual ~ISensorDbusObject() = default;
    ISensorDbusObject(const ISensorDbusObject&) = delete;
    ISensorDbusObject(ISensorDbusObject&&) = delete;
    ISensorDbusObject& operator=(const ISensorDbusObject&) = delete;
    ISensorDbusObject& operator=(ISensorDbusObject&&) = delete;

    virtual sdbusplus::async::task<> update(Sensor sensor) = 0;
};

std::shared_ptr<ISensorDbusObject> createSensorDbusObjectForTest(
    sdbusplus::async::context& ctx, const char* metricPath,
    const std::string& associationPath);

} // namespace redfish_client_daemon
