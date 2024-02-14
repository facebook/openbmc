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
    std::map<sdbusplus::message::object_path, std::vector<std::string>>;

namespace details
{
// I would have put this whole implementation directly into subtree_services
// but I was running into ICEs in GCC-13.  Breaking it up into smaller pieces
// seems to get away from the ICE.

inline auto subtree_to_services(auto mapperResults) -> services_t
{
    services_t services{};

    for (const auto& [objPath, objServices] : mapperResults)
    {
        for (const auto& [service, _] : objServices)
        {
            services[objPath].push_back(service);
        }
    }
    return services;
}

auto subtree(sdbusplus::async::context& ctx, const auto& subpath,
             const auto& interface, size_t depth = 0)
{
    using ObjectMapper =
        sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;

    auto mapper = ObjectMapper(ctx)
                      .service(ObjectMapper::default_service)
                      .path(ObjectMapper::instance_path);

    return mapper.get_sub_tree(subpath, depth, {interface});
}

auto object_service(sdbusplus::async::context& ctx, const auto& path,
                    const auto& interface)
{
    using ObjectMapper =
        sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;

    auto mapper = ObjectMapper(ctx)
                      .service(ObjectMapper::default_service)
                      .path(ObjectMapper::instance_path);

    return mapper.get_object(path, {interface});
}

} // namespace details

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
auto subtree_services(sdbusplus::async::context& ctx, const auto& subpath,
                      const auto& interface, size_t depth = 0)
    -> sdbusplus::async::task<services_t>
{
    co_return details::subtree_to_services(
        co_await details::subtree(ctx, subpath, interface, depth));
}

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
auto subtree_for_each(sdbusplus::async::context& ctx, const auto& subpath,
                      const auto& interface, const auto& coroutine,
                      size_t depth = 0) -> sdbusplus::async::task<>
{
    PHOSPHOR_LOG2_USING;

    auto objects = co_await subtree_services(ctx, subpath, interface, depth);

    debug("Iterating over entries.");
    for (auto& [path, services] : objects)
    {
        if (services.size() > 1)
        {
            warning("Multiple services ({COUNT}) provide {PATH}.", "PATH", path,
                    "COUNT", services.size());
            for (auto& s : services)
            {
                warning("Service available at {SERVICE}.", "SERVICE", s);
            }
        }

        info("Examining {INTERFACE} at {PATH}.", "INTERFACE", interface, "PATH",
             path);
        co_await coroutine(path, services[0]);
    }
}

/** Find the service hosting an object.
 *
 *  @param[in] ctx - The dbus async context to execute against.
 *  @param[in] path - The expected object path.
 *  @param[in] interface - The interface to find.
 *
 *  @return An optional string of the service or nullopt.
 */
auto object_service(sdbusplus::async::context& ctx, const auto& path,
                    const auto& interface)
    -> sdbusplus::async::task<std::optional<std::string>>
{
    // Mapper look up will return an exception of ResourceNotFound if the path
    // doesn't exist.  Catch the exception and turn it into a nullopt.
    try
    {
        auto result = co_await details::object_service(ctx, path, interface);
        co_return result.begin()->first;
    }
    catch (...)
    {
        co_return std::nullopt;
    }
}

} // namespace mfgtool::utils::mapper
