#include <xyz/openbmc_project/Collection/DeleteAll/client.hpp>
#include <xyz/openbmc_project/Logging/Entry/client.hpp>
#include <xyz/openbmc_project/State/Chassis/client.hpp>
#include <xyz/openbmc_project/State/Host/client.hpp>

#include <string>

namespace mfgtool::dbuspath
{

namespace chassis
{
using Proxy = sdbusplus::client::xyz::openbmc_project::state::Chassis<>;
static constexpr auto ns_path = Proxy::namespace_path::value;
static constexpr auto interface = Proxy::interface;

inline auto path(size_t id)
{
    return std::string(ns_path) + "/" +
           std::string(Proxy::namespace_path::chassis) + std::to_string(id);
}

inline auto path_prefix()
{
    return std::string(ns_path) + "/" +
           std::string(Proxy::namespace_path::chassis);
}

} // namespace chassis

namespace host
{
using Proxy = sdbusplus::client::xyz::openbmc_project::state::Host<>;
static constexpr auto ns_path = Proxy::namespace_path::value;
static constexpr auto interface = Proxy::interface;

inline auto path(size_t id)
{
    return std::string(ns_path) + "/" +
           std::string(Proxy::namespace_path::host) + std::to_string(id);
}

inline auto path_prefix()
{
    return std::string(ns_path) + "/" +
           std::string(Proxy::namespace_path::host);
}

} // namespace host

namespace delete_all
{
using Proxy = sdbusplus::client::xyz::openbmc_project::collection::DeleteAll<>;
}

namespace log_entry
{
using Proxy = sdbusplus::client::xyz::openbmc_project::logging::Entry<>;
static constexpr auto service = Proxy::default_service;
static constexpr auto ns_path = Proxy::namespace_path::value;
static constexpr auto interface = Proxy::interface;

inline auto path(size_t id)
{
    return std::string(ns_path) + "/" +
           std::string(Proxy::namespace_path::entry) + std::to_string(id);
}
} // namespace log_entry

} // namespace mfgtool::dbuspath
