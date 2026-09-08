#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"
#include "utils/string.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/message.hpp>

#include <array>
#include <string>

namespace mfgtool::cmds::version_display
{
PHOSPHOR_LOG2_USING;
namespace version = dbuspath::software::version;
using namespace utils::string;

using DBusInterfacesMap =
    utils::mapper::interfaces_map_t<version::Proxy::PropertiesVariant>;

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

        debug("Finding software version objects.");
        co_await utils::mapper::managed_objects_for_each<
            version::Proxy::PropertiesVariant>(
            ctx, std::array{version::ns_path}, std::array{version::interface},
            [&](const auto& objpath, const auto& interfaces, const auto&) {
                parse_managed_object(result, objpath, interfaces);
            });

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
