#include "daemon.hpp"
#include "helper.hpp"

#include <xyz/openbmc_project/Sensor/Value/client.hpp>

#include <chrono>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace redfish_client_daemon;

namespace
{

static constexpr auto kBusName = "xyz.openbmc_project.test.AService";

static constexpr auto kTestConfigFormat = R"(
  {{
    "service_name": "xyz.openbmc_project.test.AService",
    "host_port": "localhost:{}",
    "interval_milliseconds": 10,
    "retries": 1,
    "wait_milliseconds": 1,
    "sensor_configs": {{
      "/Host0_Temp0": {{
        "url": "/redfish/v1/Chassis/Host0/Sensors/Temp0",
        "symbolic_namespace": "temperature"
      }},
      "/Host0_Temp1": {{
        "url": "/redfish/v1/Chassis/Host0/Sensors/Temp1",
        "symbolic_namespace": "temperature"
      }},
      "/Host0_Pressure0": {{
        "url": "/redfish/v1/Chassis/Host0/Sensors/Pressure0",
        "symbolic_namespace": "pressure"
      }}
    }}
  }}
)";

static constexpr auto kTestResponse0 = R"(
  {
    "Name": "Unused",
    "Reading": 100.0,
    "ReadingType": "Temperature",
    "ReadingUnits": "Cel",
    "Status": {
      "State": "Enabled"
    }
  }
)";

static constexpr auto kTestResponse1 = R"(
  {
    "Name": "Unused",
    "Reading": 200.0,
    "ReadingType": "Temperature",
    "ReadingUnits": "Cel",
    "Status": {
      "State": "Enabled"
    }
  }
)";

std::string generateResponse(
    const SimpleTestHttpServer::ReceivedHttpRequest& request)
{
    if (request.path.ends_with("/Temp0"))
    {
        return kTestResponse0;
    }
    else if (request.path.ends_with("/Temp1"))
    {
        return kTestResponse1;
    }
    else
    {
        // Bad response
        return "[]";
    }
}

auto getSensorClient(sdbusplus::async::context& ctx, const char* metricPath)
{
    using ValueClient =
        sdbusplus::client::xyz::openbmc_project::sensor::Value<>;
    return ValueClient(ctx).service(kBusName).path(metricPath);
}

} // namespace

TEST(DBusServerTests, SimpleDaemonRun)
{
    installSignalHandlers();
    sdbusplus::async::context ctx;
    std::unordered_map<std::string, std::string> responseHeaders;
    SimpleTestHttpServer server(generateResponse, responseHeaders);
    auto config = std::format(kTestConfigFormat, server.getPort());

    auto daemonThread = std::make_unique<std::thread>([&ctx, &config]() {
        auto daemonConfig = DaemonConfig::fromJson(config);
        runDbusServerTillInterrupted(daemonConfig, ctx);
    });

    ctx.spawn(
        [&server](sdbusplus::async::context& ctx) -> sdbusplus::async::task<> {
            // First wait for the getBodyCount value to go up to a large enough
            // number. This gives the test daemon enough time to write those
            // sensor values to dbus.
            static constexpr auto kMaxRequests = 50;
            static constexpr auto kSleepMilliseconds = 100;
            while (server.getReceivedRequests().size() < kMaxRequests)
            {
                co_await sdbusplus::async::sleep_for(
                    ctx, std::chrono::milliseconds(kSleepMilliseconds));
            }

            auto sensorClient0 = getSensorClient(
                ctx, "/xyz/openbmc_project/sensors/temperature/Host0_Temp0");
            EXPECT_EQ(100.0, co_await sensorClient0.value());

            auto sensorClient1 = getSensorClient(
                ctx, "/xyz/openbmc_project/sensors/temperature/Host0_Temp1");
            EXPECT_EQ(200.0, co_await sensorClient1.value());

            // Source sends bad json for this, so we should not be able to read
            // it.
            auto sensorClient2 = getSensorClient(
                ctx, "/xyz/openbmc_project/sensors/pressure/Host0_Pressure0");
            EXPECT_THROW(co_await sensorClient2.value(),
                         sdbusplus::exception::SdBusError);

            ctx.request_stop();
            co_return;
        }(ctx));
    daemonThread->join();
    daemonThread = nullptr; // Make sure it's destroyed before the context.
}
