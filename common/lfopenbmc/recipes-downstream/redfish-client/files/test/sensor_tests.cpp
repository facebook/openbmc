#include <redfish_client/core/sensor.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace redfish_client::core;

TEST(SensorTests, FailWithBadJson)
{
    std::string badJson = "[";
    EXPECT_THROW((void)Sensor::parseSensor(badJson), std::exception);
}

namespace
{

std::string getGoodJson()
{
    std::string sensorJson = R"(
        {
          "@odata.id": "/redfish/v1/Chassis/SomeObject/Sensors/SomeObject_Temp_0",
          "Id": "SomeObject_Temp_0",
          "Name": "Some Object 0 Temp 0",
          "Reading": 31.125,
          "ReadingRangeMax": 127.0,
          "ReadingRangeMin": -128.0,
          "ReadingType": "Temperature",
          "ReadingUnits": "Cel"
        }
)";

    return sensorJson;
}

} // namespace

TEST(SensorTests, Success)
{
    auto testJson = getGoodJson();
    auto maybeSensor = Sensor::parseSensor(testJson);
    EXPECT_TRUE(maybeSensor.has_value());

    auto sensor = maybeSensor.value();
    EXPECT_EQ(31.125, sensor.getReading());
    EXPECT_EQ("Cel", sensor.getSensorUnitText());

    EXPECT_TRUE(sensor.getMinValue().has_value());
    EXPECT_EQ(-128.0, sensor.getMinValue().value());

    EXPECT_TRUE(sensor.getMaxValue().has_value());
    EXPECT_EQ(127.0, sensor.getMaxValue().value());
}

namespace
{

std::string getJsonWithMissingReading()
{
    std::string sensorJson = R"(
        {
          "@odata.id": "/redfish/v1/Chassis/SomeObject/Sensors/SomeObject_Temp_0",
          "Id": "SomeObject_Temp_0",
          "Name": "Some Object 0 Temp 0",
          "ReadingRangeMax": 127.0,
          "ReadingRangeMin": -128.0,
          "ReadingType": "Temperature",
          "ReadingUnits": "Cel"
        }
)";

    return sensorJson;
}

} // namespace

TEST(SensorTests, FailWithMissingReading)
{
    auto badJson = getJsonWithMissingReading();
    auto maybeSensor = Sensor::parseSensor(badJson);
    EXPECT_TRUE(maybeSensor.has_value());

    auto sensor = maybeSensor.value();
    EXPECT_TRUE(std::isnan(sensor.getReading()));
}

namespace
{

std::string getJsonWithMissingUnits()
{
    std::string sensorJson = R"(
        {
          "@odata.id": "/redfish/v1/Chassis/SomeObject/Sensors/SomeObject_Temp_0",
          "Id": "SomeObject_Temp_0",
          "Name": "Some Object 0 Temp 0",
          "Reading": 31.125,
          "ReadingRangeMax": 127.0,
          "ReadingRangeMin": -128.0,
          "ReadingType": "Temperature"
        }
)";

    return sensorJson;
}

} // namespace

TEST(SensorTests, FailWithMissingUnits)
{
    auto badJson = getJsonWithMissingUnits();
    auto maybeSensor = Sensor::parseSensor(badJson);
    EXPECT_FALSE(maybeSensor.has_value());
}

namespace
{

std::string getJsonWithMissingMinMax()
{
    std::string sensorJson = R"(
        {
          "@odata.id": "/redfish/v1/Chassis/SomeObject/Sensors/SomeObject_Temp_0",
          "Id": "SomeObject_Temp_0",
          "Name": "Some Object 0 Temp 0",
          "Reading": 31.125,
          "ReadingType": "Temperature",
          "ReadingUnits": "Cel"
        }
)";

    return sensorJson;
}

} // namespace

TEST(SensorTests, SuccessWithMissingMinMax)
{
    auto badJson = getJsonWithMissingMinMax();
    auto maybeSensor = Sensor::parseSensor(badJson);
    EXPECT_TRUE(maybeSensor.has_value());

    auto sensor = maybeSensor.value();
    EXPECT_EQ(31.125, sensor.getReading());
    EXPECT_EQ("Cel", sensor.getSensorUnitText());
    EXPECT_FALSE(sensor.getMinValue().has_value());
    EXPECT_FALSE(sensor.getMaxValue().has_value());
}
