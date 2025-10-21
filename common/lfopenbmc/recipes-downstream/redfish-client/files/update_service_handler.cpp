#include "update_service_handler.hpp"

#include "redfish-binding/SoftwareInventoryCollection_SoftwareInventoryCollection.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/timer.hpp>

#include <format>
#include <memory>

PHOSPHOR_LOG2_USING;

namespace redfish_client_daemon
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
        try
        {
            co_await handler->load(ctx);
        }
        catch (const std::exception& exn)
        {
            info("Exception loading inventory: {EXC}", "EXC", exn.what());
        }
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
    handle(std::make_unique<AsyncHttpHandle>(
        std::format("http://{}/redfish/v1/UpdateService/{}?$expand=.", host,
                    inventoryName))),
    mappers(mappers)
{}

auto UpdateServiceHandler::load(sdbusplus::async::context& ctx)
    -> sdbusplus::async::task<void>
{
    auto response = co_await handle->get(ctx);
    if (response.code != 200)
    {
        throw std::runtime_error(
            std::format("Http response error code: {}", response.code));
    }
    auto inventoryCollection = redfish_binding::SoftwareInventoryCollection::
        parseSoftwareInventoryCollection(response.body);
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

} // namespace redfish_client_daemon
