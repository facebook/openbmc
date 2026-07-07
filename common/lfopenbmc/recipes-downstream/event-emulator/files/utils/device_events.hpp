#pragma once

#include <sdbusplus/async.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <string>
#include <vector>

namespace event_emulator
{

// Per-device event names (leaf). Final path = <interface-prefix> + name +
// "_EMULATED"; the prefix is owned by each generate/resolve function.
struct DeviceEventData
{
    std::string sensorName;
    std::string powerRailName;
    std::string fanName;
    std::string smcName;
    std::string sensorFailureName;
    std::string failureType;
    std::string powerFailureData;
    std::vector<std::string> supportedEvents;
};

auto generateReadingCritical(sdbusplus::async::context& ctx,
                             const DeviceEventData& data)
    -> sdbusplus::async::task<sdbusplus::object_path>;

auto generatePowerFault(sdbusplus::async::context& ctx,
                        const DeviceEventData& data)
    -> sdbusplus::async::task<sdbusplus::object_path>;

auto generateFanFailure(sdbusplus::async::context& ctx,
                        const DeviceEventData& data)
    -> sdbusplus::async::task<sdbusplus::object_path>;

auto generateControllerFailure(sdbusplus::async::context& ctx,
                               const DeviceEventData& data)
    -> sdbusplus::async::task<sdbusplus::object_path>;

auto generateSensorFailure(sdbusplus::async::context& ctx,
                           const DeviceEventData& data)
    -> sdbusplus::async::task<sdbusplus::object_path>;

auto resolveReadingCritical(sdbusplus::async::context& ctx,
                            const sdbusplus::object_path& eventPath,
                            const DeviceEventData& data)
    -> sdbusplus::async::task<>;

auto resolvePowerFault(sdbusplus::async::context& ctx,
                       const sdbusplus::object_path& eventPath,
                       const DeviceEventData& data) -> sdbusplus::async::task<>;

auto resolveFanFailure(sdbusplus::async::context& ctx,
                       const sdbusplus::object_path& eventPath,
                       const DeviceEventData& data) -> sdbusplus::async::task<>;

auto resolveControllerFailure(sdbusplus::async::context& ctx,
                              const sdbusplus::object_path& eventPath,
                              const DeviceEventData& data)
    -> sdbusplus::async::task<>;

auto resolveSensorFailure(sdbusplus::async::context& ctx,
                          const sdbusplus::object_path& eventPath,
                          const DeviceEventData& data)
    -> sdbusplus::async::task<>;

} // namespace event_emulator
