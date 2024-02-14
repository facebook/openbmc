#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"
#include "utils/string.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>

namespace mfgtool::cmds::fan_mode
{

PHOSPHOR_LOG2_USING;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("fan-mode", "Manipulate the fan mode");
        auto manual = cmd->add_flag("-m,--manual", arg_manual,
                                    "Set manual mode");
        cmd->add_flag("-a,--auto", arg_auto, "Set auto mode")->excludes(manual);

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        namespace control = dbuspath::control;
        using utils::string::last_element;

        auto result = R"({})"_json;

        co_await utils::mapper::subtree_for_each(
            ctx, "/", control::mode::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            if (!path.str.starts_with(FanControlPrefix))
            {
                co_return;
            }

            auto mode =
                control::mode::Proxy(ctx).service(service).path(path.str);

            if (arg_manual || arg_auto)
            {
                co_await mode.manual(arg_manual);
                result[last_element(path)] = arg_manual ? "manual" : "auto";
            }
            else
            {
                result[last_element(path)] = (co_await mode.manual()) ? "manual"
                                                                      : "auto";
            }
        });

        json::display(result);
    }

    bool arg_manual = false;
    bool arg_auto = false;

    static constexpr auto FanControlPrefix =
        "/xyz/openbmc_project/settings/fanctrl";
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::fan_mode
