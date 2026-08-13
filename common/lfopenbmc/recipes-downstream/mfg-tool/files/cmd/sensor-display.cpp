#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"
#include "utils/string.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/utility/merge_variants.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_map>

namespace mfgtool::cmds::sensor_display
{
PHOSPHOR_LOG2_USING;
namespace sensor = dbuspath::sensor;
namespace metric = dbuspath::metric;
namespace threshold = dbuspath::threshold;
using namespace utils::string;

using DbusVariantType = sdbusplus::utility::merge_variants_t<
    sensor::Proxy::PropertiesVariant, sensor::warning::Proxy::PropertiesVariant,
    sensor::critical::Proxy::PropertiesVariant,
    sensor::hard_shutdown::Proxy::PropertiesVariant,
    threshold::Proxy::PropertiesVariant, metric::Proxy::PropertiesVariant>;

using DBusPropertiesMap = std::unordered_map<std::string, DbusVariantType>;
using DBusInterfacesMap = std::unordered_map<std::string, DBusPropertiesMap>;
using ManagedObjectType = std::map<sdbusplus::object_path, DBusInterfacesMap>;

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
        std::set<std::string> target_services;

        auto sensor_srvs = co_await utils::mapper::subtree_services(
            ctx, sensor::ns_path, sensor::interface, 0);
        for (const auto& [path, services] : sensor_srvs)
        {
            for (const auto& srv : services)
                target_services.insert(srv);
        }

        auto metric_srvs = co_await utils::mapper::subtree_services(
            ctx, metric::ns_path, metric::interface, 0);
        for (const auto& [path, services] : metric_srvs)
        {
            for (const auto& srv : services)
                target_services.insert(srv);
        }

        auto om_objects = co_await utils::mapper::subtree_services(
            ctx, "/", "org.freedesktop.DBus.ObjectManager", 0);

        for (const auto& service : target_services)
        {
            std::vector<std::string_view> om_candidates;
            for (const auto& [path, services] : om_objects)
            {
                if (std::ranges::find(services, service) != services.end())
                {
                    if (path.str.starts_with(sensor::ns_path) ||
                        path.str.starts_with(metric::ns_path) ||
                        path.str == "/")
                    {
                        om_candidates.push_back(path.str);
                    }
                }
            }
            if (om_candidates.empty())
            {
                warning("No ObjectManager found for service {SERVICE}.",
                        "SERVICE", service);
                continue;
            }

            for (const auto& om_path : om_candidates)
            {
                try
                {
                    auto proxy =
                        sdbusplus::async::proxy()
                            .service(service)
                            .path(om_path)
                            .interface("org.freedesktop.DBus.ObjectManager");
                    auto objs = co_await proxy.call<ManagedObjectType>(
                        ctx, "GetManagedObjects");

                    for (const auto& [objpath, interfaces] : objs)
                    {
                        parse_managed_object(result, objpath, interfaces,
                                             service);
                    }
                }
                catch (const sdbusplus::exception_t& e)
                {
                    warning(
                        "Failed GetManagedObjects for {SERVICE} at {PATH}: {ERROR}",
                        "SERVICE", service, "PATH", om_path, "ERROR", e.what());
                }
            }
        }

        json::display(result);

