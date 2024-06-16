#include <xyz/openbmc_project/Collection/DeleteAll/client.hpp>
#include <xyz/openbmc_project/Common/Threshold/client.hpp>
#include <xyz/openbmc_project/Control/FanPwm/client.hpp>
#include <xyz/openbmc_project/Control/Mode/client.hpp>
#include <xyz/openbmc_project/Inventory/Decorator/Asset/client.hpp>
#include <xyz/openbmc_project/Inventory/Item/client.hpp>
#include <xyz/openbmc_project/Logging/Entry/client.hpp>
#include <xyz/openbmc_project/Metric/Value/client.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/Critical/client.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/HardShutdown/client.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/Warning/client.hpp>
#include <xyz/openbmc_project/Sensor/Value/client.hpp>
#include <xyz/openbmc_project/Software/Version/client.hpp>
#include <xyz/openbmc_project/State/BMC/client.hpp>
#include <xyz/openbmc_project/State/Boot/PostCode/client.hpp>
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

namespace postcode
{
using Proxy = sdbusplus::client::xyz::openbmc_project::state::boot::PostCode<>;
static constexpr auto ns_path = Proxy::namespace_path::value;
static constexpr auto interface = Proxy::interface;

inline auto path(size_t id)
{
    return std::string(ns_path) + "/" +
           std::string(Proxy::namespace_path::host) + std::to_string(id);
}

} // namespace postcode

} // namespace host

namespace bmc
{
using Proxy = sdbusplus::client::xyz::openbmc_project::state::BMC<>;
static constexpr auto ns_path = Proxy::namespace_path::value;
static const auto interface = Proxy::interface;

inline auto path()
{
    return std::string(ns_path) + "/" + std::string(Proxy::namespace_path::bmc);
}

} // namespace bmc

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
           std::string(Proxy::namespace_path::entry) + "/" + std::to_string(id);
}
} // namespace log_entry

namespace metric
{
using Proxy = sdbusplus::client::xyz::openbmc_project::metric::Value<>;
static constexpr auto ns_path = Proxy::namespace_path::value;
static constexpr auto interface = Proxy::interface;
} // namespace metric

namespace threshold
{
using Proxy = sdbusplus::client::xyz::openbmc_project::common::Threshold<>;
static constexpr auto interface = Proxy::interface;
} // namespace threshold

namespace sensor
{
using Proxy = sdbusplus::client::xyz::openbmc_project::sensor::Value<>;
static constexpr auto ns_path = Proxy::namespace_path::value;
static constexpr auto interface = Proxy::interface;

namespace warning
{
using Proxy =
    sdbusplus::client::xyz::openbmc_project::sensor::threshold::Warning<>;
static constexpr auto interface = Proxy::interface;
} // namespace warning

namespace critical
{
using Proxy =
    sdbusplus::client::xyz::openbmc_project::sensor::threshold::Critical<>;
static constexpr auto interface = Proxy::interface;
} // namespace critical

namespace hard_shutdown
{
using Proxy =
    sdbusplus::client::xyz::openbmc_project::sensor::threshold::HardShutdown<>;
static constexpr auto interface = Proxy::interface;
} // namespace hard_shutdown

} // namespace sensor

namespace inventory
{
namespace item
{
using Proxy = sdbusplus::client::xyz::openbmc_project::inventory::Item<>;
static constexpr auto ns_path = Proxy::namespace_path;
static constexpr auto interface = Proxy::interface;
} // namespace item

namespace asset
{
using Proxy =
    sdbusplus::client::xyz::openbmc_project::inventory::decorator::Asset<>;
static constexpr auto interface = Proxy::interface;
} // namespace asset
} // namespace inventory

namespace control
{

namespace fan_pwm
{
using Proxy = sdbusplus::client::xyz::openbmc_project::control::FanPwm<>;
static constexpr auto interface = Proxy::interface;
} // namespace fan_pwm

namespace mode
{
using Proxy = sdbusplus::client::xyz::openbmc_project::control::Mode<>;
static constexpr auto interface = Proxy::interface;
} // namespace mode
} // namespace control

namespace software
{
namespace version
{
using Proxy = sdbusplus::client::xyz::openbmc_project::software::Version<>;
static constexpr auto ns_path = Proxy::namespace_path;
static constexpr auto interface = Proxy::interface;

inline auto path_prefix()
{
    return std::string(ns_path) + "/";
}

} // namespace version
} // namespace software

} // namespace mfgtool::dbuspath
