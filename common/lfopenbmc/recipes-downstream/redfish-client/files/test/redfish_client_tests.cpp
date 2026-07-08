#include "helper.hpp"

#include <redfish_client/core/redfish_client.hpp>
#include <redfish_client/core/update_service_handler.hpp>
#include <xyz/openbmc_project/Software/Activation/client.hpp>
#include <xyz/openbmc_project/Software/Version/client.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace redfish_client::core;

namespace
{

static constexpr auto kServiceName = "xyz.openbmc_project.test.AService";

static constexpr auto kTestConfigFormat = R"(
  {{
    "host": "localhost:{}",
    "compatible": "com.meta.Hardware.BMC.TEST",
    "updateServiceConfig": {{
      "intervalMilliseconds": 10,
      "firmwareMappers": [
        {{
          "fromId": "FW0",
          "toId": "Host0_FW0"
        }},
        {{
          "fromId": "FW1",
          "toId": "BMC_FW1"
        }}
      ],
      "softwareMappers": [
        {{
          "fromId": "SW0",
          "toId": "CPLD_SW0"
        }}
      ]
    }}
  }}
)";

static constexpr auto kFirmwareInventoryResponse = R"(
{
  "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory",
  "@odata.type": "#SoftwareInventoryCollection.SoftwareInventoryCollection",
  "Members": [
    {
      "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/FW0",
      "@odata.type": "#SoftwareInventory.v1_4_0.SoftwareInventory",
      "Id": "FW0",
      "Version": "FW0-V1"
    },
    {
      "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/FW1",
      "@odata.type": "#SoftwareInventory.v1_4_0.SoftwareInventory",
      "Id": "FW1",
      "Version": "FW1-V2"
    },
    {
      "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/FW2",
      "@odata.type": "#SoftwareInventory.v1_4_0.SoftwareInventory",
      "Id": "FW2",
      "Version": "FW2-V4"
    }
  ],
  "Members@odata.count": 3,
  "Name": "Software Inventory Collection"
}
)";

static constexpr auto kSoftwareInventoryResponse = R"(
{
  "@odata.id": "/redfish/v1/UpdateService/SoftwareInventory",
  "@odata.type": "#SoftwareInventoryCollection.SoftwareInventoryCollection",
  "Members": [
    {
      "@odata.id": "/redfish/v1/UpdateService/SoftwareInventory/SW0",
      "@odata.type": "#SoftwareInventory.v1_4_0.SoftwareInventory",
      "Id": "SW0",
      "Version": "SW-v1.5"
    },
    {
      "@odata.id": "/redfish/v1/UpdateService/SoftwareInventory/SW1",
      "@odata.type": "#SoftwareInventory.v1_4_0.SoftwareInventory",
      "Id": "SW1",
      "Version": "SW-v2.0.0"
    }
  ],
  "Members@odata.count": 2,
  "Name": "Software Inventory Collection"
}
)";

std::string generateResponse(
    const SimpleTestHttpServer::ReceivedHttpRequest& request)
{
    if (request.path.ends_with("/UpdateService/FirmwareInventory?$expand=."))
    {
        return kFirmwareInventoryResponse;
    }
    else if (request.path.ends_with(
                 "/UpdateService/SoftwareInventory?$expand=."))
    {
        return kSoftwareInventoryResponse;
    }
    else
    {
        // Bad response
        return "[]";
    }
}

auto getSoftwareVersionClient(sdbusplus::async::context& ctx, const char* path)
{
    return sdbusplus::client::xyz::openbmc_project::software::Version<>(ctx)
        .service(kServiceName)
        .path(path);
}

auto getSoftwareActivationClient(sdbusplus::async::context& ctx,
                                 const char* path)
{
    return sdbusplus::client::xyz::openbmc_project::software::Activation<>(ctx)
        .service(kServiceName)
        .path(path);
}

} // namespace

