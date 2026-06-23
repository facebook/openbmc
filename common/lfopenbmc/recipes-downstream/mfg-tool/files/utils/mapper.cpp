#include "utils/clowntown.hpp"
#include "utils/mapper.hpp"

#include <format>
#include <stdexcept>

namespace mfgtool::utils::mapper
{

namespace details
{

static inline auto subtree(sdbusplus::async::context& ctx, const auto& subpath,
                           const auto& interface, size_t depth = 0)
{
    using ObjectMapper =
        sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;

    auto mapper = ObjectMapper(ctx)
                      .service(ObjectMapper::default_service)
                      .path(ObjectMapper::instance_path);

    return mapper.get_sub_tree(subpath, depth, {interface});
}

static inline auto object_service(sdbusplus::async::context& ctx,
                                  const auto& path, const auto& interface)
{
    using ObjectMapper =
        sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;

    auto mapper = ObjectMapper(ctx)
                      .service(ObjectMapper::default_service)
                      .path(ObjectMapper::instance_path);

    return mapper.get_object(path, {interface});
}

/** Property value variant, broad enough for the inventory/asset data mfg-tool
 *  compares when more than one service publishes the same object.  Mirrors the
 *  set used by the `inventory` command. */
using property_variant = sdbusplus::utility::dedup_variant<
    bool, size_t, int64_t, uint64_t, uint16_t, double, std::string,
    std::vector<uint8_t>, std::vector<std::string>>;
using property_map = std::map<std::string, property_variant>;

/** Read all properties of an interface from a specific service. */
static inline auto all_properties(sdbusplus::async::context& ctx,
                                  const std::string& service,
                                  const std::string& path,
                                  const std::string& interface)
    -> sdbusplus::async::task<property_map>
{
    property_map props{};
    for (auto&& [name, value] :
         co_await sdbusplus::async::proxy()
             .service(service)
             .path(path)
             .interface(interface)
             .get_all_properties<property_variant>(ctx))
    {
        props.emplace(name, value);
    }
    co_return props;
}

/** Verify that every service publishing `interface` at `path` reports identical
 *  property data.
 *
 *  Multiple publishers are fine as long as they agree -- there is then no
 *  ambiguity about the source of truth.  If they disagree, log every publisher
 *  and throw, rather than silently picking one set of values (or letting
 *  last-writer-wins clobber good data with blanks).
 */
static inline auto assert_consistent(
    sdbusplus::async::context& ctx,
    const sdbusplus::object_path& path, const std::string& interface,
    const std::vector<std::string>& services) -> sdbusplus::async::task<>
{
    PHOSPHOR_LOG2_USING;

    if (clowntown::enabled())
    {
        // Expert mode (--clowntown): the user opted out of the guardrail, so do
        // not second-guess conflicting publishers -- let the caller proceed.
        co_return;
    }

    auto reference =
        co_await all_properties(ctx, services.front(), path.str, interface);
    bool consistent = true;
    for (const auto& service : services)
    {
        if (co_await all_properties(ctx, service, path.str, interface) !=
            reference)
        {
            consistent = false;
        }
    }
    if (consistent)
    {
        co_return;
    }

    error("Conflicting data from {COUNT} services for {INTERFACE} at {PATH}.",
          "COUNT", services.size(), "INTERFACE", interface, "PATH", path);
    for (const auto& service : services)
    {
        error("Service available at {SERVICE}.", "SERVICE", service);
    }
    throw std::runtime_error(std::format(
        "Conflicting D-Bus data for {} at {}: {} services publish different "
        "values. mfg-tool cannot pick a source of truth -- resolve the "
        "duplicate publisher before retrying.",
        interface, path.str, services.size()));
}

} // namespace details

auto subtree_services(sdbusplus::async::context& ctx,
                      const std::string& subpath, const std::string& interface,
                      size_t depth) -> sdbusplus::async::task<services_t>
{
    auto objects = co_await details::subtree(ctx, subpath, interface, depth);
    services_t services{};

    for (const auto& [objPath, objServices] : objects)
    {
        for (const auto& [service, _] : objServices)
        {
            services[objPath].push_back(service);
        }
    }
    co_return services;
}

auto subtree_for_each(
    sdbusplus::async::context& ctx, const std::string& subpath,
    const std::string& interface,
    const std::function<sdbusplus::async::task<>(
        const sdbusplus::object_path&, const std::string&)>& coroutine,
    size_t depth) -> sdbusplus::async::task<>
{
    PHOSPHOR_LOG2_USING;

    auto objects = co_await subtree_services(ctx, subpath, interface, depth);

    debug("Iterating over entries.");
    for (const auto& [path, services] : objects)
    {
        if (services.size() > 1)
        {
            // More than one service publishes this object.  That's only a
            // problem if they disagree: if every publisher reports identical
            // data there is no ambiguity about the source of truth.  When they
            // differ, assert_consistent logs each publisher and throws rather
            // than guessing which one is correct.
            co_await details::assert_consistent(ctx, path, interface, services);
        }

        debug("Examining {INTERFACE} at {PATH}.", "INTERFACE", interface,
              "PATH", path);
        co_await coroutine(path, services[0]);
    }
}

auto subtree_for_each_interface(
    sdbusplus::async::context& ctx, const std::string& subpath,
    const std::string& interface,
    const std::function<sdbusplus::async::task<>(
        const std::string&, const std::string&, const std::string&)>& coroutine,
    size_t depth) -> sdbusplus::async::task<>
{
    PHOSPHOR_LOG2_USING;

    debug("Looking up objects under {PATH}.", "PATH", subpath);
    auto objects = co_await details::subtree(ctx, subpath, interface, depth);

    debug("iterating over entries.");
    for (const auto& [path, services] : objects)
    {
        if (services.size() > 1)
        {
            // More than one service publishes this object.  Allow it only if
            // they all agree on the data; otherwise the last-writer-wins
            // iteration below would silently clobber good data with blanks, so
            // assert_consistent fails loud instead (e.g. EntityManager and
            // pldmd both advertising Inventory.Decorator.Asset for the same
            // FRU).
            std::vector<std::string> names;
            names.reserve(services.size());
            for (const auto& [service, _] : services)
            {
                names.push_back(service);
            }
            co_await details::assert_consistent(ctx, path, interface, names);
        }

        for (const auto& [service, interfaces] : services)
        {
            for (const auto& iface : interfaces)
            {
                debug("Examining {INTERFACE} at {PATH} by {SERVICE}",
                      "INTERFACE", interface, "PATH", path, "SERVICE", service);
                co_await coroutine(path, service, iface);
            }
        }
    }
}

auto object_service(sdbusplus::async::context& ctx, const std::string& path,
                    const std::string& interface)
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
