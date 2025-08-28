#include "daemon.hpp"

#include <stdio.h>

#include <CLI/CLI.hpp>
#include <phosphor-logging/lg2.hpp>

#include <exception>
#include <optional>
#include <string>

PHOSPHOR_LOG2_USING;

using namespace redfish_client_daemon;

int main(int argc, const char** argv)
{
    std::string configPath;
    std::string persistDir;
    CLI::App app{"daemon that runs as a redfish client talking to SMC"};
    app.add_option("config", configPath, "config file path")->required();
    app.add_option("-p,--persist-dir", persistDir,
                   "directory where persist data can be stored");
    CLI11_PARSE(app, argc, argv);
    installSignalHandlers();
    // Create a new scope to ensure the context is destroyed cleanly before
    // http client is destroyed.
    {
        const std::string kServiceName = "xyz.openbmc_project.RedfishClient";
        sdbusplus::async::context ctx;
        runRedfishClient(kServiceName, ctx, configPath, persistDir);
    }

    info("redfish client clean exit\n");
    return 0;
}
