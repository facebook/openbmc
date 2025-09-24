#pragma once

#include "async_http_client.hpp"
#include "config.hpp"
#include "redfish-binding/SoftwareInventory_SoftwareInventory.hpp"

#include <sdbusplus/async/context.hpp>
#include <xyz/openbmc_project/Software/Version/aserver.hpp>

#include <functional>
#include <string>

namespace redfish_client_daemon
{

class Software;

using SoftwareIntf = sdbusplus::async::server_t<
    Software, sdbusplus::aserver::xyz::openbmc_project::software::Version>;

class Software : public SoftwareIntf
{
  public:
    Software(sdbusplus::async::context& ctx, const std::string& path);

    static std::function<int()>& randomIdGenerator();
};

class UpdateServiceHandler : private sdbusplus::async::context_ref
{
  public:
    UpdateServiceHandler() = delete;

    UpdateServiceHandler(sdbusplus::async::context& ctx,
                         const std::string& host,
                         const UpdateServiceConfig& config);

  private:
    auto loop(const std::string url,
              const std::vector<UpdateServiceMapper>& mappers,
              size_t intervalMilliseconds) -> sdbusplus::async::task<void>;

    void update(
        redfish_binding::SoftwareInventory::SoftwareInventory& newSoftware,
        const std::vector<UpdateServiceMapper>& mappers);

    std::unordered_map<std::string, std::unique_ptr<Software>>
        pathToSoftwareMap;
};

} // namespace redfish_client_daemon
