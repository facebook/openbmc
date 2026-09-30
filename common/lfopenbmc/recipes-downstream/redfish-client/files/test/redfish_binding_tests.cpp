#include "redfish-binding/LogEntryCollection_LogEntryCollection.hpp"
#include "redfish-binding/Sensor_Sensor.hpp"
#include "redfish-binding/ServiceRoot_ServiceRoot.hpp"
#include "redfish-binding/common.hpp"

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

TEST(EventLogParserTests, ZeroEntriesTest)
{
    static std::string kEventlogEntryCollectionJson = R"(
{
  "@odata.id": "/redfish/v1/Systems/SomeBaseBoard/LogServices/EventLog/Entries",
  "@odata.type": "#LogEntryCollection.LogEntryCollection",
  "Description": "Collection of System Event Log Entries",
  "Members@odata.count": 0,
  "Members": [
  ],
  "Name": "System Event Log Entries"
}
)";
    auto coll = redfish_binding::LogEntryCollection::parseLogEntryCollection(
        kEventlogEntryCollectionJson);
    EXPECT_STREQ(
        "/redfish/v1/Systems/SomeBaseBoard/LogServices/EventLog/Entries",
        coll.getOdataId().value().c_str());
    EXPECT_STREQ("#LogEntryCollection.LogEntryCollection",
                 coll.getOdataType().value().c_str());
    EXPECT_STREQ("Collection of System Event Log Entries",
                 coll.getDescription().value().c_str());
    EXPECT_STREQ("System Event Log Entries", coll.getName().value().c_str());
    EXPECT_EQ(0, coll.getMembersOdataCount().value());

    EXPECT_FALSE(coll.getOem().hasValue());
    EXPECT_FALSE(coll.getOdataContext().hasValue());
    EXPECT_FALSE(coll.getOdataEtag().hasValue());

    auto& maybeMembers = coll.getMembers();
    EXPECT_TRUE(maybeMembers.hasValue());
    EXPECT_EQ(0, maybeMembers.value().size());
}

