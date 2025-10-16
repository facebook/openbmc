#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/redfish-registry.hpp"
#include "utils/register.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/message.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
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

        const auto event_defs = utils::redfish_registry::load();

        auto result = json::empty_map();
        auto log_value = std::vector<std::pair<std::string, js>>{};

        debug("Finding log entries.");
        co_await utils::mapper::subtree_for_each(
            ctx, log_entry::ns_path, log_entry::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
                if (service != log_entry::service)
                {
                    warning("Entry ({PATH}) not hosted by logging service.",
                            "PATH", path);
                    co_return;
                }

                try
                {
                    auto entry = log_entry::Proxy(ctx)
                                     .service(log_entry::service)
                                     .path(path.str);

                    auto properties = co_await entry.properties();

                    if (properties.resolved && arg_unresolved_only)
                    {
                        debug("Resolved and filtered out.");
                        co_return;
                    }

                    auto& entry_json = result[std::to_string(properties.id)];

                    entry_json["message"] = properties.message;
                    entry_json["severity"] =
                        sdbusplus::message::convert_to_string(
                            properties.severity);
                    entry_json["event_id"] = properties.event_id;

                    entry_json["additional_data"] = properties.additional_data;
                    entry_json["resolution"] = properties.resolution;
                    entry_json["resolved"] = properties.resolved;

                    auto epoch_to_iso8601 = [](auto ts) {
                        using namespace std::chrono;
                        return std::format(
                            "{:%FT%TZ}",
                            time_point<system_clock>(milliseconds(ts)));
                    };

                    entry_json["timestamp"] =
                        epoch_to_iso8601(properties.timestamp);
                    entry_json["updated_timestamp"] =
                        epoch_to_iso8601(properties.update_timestamp);

                    // If the event is defined in the redfish registry, map
                    // it appropriately.
                    if (auto def = event_defs.find(properties.message);
                        def != std::end(event_defs))
                    {
                        using event_t = utils::redfish_registry::event_t;
                        auto redfish = json::empty_map();

                        redfish["id"] = std::get<event_t>(*def).id;

                        std::vector<std::string> args = {};
                        for (const auto& arg : std::get<event_t>(*def).args)
                        {
                            if (properties.additional_data.contains(arg))
                            {
                                args.emplace_back(
                                    properties.additional_data[arg]);
                            }
                            else
                            {
                                args.emplace_back("");
                            }
                        }

                        if (auto message = std::get<event_t>(*def).message;
                            !message.empty())
                        {
                            for (size_t i = args.size(); i > 0; --i)
                            {
                                auto tag = "%" + std::to_string(i);
                                if (auto pos = message.find(tag);
                                    pos != std::string::npos)
                                {
                                    message.replace(pos, tag.size(),
                                                    args[i - 1]);
                                }
                            }
                            redfish["message"] = std::move(message);
                        }

                        redfish["args"] = std::move(args);
                        entry_json["redfish"] = std::move(redfish);
                    }
                    log_value.emplace_back(std::to_string(properties.id),
                                           std::move(entry_json));
                }
                catch (...)
                {
                    warning("Failed to parse log entry: {PATH}", "PATH",
                            path.str);
                }
            });
        json::sort_json_values(log_value);
        auto json_result = json::merge_duplicate_keys_to_json(log_value);
        json::display(json_result);
        co_return;
    }

    bool arg_unresolved_only = false;
};
MFGTOOL_REGISTER(command);
} // namespace mfgtool::cmds::log_display
