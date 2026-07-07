#include "utils/device_events.hpp"
#include "utils/device_registry.hpp"
#include "utils/event_actions.hpp"

#include <CLI/CLI.hpp>
#include <sdbusplus/async.hpp>

#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <vector>

namespace event_emulator
{

// Per-device parsed arguments bound to CLI11 options.
struct DeviceArgs
{
    std::string device;
    std::string event;
    std::string name;
    bool resolve = false;
    bool selected = false;
};

} // namespace event_emulator

int main(int argc, char* argv[])
{
    using namespace event_emulator;

    CLI::App app{
        "Emulate OpenBMC device events (PSU/BBU/CBU etc.) for testing.\n"
        "\n"
        "Every generated event has '_EMULATED' appended to its name to\n"
        "distinguish emulated events from production ones."};
    app.require_subcommand(1);

    std::list<DeviceArgs> devArgs;
    for (const auto& [name, factory] : getDeviceRegistry())
    {
        auto data = factory();
        std::vector<std::string> events = data.supportedEvents;
        events.emplace_back("all");

        DeviceArgs& args = devArgs.emplace_back();
        args.device = name;

        auto* sub = app.add_subcommand(name, "Emulate " + name + " events");
        sub->add_option("event", args.event, "Event type")
            ->required()
            ->check(CLI::IsMember(events));
        sub->add_option("name", args.name,
                        "Optional EventName override (leaf of the object "
                        "path). Cannot be used with 'all'.");
        sub->add_flag("-r,--resolve", args.resolve,
                      "Resolve a previously generated event instead of "
                      "generating a new one");
        sub->callback([&args]() { args.selected = true; });
    }

    app.footer("Examples:\n"
               "  event-emulator psu power-fault\n"
               "  event-emulator psu power-fault PSU_3_2_CUSTOM_ALARM\n"
               "  event-emulator psu power-fault --resolve\n"
               "  event-emulator bbu all");

    CLI11_PARSE(app, argc, argv);

    auto it = std::ranges::find_if(
        devArgs, [](const DeviceArgs& args) { return args.selected; });
    // require_subcommand(1) guarantees exactly one device was selected.
    if (it == devArgs.end())
    {
        return 1;
    }
    const DeviceArgs* selected = &*it;

    if (!selected->name.empty() && selected->event == "all")
    {
        std::cerr << "Error: EventName cannot be used with 'all'\n";
        return 1;
    }

    sdbusplus::async::context ctx;
    if (selected->resolve)
    {
        ctx.spawn(
            runResolve(ctx, selected->device, selected->event, selected->name));
    }
    else
    {
        ctx.spawn(
            runGenerate(ctx, selected->device, selected->event,
                        selected->name));
    }
    ctx.run();
    return 0;
}
