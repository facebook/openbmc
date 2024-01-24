#pragma once

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
    // Mapper look ups are possible to fail, so catch the exception and
    // terminate so we can at least see what the exception was.
    try
    {
        co_return details::subtree_to_services(
            co_await details::subtree(ctx, subpath, interface, depth));
    }
    catch (...)
    {
        std::terminate();
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
