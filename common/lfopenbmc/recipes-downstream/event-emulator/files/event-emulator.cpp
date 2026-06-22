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
              << "  " << prog << " <device-type> <event-type>\n"
              << "  " << prog << " resolve <device-type> <event-type>\n"
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
              << "  " << prog << " bbu all\n"
              << "  " << prog << " resolve psu power-fault\n"
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

    if (argc == 2)
    {
        std::string device = argv[1];
        if (isValidDevice(device))
        {
            auto data = getDeviceData(device);
            printDeviceHelp(argv[0], device, data);
            return 0;
        }
        printUsage(argv[0]);
        return 1;
    }

    if (argc == 3)
    {
        std::string device = argv[1];
        std::string eventType = argv[2];

        if (!isValidDevice(device))
        {
            std::cerr << "Error: Unknown device type '" << device << "'\n\n";
            printUsage(argv[0]);
            return 1;
        }

        auto data = getDeviceData(device);
        if (!isValidEventForDevice(eventType, data))
        {
            std::cerr << "Error: '" << eventType << "' is not supported for "
                      << device << "\n\n";
            printDeviceHelp(argv[0], device, data);
            return 1;
        }

        sdbusplus::async::context ctx;
        ctx.spawn(runGenerate(ctx, device, eventType));
        ctx.run();
        return 0;
    }

    if (argc == 4 && std::string(argv[1]) == "resolve")
    {
        std::string device = argv[2];
        std::string eventType = argv[3];

        if (!isValidDevice(device))
        {
            std::cerr << "Error: Unknown device type '" << device << "'\n\n";
            printUsage(argv[0]);
            return 1;
        }

        auto data = getDeviceData(device);
        if (!isValidEventForDevice(eventType, data))
        {
            std::cerr << "Error: '" << eventType << "' is not supported for "
                      << device << "\n\n";
            printDeviceHelp(argv[0], device, data);
            return 1;
        }

        sdbusplus::async::context ctx;
        ctx.spawn(runResolve(ctx, device, eventType));
        ctx.run();
        return 0;
    }

    printUsage(argc > 0 ? argv[0] : "event-emulator");
    return 1;
}
