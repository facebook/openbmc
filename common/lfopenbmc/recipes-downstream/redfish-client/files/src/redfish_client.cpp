#include <redfish_client/core/redfish_client.hpp>
#include <redfish_client/core/update_service_handler.hpp>
#include <redfish_client/core/log_entry_mapper_registry.hpp>
#include <redfish_client/core/unhandled_mapper.hpp>
#include <redfish_client/component_configs/component_registry.hpp>

#include <phosphor-logging/lg2.hpp>

#include <algorithm>
#include <charconv>
#include <fstream>

PHOSPHOR_LOG2_USING;

namespace redfish_client::core
{

static double parseMetricValue(const nlohmann::json& val)
{
    if (val.is_string())
    {
        const auto& strVal = val.get_ref<const std::string&>();
        double result = 0;
        if (auto [ptr, ec] = std::from_chars(
                strVal.data(), strVal.data() + strVal.size(), result);
            ec == std::errc())
        {
            return result;
        }
    }
    return std::numeric_limits<double>::quiet_NaN();
}

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
        info("Configured Sensor objects: {SIZE}", "SIZE",
             sensorConfig.mappers.size());
        // Sensor objects are created lazily on first successful read (see
        // pollSensor), so nothing is published here.
        ctx.spawn(runSensorLoop());
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

RedfishClient::~RedfishClient() = default;

auto RedfishClient::readWithRetries(const SensorMapper& mapper)
    -> sdbusplus::async::task<std::optional<redfish_binding::Sensor::Sensor>>
{
    for (size_t i = 0; i < config->sensorConfig.value().maxRetries; ++i)
    {
        std::string sensorJson;
        auto expandedUrl =
            std::format("http://{}{}", config->host, mapper.fromUrl);
        try
        {
            auto& httpHandle = httpHandles[expandedUrl];
            if (!httpHandle)
            {
                httpHandle = std::make_unique<AsyncHttpHandle>(expandedUrl);
            }
            auto response = co_await httpHandle->get(ctx);
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
            co_return redfish_binding::Sensor::parseSensor(sensorJson);
        }
        catch (const std::exception& exn)
        {
            info("Exception while parsing sensor json from ({URL}): {EXC}, {JSON}",
                 "URL", expandedUrl.c_str(), "EXC", exn, "JSON",
                 sensorJson.c_str());
        };

        co_await sdbusplus::async::sleep_for(
            ctx, std::chrono::milliseconds(
                     config->sensorConfig.value().retryIntervalMilliseconds));
    }
    co_return std::nullopt;
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

auto RedfishClient::ingestMetricReport(
    const nlohmann::json& report,
    std::unordered_set<std::string>& reportCovered)
    -> sdbusplus::async::task<>
{
    if (!report.contains("MetricValues") || !report["MetricValues"].is_array())
    {
        co_return;
    }

    for (const auto& metricValue : report["MetricValues"])
    {
        if (!metricValue.contains("MetricProperty") ||
            !metricValue["MetricProperty"].is_string() ||
            !metricValue.contains("MetricValue"))
        {
            continue;
        }

        std::string fromUrl =
            metricValue["MetricProperty"].get_ref<const std::string&>();
        if (size_t hashPos = fromUrl.find('#'); hashPos != std::string::npos)
        {
            fromUrl = fromUrl.substr(0, hashPos);
        }

        auto itSensor = sensors.find(fromUrl);
        if (itSensor != sensors.end())
        {
            // Already published: the report carries only a bare value.
            double val = parseMetricValue(metricValue["MetricValue"]);
            ctx.spawn(itSensor->second->updateValue(val));
            continue;
        }

        // Not yet created. Find the configured mapper for this URL (cold path,
        // happens at most once per sensor) and do a full read to establish the
        // sensor's unit and range, then publish it.
        const SensorMapper* mapper = findMapper(fromUrl);
        if (mapper == nullptr)
        {
            continue;
        }

        // Reports drive this sensor from now on, so stop polling it
        // individually even if this first read fails.
        reportCovered.insert(mapper->fromUrl);

        auto maybeSensor = co_await readWithRetries(*mapper);
        if (!maybeSensor.has_value())
        {
            continue;
        }
        auto sensor = Sensor::create(ctx, deriveObjectPath(*mapper),
                                     config->sensorConfig->associationPath,
                                     maybeSensor.value());
        if (!sensor)
        {
            continue;
        }
        sensors.emplace(mapper->fromUrl, std::move(sensor));
        // The persistent read handle is no longer needed once reports drive
        // this sensor.
        httpHandles.erase(std::format("http://{}{}", config->host,
                                      mapper->fromUrl));
    }
}

const SensorMapper* RedfishClient::findMapper(std::string_view fromUrl) const
{
    const auto& mappers = config->sensorConfig->mappers;
    auto it = std::ranges::find_if(mappers, [&](const auto& mapper) {
        return mapper.fromUrl == fromUrl;
    });
    return it != mappers.end() ? &*it : nullptr;
}

std::string RedfishClient::deriveObjectPath(const SensorMapper& mapper) const
{
    // mapper.toNamespace is validated against the supported namespaces when the
    // config is parsed, so it is used verbatim here.
    return std::string(sensorRootPath) + "/" + mapper.toNamespace + "/" +
           mapper.toId;
}

auto RedfishClient::pollSensor(const SensorMapper& mapper)
    -> sdbusplus::async::task<>
{
    auto maybeSensor = co_await readWithRetries(mapper);
    if (!maybeSensor.has_value())
    {
        co_return;
    }
    auto& parsed = maybeSensor.value();

    auto it = sensors.find(mapper.fromUrl);
    if (it != sensors.end())
    {
        // Already published; push the new reading value.
        double val = parsed.getReading().hasValue()
                         ? parsed.getReading().value()
                         : std::numeric_limits<double>::quiet_NaN();
        ctx.spawn(it->second->updateValue(val));
        co_return;
    }

    auto sensor = Sensor::create(ctx, deriveObjectPath(mapper),
                                 config->sensorConfig->associationPath, parsed);
    if (sensor)
    {
        sensors.emplace(mapper.fromUrl, std::move(sensor));
    }
}

auto RedfishClient::runSensorLoop() -> sdbusplus::async::task<>
{
    info("Running sensor loop");

    const auto& sensorConfig = *config->sensorConfig;
    // Sensors that have appeared in a metric report are driven by reports and
    // are no longer polled individually.
    std::unordered_set<std::string> reportCovered;

    try
    {
        while (!ctx.stop_requested())
        {
            if (sensorConfig.metricReportUrls &&
                !sensorConfig.metricReportUrls->empty())
            {
                const auto& urls = *sensorConfig.metricReportUrls;
                for (const auto& url : urls)
                {
                    std::string expandedUrl =
                        std::format("http://{}{}", config->host, url);
                    try
                    {
                        auto& httpHandle = httpHandles[expandedUrl];
                        if (!httpHandle)
                        {
                            httpHandle =
                                std::make_unique<AsyncHttpHandle>(expandedUrl);
                        }

                        auto response = co_await httpHandle->get(ctx);
                        if (response.code != 200)
                        {
                            throw std::runtime_error(std::format(
                                "Http response error code: {}", response.code));
                        }

                        auto report = nlohmann::json::parse(response.body);
                        co_await ingestMetricReport(report, reportCovered);
                    }
                    catch (const std::exception& exn)
                    {
                        info("Exception while querying url ({URL}): {EXC}",
                             "URL", expandedUrl.c_str(), "EXC", exn);
                    }
                }

                // Individually poll the sensors not (yet) covered by a report.
                for (const auto& mapper : sensorConfig.mappers)
                {
                    if (reportCovered.contains(mapper.fromUrl))
                    {
                        continue;
                    }
                    co_await pollSensor(mapper);
                }
            }
            else
            {
                for (const auto& mapper : sensorConfig.mappers)
                {
                    co_await pollSensor(mapper);
                }
            }
            co_await sdbusplus::async::sleep_for(
                ctx,
                std::chrono::milliseconds(sensorConfig.intervalMilliseconds));
        }
    }
    catch (const std::logic_error& exn)
    {
        debug("Unhandled logic error: {NAME}", "WHAT", exn.what());
    };
    co_return;
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
