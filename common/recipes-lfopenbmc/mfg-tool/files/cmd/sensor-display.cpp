#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"
#include "utils/string.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/message.hpp>

#include <array>
#include <cmath>
#include <ranges>

namespace mfgtool::cmds::sensor_display
{
PHOSPHOR_LOG2_USING;
namespace sensor = dbuspath::sensor;
namespace metric = dbuspath::metric;
namespace threshold = dbuspath::threshold;
using namespace utils::string;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("sensor-display", "Display sensors.");

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        auto result = json::empty_map();

        info("Finding sensor entries.");
        co_await utils::mapper::subtree_for_each(
            ctx, sensor::ns_path, sensor::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            auto& entry_json = result[last_element(path)];
            try
            {
                auto proxy = sensor::Proxy(ctx).service(service).path(path.str);

                auto value = co_await proxy.value();
                entry_json["value"] = value;
                entry_json["status"] = std::isfinite(value) ? "ok"
                                                            : "unavailable";

                if (auto v = co_await proxy.max_value(); std::isfinite(v))
                {
                    entry_json["max"] = v;
                }
                if (auto v = co_await proxy.min_value(); std::isfinite(v))
                {
                    entry_json["min"] = v;
                }
                entry_json["unit"] =
                    last_element(sdbusplus::message::convert_to_string(
                                     co_await proxy.unit()),
                                 '.');
            }
            catch (const sdbusplus::exception::SdBusError& e)
            {
                warning("Failed to get sensor value: {PATH}, error: {ERROR}",
                        "PATH", path.str, "ERROR", e.what());
                entry_json["status"] = "dbus error";
            }
        });

        info("Finding HardShutdown thresholds");
        co_await utils::mapper::subtree_for_each(
            ctx, sensor::ns_path, sensor::hard_shutdown::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            auto& sensor_json = result[last_element(path)];
            auto& entry_json = sensor_json["hard-shutdown"];
            try
            {
                auto proxy =
                    sensor::hard_shutdown::Proxy(ctx).service(service).path(
                        path.str);

                if (auto v = co_await proxy.hard_shutdown_high();
                    std::isfinite(v))
                {
                    entry_json["high"] = v;
                    update_status<std::greater>(sensor_json, v, "critical");
                }
                if (auto v = co_await proxy.hard_shutdown_low();
                    std::isfinite(v))
                {
                    entry_json["low"] = v;
                    update_status<std::less>(sensor_json, v, "critical");
                }
            }
            catch (const sdbusplus::exception::SdBusError& e)
            {
                warning(
                    "Failed to get hard-shutdown thresholds: {PATH}, error: {ERROR}",
                    "PATH", path.str, "ERROR", e.what());
                sensor_json["status"] = "dbus error";
            }
        });

        info("Finding Critical thresholds");
        co_await utils::mapper::subtree_for_each(
            ctx, sensor::ns_path, sensor::critical::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            auto& sensor_json = result[last_element(path)];
            auto& entry_json = sensor_json["critical"];
            try
            {
                auto proxy = sensor::critical::Proxy(ctx).service(service).path(
                    path.str);

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
            }
            catch (const sdbusplus::exception::SdBusError& e)
            {
                warning(
                    "Failed to get critical thresholds: {PATH}, error: {ERROR}",
                    "PATH", path.str, "ERROR", e.what());
                sensor_json["status"] = "dbus error";
            }
        });

        info("Finding Warning thresholds");
        co_await utils::mapper::subtree_for_each(
            ctx, sensor::ns_path, sensor::warning::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            auto& sensor_json = result[last_element(path)];
            auto& entry_json = sensor_json["warning"];
            try
            {
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
            }
            catch (const sdbusplus::exception::SdBusError& e)
            {
                warning(
                    "Failed to get warning thresholds: {PATH}, error: {ERROR}",
                    "PATH", path.str, "ERROR", e.what());
                sensor_json["status"] = "dbus error";
            }
        });

        info("Finding sensor threshold entries.");
        co_await utils::mapper::subtree_for_each(
            ctx, sensor::ns_path, threshold::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            using namespace std::string_literals;

            auto& sensor_json = result[last_element(path)];
            try
            {
                auto proxy =
                    threshold::Proxy(ctx).service(service).path(path.str);

                auto values = co_await proxy.value();
                auto asserted = co_await proxy.asserted();

                for (const auto& [type, type_str] : thresholds)
                {
                    for (const auto& [bound, bound_str] : bounds)
                    {
                        if (values.contains(type) &&
                            values.at(type).contains(bound))
                        {
                            if (auto v = values.at(type).at(bound);
                                std::isfinite(v))
                            {
                                sensor_json[type_str][bound_str] = v;
                                if (asserted.contains({type, bound}))
                                {
                                    update_status(sensor_json, bound_str);
                                }
                            }
                        }
                    }
                }
            }
            catch (const sdbusplus::exception::SdBusError& e)
            {
                warning(
                    "Failed to get sensor threshold entries: {PATH}, error: {ERROR}",
                    "PATH", path.str, "ERROR", e.what());
                sensor_json["status"] = "dbus error";
            }
        });

        info("Finding metric entries.");
        co_await utils::mapper::subtree_for_each(
            ctx, metric::ns_path, metric::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            using namespace std::string_literals;

            auto& entry_json =
                result[replace_substring(path, metric::ns_path + "/"s, "")];
            try
            {
                auto proxy = metric::Proxy(ctx).service(service).path(path.str);

                auto value = co_await proxy.value();
                entry_json["value"] = value;
                entry_json["status"] = std::isfinite(value) ? "ok"
                                                            : "unavailable";
                if (auto v = co_await proxy.max_value(); std::isfinite(v))
                {
                    entry_json["max"] = v;
                }
                if (auto v = co_await proxy.min_value(); std::isfinite(v))
                {
                    entry_json["min"] = v;
                }
                entry_json["unit"] =
                    last_element(sdbusplus::message::convert_to_string(
                                     co_await proxy.unit()),
                                 '.');
            }
            catch (const sdbusplus::exception::SdBusError& e)
            {
                warning("Failed to get metric entries: {PATH}, error: {ERROR}",
                        "PATH", path.str, "ERROR", e.what());
                entry_json["status"] = "dbus error";
            }
        });

        info("Finding metric threshold entries.");
        co_await utils::mapper::subtree_for_each(
            ctx, metric::ns_path, threshold::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            using namespace std::string_literals;

            auto& sensor_json =
                result[replace_substring(path, metric::ns_path + "/"s, "")];
            try
            {
                auto proxy =
                    threshold::Proxy(ctx).service(service).path(path.str);

                auto values = co_await proxy.value();
                auto asserted = co_await proxy.asserted();

                for (const auto& [type, type_str] : thresholds)
                {
                    for (const auto& [bound, bound_str] : bounds)
                    {
                        if (values.contains(type) &&
                            values.at(type).contains(bound))
                        {
                            if (auto v = values.at(type).at(bound);
                                std::isfinite(v))
                            {
                                sensor_json[type_str][bound_str] = v;
                                if (asserted.contains({type, bound}))
                                {
                                    update_status(sensor_json, bound_str);
                                }
                            }
                        }
                    }
                }
            }
            catch (const sdbusplus::exception::SdBusError& e)
            {
                warning(
                    "Failed to get metric threshold entries: {PATH}, error: {ERROR}",
                    "PATH", path.str, "ERROR", e.what());
                sensor_json["status"] = "dbus error";
            }
        });

        json::display(result);

        co_return;
    }

    static constexpr auto thresholds =
        std::to_array<std::tuple<threshold::Proxy::Type, std::string_view>>(
            {{threshold::Proxy::Type::HardShutdown, "hard-shutdown"},
             {threshold::Proxy::Type::Critical, "critical"},
             {threshold::Proxy::Type::Warning, "warning"}});

    static constexpr auto bounds =
        std::to_array<std::tuple<threshold::Proxy::Bound, std::string_view>>(
            {{threshold::Proxy::Bound::Upper, "high"},
             {threshold::Proxy::Bound::Lower, "low"}});

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

    static auto update_status(auto& entry_json, const auto& name)
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

        status = name;
    }
};
MFGTOOL_REGISTER(command);
} // namespace mfgtool::cmds::sensor_display
