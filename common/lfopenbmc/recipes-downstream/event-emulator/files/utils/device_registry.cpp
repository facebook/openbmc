#include "utils/device_registry.hpp"

#include <algorithm>

namespace event_emulator
{

auto getDeviceRegistry() -> DeviceRegistryMap&
{
    static DeviceRegistryMap registry;
    return registry;
}

DeviceRegistration::DeviceRegistration(const std::string& name,
                                       DeviceFactory factory)
{
    getDeviceRegistry()[name] = std::move(factory);
}

auto getDeviceData(const std::string& device) -> DeviceEventData
{
    return getDeviceRegistry().at(device)();
}

bool isValidDevice(const std::string& device)
{
    return getDeviceRegistry().contains(device);
}

bool isValidEventForDevice(const std::string& eventType,
                           const DeviceEventData& data)
{
    if (eventType == "all")
    {
        return true;
    }
    return std::find(data.supportedEvents.begin(), data.supportedEvents.end(),
                     eventType) != data.supportedEvents.end();
}

} // namespace event_emulator
