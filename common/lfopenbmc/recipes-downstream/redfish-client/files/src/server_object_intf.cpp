#include <redfish_client/core/server_object_intf.hpp>

namespace redfish_client::core
{

ServerObjectIntf::ServerObjectIntf(sdbusplus::async::context& ctx, const char* path) :
      SensorInterfaces(ctx, path)
  {}

void ServerObjectIntf::emit_added()
{
    Definitions::emit_added();
    Value::emit_added();
}

} // namespace redfish_client::core
