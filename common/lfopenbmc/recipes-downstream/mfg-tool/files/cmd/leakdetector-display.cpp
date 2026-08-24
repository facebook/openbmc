#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"
#include "utils/string.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/message.hpp>

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mfgtool::cmds::leakdetector_display
{
PHOSPHOR_LOG2_USING;
namespace leakdetector = dbuspath::leak::detector;
using namespace utils::string;

using DBusPropertiesMap =
    std::unordered_map<std::string, leakdetector::Proxy::PropertiesVariant>;
using DBusInterfacesMap = std::unordered_map<std::string, DBusPropertiesMap>;
using ManagedObjectType = std::map<sdbusplus::object_path, DBusInterfacesMap>;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("leakdetector-display",
                                      "Display leak detectors.");

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        auto result = json::empty_map();

        debug("Finding leak detector entries.");
        try
        {
            std::set<std::string> target_services;
            auto detector_srvs = co_await utils::mapper::subtree_services(
                ctx, leakdetector::ns_path, leakdetector::interface, 0);
            for (const auto& [path, services] : detector_srvs)
            {
                for (const auto& srv : services)
                    target_services.insert(srv);
            }

            // Find the ObjectManager objects to query.
            auto om_objects = co_await utils::mapper::subtree_services(
                ctx, "/", "org.freedesktop.DBus.ObjectManager", 0);

            for (const auto& service : target_services)
            {
                std::vector<std::string_view> om_candidates;
                for (const auto& [path, services] : om_objects)
                {
                    if (std::ranges::find(services, service) != services.end())
                    {
                        if (path.str.starts_with(leakdetector::ns_path) ||
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
                                .interface(
                                    "org.freedesktop.DBus.ObjectManager");
                        auto objs = co_await proxy.call<ManagedObjectType>(
                            ctx, "GetManagedObjects");

                        for (const auto& [objpath, interfaces] : objs)
                        {
                            parse_managed_object(result, objpath, interfaces);
                        }
                    }
                    catch (const sdbusplus::exception::SdBusError& e)
                    {
                        warning(
                            "Failed GetManagedObjects for {SERVICE} at {PATH}: {ERROR}",
                            "SERVICE", service, "PATH", om_path, "ERROR", e);
                    }
                }
            }
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            warning("No leak detectors found: {ERROR}", "ERROR", e);
        }

        json::display(result);

        co_return;
    }

    static void parse_managed_object(nlohmann::json& result,
                                     const sdbusplus::object_path& objpath,
                                     const DBusInterfacesMap& interfaces)
    {
        auto iface = interfaces.find(leakdetector::interface);
        if (iface == interfaces.end() ||
            !objpath.str.starts_with(leakdetector::ns_path))
        {
            return;
        }

        auto& entry_json = result[last_element(objpath.str)];
        try
        {
            auto properties =
                leakdetector::Proxy::properties_t::unpack(iface->second);

            entry_json["name"] = properties.pretty_name;

            auto state = properties.state;
            entry_json["status"] =
                (state == leakdetector::Proxy::DetectorState::Normal
                     ? "ok"
                     : "critical");
            entry_json["type"] = "Moisture";
        }
        catch (const sdbusplus::exception_t& e)
        {
            warning("Failed to get leak detector state: {PATH}, error: {ERROR}",
                    "PATH", objpath.str, "ERROR", e);
            entry_json["status"] = "dbus error";
        }
    }
};

MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::leakdetector_display
