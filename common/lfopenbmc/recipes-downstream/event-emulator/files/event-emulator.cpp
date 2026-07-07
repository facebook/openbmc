#include "utils/device_registry.hpp"
#include "utils/event_actions.hpp"

#include <sdbusplus/async.hpp>

#include <iostream>
#include <string>

namespace event_emulator
{

void printUsage(const char* prog)
{
    std::cerr << "Usage:\n"
              << "  " << prog << " <device-type> <event-type> [EventName]\n"
              << "  " << prog
              << " resolve <device-type> <event-type> [EventName]\n"
              << "\n"
              << "EventName (optional) overrides the leaf of the event object\n"
              << "path/name. If omitted, the default is used. It cannot be\n"
              << "used with 'all'.\n"
              << "\n"
              << "Device types:\n";

    for (const auto& [name, _] : getDeviceRegistry())
    {
        std::cerr << "  " << name << "\n";
    }

    std::cerr << "\n"
              << "Use '" << prog << " <device-type>' to see supported events.\n"
              << "\n"
              << "Examples:\n"
              << "  " << prog << " psu power-fault\n"
              << "  " << prog << " psu power-fault PSU_3_2_CUSTOM_ALARM\n"
              << "  " << prog << " bbu all\n"
              << "  " << prog << " resolve psu power-fault\n"
              << "  " << prog
              << " resolve psu power-fault PSU_3_2_CUSTOM_ALARM\n"
              << "  " << prog << " resolve cbu all\n";
}

void printDeviceHelp(const char* prog, const std::string& device,
                     const DeviceEventData& data)
{
    std::cerr << "Supported events for " << device << ":\n";
    for (const auto& event : data.supportedEvents)
    {
        std::cerr << "  " << event << "\n";
    }
    std::cerr << "  all                 Generate/resolve all events\n"
              << "\n"
              << "Examples:\n"
              << "  " << prog << " " << device << " " << data.supportedEvents[0]
              << "\n"
              << "  " << prog << " " << device << " all\n"
              << "  " << prog << " resolve " << device << " "
              << data.supportedEvents[0] << "\n";
}

} // namespace event_emulator

int main(int argc, char* argv[])
{
    using namespace event_emulator;

    const char* prog = argv[0];

    if (argc < 2)
    {
        printUsage(prog);
        return 1;
    }

    if (argc == 2)
    {
        std::string device = argv[1];
        if (isValidDevice(device))
        {
            auto data = getDeviceData(device);
            printDeviceHelp(prog, device, data);
            return 0;
        }
        printUsage(prog);
        return 1;
    }

    bool resolve = std::string(argv[1]) == "resolve";
    std::string device;
    std::string eventType;
    std::string eventName;

    if (resolve)
    {
        // resolve <device-type> <event-type> [EventName]
        if (argc != 4 && argc != 5)
        {
            printUsage(prog);
            return 1;
        }
        device = argv[2];
        eventType = argv[3];
        if (argc == 5)
        {
            eventName = argv[4];
        }
    }
    else
    {
        // <device-type> <event-type> [EventName]
        if (argc != 3 && argc != 4)
        {
            printUsage(prog);
            return 1;
        }
        device = argv[1];
        eventType = argv[2];
        if (argc == 4)
        {
            eventName = argv[3];
        }
    }

    if (!isValidDevice(device))
    {
        std::cerr << "Error: Unknown device type '" << device << "'\n\n";
        printUsage(prog);
        return 1;
    }

    auto data = getDeviceData(device);
    if (!isValidEventForDevice(eventType, data))
    {
        std::cerr << "Error: '" << eventType << "' is not supported for "
                  << device << "\n\n";
        printDeviceHelp(prog, device, data);
        return 1;
    }

    if (!eventName.empty() && eventType == "all")
    {
        std::cerr << "Error: EventName cannot be used with 'all'\n\n";
        printDeviceHelp(prog, device, data);
        return 1;
    }

    sdbusplus::async::context ctx;
    if (resolve)
    {
        ctx.spawn(runResolve(ctx, device, eventType, eventName));
    }
    else
    {
        ctx.spawn(runGenerate(ctx, device, eventType, eventName));
    }
    ctx.run();
    return 0;
}
