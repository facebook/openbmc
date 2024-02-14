#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"
#include "utils/string.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/message.hpp>

#include <cmath>
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
        using utils::string::last_element;

        auto result = R"({})"_json;

        info("Finding sensor entries.");
        co_await utils::mapper::subtree_for_each(
            ctx, sensor::ns_path, sensor::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            auto& entry_json = result[last_element(path)];
            auto proxy = sensor::Proxy(ctx).service(service).path(path.str);

            auto value = co_await proxy.value();
            entry_json["value"] = value;
            entry_json["status"] = std::isfinite(value) ? "ok" : "unavailable";

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

        info("Finding HardShutdown thresholds");
        co_await utils::mapper::subtree_for_each(
            ctx, sensor::ns_path, sensor::hard_shutdown::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            auto& sensor_json = result[last_element(path)];
            auto& entry_json = sensor_json["hard-shutdown"];
            auto proxy =
                sensor::hard_shutdown::Proxy(ctx).service(service).path(
                    path.str);

            if (auto v = co_await proxy.hard_shutdown_high(); std::isfinite(v))
            {
                entry_json["high"] = v;
                update_status<std::greater>(sensor_json, v, "critical");
            }
            if (auto v = co_await proxy.hard_shutdown_low(); std::isfinite(v))
            {
                entry_json["low"] = v;
                update_status<std::less>(sensor_json, v, "critical");
            }
        });

        info("Finding Critical thresholds");
        co_await utils::mapper::subtree_for_each(
            ctx, sensor::ns_path, sensor::critical::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            auto& sensor_json = result[last_element(path)];
            auto& entry_json = sensor_json["critical"];
            auto proxy =
                sensor::critical::Proxy(ctx).service(service).path(path.str);

            if (auto v = co_await proxy.critical_high(); std::isfinite(v))
            {
                entry_json["high"] = v;
                update_status<std::greater>(sensor_json, v, "critical");
            }
            if (auto v = co_await proxy.critical_low(); std::isfinite(v))
            {
                entry_json["low"] = v;
                update_status<std::less>(sensor_json, v, "critical");
            }
        });

        info("Finding Warning thresholds");
        co_await utils::mapper::subtree_for_each(
            ctx, sensor::ns_path, sensor::warning::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            auto& sensor_json = result[last_element(path)];
            auto& entry_json = sensor_json["warning"];
            auto proxy =
                sensor::warning::Proxy(ctx).service(service).path(path.str);

            if (auto v = co_await proxy.warning_high(); std::isfinite(v))
            {
                entry_json["high"] = v;
                update_status<std::greater>(sensor_json, v, "warning");
            }
            if (auto v = co_await proxy.warning_low(); std::isfinite(v))
            {
                entry_json["low"] = v;
                update_status<std::less>(sensor_json, v, "warning");
            }
        });

        json::display(result);

        co_return;
    }

    template <template <typename> typename comparison>
    static auto update_status(auto& entry_json, auto value, const auto& name)
    {
        if (!entry_json.contains("status"))
        {
            entry_json["status"] = "unavailable";
            return;
        }

        auto& status = entry_json["status"];

        if (status != "ok")
        {
            return;
        }

        if (comparison<decltype(value)>()(entry_json["value"], value))
        {
            status = name;
        }
    }
};
MFGTOOL_REGISTER(command);
} // namespace mfgtool::cmds::sensor_display
