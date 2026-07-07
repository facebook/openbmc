#include <redfish_client/daemon.hpp>

#include <redfish_client/core/async_http_client.hpp>
#include <redfish_client/core/log_service_handler.hpp>
#include <redfish_client/core/update_service_handler.hpp>
#include <redfish_client/core/redfish_client.hpp>

#include <phosphor-logging/lg2.hpp>
#include <boost/stacktrace.hpp>

#include <csignal>

PHOSPHOR_LOG2_USING;

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
    sdbusplus::server::manager_t manager{ctx, Sensor::rootPath};
    RedfishClient client(ctx, configDir, persistDir);
    ctx.spawn(client.run());
    ctx.run();
}

void runRedfishClient(const std::string& serviceName,
                      sdbusplus::async::context& ctx, const Config& config,
                      std::string persistDir)
{
    ctx.request_name(serviceName.c_str());
    sdbusplus::server::manager_t manager{ctx, Sensor::rootPath};
    RedfishClient client(ctx, config, persistDir);
    ctx.spawn(client.run());
    ctx.run();
}

} // namespace redfish_client_daemon
