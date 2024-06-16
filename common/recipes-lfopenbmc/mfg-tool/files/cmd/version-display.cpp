#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"
#include "utils/string.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/message.hpp>

namespace mfgtool::cmds::version_display
{
PHOSPHOR_LOG2_USING;
namespace version = dbuspath::software::version;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("version-display",
                                      "Display software versions.");

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        auto result = json::empty_map();

        info("Finding software version objects.");
        co_await utils::mapper::subtree_for_each(
            ctx, version::ns_path, version::interface,

            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
            auto proxy = version::Proxy(ctx).service(service).path(path.str);

            auto version = co_await proxy.version();

            if (!version.size())
            {
                co_return;
            }

            // BMC versions have a hash number as the path, but have Purpose
            // set.
            if (co_await proxy.purpose() == version::Proxy::VersionPurpose::BMC)
            {
                result["bmc"] = version;
            }
            else
            {
                // Non-BMC versions are current something like:
                // "xyz/openbmc_project/version/device/location"
                auto id = path.str.substr(version::path_prefix().size());
                auto device = utils::string::first_element(id);
                auto location = utils::string::last_element(id);

                if (!result.contains(device))
                {
                    result[device] = json::empty_map();
                }

                result[device][location] = version;
            }

            co_return;
        });

        json::display(result);
        co_return;
    }
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::version_display
