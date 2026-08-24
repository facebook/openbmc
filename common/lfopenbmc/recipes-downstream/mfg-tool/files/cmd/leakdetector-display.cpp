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

namespace mfgtool::cmds::leakdetector_display
{
PHOSPHOR_LOG2_USING;
namespace leakdetector = dbuspath::leak::detector;
using namespace utils::string;

using DBusInterfacesMap =
    utils::mapper::interfaces_map_t<leakdetector::Proxy::PropertiesVariant>;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("leakdetector-display",
                                      "Display leak detectors.");

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        auto result = json::empty_map();

        debug("Finding leak detector entries.");
        try
        {
            co_await utils::mapper::managed_objects_for_each<
                leakdetector::Proxy::PropertiesVariant>(
                ctx, std::array{leakdetector::ns_path},
                std::array{leakdetector::interface},
                [&](const auto& objpath, const auto& interfaces, const auto&) {
                    parse_managed_object(result, objpath, interfaces);
                });
        }
        catch (const sdbusplus::exception_t& e)
        {
            warning("No leak detectors found: {ERROR}", "ERROR", e);
        }

        json::display(result);

        co_return;
    }

    static void parse_managed_object(nlohmann::json& result,
                                     const sdbusplus::object_path& objpath,
                                     const DBusInterfacesMap& interfaces)
    {
        auto iface = interfaces.find(leakdetector::interface);
        if (iface == interfaces.end() ||
            !objpath.str.starts_with(leakdetector::ns_path))
        {
            return;
        }

        auto& entry_json = result[last_element(objpath.str)];
        try
        {
            auto properties =
                leakdetector::Proxy::properties_t::unpack(iface->second);

            entry_json["name"] = properties.pretty_name;

            auto state = properties.state;
            entry_json["status"] =
                (state == leakdetector::Proxy::DetectorState::Normal
                     ? "ok"
                     : "critical");
            entry_json["type"] = "Moisture";
        }
        catch (const sdbusplus::exception_t& e)
        {
            warning("Failed to get leak detector state: {PATH}, error: {ERROR}",
                    "PATH", objpath.str, "ERROR", e);
            entry_json["status"] = "dbus error";
        }
    }
};

MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::leakdetector_display
