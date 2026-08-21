#pragma once

#include <redfish_client/core/async_http_client.hpp>
#include <redfish_client/core/config.hpp>
#include <redfish_client/core/sensor.hpp>

#include <redfish-binding/Sensor_Sensor.hpp>

#include <nlohmann/json.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/async/context.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace redfish_client::core
{

// Polls a Redfish host for sensor readings and updates the matching D-Bus
// Sensor objects. run() spawns a self-owned polling loop; each pass is one
// load().
class SensorHandler
{
  public:
    SensorHandler() = delete;

    SensorHandler(std::string host, const SensorConfig& config) :
        host(std::move(host)), config(config)
    {}

    // Spawn a loop that refreshes every configured sensor on the configured
    // interval until the context stops.
    static auto run(sdbusplus::async::context& ctx, const std::string& host,
                    const SensorConfig& config) -> sdbusplus::async::task<void>;

    // Refresh every configured sensor once: pull the metric reports, then
    // individually read any sensor a report does not already drive.
    auto load(sdbusplus::async::context& ctx) -> sdbusplus::async::task<void>;

  private:
    // Fetch and parse the sensor at the given host-relative path, retrying up to
    // the configured limit. Returns nullopt if every attempt fails. With
    // cacheConnection the HTTP connection is kept in httpHandles for reuse
    // across calls; a one-shot read should pass false so nothing lingers.
    auto fetchSensor(sdbusplus::async::context& ctx, const std::string& path,
                   bool cacheConnection = true)
        -> sdbusplus::async::task<
            std::optional<redfish_binding::Sensor::Sensor>>;

    // Fetch one metric report and update the sensors it carries: a bare value
    // for one already created, a full read to create one seen for the first
    // time.
    auto fetchAndUpdateSensorsFromMetricReport(sdbusplus::async::context& ctx,
                                             const std::string& reportPath)
        -> sdbusplus::async::task<void>;

    // Update a freshly read sensor: create its D-Bus object the first time the
    // sensor is seen, otherwise push the new reading onto the existing object.
    void updateSensor(sdbusplus::async::context& ctx,
                      const SensorMapper& mapper,
                redfish_binding::Sensor::Sensor& parsed);

    const std::string host;
    const SensorConfig& config;

    // Live sensor objects, keyed by the original Redfish sensor URL
    // (mapper.fromUrl). Populated lazily once a sensor has been read and
    // created.
    std::unordered_map<std::string, std::shared_ptr<Sensor>> sensors;

    // Sensors already driven by a metric report; no longer polled individually.
    std::unordered_set<std::string> reportCovered;

    std::unordered_map<std::string, std::unique_ptr<AsyncHttpHandle>>
        httpHandles;
};

} // namespace redfish_client::core
