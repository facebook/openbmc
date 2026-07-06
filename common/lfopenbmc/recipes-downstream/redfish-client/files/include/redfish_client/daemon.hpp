#pragma once

#include <redfish_client/core/config.hpp>
#include <redfish_client/core/sensor.hpp>

#include <sdbusplus/async.hpp>
#include <sdbusplus/async/task.hpp>
#include <sdbusplus/async/context.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace redfish_client_daemon
{

using namespace redfish_client::core;

void installSignalHandlers();

// Run redfish client till process receives SIGINT or SIGTERM
void runRedfishClient(const std::string& serviceName,
                      sdbusplus::async::context& ctx,
                      const std::string configDir, std::string persistDir = "");

void runRedfishClient(const std::string& serviceName,
                      sdbusplus::async::context& ctx, const Config& config,
                      std::string persistDir = "");

} // namespace redfish_client_daemon
