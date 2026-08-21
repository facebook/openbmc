#include <redfish_client/core/config.hpp>

#include <sdbusplus/async.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
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
    std::string configDir;
    std::optional<Config> config;
    std::string persistDir;
};

} // namespace redfish_client::core
