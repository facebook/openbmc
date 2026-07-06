#include <nlohmann/json.hpp>
#include <redfish-binding/Sensor_Sensor.hpp>
#include <redfish_client/core/sensor.hpp>
#include <xyz/openbmc_project/Association/Definitions/client.hpp>
#include <xyz/openbmc_project/Sensor/Value/client.hpp>

#include <cmath>
#include <memory>
#include <optional>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace redfish_client::core;

class SensorTest : public ::testing::Test
{
  protected:
    static constexpr auto kBusName = "xyz.openbmc_project.test.NotARealService";

    static constexpr auto kMetricPath =
        "/xyz/openbmc_project/sensors/some/Metric";

    static constexpr auto kAssociationPath =
        "/xyz/openbmc_project/inventory/system/NotARealBoard/XXX";

    SensorTest() : manager(ctx, sensorRootPath) {}

    ~SensorTest() noexcept override {}

    void SetUp() override
    {
        ctx.spawn(
            [](sdbusplus::async::context& ctx) -> sdbusplus::async::task<> {
                ctx.request_name(kBusName);
                co_return;
            }(ctx));
    }

    void TearDown() override
    {
        ctx.spawn(
            [](sdbusplus::async::context& ctx) -> sdbusplus::async::task<> {
                ctx.request_stop();
                co_return;
            }(ctx));
        ctx.run();
        ASSERT_TRUE(testBodyExecuted);
    }

    // Create (and publish) a Sensor from raw reading fields, exercising the
    // D-Bus object directly. Returns nullptr when the unit is absent (mirroring
    // Sensor::create).
    std::unique_ptr<Sensor> createSensor(
        const std::string& objectPath, const std::string& associationPath,
        double reading, const std::string& unit,
        std::optional<double> minValue = std::nullopt,
        std::optional<double> maxValue = std::nullopt)
    {
        nlohmann::json json;
        json["Reading"] = reading;
        json["ReadingUnits"] = unit;
        if (minValue.has_value())
        {
            json["ReadingRangeMin"] = minValue.value();
        }
        if (maxValue.has_value())
        {
            json["ReadingRangeMax"] = maxValue.value();
        }
        auto parsed = redfish_binding::Sensor::parseSensor(json.dump());
        return Sensor::create(ctx, objectPath, associationPath, parsed);
    }

    auto getSensorClient()
    {
        using ValueClient =
            sdbusplus::client::xyz::openbmc_project::sensor::Value<>;
        return ValueClient(ctx).service(kBusName).path(kMetricPath);
    }

    auto getAssociationClient()
    {
        using AssociationClient =
            sdbusplus::client::xyz::openbmc_project::association::Definitions<>;
        return AssociationClient(ctx).service(kBusName).path(kMetricPath);
    }

    // Tests that deal with lambdas can silently pass if the code that triggers
    // the lambda is accidentally deleted. Guard against that with this flag.
    bool testBodyExecuted = false;

    sdbusplus::async::context ctx;
    sdbusplus::server::manager_t manager;
};

TEST_F(SensorTest, FailWithBadJson)
{
    std::string badJson = "[";
    EXPECT_THROW((void)redfish_binding::Sensor::parseSensor(badJson),
                 std::exception);
    testBodyExecuted = true;
}

TEST_F(SensorTest, SensorRootPath)
{
    EXPECT_STREQ("/xyz/openbmc_project/sensors", sensorRootPath);
    testBodyExecuted = true;
}

TEST_F(SensorTest, ToMaybeUnit)
{
    auto testSuccessHelper =
        [](SensorValueIntf::Unit expected, const std::string& units) {
            auto maybeActual = Sensor::toMaybeUnit(units);
            EXPECT_TRUE(maybeActual.has_value());
            EXPECT_EQ(expected, maybeActual.value());
        };

    testSuccessHelper(SensorValueIntf::Unit::Percent, "%");
    testSuccessHelper(SensorValueIntf::Unit::DegreesC, "Cel");
    testSuccessHelper(SensorValueIntf::Unit::Joules, "J");
    testSuccessHelper(SensorValueIntf::Unit::Pascals, "Pa");
    testSuccessHelper(SensorValueIntf::Unit::Volts, "V");
    testSuccessHelper(SensorValueIntf::Unit::Watts, "W");

    auto testFailureHelper = [](const std::string& units) {
        auto maybeActual = Sensor::toMaybeUnit(units);
        EXPECT_FALSE(maybeActual.has_value());
    };

    testFailureHelper("unknown_unit");
    testBodyExecuted = true;
}

TEST_F(SensorTest, NoOpBeforeCreation)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        auto sensorClient = getSensorClient();

        // No Sensor has been created, so nothing is published on the bus.
        EXPECT_THROW(co_await sensorClient.value(),
                     sdbusplus::exception::SdBusError);

        testBodyExecuted = true;
        co_return;
    }());
}

TEST_F(SensorTest, EmptyAssociationPath)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        std::string emptyAssociation;
        auto sensorServer =
            createSensor(kMetricPath, emptyAssociation, 10.0, "");
        auto associationClient = getAssociationClient();

        auto associations = co_await associationClient.associations();
        EXPECT_EQ(0, associations.size());

        testBodyExecuted = true;
        co_return;
    }());
}

