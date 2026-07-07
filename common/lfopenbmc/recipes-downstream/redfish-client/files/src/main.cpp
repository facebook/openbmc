#include <stdio.h>
#include <unistd.h>

#include <CLI/CLI.hpp>
#include <boost/stacktrace.hpp>
#include <phosphor-logging/lg2.hpp>
#include <redfish_client/core/redfish_client.hpp>

#include <csignal>
#include <string>

PHOSPHOR_LOG2_USING;

using namespace redfish_client::core;

namespace
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

} // namespace

int main(int argc, const char** argv)
{
    std::string configDir;
    std::string persistDir;
    CLI::App app{"Redfish client talking to the SMC"};
    app.add_option("config-dir", configDir, "config file directory")
        ->required();
    app.add_option("-p,--persist-dir", persistDir,
                   "directory where persist data can be stored");
    CLI11_PARSE(app, argc, argv);

    installSignalHandlers();

    // New scope so the context is destroyed cleanly before the http client is.
    {
        const std::string kServiceName = "xyz.openbmc_project.RedfishClient";
        sdbusplus::async::context ctx;
        ctx.request_name(kServiceName.c_str());
        RedfishClient client(ctx, configDir, persistDir);
        ctx.spawn(client.run());
        ctx.run();
    }

    info("redfish client clean exit\n");
    return 0;
}
