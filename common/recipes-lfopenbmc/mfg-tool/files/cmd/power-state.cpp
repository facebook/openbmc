#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>

namespace mfgtool::cmds::power_state
{
PHOSPHOR_LOG2_USING;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("power-state", "Get state of device");
        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        using namespace dbuspath;
        using utils::mapper::subtree_for_each;

        auto result = R"({})"_json;

        debug("Finding chasses.");
        co_await subtree_for_each(
            ctx, chassis::ns_path, chassis::interface,

            [&](auto& path, auto& service) -> sdbusplus::async::task<> {
            static auto pathPrefix = chassis::path_prefix();
            if (path.str.size() <= pathPrefix.size())
            {
                error("Unexpected chassis path found: {PATH}", "PATH", path);
                co_return;
            }

            auto proxy = chassis::Proxy(ctx).service(service).path(path.str);
            auto state = (chassis::Proxy::PowerState::Off ==
                          co_await proxy.current_power_state())
                             ? "off"
                             : "on";

            info("State for {PATH}: {STATE}", "PATH", path, "STATE", state);
            auto id = path.str.substr(pathPrefix.size());
            result[id]["standby"] = state;
        });

        debug("Finding hosts.");
        co_await subtree_for_each(
            ctx, host::ns_path, host::interface,

            [&](auto& path, auto& service) -> sdbusplus::async::task<> {
            static auto pathPrefix = host::path_prefix();
            if (path.str.size() <= pathPrefix.size())
            {
                error("Unexpected host path found: {PATH}", "PATH", path);
                co_return;
            }

            auto proxy = host::Proxy(ctx).service(service).path(path.str);
            auto state = (host::Proxy::HostState::Off ==
                          co_await proxy.current_host_state())
                             ? "off"
                             : "on";

            info("State for {PATH}: {STATE}", "PATH", path, "STATE", state);
            auto id = path.str.substr(pathPrefix.size());
            result[id]["runtime"] = state;
        });

        json::display(result);

        co_return;
    }
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::power_state
