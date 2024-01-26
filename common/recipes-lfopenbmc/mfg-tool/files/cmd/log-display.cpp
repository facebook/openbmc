#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/message.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <format>
#include <string>

namespace mfgtool::cmds::log_display
{
PHOSPHOR_LOG2_USING;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("log-display", "Display pending logs.");
        cmd->add_flag("-u,--unresolved-only", arg_unresolved_only,
                      "Only show unresolved logs.");

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        namespace log_entry = dbuspath::log_entry;

        auto result = R"({})"_json;

        info("Finding log entries.");
        auto objects = co_await utils::mapper::subtree_services(
            ctx, log_entry::ns_path, log_entry::interface);

        for (auto& [path, services] : objects)
        {
            if (!std::ranges::contains(services, log_entry::service))
            {
                warning("Entry ({PATH}) not hosted by logging service.", "PATH",
                        path);
                continue;
            }

            auto entry = log_entry::Proxy(ctx)
                             .service(log_entry::service)
                             .path(path.str);

            info("Getting properties for {PATH}", "PATH", path);
            if (auto resolved = co_await entry.resolved();
                !resolved || !arg_unresolved_only)
            {
                auto& entry_json = result[std::to_string(co_await entry.id())];

                entry_json["message"] = co_await entry.message();
                entry_json["severity"] = sdbusplus::message::convert_to_string(
                    co_await entry.severity());
                entry_json["event_id"] = co_await entry.event_id();
                entry_json["additional_data"] =
                    co_await entry.additional_data();
                entry_json["resolution"] = co_await entry.resolution();
                entry_json["resolved"] = resolved;

                auto epoch_to_iso8601 = [](auto ts) {
                    using namespace std::chrono;
                    return std::format("{:%FT%TZ}", time_point<system_clock>(
                                                        milliseconds(ts)));
                };

                entry_json["timestamp"] =
                    epoch_to_iso8601(co_await entry.timestamp());
                entry_json["updated_timestamp"] =
                    epoch_to_iso8601(co_await entry.update_timestamp());
            }
            else
            {
                info("Resolved and filtered out.");
            }
        }

        json::display(result);
        co_return;
    }

    bool arg_unresolved_only = false;
};
MFGTOOL_REGISTER(command);
} // namespace mfgtool::cmds::log_display
