#pragma once

#include "utils/device_events.hpp"

#include <functional>
#include <map>
#include <string>

namespace event_emulator
{

using DeviceFactory = std::function<DeviceEventData()>;
using DeviceRegistryMap = std::map<std::string, DeviceFactory>;

auto getDeviceRegistry() -> DeviceRegistryMap&;

struct DeviceRegistration
{
    DeviceRegistration(const std::string& name, DeviceFactory factory);
};

auto getDeviceData(const std::string& device) -> DeviceEventData;
bool isValidDevice(const std::string& device);
bool isValidEventForDevice(const std::string& eventType,
                           const DeviceEventData& data);

} // namespace event_emulator
