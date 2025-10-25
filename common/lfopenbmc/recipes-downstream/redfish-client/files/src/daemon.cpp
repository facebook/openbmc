#include <redfish_client/daemon.hpp>

#include <redfish_client/async_http_client.hpp>
#include <redfish_client/log_service_handler.hpp>
#include <redfish_client/update_service_handler.hpp>

#include <boost/stacktrace.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>

#include <csignal>
#include <fstream>
#include <streambuf>

PHOSPHOR_LOG2_USING;

namespace redfish_client_daemon
{

using PathIntf = ValueIntf::namespace_path;

class ServerObjectIntf;
using SensorInterfaces = sdbusplus::async::server_t<
    ServerObjectIntf,
    sdbusplus::aserver::xyz::openbmc_project::association::Definitions,
    sdbusplus::aserver::xyz::openbmc_project::sensor::Value>;

class ServerObjectIntf : public SensorInterfaces
{
  public:
    ServerObjectIntf(sdbusplus::async::context& ctx, const char* path) :
        SensorInterfaces(ctx, path)
    {}

    void emit_added()
    {
        Definitions::emit_added();
        Value::emit_added();
    }
};

// Source:
// https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Sensor/Value.interface.yaml

std::optional<ValueIntf::Unit> toMaybeIntfUnits(const std::string& unitsStr)
{
    // The strings used in this function and the mapping to items in the
    // ValueIntf::Unit enumeration were obtained experimentally and should be
    // extended as needed.

    if (unitsStr == "%")
    {
        return ValueIntf::Unit::Percent;
    }

    if (unitsStr == "Cel")
    {
        return ValueIntf::Unit::DegreesC;
    }

    if (unitsStr == "J")
    {
        return ValueIntf::Unit::Joules;
    }

    if (unitsStr == "Pa")
    {
        return ValueIntf::Unit::Pascals;
    }

    if (unitsStr == "V")
    {
        return ValueIntf::Unit::Volts;
    }

    if (unitsStr == "W")
    {
        return ValueIntf::Unit::Watts;
    }

    return std::nullopt;
}

// Source:
// https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Sensor/Value.interface.yaml

const char* getActualMetricNamespace(const char* logicalNameParam)
{
    // This function is an identify function for the time being, but
    // having a lookup table infrastructure makes it possible for us to
    // change the values supported in the interface without worrying about
    // the values actually emitted by the redfish server.

    std::string logicalName(logicalNameParam);
    if (logicalName == "airflow")
    {
        return PathIntf::airflow;
    }
    if (logicalName == "altitude")
    {
        return PathIntf::altitude;
    }
    if (logicalName == "current")
    {
        return PathIntf::current;
    }
    if (logicalName == "energy")
    {
        return PathIntf::energy;
    }
    if (logicalName == "fan_tach")
    {
        return PathIntf::fan_tach;
    }
    if (logicalName == "humidity")
    {
        return PathIntf::humidity;
    }
    if (logicalName == "liquidflow")
    {
        return PathIntf::liquidflow;
    }
    if (logicalName == "power")
    {
        return PathIntf::power;
    }
    if (logicalName == "pressure")
    {
        return PathIntf::pressure;
    }
    if (logicalName == "temperature")
    {
        return PathIntf::temperature;
    }
    if (logicalName == "utilization")
    {
        return PathIntf::utilization;
    }
    if (logicalName == "voltage")
    {
        return PathIntf::voltage;
    }
    throw std::invalid_argument(logicalName.c_str());
}

const char* getSensorRootPath()
{
    return PathIntf::value;
}

struct SensorDbusObject
{
    SensorDbusObject() = delete;
    SensorDbusObject(const SensorDbusObject&) = delete;
    SensorDbusObject(SensorDbusObject&&) = delete;

    SensorDbusObject(sdbusplus::async::context& ctx, const char* metricPath,
                     const SensorMapper& mapper,
                     const std::string& associationPath) :
        ctx(ctx), metricPath(metricPath), mapper(mapper),
        associationPath(associationPath)
    {}

