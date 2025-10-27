#pragma once

#include <redfish_client/core/config.hpp>
#include <redfish_client/core/sensor.hpp>

#include <sdbusplus/async.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace redfish_client_daemon
{

using namespace redfish_client::core;

void installSignalHandlers();

// Run redfish client till process receives SIGINT or SIGTERM
void runRedfishClient(const std::string& serviceName,
                      sdbusplus::async::context& ctx,
                      const std::string configDir, std::string persistDir = "");

void runRedfishClient(const std::string& serviceName,
                      sdbusplus::async::context& ctx, const Config& config,
                      std::string persistDir = "");

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
