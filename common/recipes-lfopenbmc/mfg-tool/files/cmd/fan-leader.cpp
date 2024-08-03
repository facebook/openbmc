#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"
#include "utils/string.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>

namespace mfgtool::cmds::fan_leader
{

PHOSPHOR_LOG2_USING;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand(
            "fan-leader", "Display the current leader for each Zone.");

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        namespace pid_zone = dbuspath::debug::pid_zone;
        using utils::string::last_element;

        auto result = json::empty_map();

        debug("Finding Debug.Pid.Zone objects.");
        co_await utils::mapper::subtree_for_each(
            ctx, "/", pid_zone::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            result[last_element(path)] = co_await pid_zone::Proxy(ctx)
                                             .service(service)
                                             .path(path.str)
                                             .leader();
        });

        json::display(result);
    }
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::fan_leader
