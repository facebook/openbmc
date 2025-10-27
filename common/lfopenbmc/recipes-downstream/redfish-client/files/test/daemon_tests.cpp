#include <redfish_client/daemon.hpp>
#include <redfish_client/core/sensor_dbus_object.hpp>

#include <xyz/openbmc_project/Association/Definitions/client.hpp>
#include <xyz/openbmc_project/Sensor/Value/client.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace redfish_client_daemon;
using namespace redfish_client::core;

TEST(DbusUtilsTest, SensorRootPath)
{
    EXPECT_STREQ("/xyz/openbmc_project/sensors", getSensorRootPath());
}

TEST(DbusUtilsTest, ActualMetricNamespace)
{
    EXPECT_STREQ("airflow", getActualMetricNamespace("airflow"));
    EXPECT_STREQ("altitude", getActualMetricNamespace("altitude"));
    EXPECT_STREQ("current", getActualMetricNamespace("current"));
    EXPECT_STREQ("energy", getActualMetricNamespace("energy"));
    EXPECT_STREQ("fan_tach", getActualMetricNamespace("fan_tach"));
    EXPECT_STREQ("humidity", getActualMetricNamespace("humidity"));
    EXPECT_STREQ("liquidflow", getActualMetricNamespace("liquidflow"));
    EXPECT_STREQ("power", getActualMetricNamespace("power"));
    EXPECT_STREQ("pressure", getActualMetricNamespace("pressure"));
    EXPECT_STREQ("temperature", getActualMetricNamespace("temperature"));
    EXPECT_STREQ("utilization", getActualMetricNamespace("utilization"));
    EXPECT_STREQ("voltage", getActualMetricNamespace("voltage"));
    EXPECT_THROW(getActualMetricNamespace("unknown_namespace"),
                 std::invalid_argument);
}

TEST(DbusUtilsTest, MaybeIntfUnits)
{
    auto testSuccessHelper =
        [](ValueIntf::Unit expected, const char* unitsCstr) {
            std::string units = std::string(unitsCstr);
            auto maybeActual = toMaybeIntfUnits(units);
            EXPECT_TRUE(maybeActual.has_value());
            EXPECT_EQ(expected, maybeActual.value());
        };

    testSuccessHelper(ValueIntf::Unit::Percent, "%");
    testSuccessHelper(ValueIntf::Unit::DegreesC, "Cel");
    testSuccessHelper(ValueIntf::Unit::Joules, "J");
    testSuccessHelper(ValueIntf::Unit::Pascals, "Pa");
    testSuccessHelper(ValueIntf::Unit::Volts, "V");
    testSuccessHelper(ValueIntf::Unit::Watts, "W");

    auto testFailureHelper = [](const char* unitsCstr) {
        std::string units = std::string(unitsCstr);
        auto maybeActual = toMaybeIntfUnits(units);
        EXPECT_FALSE(maybeActual.has_value());
    };

    testFailureHelper("unknown_unit");
}

class SensorDBusObjectTests : public ::testing::Test
{
  protected:
    static constexpr auto kBusName = "xyz.openbmc_project.test.NotARealService";

    static constexpr auto kMetricPath =
        "/xyz/openbmc_project/sensors/some/Metric";

    static constexpr auto kPositiveInfinity =
        std::numeric_limits<double>::infinity();
    static constexpr auto kNegativeInfinity =
        -std::numeric_limits<double>::infinity();

    const std::string kAssociationPath =
        "/xyz/openbmc_project/inventory/system/NotARealBoard/XXX";

    SensorDBusObjectTests() : manager(ctx, getSensorRootPath()) {}

    ~SensorDBusObjectTests() noexcept override {}

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

    static auto updateServer(std::shared_ptr<ISensorDbusObject> sensorServer,
                             double reading, const char* sensorUnit,
                             std::optional<double> minValue = std::nullopt,
                             std::optional<double> maxValue = std::nullopt)
        -> sdbusplus::async::task<>
    {
        auto sensor =
            Sensor::createTestSensor(reading, sensorUnit, minValue, maxValue);
        co_await sensorServer->update(sensor);
        co_return;
    }

    // Tests that deal with lamdba's can sometimes silently pass because the
    // code that triggers the lambda got deleted accidentally. Make sure that
    // has low probabilility by using this flag.
    bool testBodyExecuted = false;

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

    sdbusplus::async::context ctx;
    sdbusplus::server::manager_t manager;
};

TEST_F(SensorDBusObjectTests, NoOpBeforeFirstUpdate)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        auto sensorServer =
            createSensorDbusObjectForTest(ctx, kMetricPath, kAssociationPath);
        auto sensorClient = getSensorClient();

        // Cannot read the value, nothing has been written yet.
        EXPECT_THROW(co_await sensorClient.value(),
                     sdbusplus::exception::SdBusError);

        testBodyExecuted = true;
        co_return;
    }());
}

TEST_F(SensorDBusObjectTests, EmptyAssociationPath)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        std::string emptyAssociation;
        auto sensorServer =
            createSensorDbusObjectForTest(ctx, kMetricPath, emptyAssociation);
        auto associationClient = getAssociationClient();

        co_await updateServer(sensorServer, 10.0, "");
        auto associations = co_await associationClient.associations();
        EXPECT_EQ(0, associations.size());

        testBodyExecuted = true;
        co_return;
    }());
}

