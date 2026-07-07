#include "utils/event_actions.hpp"

#include "utils/device_events.hpp"
#include "utils/device_registry.hpp"
#include "utils/event_state.hpp"

#include <phosphor-logging/lg2.hpp>

#include <iostream>

namespace event_emulator
{

PHOSPHOR_LOG2_USING;

auto dispatchGenerate(sdbusplus::async::context& ctx,
                      const std::string& eventType, const DeviceEventData& data)
    -> sdbusplus::async::task<sdbusplus::object_path>
{
    if (eventType == "reading-critical")
    {
        co_return co_await generateReadingCritical(ctx, data);
    }
    if (eventType == "power-fault")
    {
        co_return co_await generatePowerFault(ctx, data);
    }
    if (eventType == "fan-failure")
    {
        co_return co_await generateFanFailure(ctx, data);
    }
    if (eventType == "controller-failure")
    {
        co_return co_await generateControllerFailure(ctx, data);
    }
    co_return co_await generateSensorFailure(ctx, data);
}

auto dispatchResolve(sdbusplus::async::context& ctx,
                     const std::string& eventType,
                     const sdbusplus::object_path& eventPath,
                     const DeviceEventData& data) -> sdbusplus::async::task<>
{
    if (eventType == "reading-critical")
    {
        co_await resolveReadingCritical(ctx, eventPath, data);
    }
    else if (eventType == "power-fault")
    {
        co_await resolvePowerFault(ctx, eventPath, data);
    }
    else if (eventType == "fan-failure")
    {
        co_await resolveFanFailure(ctx, eventPath, data);
    }
    else if (eventType == "controller-failure")
    {
        co_await resolveControllerFailure(ctx, eventPath, data);
    }
    else if (eventType == "sensor-failure")
    {
        co_await resolveSensorFailure(ctx, eventPath, data);
    }
    co_return;
}

auto processGenerate(sdbusplus::async::context& ctx, const std::string& device,
                     const std::string& eventType, const DeviceEventData& data,
                     EventStateMap& state) -> sdbusplus::async::task<>
{
    auto key = makeKey(device, eventType);
    if (state.contains(key))
    {
        std::cerr << eventType << ": already pending (use resolve first)\n";
        co_return;
    }

    auto path = co_await dispatchGenerate(ctx, eventType, data);
    state[key] = path.str;
    std::cout << eventType << ": " << path.str << "\n";
    co_return;
}

auto processResolve(sdbusplus::async::context& ctx, const std::string& device,
                    const std::string& eventType, const DeviceEventData& data,
                    EventStateMap& state) -> sdbusplus::async::task<>
{
    auto key = makeKey(device, eventType);
    auto it = state.find(key);
    if (it == state.end())
    {
        std::cerr << eventType << ": no pending event to resolve\n";
        co_return;
    }

    sdbusplus::object_path path(it->second);
    co_await dispatchResolve(ctx, eventType, path, data);
    std::cout << eventType << ": resolved " << path.str << "\n";
    state.erase(it);
}

auto runGenerate(sdbusplus::async::context& ctx, const std::string& device,
                 const std::string& eventType) -> sdbusplus::async::task<>
{
    auto data = getDeviceData(device);
    auto state = loadState();

    if (eventType == "all")
    {
        for (const auto& et : data.supportedEvents)
        {
            co_await processGenerate(ctx, device, et, data, state);
        }
    }
    else
    {
        co_await processGenerate(ctx, device, eventType, data, state);
    }

    saveState(state);
    info("Done");
    ctx.request_stop();
}

auto runResolve(sdbusplus::async::context& ctx, const std::string& device,
                const std::string& eventType) -> sdbusplus::async::task<>
{
    auto data = getDeviceData(device);
    auto state = loadState();

    if (eventType == "all")
    {
        for (const auto& et : data.supportedEvents)
        {
            co_await processResolve(ctx, device, et, data, state);
        }
    }
    else
    {
        co_await processResolve(ctx, device, eventType, data, state);
    }

    saveState(state);
    info("Done");
    ctx.request_stop();
}

} // namespace event_emulator
