#include "redfish-binding/Sensor_Sensor.hpp"
#include "redfish-binding/ServiceRoot_ServiceRoot.hpp"

#include <gtest/gtest.h>

TEST(RedfishBindingTest, ParseErrorTest)
{
  std::string invalidJson = "{invalid=";
  auto res = redfish_binding::Sensor::tryParseSensor(invalidJson);
  EXPECT_EQ(res.has_value(), false);
}

TEST(RedfishBindingTest, ParseSensorTest)
{
    std::string sensorJson = R"(
  {
    "Reading": 1.5,
    "ReadingUnits": "Cel",
    "Name": "Test_Sensor",
    "Status": {
        "State": "Enabled"
    }
  }
)";
    auto sensor = redfish_binding::Sensor::parseSensor(sensorJson);
    EXPECT_EQ(sensor.getReading().value(), 1.5);
    EXPECT_EQ(sensor.getReadingUnits().value(), "Cel");
    EXPECT_EQ(sensor.getName().value(), "Test_Sensor");
    EXPECT_EQ(sensor.getStatus().value().getState().value(),
              redfish_binding::Resource::State::Enabled);
}

TEST(RedfishBindingTest, ParseServiceRootTest)
{
    std::string serviceRootJson = R"(
{
  "@odata.id": "/redfish/v1",
  "@odata.type": "#ServiceRoot.v1_15_0.ServiceRoot",
  "Chassis": {
    "@odata.id": "/redfish/v1/Chassis"
  },
  "ComponentIntegrity": {
    "@odata.id": "/redfish/v1/ComponentIntegrity"
  },
  "EventService": {
    "@odata.id": "/redfish/v1/EventService"
  },
  "Fabrics": {
    "@odata.id": "/redfish/v1/Fabrics"
  },
  "Id": "RootService",
  "JsonSchemas": {
    "@odata.id": "/redfish/v1/JsonSchemas"
  },
  "Links": {
    "ManagerProvidingService": {
      "@odata.id": "/redfish/v1/Managers/HGX_BMC_0"
    }
  },
  "Managers": {
    "@odata.id": "/redfish/v1/Managers"
  },
  "Name": "Root Service",
  "Product": "P2312-A04",
  "ProtocolFeaturesSupported": {
    "DeepOperations": {
      "DeepPATCH": false,
      "DeepPOST": false
    },
    "ExcerptQuery": false,
    "ExpandQuery": {
      "ExpandAll": true,
      "Levels": true,
      "Links": true,
      "MaxLevels": 6,
      "NoLinks": true
    },
    "FilterQuery": false,
    "OnlyMemberQuery": true,
    "SelectQuery": true
  },
  "RedfishVersion": "1.9.0",
  "Registries": {
    "@odata.id": "/redfish/v1/Registries"
  },
  "ServiceConditions": {
    "@odata.id": "/redfish/v1/ServiceConditions"
  },
  "Systems": {
    "@odata.id": "/redfish/v1/Systems"
  },
  "Tasks": {
    "@odata.id": "/redfish/v1/TaskService"
  },
  "TelemetryService": {
    "@odata.id": "/redfish/v1/TelemetryService"
  },
  "UUID": "ff3c39e4-bd7a-4bb4-94e7-ac3e497e4727",
  "UpdateService": {
    "@odata.id": "/redfish/v1/UpdateService"
  },
  "Vendor": "NVIDIA"
}
)";
    auto serviceRoot =
        redfish_binding::ServiceRoot::parseServiceRoot(serviceRootJson);
    EXPECT_EQ(serviceRoot.getVendor().value(), "NVIDIA");
    EXPECT_EQ(serviceRoot.getUUID().value(),
              "ff3c39e4-bd7a-4bb4-94e7-ac3e497e4727");
    EXPECT_EQ(serviceRoot.getName().value(), "Root Service");
    EXPECT_EQ(serviceRoot.getProduct().value(), "P2312-A04");
    EXPECT_EQ(serviceRoot.getSystems().value().getOdataId().value(),
              "/redfish/v1/Systems");
}
