#include "device_events.hpp"

#include <phosphor-logging/commit.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/event.hpp>
#include <xyz/openbmc_project/Sensor/event.hpp>
#include <xyz/openbmc_project/State/Fan/event.hpp>
#include <xyz/openbmc_project/State/Power/event.hpp>
#include <xyz/openbmc_project/State/SMC/event.hpp>

namespace event_emulator
{

PHOSPHOR_LOG2_USING;

using SensorUnit = sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit;

// Interface object-path prefixes. The leaf name comes from DeviceEventData.
static constexpr auto sensorPrefix = "/xyz/openbmc_project/sensor/";
static constexpr auto powerRailPrefix =
    "/xyz/openbmc_project/state/power_rail/";
static constexpr auto fanPrefix = "/xyz/openbmc_project/state/fan/";
static constexpr auto smcPrefix = "/xyz/openbmc_project/state/smc/";

static auto emulatedPath(std::string_view prefix, const std::string& name)
    -> sdbusplus::object_path
{
    return sdbusplus::object_path(std::string(prefix) + name + "_EMULATED");
}

namespace sensorError =
    sdbusplus::error::xyz::openbmc_project::sensor::Threshold;
namespace sensorEvent =
    sdbusplus::event::xyz::openbmc_project::sensor::Threshold;
namespace sensorIntfError = sdbusplus::error::xyz::openbmc_project::Sensor;
namespace sensorIntfEvent = sdbusplus::event::xyz::openbmc_project::Sensor;
namespace powerError = sdbusplus::error::xyz::openbmc_project::state::Power;
namespace powerEvent = sdbusplus::event::xyz::openbmc_project::state::Power;
namespace smcError = sdbusplus::error::xyz::openbmc_project::state::SMC;
namespace smcEvent = sdbusplus::event::xyz::openbmc_project::state::SMC;
namespace fanError = sdbusplus::error::xyz::openbmc_project::state::Fan;
namespace fanEvent = sdbusplus::event::xyz::openbmc_project::state::Fan;

auto generateReadingCritical(sdbusplus::async::context& ctx,
                             const DeviceEventData& data)
    -> sdbusplus::async::task<sdbusplus::object_path>
{
    info("Creating SensorReadingCritical event...");
    co_return co_await lg2::commit(
        ctx, sensorError::ReadingCritical(
                 "SENSOR_NAME", emulatedPath(sensorPrefix, data.sensorName),
                 "READING_VALUE", 85.0, "UNITS", SensorUnit::DegreesC));
}

auto generatePowerFault(sdbusplus::async::context& ctx,
                        const DeviceEventData& data)
    -> sdbusplus::async::task<sdbusplus::object_path>
{
    info("Creating PowerRailFault event...");
    co_return co_await lg2::commit(
        ctx,
        powerError::PowerRailFault(
            "POWER_RAIL", emulatedPath(powerRailPrefix, data.powerRailName),
            "FAILURE_DATA", data.powerFailureData));
}

auto generateFanFailure(sdbusplus::async::context& ctx,
                        const DeviceEventData& data)
    -> sdbusplus::async::task<sdbusplus::object_path>
{
    info("Creating FanFailure event...");
    co_return co_await lg2::commit(
        ctx,
        fanError::FanFailed("FAN_NAME", emulatedPath(fanPrefix, data.fanName)));
}

auto generateControllerFailure(sdbusplus::async::context& ctx,
                               const DeviceEventData& data)
    -> sdbusplus::async::task<sdbusplus::object_path>
{
    info("Creating SMCFailed event...");
    co_return co_await lg2::commit(
        ctx,
        smcError::SMCFailed("IDENTIFIER", emulatedPath(smcPrefix, data.smcName),
                            "FAILURE_TYPE", data.failureType));
}

auto generateSensorFailure(sdbusplus::async::context& ctx,
                           const DeviceEventData& data)
    -> sdbusplus::async::task<sdbusplus::object_path>
{
    info("Creating SensorFailure event...");
    co_return co_await lg2::commit(
        ctx,
        sensorIntfError::SensorFailure(
            "SENSOR_NAME", emulatedPath(sensorPrefix, data.sensorFailureName)));
}

auto resolveReadingCritical(sdbusplus::async::context& ctx,
                            const sdbusplus::object_path& eventPath,
                            const DeviceEventData& data)
    -> sdbusplus::async::task<>
{
    info("Resolving SensorReadingCritical event {PATH}...", "PATH", eventPath);
    co_await lg2::resolve(ctx, eventPath);
    co_await lg2::commit(
        ctx, sensorEvent::SensorReadingNormalRange(
                 "SENSOR_NAME", emulatedPath(sensorPrefix, data.sensorName),
                 "READING_VALUE", 25.0, "UNITS", SensorUnit::DegreesC));
}

auto resolvePowerFault(sdbusplus::async::context& ctx,
                       const sdbusplus::object_path& eventPath,
                       const DeviceEventData& data) -> sdbusplus::async::task<>
{
    info("Resolving PowerRailFault event {PATH}...", "PATH", eventPath);
    co_await lg2::resolve(ctx, eventPath);
    co_await lg2::commit(
        ctx,
        powerEvent::PowerRailFaultRecovered(
            "POWER_RAIL", emulatedPath(powerRailPrefix, data.powerRailName)));
}

auto resolveFanFailure(sdbusplus::async::context& ctx,
                       const sdbusplus::object_path& eventPath,
                       const DeviceEventData& data) -> sdbusplus::async::task<>
{
    info("Resolving FanFailure event {PATH}...", "PATH", eventPath);
    co_await lg2::resolve(ctx, eventPath);
    co_await lg2::commit(
        ctx, fanEvent::FanRestored("FAN_NAME",
                                   emulatedPath(fanPrefix, data.fanName)));
}

auto resolveControllerFailure(sdbusplus::async::context& ctx,
                              const sdbusplus::object_path& eventPath,
                              const DeviceEventData& data)
    -> sdbusplus::async::task<>
{
    info("Resolving SMCFailed event {PATH}...", "PATH", eventPath);
    co_await lg2::resolve(ctx, eventPath);
    co_await lg2::commit(
        ctx, smcEvent::SMCRestored("IDENTIFIER",
                                   emulatedPath(smcPrefix, data.smcName)));
}

auto resolveSensorFailure(sdbusplus::async::context& ctx,
                          const sdbusplus::object_path& eventPath,
                          const DeviceEventData& data)
    -> sdbusplus::async::task<>
{
    info("Resolving SensorFailure event {PATH}...", "PATH", eventPath);
    co_await lg2::resolve(ctx, eventPath);
    co_await lg2::commit(
        ctx,
        sensorIntfEvent::SensorRestored(
            "SENSOR_NAME", emulatedPath(sensorPrefix, data.sensorFailureName)));
}

} // namespace event_emulator