TEST_F(SensorTest, CreateAndUpdate)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        auto sensorServer =
            createSensor(kMetricPath, kAssociationPath, 25.0, "%", 0.0, 100.0);
        auto sensorClient = getSensorClient();
        auto associationClient = getAssociationClient();

        EXPECT_EQ(25.0, co_await sensorClient.value());
        EXPECT_EQ(SensorValueIntf::Unit::Percent, co_await sensorClient.unit());
        EXPECT_EQ(0.0, co_await sensorClient.min_value());
        EXPECT_EQ(100.0, co_await sensorClient.max_value());

        auto associations = co_await associationClient.associations();
        EXPECT_EQ(1, associations.size());
        EXPECT_EQ("chassis", std::get<0>(associations[0]));
        EXPECT_EQ("all_sensors", std::get<1>(associations[0]));
        EXPECT_EQ(kAssociationPath, std::get<2>(associations[0]));

        // A subsequent value update changes only the value.
        co_await sensorServer->updateValue(42.0);
        EXPECT_EQ(42.0, co_await sensorClient.value());

        testBodyExecuted = true;
        co_return;
    }());
}

TEST_F(SensorTest, ValueUpdateKeepsUnitAndRange)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        auto sensorServer =
            createSensor(kMetricPath, kAssociationPath, 25.0, "Cel", -10, 10);
        auto sensorClient = getSensorClient();

        EXPECT_EQ(25.0, co_await sensorClient.value());
        EXPECT_EQ(SensorValueIntf::Unit::DegreesC,
                  co_await sensorClient.unit());
        EXPECT_EQ(-10, co_await sensorClient.min_value());
        EXPECT_EQ(10, co_await sensorClient.max_value());

        // The unit and range are fixed at construction; only the value changes.
        co_await sensorServer->updateValue(35.0);
        EXPECT_EQ(35.0, co_await sensorClient.value());
        EXPECT_EQ(SensorValueIntf::Unit::DegreesC,
                  co_await sensorClient.unit());
        EXPECT_EQ(-10, co_await sensorClient.min_value());
        EXPECT_EQ(10, co_await sensorClient.max_value());

        testBodyExecuted = true;
        co_return;
    }());
}

TEST_F(SensorTest, MaxMinOmitted)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        // When the Redfish sensor omits the range, per-unit defaults are
        // applied (J -> 0..100000).
        auto sensorServer =
            createSensor(kMetricPath, kAssociationPath, 25.0, "J");
        auto sensorClient = getSensorClient();

        EXPECT_EQ(25.0, co_await sensorClient.value());
        EXPECT_EQ(SensorValueIntf::Unit::Joules, co_await sensorClient.unit());
        EXPECT_EQ(0.0, co_await sensorClient.min_value());
        EXPECT_EQ(100000.0, co_await sensorClient.max_value());

        testBodyExecuted = true;
        co_return;
    }());
}

TEST_F(SensorTest, UnrecognizedUnitDefaultsToAmperes)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        auto sensorServer = createSensor(kMetricPath, kAssociationPath, 3.0,
                                         "This is not a real unit");
        auto sensorClient = getSensorClient();

        EXPECT_EQ(3.0, co_await sensorClient.value());
        // Unrecognized units fall back to Amperes so the object is valid.
        EXPECT_EQ(SensorValueIntf::Unit::Amperes, co_await sensorClient.unit());

        co_await sensorServer->updateValue(6.0);
        EXPECT_EQ(6.0, co_await sensorClient.value());
        EXPECT_EQ(SensorValueIntf::Unit::Amperes, co_await sensorClient.unit());

        testBodyExecuted = true;
        co_return;
    }());
}

TEST_F(SensorTest, MissingUnitsIsNotCreated)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        // A Redfish sensor with no ReadingUnits produces no D-Bus object.
        nlohmann::json json;
        json["Reading"] = 5.0;
        auto parsed = redfish_binding::Sensor::parseSensor(json.dump());
        auto sensorServer =
            Sensor::create(ctx, kMetricPath, kAssociationPath, parsed);
        EXPECT_EQ(nullptr, sensorServer);

        // Nothing is published on the bus.
        auto sensorClient = getSensorClient();
        EXPECT_THROW(co_await sensorClient.value(),
                     sdbusplus::exception::SdBusError);

        testBodyExecuted = true;
        co_return;
    }());
}

TEST_F(SensorTest, MissingReadingBecomesNan)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        // No Reading field -> value is published as NaN.
        nlohmann::json json;
        json["ReadingUnits"] = "Cel";
        auto parsed = redfish_binding::Sensor::parseSensor(json.dump());
        auto sensorServer =
            Sensor::create(ctx, kMetricPath, kAssociationPath, parsed);
        EXPECT_NE(nullptr, sensorServer);

        auto sensorClient = getSensorClient();
        EXPECT_TRUE(std::isnan(co_await sensorClient.value()));
        // Cel gets per-unit default range.
        EXPECT_EQ(-128.0, co_await sensorClient.min_value());
        EXPECT_EQ(127.0, co_await sensorClient.max_value());

        testBodyExecuted = true;
        co_return;
    }());
}