    auto update(Sensor sensor) -> sdbusplus::async::task<>
    {
        // Uncomment this to debug. If we leave this uncommented in the code
        // then phosphor logging always prints these in TTY mode and that makes
        // manual testing a bit difficult.
        //
        // Note:
        // https://github.com/openbmc/phosphor-logging/blob/master/docs/structured-logging.md
        // "The lg2 APIs detect if the application is running on a TTY and
        // additionally log to the TTY."
        //
        // debug("Updating metric {NAME}", "NAME", metricPath.c_str());
        std::lock_guard<std::mutex> guard(this->lock);
        bool addedThisTime = false;

        if (object == nullptr)
        {
            // Don't signal the bus while the object is getting created.
            constexpr bool emitSignal = false;
            object =
                std::make_unique<ServerObjectIntf>(ctx, metricPath.c_str());

            if (!associationPath.empty())
            {
                std::vector<std::tuple<std::string, std::string, std::string>>
                    associations;
                associations.emplace_back("chassis", "all_sensors",
                                          associationPath);
                object->associations<emitSignal>(associations);
            }

            auto maybeUnit = toMaybeIntfUnits(sensor.getSensorUnitText());
            if (maybeUnit.has_value())
            {
                object->unit<emitSignal>(maybeUnit.value());
            }
            else
            {
                // Initialize it to something, otherwise we see errors on some
                // OS versions.
                object->unit<emitSignal>(ValueIntf::Unit::Amperes);
            }

            object->value<emitSignal>(std::numeric_limits<double>::quiet_NaN());
            assert(std::isnan(lastNotifiedValue));
            addedThisTime = true;
        }

        if (!minValueNotified)
        {
            auto minValue = sensor.getMinValue();
            if (minValue.has_value())
            {
                object->min_value<false>(minValue.value());
                minValueNotified = true;
            }
        }
        if (!maxValueNotified)
        {
            auto maxValue = sensor.getMaxValue();
            if (maxValue.has_value())
            {
                object->max_value<false>(maxValue.value());
                maxValueNotified = true;
            }
        }

        double current = sensor.getReading();

        const bool emitSignal = !shouldSkipSignal(current);
        if (emitSignal)
        {
            lastNotifiedValue = current;
        }
        if (emitSignal && !addedThisTime)
        {
            object->value<true>(current);
        }
        else
        {
            object->value<false>(current);
        }
        if (addedThisTime)
        {
            object->emit_added();
        }
        co_return;
    }

    auto shouldSkipSignal(double current) -> bool
    {
        if (std::isnan(lastNotifiedValue) && std::isnan(current))
        {
            return true;
        }
        if (std::isnan(lastNotifiedValue) && !(std::isnan(current)))
        {
            return false;
        }
        if ((!std::isnan(lastNotifiedValue)) && std::isnan(current))
        {
            return false;
        }
        assert(!std::isnan(lastNotifiedValue));
        assert(!std::isnan(current));
        if (std::abs(lastNotifiedValue) < 1e-6)
        {
            // avoid division by zero
            return true;
        }
        auto changed =
            std::abs((current - lastNotifiedValue) / lastNotifiedValue * 100.0);
        if (changed >= 1.0)
        {
            return false;
        }
        return true;
    }

    sdbusplus::async::context& ctx;
    std::mutex lock;
    std::string metricPath;
    std::unique_ptr<ServerObjectIntf> object;
    double lastNotifiedValue = std::numeric_limits<double>::quiet_NaN();
    bool minValueNotified = false;
    bool maxValueNotified = false;
    SensorMapper mapper;
    const std::string& associationPath;
};

class RedfishClient
{
  public:
    RedfishClient() = delete;

    RedfishClient(sdbusplus::async::context& ctx, const std::string& configDir,
                  const std::string& persistDir) :
        ctx(ctx), configDir(configDir), persistDir(persistDir)
    {}

    RedfishClient(sdbusplus::async::context& ctx, const Config& config,
                  const std::string& persistDir) :
        ctx(ctx), config(config), persistDir(persistDir)
    {}

    auto run() -> sdbusplus::async::task<>
    {
        info("Running RedfishClient");
        if (!config.has_value())
        {
            co_await loadConfig();
        }
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
                    std::make_shared<LogServiceHandler>(ctx, expandedUrl,
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

    ~RedfishClient()
    {
        if (sensorThread.joinable())
        {
            sensorThread.join();
        }
    }

  private:
    std::optional<Sensor> readWithRetries(const SensorMapper& mapper)
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
                info("Exception while querying url {EXC}", "EXC", exn.what());
                debug("Exception while querying url: {URL}", "URL",
                      expandedUrl.c_str());
            };

            try
            {
                return Sensor::parseSensor(sensorJson);
            }
            catch (const std::exception& exn)
            {
                info("Exception while parsing sensor json: {EXC}", "EXC",
                     exn.what());
                debug("Exception while parsing sensor json: {JSON}", "JSON",
                      sensorJson.c_str());
            };

            std::this_thread::sleep_for(std::chrono::milliseconds(
                config->sensorConfig.value().retryIntervalMilliseconds));
        }
        return std::nullopt;
    }

