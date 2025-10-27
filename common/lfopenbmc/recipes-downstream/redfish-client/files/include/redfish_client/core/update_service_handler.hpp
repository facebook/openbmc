#pragma once

#include <redfish_client/core/async_http_client.hpp>
#include <redfish_client/core/config.hpp>
#include "redfish-binding/SoftwareInventory_SoftwareInventory.hpp"

#include <sdbusplus/async/context.hpp>
#include <xyz/openbmc_project/Software/Activation/aserver.hpp>
#include <xyz/openbmc_project/Software/Version/aserver.hpp>

#include <functional>
#include <string>

namespace redfish_client::core
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

class UpdateServiceHandler
{
  public:
    UpdateServiceHandler() = delete;

    UpdateServiceHandler(const std::string& host,
                         const std::string& inventoryName,
                         const std::vector<UpdateServiceMapper>& mappers);

    static auto run(sdbusplus::async::context& ctx, const std::string& host,
                    const UpdateServiceConfig& config)
        -> sdbusplus::async::task<void>;

    auto load(sdbusplus::async::context& ctx) -> sdbusplus::async::task<void>;

  private:
    const std::string inventoryUrl;
    std::unique_ptr<AsyncHttpHandle> handle;
    const std::vector<UpdateServiceMapper>& mappers;
    std::unordered_map<std::string, std::unique_ptr<Software>> inventory;
};

} // namespace redfish_client::core
