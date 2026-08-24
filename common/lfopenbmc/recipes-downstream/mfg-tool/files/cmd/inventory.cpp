#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"

#include <sdbusplus/async.hpp>

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace mfgtool::cmds::inventory
{
PHOSPHOR_LOG2_USING;
namespace item = dbuspath::inventory::item;
namespace asset = dbuspath::inventory::asset;

static constexpr auto InventoryIfacePrefix = "xyz.openbmc_project.Inventory";

using InventoryTypes = sdbusplus::utility::dedup_variant<
    bool, size_t, int64_t, uint64_t, uint16_t, double, std::string,
    std::vector<uint8_t>, std::vector<std::string>>;

using DBusPropertiesMap = std::unordered_map<std::string, InventoryTypes>;
using DBusInterfacesMap = std::unordered_map<std::string, DBusPropertiesMap>;
using ManagedObjectType = std::map<sdbusplus::object_path, DBusInterfacesMap>;

static auto strip_path(const auto& p)
{
    return p.substr(std::string(item::ns_path).length() + 1);
}

static auto strip_intf(const auto& i)
{
    return i.substr(std::string(InventoryIfacePrefix).length() + 1);
}

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("inventory", "Get inventory");
        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        auto result = json::empty_map();

        // TODO: We are using asset::interface here because a lot of the
        //       entity-manager content appears to be missing the
        //       Inventory.Item.  We need to get this fixed.
        std::set<std::string> target_services;
        auto asset_srvs = co_await utils::mapper::subtree_services(
            ctx, item::ns_path, asset::interface, 0);
        for (const auto& [path, services] : asset_srvs)
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
                    if (path.str.starts_with(item::ns_path) || path.str == "/")
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
        if (!objpath.str.starts_with(item::ns_path) ||
            !interfaces.contains(asset::interface))
        {
            return;
        }

        for (const auto& [interface, properties] : interfaces)
        {
            if (!interface.starts_with(InventoryIfacePrefix))
            {
                continue;
            }

            // Insert the interface into the JSON so it shows up even if it
            // doesn't have any properties.
            auto& iface_result =
                result[strip_path(objpath.str)][strip_intf(interface)];
            iface_result = json::empty_map();

            for (const auto& [property, value] : properties)
            {
                // Ignore the entity-manager Probe statement because nobody
                // is going to be interested in that.
                if (property == "Probe")
                {
                    continue;
                }
                std::visit([&](const auto& v) { iface_result[property] = v; },
                           value);
            }
        }
    }
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::inventory
