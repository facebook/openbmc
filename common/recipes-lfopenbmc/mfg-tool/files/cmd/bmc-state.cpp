#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>

#include <unordered_map>

namespace mfgtool::cmds::bmc_state
{
PHOSPHOR_LOG2_USING;
using namespace dbuspath;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("bmc-state", "Get state of the BMC");
        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        using utils::mapper::object_service;

        auto result = json::empty_map();

        debug("Finding BMC.");
        auto path = bmc::path();
        auto service = co_await object_service(ctx, path, bmc::interface);

        if (!service)
        {
            warning("Could not find BMC object");
            co_return;
        }

        debug("Getting BMC state.");
        auto proxy = bmc::Proxy(ctx).service(*service).path(path);
        result["state"] = mapBMCState(co_await proxy.current_bmc_state());

        json::display(result);
    }

    static auto mapBMCState(bmc::Proxy::BMCState state) -> std::string
    {
        using BMCState = bmc::Proxy::BMCState;

        static const auto values = std::unordered_map<BMCState, std::string>{
            {BMCState::Ready, "ready"},
            {BMCState::NotReady, "starting"},
            {BMCState::Quiesced, "quiesced"},
        };

        if (!values.contains(state))
        {
            return "unknown";
        }

        return values.at(state);
    }
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::bmc_state
