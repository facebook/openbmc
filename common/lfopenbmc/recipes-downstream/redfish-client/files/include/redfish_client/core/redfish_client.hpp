#include <redfish_client/core/config.hpp>
#include <redfish_client/core/sensor.hpp>
#include <redfish_client/core/log_service_handler.hpp>

#include <sdbusplus/async.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace redfish_client::core
{

class RedfishClient
{
  public:
    RedfishClient() = delete;

    RedfishClient(sdbusplus::async::context& ctx, const std::string& configDir,
                  const std::string& persistDir);

    RedfishClient(sdbusplus::async::context& ctx, const Config& config,
                  const std::string& persistDir);

    auto run() -> sdbusplus::async::task<>;

    ~RedfishClient();

  private:
    auto readWithRetries(const SensorMapper& mapper)
        -> sdbusplus::async::task<
            std::optional<redfish_binding::Sensor::Sensor>>;

    auto runEventPollingLoop() -> sdbusplus::async::task<>;

    auto ingestMetricReport(
        const nlohmann::json& report,
        std::unordered_set<std::string>& reportCovered)
        -> sdbusplus::async::task<>;

    auto runSensorLoop() -> sdbusplus::async::task<>;

    // Read a sensor and publish it: create the Sensor on first success (keyed by
    // mapper.fromUrl), otherwise push the new value onto the existing one.
    auto pollSensor(const SensorMapper& mapper) -> sdbusplus::async::task<>;

    // Look up the configured mapper for a Redfish sensor URL, or nullptr if the
    // URL does not belong to one of our sensors.
    const SensorMapper* findMapper(std::string_view fromUrl) const;

    auto loadConfig() -> sdbusplus::async::task<>;

    void registerLogMappers();

    Config loadCompatibleConfig(
        const std::string& configDir,
        const std::vector<std::string>& compatiblePlatformNames);

    auto subtree_for_target_interface(
        sdbusplus::async::context& ctx, const std::string& subpath,
        const std::string& targetInterface,
        const std::function<sdbusplus::async::task<>(
            const std::string&, const std::string&, const std::string&)>&
            coroutine,
        size_t depth) -> sdbusplus::async::task<>;

    auto getCompatiblePlatformNames()
        -> sdbusplus::async::task<std::vector<std::string>>;

    sdbusplus::async::context& ctx;
    // Live sensor objects, keyed by the original Redfish sensor URL
    // (mapper.fromUrl). Populated lazily: an entry exists only once the sensor
    // has been read successfully and published on the bus.
    std::unordered_map<std::string, std::shared_ptr<Sensor>> sensors;
    std::vector<std::shared_ptr<LogServiceHandler>> logServiceHandlers;
    std::string configDir;
    std::optional<Config> config;
    std::string persistDir;
    std::unordered_map<std::string, std::unique_ptr<AsyncHttpHandle>>
        httpHandles;
};

} // namespace redfish_client::core