TEST(EventLogParserTests, WithEntriesTest)
{
    static std::string kEventlogEntryCollectionJson = R"(
{
  "@odata.id": "/redfish/v1/Systems/SomeBaseBoard/LogServices/EventLog/Entries",
  "@odata.type": "#LogEntryCollection.LogEntryCollection",
  "Members@odata.count": 2,
  "Members": [
    {
      "@odata.id": "/redfish/v1/Systems/SomeBaseBoard/LogServices/EventLog/Entries/102",
      "@odata.type": "#LogEntry.v1_15_0.LogEntry",
      "Created": "2025-01-09T06:32:02+00:00",
      "Modified": "2025-01-14T20:55:46+00:00",
      "EntryType": "Event",
      "Id": "102",
      "MessageId": "Some message id 1.",
      "Message": "Some message 1.",
      "MessageArgs": [
        "message args 0",
        "message args 1"
      ],
      "Name": "System Event Log Entry",
      "Resolution": "Some resolution.",
      "Resolved": false,
      "Severity": "Warning"
    },
    {
      "@odata.id": "/redfish/v1/Systems/SomeBaseBoard/LogServices/EventLog/Entries/103",
      "@odata.type": "#LogEntry.v1_15_0.LogEntry",
      "Created": "2025-01-14T21:12:42+00:00",
      "EntryType": "Event",
      "Id": "103",
      "MessageId": "Some message id 2.",
      "Message": "Some message 2.",
      "Resolved": true,
      "Resolution": "Some resolution 2.",
      "Severity": "Critical",
      "CPER": {
        "NotificationType": "some notification type",
        "SectionType": "some section type",
        "Oem": {
          "some_key": "some_value"
        }
      }
    }
  ]
}
)";
    static constexpr int kExpectedEventCount = 2;
    auto coll = redfish_binding::LogEntryCollection::parseLogEntryCollection(
        kEventlogEntryCollectionJson);
    auto& maybeMembers = coll.getMembers();
    EXPECT_TRUE(maybeMembers.hasValue());
    EXPECT_EQ(kExpectedEventCount, coll.getMembersOdataCount().value());
    EXPECT_EQ(kExpectedEventCount, maybeMembers.value().size());

    // Validate entry at index 0.
    {
        auto& member = maybeMembers.value()[0];
        EXPECT_STREQ(
            "/redfish/v1/Systems/SomeBaseBoard/LogServices/EventLog/Entries/102",
            member.getOdataId().value().c_str());
        EXPECT_STREQ("#LogEntry.v1_15_0.LogEntry",
                     member.getOdataType().value().c_str());
        EXPECT_STREQ("2025-01-09T06:32:02+00:00",
                     member.getCreated().value().c_str());
        EXPECT_STREQ("2025-01-14T20:55:46+00:00",
                     member.getModified().value().c_str());
        EXPECT_EQ(redfish_binding::LogEntry::LogEntryType::Event,
                  member.getEntryType().value());
        EXPECT_STREQ("102", member.getId().value().c_str());
        EXPECT_STREQ("System Event Log Entry",
                     member.getName().value().c_str());
        EXPECT_STREQ("Some resolution.",
                     member.getResolution().value().c_str());
        EXPECT_FALSE(member.getResolved().value());
        EXPECT_EQ(redfish_binding::LogEntry::EventSeverity::Warning,
                  member.getSeverity().value());
        EXPECT_STREQ("Some message id 1.",
                     member.getMessageId().value().c_str());
        EXPECT_STREQ("Some message 1.", member.getMessage().value().c_str());
        EXPECT_EQ(2, member.getMessageArgs().value().size());
        EXPECT_STREQ("message args 0",
                     member.getMessageArgs().value()[0].c_str());
        EXPECT_STREQ("message args 1",
                     member.getMessageArgs().value()[1].c_str());
        EXPECT_FALSE(member.getOem().hasValue());
        EXPECT_FALSE(member.getCPER().hasValue());
    }

    // Validate entry at index 1.
    {
        auto& member = maybeMembers.value()[1];
        EXPECT_STREQ(
            "/redfish/v1/Systems/SomeBaseBoard/LogServices/EventLog/Entries/103",
            member.getOdataId().value().c_str());
        EXPECT_STREQ("#LogEntry.v1_15_0.LogEntry",
                     member.getOdataType().value().c_str());
        EXPECT_STREQ("2025-01-14T21:12:42+00:00",
                     member.getCreated().value().c_str());
        EXPECT_EQ(redfish_binding::LogEntry::LogEntryType::Event,
                  member.getEntryType().value());
        EXPECT_STREQ("103", member.getId().value().c_str());
        EXPECT_STREQ("Some message id 2.",
                     member.getMessageId().value().c_str());
        EXPECT_STREQ("Some message 2.", member.getMessage().value().c_str());
        EXPECT_FALSE(member.getMessageArgs().hasValue());
        EXPECT_STREQ("Some resolution 2.",
                     member.getResolution().value().c_str());
        EXPECT_TRUE(member.getResolved().value());
        EXPECT_EQ(redfish_binding::LogEntry::EventSeverity::Critical,
                  member.getSeverity().value());
        EXPECT_FALSE(member.getOem().hasValue());
        EXPECT_TRUE(member.getCPER().hasValue());
        auto& cperData = member.getCPER().value();
        EXPECT_STREQ("some notification type",
                     cperData.getNotificationType().value().c_str());
        EXPECT_STREQ("some section type",
                     cperData.getSectionType().value().c_str());
        EXPECT_TRUE(cperData.getOem().hasValue());
        nlohmann::json& oemJson = cperData.getOem().value().leftover();
        auto oemValue = oemJson["some_key"].get<std::string>();
        EXPECT_STREQ("some_value", oemValue.c_str());
    }
}

namespace redfish_binding
{

namespace
{

struct DummyObject
{
    std::string field1;
    bool field2;
    int field3;

    friend bool operator==(const DummyObject& lhs, const DummyObject& rhs)
    {
        return lhs.field1 == rhs.field1 && lhs.field2 == rhs.field2 &&
               lhs.field3 == rhs.field3;
    }
};

void from_json(const nlohmann::json& json, DummyObject& object)
{
    json.at("field1").get_to(object.field1);
    json.at("field2").get_to(object.field2);
    json.at("field3").get_to(object.field3);
}

void to_json(nlohmann::json& json, const DummyObject& object)
{
    json = nlohmann::json{
        {"field1", object.field1},
        {"field2", object.field2},
        {"field3", object.field3},
    };
}

class DummyResource : public ResourceBaseWithError
{
  public:
    Property<int>& p1()
    {
        return p1_;
    }

    Property<std::string>& p2()
    {
        return p2_;
    }

