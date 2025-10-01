#pragma once

#include "async_http_client.hpp"
#include "config.hpp"
#include "redfish-binding/SoftwareInventory_SoftwareInventory.hpp"

#include <sdbusplus/async/context.hpp>
#include <xyz/openbmc_project/Software/Activation/aserver.hpp>
#include <xyz/openbmc_project/Software/Version/aserver.hpp>

#include <functional>
#include <string>

namespace redfish_client_daemon
{

class Software;

using SoftwareVersion =
    sdbusplus::aserver::xyz::openbmc_project::software::Version<Software>;

using SoftwareActivation =
    sdbusplus::aserver::xyz::openbmc_project::software::Activation<Software>;

class Software : private sdbusplus::async::context_ref
{
  public:
    Software() = delete;

    Software(sdbusplus::async::context& ctx, const std::string& id);

    sdbusplus::message::object_path getPath() const;

    void setVersion(const std::string& versionStr);

    void setActivation(SoftwareActivation::Activations act);

    static std::function<int()>& randomIdGenerator();

  private:
    const sdbusplus::message::object_path path;
    std::unique_ptr<SoftwareVersion> version{nullptr};
    std::unique_ptr<SoftwareActivation> activation{nullptr};
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

    std::unordered_map<std::string, std::unique_ptr<Software>> softwareMap;
};

} // namespace redfish_client_daemon
