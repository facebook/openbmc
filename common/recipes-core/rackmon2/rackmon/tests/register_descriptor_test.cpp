#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "regmap.hpp"

using namespace std;
using namespace testing;

//--------------------------------------------------------
// RegisterDescriptorTests
//--------------------------------------------------------

TEST(RegisterDescriptorTest, JSONConversionDefaults) {
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 0,
    "length": 8,
    "name": "MFG_MODEL"
  })");
  RegisterDescriptor d = desc;
  ASSERT_EQ(d.begin, 0);
  ASSERT_EQ(d.length, 8);
  ASSERT_EQ(d.name, "MFG_MODEL");
  ASSERT_EQ(d.keep, 1);
  ASSERT_EQ(d.changes_only, false);
  ASSERT_EQ(d.format, RegisterValueType::HEX);
}

TEST(RegisterDescriptorTest, JSONConversionEnforceMandatory) {
  RegisterDescriptor d;
  nlohmann::json desc1 = nlohmann::json::parse(R"({
    "length": 8,
    "name": "MFG_MODEL"
  })");
  EXPECT_THROW(d = desc1, nlohmann::json::out_of_range);

  nlohmann::json desc2 = nlohmann::json::parse(R"({
    "begin": 8,
    "name": "MFG_MODEL"
  })");
  EXPECT_THROW(d = desc2, nlohmann::json::out_of_range);

  nlohmann::json desc3 = nlohmann::json::parse(R"({
    "begin": 0,
    "length": 8
  })");
  EXPECT_THROW(d = desc2, nlohmann::json::out_of_range);
}

TEST(RegisterDescriptorTest, JSONConversionString) {
  // TODO Change JSON
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 0,
    "length": 8,
    "format": "string",
    "name": "MFG_MODEL"
  })");
  RegisterDescriptor d = desc;
  ASSERT_EQ(d.begin, 0);
  ASSERT_EQ(d.length, 8);
  ASSERT_EQ(d.name, "MFG_MODEL");
  ASSERT_EQ(d.keep, 1);
  ASSERT_EQ(d.changes_only, false);
  ASSERT_EQ(d.format, RegisterValueType::STRING);
}

TEST(RegisterDescriptorTest, JSONConversionDecimalKeep) {
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 156,
    "length": 1,
    "format": "integer",
    "keep": 10,
    "name": "Set fan speed"
  })");
  RegisterDescriptor d = desc;
  ASSERT_EQ(d.begin, 156);
  ASSERT_EQ(d.length, 1);
  ASSERT_EQ(d.name, "Set fan speed");
  ASSERT_EQ(d.keep, 10);
  ASSERT_EQ(d.changes_only, false);
  ASSERT_EQ(d.format, RegisterValueType::INTEGER);
}

TEST(RegisterDescriptorTest, JSONConversionFixed) {
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 127,
    "length": 1,
    "format": "float",
    "precision": 6,
    "name": "Input VAC"
  })");
  RegisterDescriptor d = desc;
  ASSERT_EQ(d.begin, 127);
  ASSERT_EQ(d.length, 1);
  ASSERT_EQ(d.name, "Input VAC");
  ASSERT_EQ(d.keep, 1);
  ASSERT_EQ(d.changes_only, false);
  ASSERT_EQ(d.format, RegisterValueType::FLOAT);
  ASSERT_EQ(d.precision, 6);
}

TEST(RegisterDescriptorTest, JSONConversionFixedMissingPrec) {
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 127,
    "length": 1,
    "format": "float",
    "name": "Input VAC"
  })");
  RegisterDescriptor d;
  EXPECT_THROW(d = desc, nlohmann::json::out_of_range);
}

TEST(RegisterDescriptorTest, JSONConversionTableChangesOnly) {
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 105,
    "length": 1,
    "keep": 10,
    "changes_only": true,
    "format": "flags",
    "name": "Battery Status register",
    "flags": [
      [2, "End of Life"],
      [1, "Low Voltage  BBU Voltage <= 33.8"],
      [0, "BBU Fail"]
    ]
  })");
  RegisterDescriptor d = desc;
  ASSERT_EQ(d.begin, 105);
  ASSERT_EQ(d.length, 1);
  ASSERT_EQ(d.keep, 10);
  ASSERT_EQ(d.changes_only, true);
  ASSERT_EQ(d.name, "Battery Status register");
  ASSERT_EQ(d.format, RegisterValueType::FLAGS);
  std::vector<std::tuple<uint8_t, std::string>> expected = {
      {2, "End of Life"},
      {1, "Low Voltage  BBU Voltage <= 33.8"},
      {0, "BBU Fail"}};
  ASSERT_EQ(d.flags, expected);
}

TEST(RegisterDescriptorTest, JSONConversionTableMissing) {
  nlohmann::json desc = nlohmann::json::parse(R"({
    "begin": 105,
    "length": 1,
    "keep": 10,
    "changes_only": true,
    "format": "flags",
    "name": "Battery Status register"
  })");
  RegisterDescriptor d;
  EXPECT_THROW(d = desc, nlohmann::json::out_of_range);
}
