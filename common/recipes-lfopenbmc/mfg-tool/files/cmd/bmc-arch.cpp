#include "utils/json.hpp"
#include "utils/register.hpp"

#include <phosphor-logging/lg2.hpp>

namespace mfgtool::cmds::bmc_arch
{
PHOSPHOR_LOG2_USING;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("bmc-arch",
                                      "Get architecture of the BMC");
        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        constexpr auto systemd =
            sdbusplus::async::proxy()
                .service("org.freedesktop.systemd1")
                .path("/org/freedesktop/systemd1")
                .interface("org.freedesktop.systemd1.Manager");

        info("Calling systemd to get architecture.");
        auto arch = co_await systemd.get_property<std::string>(ctx,
                                                               "Architecture");

        json::display(arch);

        co_return;
    }
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::bmc_arch
