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

namespace mfgtool::cmds::version_display
{
PHOSPHOR_LOG2_USING;
namespace version = dbuspath::software::version;
using namespace utils::string;

using DBusPropertiesMap =
    std::unordered_map<std::string, version::Proxy::PropertiesVariant>;
using DBusInterfacesMap = std::unordered_map<std::string, DBusPropertiesMap>;
using ManagedObjectType = std::map<sdbusplus::object_path, DBusInterfacesMap>;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd =
            app.add_subcommand("version-display", "Display software versions.");

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        auto result = json::empty_map();

        // Collect every service hosting a software version object.
        std::set<std::string> target_services;
        auto version_srvs = co_await utils::mapper::subtree_services(
            ctx, version::ns_path, version::interface, 0);
        for (const auto& [path, services] : version_srvs)
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
                    if (path.str.starts_with(version::ns_path) ||
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
                        parse_managed_object(result, objpath, interfaces);
                    }
                }
                catch (const sdbusplus::exception_t& e)
                {
                    warning(
                        "Failed GetManagedObjects for {SERVICE} at {PATH}: {ERROR}",
                        "SERVICE", service, "PATH", om_path, "ERROR", e);
                }
            }
        }

        json::display(result);
        co_return;
    }

    static void parse_managed_object(nlohmann::json& result,
                                     const sdbusplus::object_path& objpath,
                                     const DBusInterfacesMap& interfaces)
    {
        auto iface = interfaces.find(version::interface);
        if (iface == interfaces.end() ||
            !objpath.str.starts_with(version::ns_path))
        {
            return;
        }

        version::Proxy::properties_t props;
        try
        {
            props = version::Proxy::properties_t::unpack(iface->second);
        }
        catch (const sdbusplus::exception_t& e)
        {
            warning("Failed to unpack version: {PATH}, error: {ERROR}", "PATH",
                    objpath.str, "ERROR", e);
            return;
        }

        if (props.version.empty())
        {
            return;
        }

        // BMC versions have a hash number as the path, but have Purpose set.
        if (props.purpose == version::Proxy::VersionPurpose::BMC)
        {
            result["bmc"] = props.version;
        }
        else
        {
            // Non-BMC versions have a path like:
            // "xyz/openbmc_project/version/device/location"
            auto id = objpath.str.substr(version::path_prefix().size());
            auto device = first_element(id);
            auto location = last_element(id);

            if (!result.contains(device))
            {
                result[device] = json::empty_map();
            }

            result[device][location] = props.version;
        }
    }
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::version_display
