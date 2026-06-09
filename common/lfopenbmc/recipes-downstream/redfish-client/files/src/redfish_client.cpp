#include <redfish_client/core/redfish_client.hpp>
#include <redfish_client/core/update_service_handler.hpp>
#include <redfish_client/core/log_entry_mapper_registry.hpp>
#include <redfish_client/core/unhandled_mapper.hpp>
#include <redfish_client/component_configs/component_registry.hpp>

#include <phosphor-logging/lg2.hpp>

#include <fstream>

PHOSPHOR_LOG2_USING;

namespace redfish_client::core
{

static inline auto subtree(sdbusplus::async::context& ctx,
                           const auto& subpath, const auto& interface,
                           size_t depth = 0)
{
    using ObjectMapper =
        sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;
    auto mapper = ObjectMapper(ctx)
                      .service(ObjectMapper::default_service)
                      .path(ObjectMapper::instance_path);
    return mapper.get_sub_tree(subpath, depth, {interface});
}

RedfishClient::RedfishClient(sdbusplus::async::context& ctx, const std::string& configDir,
              const std::string& persistDir) :
    ctx(ctx), configDir(configDir), persistDir(persistDir)
{}

RedfishClient::RedfishClient(sdbusplus::async::context& ctx, const Config& config,
              const std::string& persistDir) :
    ctx(ctx), config(config), persistDir(persistDir)
{}

auto RedfishClient::run() -> sdbusplus::async::task<>
{
    info("Running RedfishClient");
    if (!config.has_value())
    {
        co_await loadConfig();
    }

    registerLogMappers();

    if (config->sensorConfig.has_value())
    {
        const auto& sensorConfig = config->sensorConfig.value();
        info("Creating Sensor objects: {SIZE}", "SIZE",
             sensorConfig.mappers.size());
        for (const auto& mapper : sensorConfig.mappers)
        {
            auto metricNamespace = std::string(
                getActualMetricNamespace(mapper.toNamespace.c_str()));

            std::string fullMetricPath =
                std::string(getSensorRootPath()) + "/" + metricNamespace +
                "/" + mapper.toId;

            metrics[mapper.toId] = std::make_shared<SensorDbusObject>(
                ctx, fullMetricPath.c_str(), mapper,
                sensorConfig.associationPath);
        }
        sensorThread = std::thread([this] { runSensorLoop(); });
    }

    if (config->logServiceConfig.has_value())
    {
        const auto& logServiceConfig = config->logServiceConfig.value();
        info("logServiceConfig intervalMilliseconds = {INTERVAL}",
             "INTERVAL", logServiceConfig.intervalMilliseconds);
        for (const auto& url : logServiceConfig.urls)
        {
            auto expandedUrl =
                std::format("http://{}{}", config->host, url);
            info("logServiceConfig url = {URL}", "URL",
                 expandedUrl.c_str());
            info("persistDir = {PERSIST_DIR}", "PERSIST_DIR", persistDir);

            logServiceHandlers.push_back(
                std::make_shared<LogServiceHandler>(
                    ctx, expandedUrl,
                    logServiceConfig.skipHistoricalEntriesThresholdSeconds,
                    persistDir));
        }
        ctx.spawn(runEventPollingLoop());
    }

    if (config->updateServiceConfig.has_value())
    {
        ctx.spawn(UpdateServiceHandler::run(
                ctx, config->host, config->updateServiceConfig.value()));
    }
    co_return;
}

RedfishClient::~RedfishClient()
{
    if (sensorThread.joinable())
    {
        sensorThread.join();
    }
}

std::optional<Sensor> RedfishClient::readWithRetries(const SensorMapper& mapper)
{
    for (size_t i = 0; i < config->sensorConfig.value().maxRetries; ++i)
    {
        std::string sensorJson;
        auto expandedUrl =
            std::format("http://{}{}", config->host, mapper.fromUrl);
        try
        {
            auto it = httpHandles.find(expandedUrl);
            if (it == httpHandles.end())
            {
                it = httpHandles
                         .insert({expandedUrl,
                                  std::make_unique<AsyncHttpHandle>(
                                      expandedUrl)})
                         .first;
            }
            auto& httpHandle = it->second;
            // TODO: Switch to co_await when this function is switched to
            // coroutine
            auto maybeResponse = stdexec::sync_wait(httpHandle->get(ctx));
            if (!maybeResponse.has_value())
            {
                throw std::runtime_error("Http request stopped");
            }
            const auto& response = std::get<0>(maybeResponse.value());
            if (response.code != 200)
            {
                throw std::runtime_error(std::format(
                    "Http response error code: {}", response.code));
            }
            sensorJson = response.body;
        }
        catch (const std::exception& exn)
        {
            info("Exception while querying url ({URL}): {EXC}", "URL",
                 expandedUrl.c_str(), "EXC", exn);
        };

        try
        {
            return Sensor::parseSensor(sensorJson);
        }
        catch (const std::exception& exn)
        {
            info("Exception while parsing sensor json from ({URL}): {EXC}, {JSON}",
                 "URL", expandedUrl.c_str(), "EXC", exn, "JSON",
                 sensorJson.c_str());
        };

        std::this_thread::sleep_for(std::chrono::milliseconds(
            config->sensorConfig.value().retryIntervalMilliseconds));
    }
    return std::nullopt;
}

auto RedfishClient::runEventPollingLoop() -> sdbusplus::async::task<>
{
    if (logServiceHandlers.size() == 0)
    {
        co_return;
    }

    info("Running event polling loop");

    try
    {
        while (!ctx.stop_requested())
        {
            for (auto& logServiceHandler : logServiceHandlers)
            {
                co_await logServiceHandler->runOnce();
            }

            co_await sdbusplus::async::sleep_for(
                ctx,
                std::chrono::milliseconds(
                    config->logServiceConfig.value().intervalMilliseconds));
        }
    }
    catch (const std::logic_error& exn)
    {
        debug("Unhandled logic error: {NAME}", "WHAT", exn.what());
    };

    co_return;
}

void RedfishClient::runSensorLoop()
{
    info("Running sensor loop");
    try
    {
        while (!ctx.stop_requested())
        {
            for (const auto& [metricKey, metric] : metrics)
            {
                auto maybeSensor = readWithRetries(metric->mapper);
                if (!maybeSensor.has_value())
                {
                    continue;
                }
                ctx.spawn(metric->update(maybeSensor.value()));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(
                config->sensorConfig.value().intervalMilliseconds));
        }
    }
    catch (const std::logic_error& exn)
    {
        debug("Unhandled logic error: {NAME}", "WHAT", exn.what());
    };
}

auto RedfishClient::loadConfig() -> sdbusplus::async::task<>
{
    auto compatiblePlatformName = co_await getCompatiblePlatformNames();
    config = loadCompatibleConfig(configDir, compatiblePlatformName);
    co_return;
}

void RedfishClient::registerLogMappers()
{
    auto& registry = LogEntryMapperRegistry::instance();

    if (config->components.has_value()) {
        for (const auto& componentName : config->components.value()) {
            info("Registering component: {COMPONENT}", "COMPONENT", componentName);
            component_config::registerComponent(componentName, *config);
        }
    }

    registry.registerMapper(std::make_unique<UnhandledMapper>(), 0);
    info("Mapper registration complete");
}

Config RedfishClient::loadCompatibleConfig(
    const std::string& configDir,
    const std::vector<std::string>& compatiblePlatformNames)
{
    namespace fs = std::filesystem;
    for (const auto& entry : fs::directory_iterator(configDir))
    {
        std::ifstream file(entry.path());
        if (!file.is_open())
        {
            error("Failed to open file: {FILE}", "FILE",
                  entry.path().string());
            continue;
        }
        std::string jsonContents((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
        auto config = Config::parse(jsonContents);

        for (auto platformName: compatiblePlatformNames)
        {
            if (config.compatible == platformName)
            {
                info("Matched config file: {FILE}", "FILE",
                     entry.path().string());
                return config;
            }
        }
    }
    error("No matching config file found for platform list: {PLATFORM}",
          "PLATFORM", std::format("{}", compatiblePlatformNames));
    throw std::runtime_error("No matching config file found");
}

auto RedfishClient::subtree_for_target_interface(
    sdbusplus::async::context& ctx, const std::string& subpath,
    const std::string& targetInterface,
    const std::function<sdbusplus::async::task<>(
        const std::string&, const std::string&, const std::string&)>&
        coroutine,
    size_t depth) -> sdbusplus::async::task<>
{
    auto objects = co_await subtree(ctx, subpath, targetInterface, depth);
    info("iterating over entries.");
    for (const auto& [path, services] : objects)
    {
        for (const auto& [service, interfaces] : services)
        {
            info("Examining {INTERFACE} at {PATH} by {SERVICE}",
                 "INTERFACE", targetInterface, "PATH", path, "SERVICE",
                 service);
            co_await coroutine(path, service, targetInterface);
        }
    }
    co_return;
}

auto RedfishClient::getCompatiblePlatformNames()
    -> sdbusplus::async::task<std::vector<std::string>>
{
    std::vector<std::string> platformNames;
    while (!ctx.stop_requested() && platformNames.empty())
    {
        try
        {
            co_await subtree_for_target_interface(
                ctx, "/xyz/openbmc_project/inventory/system/board/",
                "xyz.openbmc_project.Inventory.Decorator.Compatible",
                [&](const auto& path, const auto& service,
                    const auto& interface) -> sdbusplus::async::task<> {
                    auto names =
                        co_await sdbusplus::async::proxy()
                            .service(service)
                            .path(path)
                            .interface(
                                "xyz.openbmc_project.Inventory.Decorator.Compatible")
                            .template get_property<
                                std::vector<std::string>>(ctx, "Names");
                    for (auto name : names)
                    {
                        info("Compatible : Names : {PLATFORMNAME}",
                             "PLATFORMNAME", name);
                    }

                    platformNames.insert(platformNames.end(), names.begin(),
                                         names.end());
                    co_return;
                },
                1);

            if (platformNames.empty())
            {
                info("platformNames is still empty. Retry again");
            }
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            info("subtree query failed: {ERR}", "ERR", e.what());
        }
        co_await sdbusplus::async::sleep_for(
            ctx, std::chrono::milliseconds(500));
    }
    co_return platformNames;
}

} // namespace redfish_client::core
