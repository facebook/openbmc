#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"
#include "utils/string.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>

namespace mfgtool::cmds::valve_control
{

PHOSPHOR_LOG2_USING;

enum class action
{
    open,
    close
};

enum class direction
{
    supplyDirection,
    returnDirection
};

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("valve-control",
                                      "Manipulate valve open/close state");

        cmd->add_option("-p,--position", arg_pos, "Valve position")->required();

        cmd->add_option("-d,--direction", arg_direction,
                        "Valve direction for liquid flow")
            ->required()
            ->check(CLI::IsMember(keys(direction_map())));

        cmd->add_option("-a,--action", arg_action, "Control action")
            ->required()
            ->check(CLI::IsMember(keys(action_map())));

        init_callback(cmd, *this);
    }

    size_t arg_pos = 0;
    std::string arg_direction = "";
    std::string arg_action = "";

    static auto action_map() -> std::map<std::string, action>
    {
        return {{"open", action::open}, {"close", action::close}};
    }

    static auto direction_map() -> std::map<std::string, direction>
    {
        return {{"supply", direction::supplyDirection},
                {"return", direction::returnDirection}};
    }

    static auto keys(const auto&& m) -> std::vector<std::string>
    {
        return std::views::keys(m) | std::ranges::to<std::vector>();
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        namespace valveControl = dbuspath::control::valve;
        using utils::string::last_element;

        auto result = json::empty_map();

        debug("Finding Control.Valve objects.");
        co_await utils::mapper::subtree_for_each(
            ctx, "/", valveControl::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
                if (!path.str.starts_with(ControlPathPrefix))
                {
                    co_return;
                }

                auto pathSuffix = last_element(path);
                auto lowerPathSuffix = pathSuffix;
                std::transform(lowerPathSuffix.begin(), lowerPathSuffix.end(),
                               lowerPathSuffix.begin(),
                               [](unsigned char c) { return std::tolower(c); });

                if (!pathSuffix.ends_with(std::format("_{}", arg_pos)) ||
                    !lowerPathSuffix.starts_with(arg_direction))
                {
                    co_return;
                }

                auto control =
                    valveControl::Proxy(ctx).service(service).path(path.str);

                switch (action_map()[arg_action])
                {
                    case action::open:
                        co_await control.state(
                            valveControl::Proxy::State::Open);
                        break;
                    case action::close:
                        co_await control.state(
                            valveControl::Proxy::State::Close);
                        break;
                }
                result[pathSuffix] = "success";
            });

        if (result.empty())
        {
            json::display("failed");
            co_return;
        }
        json::display(result);
    }

    static constexpr auto ControlPathPrefix =
        "/xyz/openbmc_project/control/valve";
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::valve_control
