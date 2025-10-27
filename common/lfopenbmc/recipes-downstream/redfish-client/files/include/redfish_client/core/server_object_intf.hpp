#include <sdbusplus/async.hpp>
#include <xyz/openbmc_project/Association/Definitions/aserver.hpp>
#include <xyz/openbmc_project/Sensor/Value/aserver.hpp>

namespace redfish_client::core
{

class ServerObjectIntf;
using SensorInterfaces = sdbusplus::async::server_t<
    ServerObjectIntf,
    sdbusplus::aserver::xyz::openbmc_project::association::Definitions,
    sdbusplus::aserver::xyz::openbmc_project::sensor::Value>;

class ServerObjectIntf : public SensorInterfaces
{
  public:
    ServerObjectIntf(sdbusplus::async::context& ctx, const char* path);
    void emit_added();
};

} // namespace redfish_client::core
