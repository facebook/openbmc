#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/message.hpp>

#include <format>

namespace mfgtool::cmds::postcode_display
{

PHOSPHOR_LOG2_USING;
using namespace dbuspath;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("postcode-display", "Display postcodes.");
        cmd->add_option("-p,--position", arg_pos, "Device position")
            ->required();

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        std::vector<std::string> postcodes{};

        auto path = host::postcode::path(arg_pos);

        auto service = co_await utils::mapper::object_service(
            ctx, path, dbuspath::host::postcode::interface);

        if (!service)
        {
            warning("Cannot find postcode service for position {POSITION}.",
                    "POSITION", arg_pos);

            json::display(postcodes);
            co_return;
        }

        info("Getting postcode entries from {SERVICE}.", "SERVICE", *service);
        auto proxy = host::postcode::Proxy(ctx).service(*service).path(path);
        auto raw_postcodes = co_await proxy.get_post_codes(1);

        // Determine the size of the largest postcode to try to guess the length
        // of the postcodes.
        auto postcode_idx = 0;
        for (auto& [code, extra] : raw_postcodes)
        {
            if (code <= std::numeric_limits<uint8_t>::max())
            {
                postcode_idx = std::max(0, postcode_idx);
            }
            else if (code <= std::numeric_limits<uint16_t>::max())
            {
                postcode_idx = std::max(1, postcode_idx);
            }
            else if (code <= std::numeric_limits<uint32_t>::max())
            {
                postcode_idx = std::max(2, postcode_idx);
            }
            else
            {
                postcode_idx = 3;
            }
        }

        // Insert the formatted postcode.
        for (auto& [code, extra] : raw_postcodes)
        {
            if (!extra.empty())
            {
                std::string result;
                for (ssize_t i = extra.size() - 1;  i >= 0; i--)
                {
                    result += std::format("{:02x}", extra[i]);
                }
                postcodes.push_back(result);
                continue;
            }

            switch (postcode_idx)
            {
                case 0:
                    postcodes.push_back(std::format("{:02x}", code));
                    break;

                case 1:
                    postcodes.push_back(std::format("{:04x}", code));
                    break;

                case 2:
                    postcodes.push_back(std::format("{:08x}", code));
                    break;

                case 3:
                    postcodes.push_back(std::format("{:016x}", code));
                    break;
            }
        }

        json::display(postcodes);
        co_return;
    }

    size_t arg_pos = 0;
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::postcode_display
