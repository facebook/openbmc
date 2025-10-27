#include <redfish_client/core/update_service_handler.hpp>

#include "redfish-binding/SoftwareInventoryCollection_SoftwareInventoryCollection.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/timer.hpp>

#include <format>
#include <memory>

PHOSPHOR_LOG2_USING;

namespace redfish_client::core
{

namespace
{

auto loop(sdbusplus::async::context& ctx,
          std::unique_ptr<UpdateServiceHandler> handler,
          size_t intervalMilliseconds) -> sdbusplus::async::task<void>
{
    sdbusplus::server::manager_t manager{ctx, SoftwareVersion::namespace_path};
    while (!ctx.stop_requested())
    {
        co_await handler->load(ctx);
        co_await sdbusplus::async::sleep_for(
            ctx, std::chrono::milliseconds(intervalMilliseconds));
    };
    co_return;
}
} // anonymous namespace

Software::Software(sdbusplus::async::context& ctx, const std::string& id) :
    sdbusplus::async::context_ref(ctx),
    path(sdbusplus::message::object_path(SoftwareVersion::namespace_path) /
         std::format("{}_{}", id, randomIdGenerator()()))
{}

sdbusplus::message::object_path Software::getPath() const
{
    return path;
}

void Software::setVersion(const std::string& versionStr)
{
    if (!version)
    {
        version = std::make_unique<SoftwareVersion>(
            ctx, path.str.c_str(),
            SoftwareVersion::properties_t{
                .version = versionStr,
                .purpose = SoftwareVersion::VersionPurpose::Other,
            });
        version->emit_added();
        return;
    }
    version->version(versionStr);
}

void Software::setActivation(SoftwareActivation::Activations act)
{
    if (!activation)
    {
        activation = std::make_unique<SoftwareActivation>(
            ctx, path.str.c_str(),
            SoftwareActivation::properties_t{
                .activation = act,
                .requested_activation =
                    SoftwareActivation::RequestedActivations::None,
            });
        activation->emit_added();
        return;
    }
    activation->activation(act);
}

std::function<int()>& Software::randomIdGenerator()
{
    static std::function<int()> generator = []() {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        unsigned int seed = ts.tv_nsec ^ getpid();
        srandom(seed);
        return random() % 10000;
    };
    return generator;
}

UpdateServiceHandler::UpdateServiceHandler(
    const std::string& host, const std::string& inventoryName,
    const std::vector<UpdateServiceMapper>& mappers) :
    inventoryUrl(std::format("http://{}/redfish/v1/UpdateService/{}?$expand=.",
                             host, inventoryName)),
    handle(std::make_unique<AsyncHttpHandle>(inventoryUrl)), mappers(mappers)
{}

auto UpdateServiceHandler::load(sdbusplus::async::context& ctx)
    -> sdbusplus::async::task<void>
{
    auto response = co_await handle->tryGet(ctx);
    if (!response.has_value())
    {
        error("Http error for {URL}: {ERR}", "URL", inventoryUrl, "ERR",
              response.error());
        co_return;
    }
    if (response->code != 200)
    {
        error("Http response error code from {URL}: {CODE}", "URL",
              inventoryUrl, "CODE", response->code);
        co_return;
    }
    auto tryInventoryCollection = redfish_binding::SoftwareInventoryCollection::
        tryParseSoftwareInventoryCollection(response->body);
    if (!tryInventoryCollection.has_value())
    {
        error("Failed to parse software inventory collection from {URL}: {ERR}",
              "URL", inventoryUrl, "ERR", tryInventoryCollection.error());
        co_return;
    }
    auto& inventoryCollection = tryInventoryCollection.value();
    auto& maybeMembers = inventoryCollection.getMembers();
    if (!maybeMembers.hasValue())
    {
        co_return;
    }
    for (auto& member : maybeMembers.value())
    {
        auto& maybeId = member.getId();
        if (!maybeId.hasValue())
        {
            continue;
        }
        const auto& id = maybeId.value();
        auto& maybeVersion = member.getVersion();
        if (!maybeVersion.hasValue())
        {
            continue;
        }
        const auto& version = maybeVersion.value();
        const auto mapperIt =
            std::find_if(mappers.begin(), mappers.end(),
                         [&](auto mapper) { return mapper.fromId == id; });
        if (mapperIt == mappers.end())
        {
            continue;
        }
        auto softwareIt = inventory.find(id);
        if (softwareIt == inventory.end())
        {
            softwareIt =
                inventory
                    .insert(
                        {id, std::make_unique<Software>(ctx, mapperIt->toId)})
                    .first;
            info("Create software {PATH} from {ID}", "PATH",
                 softwareIt->second->getPath().str, "ID", id);
        }
        softwareIt->second->setVersion(version);
        softwareIt->second->setActivation(
            SoftwareActivation::Activations::Active);
    }
}

auto UpdateServiceHandler::run(sdbusplus::async::context& ctx,
                               const std::string& host,
                               const UpdateServiceConfig& config)
    -> sdbusplus::async::task<void>
{
    if (!config.firmwareMappers.empty())
    {
        ctx.spawn(loop(ctx,
                       std::make_unique<UpdateServiceHandler>(
                           host, "FirmwareInventory", config.firmwareMappers),
                       config.intervalMilliseconds));
    }
    if (!config.softwareMappers.empty())
    {
        ctx.spawn(loop(ctx,
                       std::make_unique<UpdateServiceHandler>(
                           host, "SoftwareInventory", config.softwareMappers),
                       config.intervalMilliseconds));
    }
    co_return;
}

} // namespace redfish_client::core
