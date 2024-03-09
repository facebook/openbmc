#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>

#include <string>
#include <unordered_map>

namespace mfgtool::cmds::power_state
{
PHOSPHOR_LOG2_USING;
using namespace dbuspath;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("power-state", "Get state of device");
        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        using utils::mapper::subtree_for_each;

        auto result = json::empty_map();

        debug("Finding chasses.");
        co_await subtree_for_each(ctx, chassis::ns_path, chassis::interface,

                                  [&](const auto& path, const auto& service)
                                      -> sdbusplus::async::task<> {
            static auto pathPrefix = chassis::path_prefix();
            if (path.str.size() <= pathPrefix.size())
            {
                error("Unexpected chassis path found: {PATH}", "PATH", path);
                co_return;
            }

            auto proxy = chassis::Proxy(ctx).service(service).path(path.str);
            auto state = mapChassisState(co_await proxy.current_power_state());

            info("State for {PATH}: {STATE}", "PATH", path, "STATE", state);
            auto id = path.str.substr(pathPrefix.size());
            result[id]["standby"] = state;
        });

        debug("Finding hosts.");
        co_await subtree_for_each(ctx, host::ns_path, host::interface,

                                  [&](const auto& path, const auto& service)
                                      -> sdbusplus::async::task<> {
            static auto pathPrefix = host::path_prefix();
            if (path.str.size() <= pathPrefix.size())
            {
                error("Unexpected host path found: {PATH}", "PATH", path);
                co_return;
            }

            auto proxy = host::Proxy(ctx).service(service).path(path.str);
            auto state = mapHostState(co_await proxy.current_host_state());

            info("State for {PATH}: {STATE}", "PATH", path, "STATE", state);
            auto id = path.str.substr(pathPrefix.size());
            result[id]["runtime"] = state;
        });

        json::display(result);

        co_return;
    }

    static auto mapChassisState(chassis::Proxy::PowerState state) -> std::string
    {
        using PowerState = chassis::Proxy::PowerState;

        static const auto values = std::unordered_map<PowerState, std::string>{
            {PowerState::Off, "off"},
            {PowerState::TransitioningToOff, "transition-off"},
            {PowerState::On, "on"},
            {PowerState::TransitioningToOn, "transition-on"},
        };

        if (!values.contains(state))
        {
            return "unknown";
        }

        return values.at(state);
    }

    static auto mapHostState(host::Proxy::HostState state) -> std::string
    {
        using HostState = host::Proxy::HostState;

        static const auto values = std::unordered_map<HostState, std::string>{
            {HostState::Off, "off"},
            {HostState::TransitioningToOff, "transition-off"},
            {HostState::Running, "on"},
            {HostState::TransitioningToRunning, "transition-on"},
        };

        if (!values.contains(state))
        {
            return "unknown";
        }

        return values.at(state);
    }
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::power_state
