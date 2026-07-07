#pragma once

#include <sdbusplus/async.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <string>
#include <vector>

namespace event_emulator
{

struct DeviceEventData
{
    std::string sensorPath;
    std::string powerRailPath;
    std::string fanPath;
    std::string smcPath;
    std::string sensorFailurePath;
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
