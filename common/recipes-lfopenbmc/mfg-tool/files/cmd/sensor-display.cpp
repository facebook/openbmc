#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/message.hpp>

#include <ranges>

namespace mfgtool::cmds::sensor_display
{
PHOSPHOR_LOG2_USING;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("sensor-display", "Display sensors.");

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        namespace sensor = dbuspath::sensor;

        auto result = R"({})"_json;

        info("Finding sensor entries.");
        co_await utils::mapper::subtree_for_each(
            ctx, sensor::ns_path, sensor::interface,

            [&](auto& path, auto& service) -> sdbusplus::async::task<> {
            auto& entry_json = result[last_element(path.str, '/')];
            auto proxy = sensor::Proxy(ctx).service(service).path(path.str);

            entry_json["value"] = co_await proxy.value();
            if (auto v = co_await proxy.max_value(); std::isfinite(v))
            {
                entry_json["max"] = v;
            }
            if (auto v = co_await proxy.min_value(); std::isfinite(v))
            {
                entry_json["min"] = v;
            }
            entry_json["unit"] = last_element(
                sdbusplus::message::convert_to_string(co_await proxy.unit()),
                '.');
        });

        info("Finding Warning thresholds");
        co_await utils::mapper::subtree_for_each(
            ctx, sensor::ns_path, sensor::warning::interface,

            [&](auto& path, auto& service) -> sdbusplus::async::task<> {
            auto& entry_json = result[last_element(path.str, '/')]["warning"];
            auto proxy =
                sensor::warning::Proxy(ctx).service(service).path(path.str);

            if (auto v = co_await proxy.warning_high(); std::isfinite(v))
            {
                entry_json["high"] = v;
            }
            if (auto v = co_await proxy.warning_low(); std::isfinite(v))
            {
                entry_json["low"] = v;
            }
        });

        info("Finding Critical thresholds");
        co_await utils::mapper::subtree_for_each(
            ctx, sensor::ns_path, sensor::critical::interface,

            [&](auto& path, auto& service) -> sdbusplus::async::task<> {
            auto& entry_json = result[last_element(path.str, '/')]["critical"];
            auto proxy =
                sensor::critical::Proxy(ctx).service(service).path(path.str);

            if (auto v = co_await proxy.critical_high(); std::isfinite(v))
            {
                entry_json["high"] = v;
            }
            if (auto v = co_await proxy.critical_low(); std::isfinite(v))
            {
                entry_json["low"] = v;
            }
        });

        info("Finding HardShutdown thresholds");
        co_await utils::mapper::subtree_for_each(
            ctx, sensor::ns_path, sensor::hard_shutdown::interface,

            [&](auto& path, auto& service) -> sdbusplus::async::task<> {
            auto& entry_json =
                result[last_element(path.str, '/')]["hard_shutdown"];
            auto proxy =
                sensor::hard_shutdown::Proxy(ctx).service(service).path(
                    path.str);

            if (auto v = co_await proxy.hard_shutdown_high(); std::isfinite(v))
            {
                entry_json["high"] = v;
            }
            if (auto v = co_await proxy.hard_shutdown_low(); std::isfinite(v))
            {
                entry_json["low"] = v;
            }
        });

        json::display(result);

        co_return;
    }

    static auto last_element(const auto& s, char c)
    {
        return s.substr(s.find_last_of(c) + 1);
    }
};
MFGTOOL_REGISTER(command);
} // namespace mfgtool::cmds::sensor_display