        co_return;
    }

    static void parse_managed_object(
        nlohmann::json& result, const sdbusplus::object_path& objpath,
        const DBusInterfacesMap& interfaces, const std::string& service)
    {
        bool is_sensor = objpath.str.starts_with(sensor::ns_path) &&
                         interfaces.contains(sensor::interface);
        bool is_metric = objpath.str.starts_with(metric::ns_path) &&
                         interfaces.contains(metric::interface);
        if (!is_sensor && !is_metric)
            return;

        debug("Examining {PATH} on {SERVICE}", "PATH", objpath.str, "SERVICE",
              service);
        std::string name =
            is_sensor ? std::string(last_element(objpath.str))
                      : objpath.str.substr(
                            std::string_view(metric::ns_path).length() + 1);
        auto& entry_json = result[name];

        auto extract_value = [&entry_json](const auto& props) {
            entry_json["value"] = props.value;
            entry_json["status"] =
                std::isfinite(props.value) ? "ok" : "unavailable";

            if (std::isfinite(props.max_value))
                entry_json["max"] = props.max_value;
            if (std::isfinite(props.min_value))
                entry_json["min"] = props.min_value;
            entry_json["unit"] = last_element(
                sdbusplus::message::convert_to_string(props.unit), '.');
        };

        try
        {
            if (is_sensor)
            {
                auto props = sensor::Proxy::properties_t::unpack(
                    interfaces.at(sensor::interface));
                extract_value(props);
            }
            else
            {
                auto props = metric::Proxy::properties_t::unpack(
                    interfaces.at(metric::interface));
                extract_value(props);
            }
        }
        catch (const sdbusplus::exception_t& e)
        {
            warning("Failed to parse value: {PATH}, error: {ERROR}", "PATH",
                    objpath.str, "ERROR", e.what());
            entry_json["status"] = "dbus error";
        }

        if (auto it = interfaces.find(sensor::hard_shutdown::interface);
            it != interfaces.end())
        {
            try
            {
                auto props = sensor::hard_shutdown::Proxy::properties_t::unpack(
                    it->second);
                bool has_high = std::isfinite(props.hard_shutdown_high);
                bool has_low = std::isfinite(props.hard_shutdown_low);
                if (has_high || has_low)
                {
                    auto& thres_json = entry_json["hard-shutdown"];
                    if (has_high)
                    {
                        thres_json["high"] = props.hard_shutdown_high;
                        if (props.hard_shutdown_alarm_high)
                            update_status(entry_json, "critical");
                    }
                    if (has_low)
                    {
                        thres_json["low"] = props.hard_shutdown_low;
                        if (props.hard_shutdown_alarm_low)
                            update_status(entry_json, "critical");
                    }
                }
            }
            catch (const sdbusplus::exception_t& e)
            {
                warning(
                    "Failed to parse hard-shutdown thresholds: {PATH}, error: {ERROR}",
                    "PATH", objpath.str, "ERROR", e.what());
                entry_json["status"] = "dbus error";
            }
        }

        if (auto it = interfaces.find(sensor::critical::interface);
            it != interfaces.end())
        {
            try
            {
                auto props =
                    sensor::critical::Proxy::properties_t::unpack(it->second);
                bool has_high = std::isfinite(props.critical_high);
                bool has_low = std::isfinite(props.critical_low);
                if (has_high || has_low)
                {
                    auto& thres_json = entry_json["critical"];
                    if (has_high)
                    {
                        thres_json["high"] = props.critical_high;
                        if (props.critical_alarm_high)
                            update_status(entry_json, "critical");
                    }
                    if (has_low)
                    {
                        thres_json["low"] = props.critical_low;
                        if (props.critical_alarm_low)
                            update_status(entry_json, "critical");
                    }
                }
            }
            catch (const sdbusplus::exception_t& e)
            {
                warning(
                    "Failed to parse critical thresholds: {PATH}, error: {ERROR}",
                    "PATH", objpath.str, "ERROR", e.what());
                entry_json["status"] = "dbus error";
            }
        }

        if (auto it = interfaces.find(sensor::warning::interface);
            it != interfaces.end())
        {
            try
            {
                auto props =
                    sensor::warning::Proxy::properties_t::unpack(it->second);
                bool has_high = std::isfinite(props.warning_high);
                bool has_low = std::isfinite(props.warning_low);
                if (has_high || has_low)
                {
                    auto& thres_json = entry_json["warning"];
                    if (has_high)
                    {
                        thres_json["high"] = props.warning_high;
                        if (props.warning_alarm_high)
                            update_status(entry_json, "warning");
                    }
                    if (has_low)
                    {
                        thres_json["low"] = props.warning_low;
                        if (props.warning_alarm_low)
                            update_status(entry_json, "warning");
                    }
                }
            }
            catch (const sdbusplus::exception_t& e)
            {
                warning(
                    "Failed to parse warning thresholds: {PATH}, error: {ERROR}",
                    "PATH", objpath.str, "ERROR", e.what());
                entry_json["status"] = "dbus error";
            }
        }

        if (auto it = interfaces.find(threshold::interface);
            it != interfaces.end())
        {
            try
            {
                auto props = threshold::Proxy::properties_t::unpack(it->second);
                for (const auto& [type_enum, type_str] : thresholds)
                {
                    auto type_it = props.value.find(type_enum);
                    if (type_it == props.value.end())
                    {
                        continue;
                    }

                    for (const auto& [bound_enum, bound_str] : bounds)
                    {
                        auto bound_it = type_it->second.find(bound_enum);
                        if (bound_it == type_it->second.end())
                        {
                            continue;
                        }

                        double val = bound_it->second;
                        if (!std::isfinite(val))
                        {
                            continue;
                        }

                        entry_json[type_str][bound_str] = val;
                        if (std::ranges::find(
                                props.asserted,
                                std::make_tuple(type_enum, bound_enum)) !=
                            props.asserted.end())
                        {
                            update_status(entry_json,
                                          (type_enum ==
                                           threshold::Proxy::Type::HardShutdown)
                                              ? "critical"
                                              : type_str);
                        }
                    }
                }
            }
            catch (const sdbusplus::exception_t& e)
            {
                warning(
                    "Failed to parse threshold entries: {PATH}, error: {ERROR}",
                    "PATH", objpath.str, "ERROR", e.what());
                entry_json["status"] = "dbus error";
            }
        }
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

    static void update_status(auto& entry_json, const auto& name)
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