    Property<std::variant<double, bool>>& p3()
    {
        return p3_;
    }

  protected:
    IProperty* findProperty(const std::string& key) override
    {
        if (key == p1_.key())
        {
            return &p1_;
        }
        if (key == p2_.key())
        {
            return &p2_;
        }
        if (key == p3_.key())
        {
            return &p3_;
        }
        return ResourceBaseWithError::findProperty(key);
    }

    void forEachProperty(
        const std::function<void(const IProperty*)>& fn) const override
    {
        fn(&p1_);
        fn(&p2_);
        fn(&p3_);
        ResourceBaseWithError::forEachProperty(fn);
    }

  private:
    Property<int> p1_{"p1"};
    Property<std::string> p2_{"p2"};
    Property<std::variant<double, bool>> p3_{"p3"};
};

enum class DummyEnum
{
    A,
    B,
    C,
};
NLOHMANN_JSON_SERIALIZE_ENUM(DummyEnum, {
                                            {DummyEnum::A, "a"},
                                            {DummyEnum::B, "b"},
                                            {DummyEnum::C, "c"},
                                        })

} // namespace

TEST(PropertyTest, PrimitiveTypeTest)
{
    Property<int> intProperty{"int"};
    EXPECT_EQ(intProperty.hasValue(), false);
    intProperty.setValue(nlohmann::json(25));
    EXPECT_EQ(intProperty.hasValue(), true);
    EXPECT_EQ(intProperty.value(), 25);
    EXPECT_EQ(intProperty.toJson(), nlohmann::json({{"int", 25}}));
    intProperty.setValue(nlohmann::json(nullptr));
    EXPECT_EQ(intProperty.hasValue(), false);
    EXPECT_EQ(intProperty.toJson(), nlohmann::json({}));

    Property<double> doubleProperty{"double"};
    EXPECT_EQ(doubleProperty.hasValue(), false);
    doubleProperty.setValue(nlohmann::json(2.01));
    EXPECT_EQ(doubleProperty.hasValue(), true);
    EXPECT_EQ(doubleProperty.value(), 2.01);
    EXPECT_EQ(doubleProperty.toJson(), nlohmann::json({{"double", 2.01}}));

    Property<bool> boolProperty{"bool"};
    EXPECT_EQ(boolProperty.hasValue(), false);
    boolProperty.setValue(nlohmann::json(false));
    EXPECT_EQ(boolProperty.hasValue(), true);
    EXPECT_EQ(boolProperty.value(), false);
    EXPECT_EQ(boolProperty.toJson(), nlohmann::json({{"bool", false}}));

    Property<std::string> stringProperty{"string"};
    EXPECT_EQ(stringProperty.hasValue(), false);
    stringProperty.setValue(nlohmann::json("test"));
    EXPECT_EQ(stringProperty.hasValue(), true);
    EXPECT_EQ(stringProperty.value(), "test");
    EXPECT_EQ(stringProperty.toJson(), nlohmann::json({{"string", "test"}}));
}

TEST(PropertyTest, ObjectTypeTest)
{
    DummyObject object{
        .field1 = "test",
        .field2 = true,
        .field3 = 10,
    };
    Property<DummyObject> objectProperty{"object"};
    EXPECT_EQ(objectProperty.hasValue(), false);
    objectProperty.setValue(nlohmann::json(object));
    EXPECT_EQ(objectProperty.hasValue(), true);
    EXPECT_EQ(objectProperty.value(), object);
    EXPECT_EQ(objectProperty.toJson(), nlohmann::json({{"object", object}}));
    objectProperty.setValue(nlohmann::json(nullptr));
    EXPECT_EQ(objectProperty.hasValue(), false);
    EXPECT_EQ(objectProperty.toJson(), nlohmann::json({}));
}

TEST(PropertyTest, ContainerTypeTest)
{
    std::vector<int> intVector{10, 1, 3};
    Property<std::vector<int>> intVectorProperty{"intVector"};
    EXPECT_EQ(intVectorProperty.hasValue(), false);
    intVectorProperty.setValue(nlohmann::json(intVector));
    EXPECT_EQ(intVectorProperty.hasValue(), true);
    EXPECT_EQ(intVectorProperty.value(), intVector);
    EXPECT_EQ(intVectorProperty.toJson(),
              nlohmann::json({{"intVector", intVector}}));

    std::vector<DummyObject> objectVector{
        {
            .field1 = "test1",
            .field2 = true,
            .field3 = 101,
        },
        {
            .field1 = "test2",
            .field2 = true,
            .field3 = 103,
        },
        {
            .field1 = "test3",
            .field2 = false,
            .field3 = 55,
        },
    };
    Property<std::vector<DummyObject>> objectVectorProperty{"objectVector"};
    EXPECT_EQ(objectVectorProperty.hasValue(), false);
    objectVectorProperty.setValue(nlohmann::json(objectVector));
    EXPECT_EQ(objectVectorProperty.hasValue(), true);
    EXPECT_EQ(objectVectorProperty.value(), objectVector);
    EXPECT_EQ(objectVectorProperty.toJson(),
              nlohmann::json({{"objectVector", objectVector}}));
}

TEST(PropertyTest, VariantTypeTest)
{
    DummyObject object{
        .field1 = "test",
        .field2 = false,
        .field3 = 1022,
    };
    Property<std::variant<int, std::string, DummyObject>> variantProperty{
        "variant"};
    EXPECT_EQ(variantProperty.hasValue(), false);
    EXPECT_EQ(variantProperty.toJson(), nlohmann::json({}));

    variantProperty.setValue(nlohmann::json(5));
    EXPECT_EQ(variantProperty.hasValue(), true);
    EXPECT_EQ(std::get<int>(variantProperty.value()), 5);
    EXPECT_EQ(variantProperty.toJson(), nlohmann::json({{"variant", 5}}));

    variantProperty.setValue(nlohmann::json("test"));
    EXPECT_EQ(variantProperty.hasValue(), true);
    EXPECT_EQ(std::get<std::string>(variantProperty.value()), "test");
    EXPECT_EQ(variantProperty.toJson(), nlohmann::json({{"variant", "test"}}));

    variantProperty.setValue(nlohmann::json(object));
    EXPECT_EQ(variantProperty.hasValue(), true);
    EXPECT_EQ(std::get<DummyObject>(variantProperty.value()), object);
    EXPECT_EQ(variantProperty.toJson(), nlohmann::json({{"variant", object}}));

    variantProperty.setValue(nlohmann::json(nullptr));
    EXPECT_EQ(variantProperty.hasValue(), false);
    EXPECT_EQ(variantProperty.toJson(), nlohmann::json({}));
}

TEST(PropertyTest, EnumTypeTest)
{
    Property<DummyEnum> enumProperty{"enum"};
    EXPECT_EQ(enumProperty.hasValue(), false);
    enumProperty.setValue(nlohmann::json("a"));
    EXPECT_EQ(enumProperty.hasValue(), true);
    EXPECT_EQ(enumProperty.value(), DummyEnum::A);
    EXPECT_EQ(enumProperty.toJson(), nlohmann::json({{"enum", "a"}}));
    enumProperty.setValue(nlohmann::json("b"));
    EXPECT_EQ(enumProperty.hasValue(), true);
    EXPECT_EQ(enumProperty.value(), DummyEnum::B);
    EXPECT_EQ(enumProperty.toJson(), nlohmann::json({{"enum", "b"}}));
}

TEST(ResourceBaseTest, ResourceBaseTest)
{
    nlohmann::json json;
    json["p1"] = 5;
    json["p2"] = nullptr;
    json["error"] = {
        {"code", "404"},
    };
    json["p3"] = 0.5;
    json["unknown"] = "unknown";
    auto resource = json.template get<DummyResource>();
    EXPECT_EQ(resource.p1().hasValue(), true);
    EXPECT_EQ(resource.p1().value(), 5);
    EXPECT_EQ(resource.p2().hasValue(), false);
    EXPECT_EQ(resource.p3().hasValue(), true);
    EXPECT_EQ(std::get<double>(resource.p3().value()), 0.5);
    EXPECT_EQ(resource.getError().hasValue(), true);
    EXPECT_EQ(resource.getError().value().getCode().hasValue(), true);
    EXPECT_EQ(resource.getError().value().getCode().value(), "404");
    EXPECT_EQ(resource.getError().value().getMessage().hasValue(), false);
    EXPECT_EQ(resource.getError().value().leftover(), nlohmann::json({}));
    EXPECT_EQ(resource.leftover(), nlohmann::json({{"unknown", "unknown"}}));
    // toJson() will drop all keys with nullptr as value
    json.erase("p2");
    EXPECT_EQ(resource.toJson(), json);
}

} // namespace redfish_binding
