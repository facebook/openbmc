#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>

#include <map>
#include <ranges>
#include <string>

namespace mfgtool::cmds::power_control
{

PHOSPHOR_LOG2_USING;
namespace mapper = mfgtool::utils::mapper;

enum class action
{
    on,
    off,
    cycle
};

enum class scope
{
    runtime,
    standby,
    acpi
};

template <action Action, scope Scope>
struct execute;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("power-control",
                                      "Manipulate power state of a device");

        cmd->add_option("-p,--position", arg_pos, "Device position")
            ->required();

        cmd->add_option("-a,--action", arg_action, "Control action")
            ->required()
            ->check(CLI::IsMember(keys(action_map())));

        cmd->add_option("-s,--scope", arg_scope, "Scope of the action")
            ->check(CLI::IsMember(keys(scope_map())));

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        info("Attempting {SCOPE}:{ACTION} on {POS}.", "POS", arg_pos, "ACTION",
             arg_action, "SCOPE", arg_scope);

        try
        {
            co_await execute(ctx);
        }
        catch (const std::exception& e)
        {
            error("Caught exception: {ERROR}", "ERROR", e);
            json::display("failed");
            co_return;
        }

        json::display("success");
        co_return;
    }

    size_t arg_pos = 0;
    std::string arg_action = "";
    std::string arg_scope = "runtime";

    static auto action_map() -> std::map<std::string, action>
    {
        return {
            {"on", action::on}, {"off", action::off}, {"cycle", action::cycle}};
    }
    static auto scope_map() -> std::map<std::string, scope>
    {
        return {{"runtime", scope::runtime},
                {"standby", scope::standby},
                {"acpi", scope::acpi}};
    }

    static auto keys(const auto&& m) -> std::vector<std::string>
    {
        // This should have been implemented as views::keys(action_map) |
        // ranges::to<std::vector> but ranges::to isn't ready until GCC14.

        std::vector<std::string> r{};
        std::ranges::for_each(std::views::keys(m),
                              [&](const auto& v) { r.push_back(v); });

        return r;
    }

    auto execute(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        switch (action_map()[arg_action])
        {
            case action::on:
                co_await execute<action::on>(ctx);
                break;

            case action::off:
                co_await execute<action::off>(ctx);
                break;

            case action::cycle:
                co_await execute<action::cycle>(ctx);
                break;
        }
    }

    template <action Action>
    auto execute(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        switch (scope_map()[arg_scope])
        {
            case scope::runtime:
                co_await power_control::execute<Action, scope::runtime>::op(
                    ctx, arg_pos);
                break;

            case scope::standby:
                co_await power_control::execute<Action, scope::standby>::op(
                    ctx, arg_pos);
                break;

            case scope::acpi:
                co_await power_control::execute<Action, scope::acpi>::op(
                    ctx, arg_pos);
                break;
        }
    }
};
MFGTOOL_REGISTER(command);

template <action Action, scope Scope>
struct execute
{
    static auto op(sdbusplus::async::context&, size_t)
        -> sdbusplus::async::task<>
    {
        error("Execution method undefined.");
        throw std::invalid_argument("execution method");
    }
};

template <action Action>
struct execute<Action, scope::standby>
{
    static auto op(sdbusplus::async::context& ctx, size_t pos)
        -> sdbusplus::async::task<>
    {
        using namespace dbuspath;
        auto path = chassis::path(pos);

        auto service = co_await mapper::object_service(ctx, path,
                                                       chassis::interface);

        if (!service)
        {
            warning("Can't find {PATH}", "PATH", path);
            error(
                "Cannot find chassis to execute operation against; giving up.");
            throw std::runtime_error("missing chassis");
        }

        auto proxy = chassis::Proxy(ctx).service(*service).path(path);

        if constexpr (Action == action::on)
        {
            co_await proxy.requested_power_transition(
                chassis::Proxy::Transition::On);
        }
        else if constexpr (Action == action::off)
        {
            co_await proxy.requested_power_transition(
                chassis::Proxy::Transition::Off);
        }
        else
        {
            co_await proxy.requested_power_transition(
                chassis::Proxy::Transition::PowerCycle);
        }
    }
};

template <action Action>
struct execute<Action, scope::runtime>
{
    static auto op(sdbusplus::async::context& ctx, size_t pos)
        -> sdbusplus::async::task<>
    {
        using namespace dbuspath;

        auto path = host::path(pos);
        auto service = co_await mapper::object_service(ctx, path,
                                                       host::interface);

        if (!service)
        {
            warning("Can't find {PATH}", "PATH", path);
            info("Trying to escalate to scope=standby");
            co_return co_await execute<Action, scope::standby>::op(ctx, pos);
        }

        auto proxy = host::Proxy(ctx).service(*service).path(path);

        if constexpr (Action == action::on)
        {
            co_await proxy.requested_host_transition(
                host::Proxy::Transition::On);
        }
        else if constexpr (Action == action::off)
        {
            co_await proxy.requested_host_transition(
                host::Proxy::Transition::Off);
        }
        else
        {
            co_await proxy.requested_host_transition(
                host::Proxy::Transition::Reboot);
        }
    }
};

template <>
struct execute<action::cycle, scope::acpi>
{
    static auto op(sdbusplus::async::context& ctx, size_t pos)
        -> sdbusplus::async::task<>
    {
        using namespace dbuspath;

        auto path = host::path(pos);
        auto service = co_await mapper::object_service(ctx, path,
                                                       host::interface);

        if (!service)
        {
            warning("Can't find {PATH}", "PATH", path);
            info("Trying to escalate to scope=standby");
            co_return co_await execute<action::cycle, scope::standby>::op(ctx,
                                                                          pos);
        }

        auto proxy = host::Proxy(ctx).service(*service).path(path);
        co_await proxy.requested_host_transition(
            host::Proxy::Transition::GracefulWarmReboot);
    }
};

} // namespace mfgtool::cmds::power_control
