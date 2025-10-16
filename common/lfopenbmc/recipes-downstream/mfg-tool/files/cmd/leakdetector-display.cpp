#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"
#include "utils/string.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/message.hpp>

namespace mfgtool::cmds::leakdetector_display
{
PHOSPHOR_LOG2_USING;
namespace leakdetector = dbuspath::leak::detector;
using namespace utils::string;

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
            co_await utils::mapper::subtree_for_each(
                ctx, leakdetector::ns_path, leakdetector::interface,

                [&](const auto& path,
                    const auto& service) -> sdbusplus::async::task<> {
                    auto& entry_json = result[last_element(path)];
                    try
                    {
                        auto proxy =
                            leakdetector::Proxy(ctx).service(service).path(
                                path.str);
                        auto properties = co_await proxy.properties();

                        entry_json["name"] = properties.pretty_name;

                        auto state = properties.state;
                        entry_json["status"] =
                            (state == leakdetector::Proxy::DetectorState::Normal
                                 ? "ok"
                                 : "critical");
                        entry_json["type"] = "Moisture";
                    }
                    catch (const sdbusplus::exception::SdBusError& e)
                    {
                        warning(
                            "Failed to get leak detector state: {PATH}, error: {ERROR}",
                            "PATH", path.str, "ERROR", e);
                        entry_json["status"] = "dbus error";
                    }
                });
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            warning("No leak detectors found: {ERROR}", "ERROR", e.what());
        }

        json::display(result);

        co_return;
    }
};

MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::leakdetector_display