TEST(RedfishClientTests, SimpleRun)
{
    sdbusplus::async::context ctx;
    std::unordered_map<std::string, std::string> responseHeaders;
    SimpleTestHttpServer server(generateResponse, responseHeaders);
    auto configJson = std::format(kTestConfigFormat, server.getPort());
    auto config = Config::parse(configJson);
    Software::randomIdGenerator() = []() { return 1234; };
    auto clientThread = std::make_unique<std::thread>([&ctx, &config]() {
        ctx.request_name(kServiceName);
        RedfishClient client(ctx, config, /*persistDir=*/"");
        ctx.spawn(client.run());
        ctx.run();
    });

    ctx.spawn([&server](
                  sdbusplus::async::context& ctx) -> sdbusplus::async::task<> {
        // First wait for the request count to go up to a large enough number.
        // This gives the client enough time to publish the software objects to
        // dbus.
        static constexpr auto kMaxRequests = 50;
        static constexpr auto kSleepMilliseconds = 100;
        while (server.getReceivedRequests().size() < kMaxRequests)
        {
            co_await sdbusplus::async::sleep_for(
                ctx, std::chrono::milliseconds(kSleepMilliseconds));
        }

        constexpr auto fw0Path = "/xyz/openbmc_project/software/Host0_FW0_1234";
        auto fw0VersionClient = getSoftwareVersionClient(ctx, fw0Path);
        EXPECT_EQ("FW0-V1", co_await fw0VersionClient.version());
        EXPECT_EQ(SoftwareVersion::VersionPurpose::Other,
                  co_await fw0VersionClient.purpose());
        auto fw0ActivationClient = getSoftwareActivationClient(ctx, fw0Path);
        EXPECT_EQ(SoftwareActivation::Activations::Active,
                  co_await fw0ActivationClient.activation());
        EXPECT_EQ(SoftwareActivation::RequestedActivations::None,
                  co_await fw0ActivationClient.requested_activation());

        constexpr auto fw1Path = "/xyz/openbmc_project/software/BMC_FW1_1234";
        auto fw1VersionClient = getSoftwareVersionClient(ctx, fw1Path);
        EXPECT_EQ("FW1-V2", co_await fw1VersionClient.version());
        EXPECT_EQ(SoftwareVersion::VersionPurpose::Other,
                  co_await fw1VersionClient.purpose());
        auto fw1ActivationClient = getSoftwareActivationClient(ctx, fw1Path);
        EXPECT_EQ(SoftwareActivation::Activations::Active,
                  co_await fw1ActivationClient.activation());
        EXPECT_EQ(SoftwareActivation::RequestedActivations::None,
                  co_await fw1ActivationClient.requested_activation());

        constexpr auto fw2Path = "/xyz/openbmc_project/software/FW2_1234";
        auto fw2VersionClient = getSoftwareVersionClient(ctx, fw2Path);
        EXPECT_THROW(co_await fw2VersionClient.version(),
                     sdbusplus::exception::SdBusError);
        EXPECT_THROW(co_await fw2VersionClient.purpose(),
                     sdbusplus::exception::SdBusError);
        auto fw2ActivationClient = getSoftwareActivationClient(ctx, fw2Path);
        EXPECT_THROW(co_await fw2ActivationClient.activation(),
                     sdbusplus::exception::SdBusError);
        EXPECT_THROW(co_await fw2ActivationClient.requested_activation(),
                     sdbusplus::exception::SdBusError);

        constexpr auto sw0Path = "/xyz/openbmc_project/software/CPLD_SW0_1234";
        auto sw0VersionClient = getSoftwareVersionClient(ctx, sw0Path);
        EXPECT_EQ("SW-v1.5", co_await sw0VersionClient.version());
        EXPECT_EQ(SoftwareVersion::VersionPurpose::Other,
                  co_await sw0VersionClient.purpose());
        auto sw0ActivationClient = getSoftwareActivationClient(ctx, sw0Path);
        EXPECT_EQ(SoftwareActivation::Activations::Active,
                  co_await sw0ActivationClient.activation());
        EXPECT_EQ(SoftwareActivation::RequestedActivations::None,
                  co_await sw0ActivationClient.requested_activation());

        constexpr auto sw1Path = "/xyz/openbmc_project/software/SW1_1234";
        auto sw1VersionClient = getSoftwareVersionClient(ctx, sw1Path);
        EXPECT_THROW(co_await sw1VersionClient.version(),
                     sdbusplus::exception::SdBusError);
        EXPECT_THROW(co_await sw1VersionClient.purpose(),
                     sdbusplus::exception::SdBusError);
        auto sw1ActivationClient = getSoftwareActivationClient(ctx, sw1Path);
        EXPECT_THROW(co_await sw1ActivationClient.activation(),
                     sdbusplus::exception::SdBusError);
        EXPECT_THROW(co_await sw1ActivationClient.requested_activation(),
                     sdbusplus::exception::SdBusError);

        ctx.request_stop();
        co_return;
    }(ctx));
    clientThread->join();
    clientThread = nullptr; // Make sure it's destroyed before the context.
}
