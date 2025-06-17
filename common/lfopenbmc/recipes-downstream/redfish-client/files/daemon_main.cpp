#include "daemon.hpp"

#include <stdio.h>

#include <CLI/CLI.hpp>
#include <phosphor-logging/lg2.hpp>

#include <exception>
#include <fstream>
#include <optional>
#include <streambuf>
#include <string>
#include <thread>

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
    std::ifstream fileStream(configPath);
    std::string jsonContents((std::istreambuf_iterator<char>(fileStream)),
                             std::istreambuf_iterator<char>());

    auto daemonConfig = DaemonConfig::fromJson(jsonContents.c_str());

    // Create a new scope to ensure the context is destroyed cleanly before
    // http client is destroyed.
    {
        sdbusplus::async::context ctx;
        runDbusServerTillInterrupted(daemonConfig, ctx, persistDir);
    }

    info("daemon-main clean exit\n");
    return 0;
}
