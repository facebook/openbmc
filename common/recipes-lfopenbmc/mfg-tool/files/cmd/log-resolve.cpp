#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/register.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>

#include <cstdint>
#include <string>

namespace mfgtool::cmds::log_resolve
{
PHOSPHOR_LOG2_USING;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("log-resolve", "Resolve a log entry");

        cmd->add_option("-i,--id", arg_id, "Entry ID")->required();

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        namespace log_entry = dbuspath::log_entry;

        try
        {
            auto path = log_entry::path(arg_id);

            info("Setting resolved on {PATH}", "PATH", path);
            co_await log_entry::Proxy(ctx)
                .service(log_entry::service)
                .path(path)
                .resolved(true);

            json::display("success");
        }
        catch (const std::exception& e)
        {
            error("Caught {ERROR}", "ERROR", e);
            json::display("failed");
        }

        co_return;
    }

    size_t arg_id = 0;
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::log_resolve