TEST_F(SensorDBusObjectTests, CreateAndUpdate)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        auto sensorServer =
            createSensorDbusObjectForTest(ctx, kMetricPath, kAssociationPath);
        auto sensorClient = getSensorClient();
        auto associationClient = getAssociationClient();

        co_await updateServer(sensorServer, 25.0, "%", 0.0, 100.0);
        EXPECT_EQ(25.0, co_await sensorClient.value());
        EXPECT_EQ(ValueIntf::Unit::Percent, co_await sensorClient.unit());
        EXPECT_EQ(0.0, co_await sensorClient.min_value());
        EXPECT_EQ(100.0, co_await sensorClient.max_value());

        auto associations = co_await associationClient.associations();
        EXPECT_EQ(1, associations.size());
        EXPECT_EQ("chassis", std::get<0>(associations[0]));
        EXPECT_EQ("all_sensors", std::get<1>(associations[0]));
        EXPECT_EQ(kAssociationPath, std::get<2>(associations[0]));

        testBodyExecuted = true;
        co_return;
    }());
}

TEST_F(SensorDBusObjectTests, DoubleUpdate)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        auto sensorServer =
            createSensorDbusObjectForTest(ctx, kMetricPath, kAssociationPath);
        auto sensorClient = getSensorClient();

        co_await updateServer(sensorServer, 25.0, "Cel", -10, 10);
        EXPECT_EQ(25.0, co_await sensorClient.value());
        EXPECT_EQ(ValueIntf::Unit::DegreesC, co_await sensorClient.unit());
        EXPECT_EQ(-10, co_await sensorClient.min_value());
        EXPECT_EQ(10, co_await sensorClient.max_value());

        // The max and min values should not change after the first update.
        co_await updateServer(sensorServer, 35.0, "Cel", -30, 30);
        EXPECT_EQ(35.0, co_await sensorClient.value());
        EXPECT_EQ(ValueIntf::Unit::DegreesC, co_await sensorClient.unit());
        EXPECT_EQ(-10, co_await sensorClient.min_value());
        EXPECT_EQ(10, co_await sensorClient.max_value());

        testBodyExecuted = true;
        co_return;
    }());
}

TEST_F(SensorDBusObjectTests, MaxMinOmitted)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        auto sensorServer =
            createSensorDbusObjectForTest(ctx, kMetricPath, kAssociationPath);
        auto sensorClient = getSensorClient();

        co_await updateServer(sensorServer, 25.0, "J");
        EXPECT_EQ(25.0, co_await sensorClient.value());
        EXPECT_EQ(ValueIntf::Unit::Joules, co_await sensorClient.unit());
        EXPECT_EQ(kNegativeInfinity, co_await sensorClient.min_value());
        EXPECT_EQ(kPositiveInfinity, co_await sensorClient.max_value());

        testBodyExecuted = true;
        co_return;
    }());
}

TEST_F(SensorDBusObjectTests, UnitsOnlyReadAtFirstUpdate)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        auto sensorServer =
            createSensorDbusObjectForTest(ctx, kMetricPath, kAssociationPath);
        auto sensorClient = getSensorClient();

        co_await updateServer(sensorServer, 33.0, "Pa");
        EXPECT_EQ(33.0, co_await sensorClient.value());
        EXPECT_EQ(ValueIntf::Unit::Pascals, co_await sensorClient.unit());
        EXPECT_EQ(kNegativeInfinity, co_await sensorClient.min_value());
        EXPECT_EQ(kPositiveInfinity, co_await sensorClient.max_value());

        co_await updateServer(sensorServer, 44.0, "J");
        EXPECT_EQ(44.0, co_await sensorClient.value());
        EXPECT_EQ(ValueIntf::Unit::Pascals, co_await sensorClient.unit());
        EXPECT_EQ(kNegativeInfinity, co_await sensorClient.min_value());
        EXPECT_EQ(kPositiveInfinity, co_await sensorClient.max_value());

        testBodyExecuted = true;
        co_return;
    }());
}

TEST_F(SensorDBusObjectTests, MissingUnitsOnFirstUpdateCannotBeModified)
{
    ctx.spawn([this]() -> sdbusplus::async::task<> {
        auto sensorServer =
            createSensorDbusObjectForTest(ctx, kMetricPath, kAssociationPath);
        auto sensorClient = getSensorClient();

        co_await updateServer(sensorServer, 3.0, "This is not a real unit");
        EXPECT_EQ(3.0, co_await sensorClient.value());

        // Generated code uses the first value if not set.
        EXPECT_EQ(ValueIntf::Unit::Amperes, co_await sensorClient.unit());

        co_await updateServer(sensorServer, 6.0, "J");
        EXPECT_EQ(6.0, co_await sensorClient.value());
        // Our code does not allow the unit to be changed after the first
        // update.
        EXPECT_EQ(ValueIntf::Unit::Amperes, co_await sensorClient.unit());

        testBodyExecuted = true;
        co_return;
    }());
}
