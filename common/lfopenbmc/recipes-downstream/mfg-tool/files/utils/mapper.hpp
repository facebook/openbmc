#pragma once

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mfgtool::utils::mapper
{

/** Map of object path to service names. */
using services_t =
    std::map<sdbusplus::object_path, std::vector<std::string>>;

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

} // namespace mfgtool::utils::mapper
