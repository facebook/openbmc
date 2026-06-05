#include <redfish_client/core/config.hpp>
#include <redfish_client/core/sensor.hpp>
#include <redfish_client/core/sensor_dbus_object.hpp>
#include <redfish_client/core/log_service_handler.hpp>

#include <sdbusplus/async.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
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
    std::optional<Sensor> readWithRetries(const SensorMapper& mapper);

    auto runEventPollingLoop() -> sdbusplus::async::task<>;

    void runSensorLoop();

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
    std::unordered_map<std::string, std::shared_ptr<SensorDbusObject>> metrics;
    std::vector<std::shared_ptr<LogServiceHandler>> logServiceHandlers;
    std::string configDir;
    std::optional<Config> config;
    std::thread sensorThread;
    std::string persistDir;
    std::unordered_map<std::string, std::unique_ptr<AsyncHttpHandle>>
        httpHandles;
};

} // namespace redfish_client::core
