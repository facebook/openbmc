#pragma once

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mfgtool::utils::mapper
{

/** Map of object path to service names. */
using services_t = std::map<sdbusplus::object_path, std::vector<std::string>>;

/** Managed object types for a given property variant. */
template <typename Variant>
using properties_map_t = std::unordered_map<std::string, Variant>;
template <typename Variant>
using interfaces_map_t =
    std::unordered_map<std::string, properties_map_t<Variant>>;
template <typename Variant>
using managed_objects_t =
    std::map<sdbusplus::object_path, interfaces_map_t<Variant>>;

/** Get a list of services hosting a dbus interface by calling mapper.
 *
 *  @param[in] ctx - The dbus async context to execute against.
 *  @param[in] subpath - The subpath filter to find objects under.
 *  @param[in] interface - The interface to find.
 *  @param[in] depth - The subpath depth to search.
 *
 *  @return A map of paths to services.
 *
 */
auto subtree_services(sdbusplus::async::context& ctx,
                      const std::string& subpath, const std::string& interface,
                      size_t depth = 0) -> sdbusplus::async::task<services_t>;

/** Iterate over the objects in a subtree.
 *
 *  Calls mapper to obtain the services hosting all of the objects in a subtree
 * (assuming there is just one service per instance).  Iterate over the objects
 * and call the supplied co-routine for each one.
 *
 *  @param[in] ctx - The dbus async context to execute against.
 *  @param[in] subpath - The subpath filter to find objects under.
 *  @param[in] interface - The interface to find.
 *  @param[in] coroutine - The co-routine to call for each instance.
 *  @param[in] depth - The subpath depth to search.
 *
 *  @return A map of paths to services.
 *
 */
auto subtree_for_each(
    sdbusplus::async::context& ctx, const std::string& subpath,
    const std::string& interface,
    const std::function<sdbusplus::async::task<>(
        const sdbusplus::object_path&, const std::string&)>& coroutine,
    size_t depth = 0) -> sdbusplus::async::task<>;

/** Iterate over the objects in a subtree.
 *
 *  Calls mapper to obtain the services hosting all of the objects in a subtree,
 *  with the interfaces at each subpath. Iterate over the objects and call the
 *  supplied co-routine for each one with the path, interface, and service.
 *
 *  @param[in] ctx - The dbus async context to execute against.
 *  @param[in] subpath - The subpath filter to find objects under.
 *  @param[in] interface - The interface to find.
 *  @param[in] coroutine - The co-routine to call for each instance.
 *  @param[in] depth - The subpath depth to search.
 *
 *  @return A map of paths to services.
 *
 */
auto subtree_for_each_interface(
    sdbusplus::async::context& ctx, const std::string& subpath,
    const std::string& interface,
    const std::function<sdbusplus::async::task<>(
        const std::string&, const std::string&, const std::string&)>& coroutine,
    size_t depth = 0) -> sdbusplus::async::task<>;

/** Find the service hosting an object.
 *
 *  @param[in] ctx - The dbus async context to execute against.
 *  @param[in] path - The expected object path.
 *  @param[in] interface - The interface to find.
 *
 *  @return An optional string of the service or nullopt.
 */
auto object_service(sdbusplus::async::context& ctx, const std::string& path,
                    const std::string& interface)
    -> sdbusplus::async::task<std::optional<std::string>>;

/** Iterate over the managed objects of every service hosting an interface.
 *
 *  Calls the supplied callback for each object returned by a single
 *  GetManagedObjects per service.
 *
 *  @param[in] ctx - The dbus async context to execute against.
 *  @param[in] subpaths - The subpaths to find objects under.
 *  @param[in] interfaces - The interfaces to find, one per subpath.
 *  @param[in] callback - Called with the path and interfaces of each object.
 *  @param[in] service_filter - Optional service to restrict the query to.
 */
template <typename Variant, std::ranges::input_range Paths,
          std::ranges::input_range Interfaces, typename Callback>
auto managed_objects_for_each(sdbusplus::async::context& ctx,
                              const Paths& subpaths,
                              const Interfaces& interfaces, Callback&& callback,
                              std::string_view service_filter = {})
    -> sdbusplus::async::task<>
{
    PHOSPHOR_LOG2_USING;

    std::set<std::string> target_services;
    for (const auto& [subpath, interface] :
         std::views::zip(subpaths, interfaces))
    {
        auto found = co_await subtree_services(ctx, std::string(subpath),
                                               std::string(interface), 0);
        for (const auto& [path, services] : found)
        {
            for (const auto& srv : services)
            {
                if (service_filter.empty() || srv == service_filter)
                {
                    target_services.insert(srv);
                }
            }
        }
    }

    // Find the ObjectManager objects to query.
    auto om_objects = co_await subtree_services(
        ctx, "/", "org.freedesktop.DBus.ObjectManager", 0);

    for (const auto& service : target_services)
    {
        std::vector<std::string_view> om_candidates;
        for (const auto& [path, services] : om_objects)
        {
            if (std::ranges::find(services, service) == services.end())
            {
                continue;
            }
            auto under_subpath =
                std::ranges::any_of(subpaths, [&path](const auto& subpath) {
                    return path.str.starts_with(subpath);
                });
            if (under_subpath || path.str == "/")
            {
                om_candidates.push_back(path.str);
            }
        }
        if (om_candidates.empty())
        {
            warning("No ObjectManager found for service {SERVICE}.", "SERVICE",
                    service);
            continue;
        }

        // Merge the object managers so an object exposed by more than one is
        // only handed to the callback once.
        managed_objects_t<Variant> objs;
        for (const auto& om_path : om_candidates)
        {
            try
            {
                auto proxy =
                    sdbusplus::async::proxy()
                        .service(service)
                        .path(om_path)
                        .interface("org.freedesktop.DBus.ObjectManager");
                objs.merge(
                    co_await proxy.template call<managed_objects_t<Variant>>(
                        ctx, "GetManagedObjects"));
            }
            catch (const sdbusplus::exception_t& e)
            {
                warning(
                    "Failed GetManagedObjects for {SERVICE} at {PATH}: {ERROR}",
                    "SERVICE", service, "PATH", om_path, "ERROR", e);
            }
        }

        for (const auto& [objpath, ifaces] : objs)
        {
            callback(objpath, ifaces, service);
        }
    }
    co_return;
}

} // namespace mfgtool::utils::mapper
