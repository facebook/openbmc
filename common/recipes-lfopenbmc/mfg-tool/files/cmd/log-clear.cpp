#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/register.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>

namespace mfgtool::cmds::log_clear
{
PHOSPHOR_LOG2_USING;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("log-clear", "Clear logs");
        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        namespace delete_all = dbuspath::delete_all;
        namespace log_entry = dbuspath::log_entry;

        info("Calling DeleteAll on Logging service");
        co_await delete_all::Proxy(ctx)
            .service(log_entry::service)
            .path(log_entry::ns_path)
            .delete_all();
    }
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::log_clear
