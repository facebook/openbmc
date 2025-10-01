#include "update_service_handler.hpp"

#include "redfish-binding/SoftwareInventoryCollection_SoftwareInventoryCollection.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/timer.hpp>

#include <format>
#include <memory>

PHOSPHOR_LOG2_USING;

namespace redfish_client_daemon
{

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
    sdbusplus::async::context& ctx, const std::string& host,
    const UpdateServiceConfig& config) : sdbusplus::async::context_ref(ctx)
{
    ctx.spawn(loop(
        std::format(
            "http://{}/redfish/v1/UpdateService/FirmwareInventory?$expand=.",
            host),
        config.firmwareMappers, config.intervalMilliseconds));
    ctx.spawn(loop(
        std::format(
            "http://{}/redfish/v1/UpdateService/SoftwareInventory?$expand=.",
            host),
        config.softwareMappers, config.intervalMilliseconds));
}

auto UpdateServiceHandler::loop(const std::string url,
                                const std::vector<UpdateServiceMapper>& mappers,
                                size_t intervalMilliseconds)
    -> sdbusplus::async::task<void>
{
    if (mappers.empty())
    {
        co_return;
    }
    sdbusplus::server::manager_t manager{ctx, SoftwareVersion::namespace_path};
    auto handle = std::make_unique<AsyncHttpHandle>(url);
    bool skipSleep = true;
    while (!ctx.stop_requested())
    {
        if (!skipSleep)
        {
            co_await sdbusplus::async::sleep_for(
                ctx, std::chrono::milliseconds(intervalMilliseconds));
            skipSleep = false;
        }
        std::string inventoryCollectionJson;
        try
        {
            auto response = co_await handle->get(ctx);
            if (response.code != 200)
            {
                throw std::runtime_error(
                    std::format("Http response error code: {}", response.code));
            }
            inventoryCollectionJson = response.body;
        }
        catch (const std::exception& exn)
        {
            info("Exception while querying url {URL}: {EXC}", "URL", url, "EXC",
                 exn.what());
            continue;
        }
        redfish_binding::SoftwareInventoryCollection::
            SoftwareInventoryCollection inventoryCollection;
        try
        {
            inventoryCollection = redfish_binding::SoftwareInventoryCollection::
                parseSoftwareInventoryCollection(inventoryCollectionJson);
        }
        catch (const std::exception& exn)
        {
            info("Exception while parsing response from url {URL}: {EXC}",
                 "URL", url, "EXC", exn.what());
            continue;
        }
        auto& maybeMembers = inventoryCollection.getMembers();
        if (!maybeMembers.hasValue())
        {
            continue;
        }

        for (auto& member : maybeMembers.value())
        {
            update(member, mappers);
        }
    }
    co_return;
}

void UpdateServiceHandler::update(
    redfish_binding::SoftwareInventory::SoftwareInventory& newSoftware,
    const std::vector<UpdateServiceMapper>& mappers)
{
    auto& maybeId = newSoftware.getId();
    if (!maybeId.hasValue())
    {
        return;
    }
    const auto& id = maybeId.value();
    auto& maybeVersion = newSoftware.getVersion();
    if (!maybeVersion.hasValue())
    {
        return;
    }
    const auto& version = maybeVersion.value();
    const auto mapperIt =
        std::find_if(mappers.begin(), mappers.end(),
                     [&](auto mapper) { return mapper.fromId == id; });
    if (mapperIt == mappers.end())
    {
        return;
    }
    auto softwareIt = softwareMap.find(id);
    if (softwareIt == softwareMap.end())
    {
        softwareIt =
            softwareMap
                .insert({id, std::make_unique<Software>(ctx, mapperIt->toId)})
                .first;
        info("Create software {PATH} from {ID}", "PATH",
             softwareIt->second->getPath().str, "ID", id);
    }
    softwareIt->second->setVersion(version);
    softwareIt->second->setActivation(SoftwareActivation::Activations::Active);
}

} // namespace redfish_client_daemon
