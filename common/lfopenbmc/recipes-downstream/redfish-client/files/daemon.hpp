#pragma once

#include "config.hpp"
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

void installSignalHandlers();

// Run redfish client till process receives SIGINT or SIGTERM
void runRedfishClient(const std::string& serviceName,
                      sdbusplus::async::context& ctx,
                      const std::string configDir, std::string persistDir = "");

void runRedfishClient(const std::string& serviceName,
                      sdbusplus::async::context& ctx, const Config& config,
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
