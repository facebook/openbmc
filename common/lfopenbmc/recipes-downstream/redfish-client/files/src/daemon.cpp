#include <redfish_client/daemon.hpp>

#include <redfish_client/core/async_http_client.hpp>
#include <redfish_client/core/log_service_handler.hpp>
#include <redfish_client/core/update_service_handler.hpp>
#include <redfish_client/core/redfish_client.hpp>

#include <boost/stacktrace.hpp>

#include <csignal>
#include <fstream>
#include <streambuf>

namespace redfish_client_daemon
{

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