    auto runEventPollingLoop() -> sdbusplus::async::task<>
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

    void runSensorLoop()
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

    auto loadConfig() -> sdbusplus::async::task<>
    {
        auto compatiblePlatformName = co_await getCompatiblePlatformName();
        config = loadCompatibleConfig(configDir, compatiblePlatformName);
        co_return;
    }

    Config loadCompatibleConfig(const std::string& configDir,
                                const std::string& compatiblePlatformName)
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

            if (config.compatible == compatiblePlatformName)
            {
                info("Matched config file: {FILE}", "FILE",
                     entry.path().string());
                return config;
            }
        }
        error("No matching config file found for platform: {PLATFORM}",
              "PLATFORM", compatiblePlatformName);
        throw std::runtime_error("No matching config file found");
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

    auto subtree_for_target_interface(
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

    auto getCompatiblePlatformName() -> sdbusplus::async::task<std::string>
    {
        std::string platformName;
        while (!ctx.stop_requested() && platformName.empty())
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
                        if (!names.empty())
                        {
                            platformName = names[0];
                            info("Compatible : Names : {PLATFORMNAME}",
                                 "PLATFORMNAME", platformName);
                        }
                        co_return;
                    },
                    1);

                if (platformName.empty())
                {
                    info("platformName is still empty. Retry again");
                }
            }
            catch (const sdbusplus::exception::SdBusError& e)
            {
                info("subtree query failed: {ERR}", "ERR", e.what());
            }
            co_await sdbusplus::async::sleep_for(
                ctx, std::chrono::milliseconds(500));
        }
        co_return platformName;
    }

    sdbusplus::async::context& ctx;
    std::unordered_map<std::string, std::shared_ptr<SensorDbusObject>> metrics;
    std::vector<std::shared_ptr<LogServiceHandler>> logServiceHandlers;
    std::string configDir;
    std::optional<Config> config;
    std::thread sensorThread;
    std::string persistDir;
    std::unordered_map<std::string, std::unique_ptr<AsyncHttpHandle>>
        httpHandles;
};

void installSignalHandlers()
{
    auto printStackTraceOnCrashHandler = [](int signal) {
        boost::stacktrace::stacktrace st;
        std::string stacktrace_str = boost::stacktrace::to_string(st);
        fprintf(stderr, "Uncaught exception:\n%s\n", stacktrace_str.c_str());
        _exit(signal);
    };
    std::signal(SIGSEGV, printStackTraceOnCrashHandler);
    std::signal(SIGABRT, printStackTraceOnCrashHandler);
}

void runRedfishClient(const std::string& serviceName,
                      sdbusplus::async::context& ctx,
                      const std::string configDir, std::string persistDir)
{
    ctx.request_name(serviceName.c_str());
    sdbusplus::server::manager_t manager{ctx, getSensorRootPath()};
    RedfishClient client(ctx, configDir, persistDir);
    ctx.spawn(client.run());
    ctx.run();
}

void runRedfishClient(const std::string& serviceName,
                      sdbusplus::async::context& ctx, const Config& config,
                      std::string persistDir)
{
    ctx.request_name(serviceName.c_str());
    sdbusplus::server::manager_t manager{ctx, getSensorRootPath()};
    RedfishClient client(ctx, config, persistDir);
    ctx.spawn(client.run());
    ctx.run();
}

struct SensorDbusObjectForTest : public ISensorDbusObject
{
    SensorDbusObjectForTest() = delete;
    SensorDbusObjectForTest(const SensorDbusObjectForTest&) = delete;
    SensorDbusObjectForTest(SensorDbusObjectForTest&&) = delete;

    SensorDbusObjectForTest(sdbusplus::async::context& ctx,
                            const char* metricPath, const SensorMapper& mapper,
                            const std::string& associationPath) :
        ctx(ctx), innerObject(std::make_shared<SensorDbusObject>(
                      ctx, metricPath, mapper, associationPath))
    {}

    sdbusplus::async::task<> update(Sensor sensor) override
    {
        return innerObject->update(sensor);
    }

    sdbusplus::async::context& ctx;
    std::shared_ptr<SensorDbusObject> innerObject;
};

std::shared_ptr<ISensorDbusObject> createSensorDbusObjectForTest(
    sdbusplus::async::context& ctx, const char* metricPath,
    const std::string& associationPath)
{
    SensorMapper fakeMapper;
    return std::make_shared<SensorDbusObjectForTest>(
        ctx, metricPath, fakeMapper, associationPath);
}

} // namespace redfish_client_daemon
