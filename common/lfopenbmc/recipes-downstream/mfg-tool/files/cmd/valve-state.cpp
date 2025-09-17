#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"
#include "utils/string.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>

namespace mfgtool::cmds::valve_state
{
PHOSPHOR_LOG2_USING;
namespace sensor = dbuspath::sensor;
using namespace utils::string;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("valve-state", "Get the state of valves.");

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        auto result = json::empty_map();
        auto valve_path = std::string(sensor::ns_path) + "/" + sensor::Proxy::namespace_path::valve;

        debug("Finding valve sensor entries.");
        co_await utils::mapper::subtree_for_each(
            ctx, valve_path.c_str(), sensor::interface,
            [&](const auto& path,
                const auto& service) -> sdbusplus::async::task<> {
                auto pathSuffix = last_element(path);
                auto& entry_json = result[pathSuffix];
                try
                {
                    auto proxy =
                        sensor::Proxy(ctx).service(service).path(path.str);
                    auto properties = co_await proxy.properties();

                    auto value = properties.value;
                    entry_json["reading"] = value;
                    entry_json["status"] = (value != 0) ? "open" : "close";
                    entry_json["position"] = getValvePosition(pathSuffix);
                    entry_json["direction"] = getValveDirection(pathSuffix);
                }
                catch (const sdbusplus::exception::SdBusError& e)
                {
                    warning(
                        "Failed to get valve sensor value: {PATH}, error: {ERROR}",
                        "PATH", path.str, "ERROR", e.what());
                    entry_json["status"] = "dbus error";
                }
            });

        json::display(result);

        co_return;
    }

    static auto getValvePosition(std::string& name) -> std::string
    {
      size_t position_index = name.find_last_of('_');

      if (position_index == std::string::npos)
      {
        return "";
      }

      return name.substr(position_index + 1);
    }

    static auto getValveDirection(std::string& name) -> std::string
    {
      if (name.starts_with("Supply"))
      {
        return "supply";
      }
      else if (name.starts_with("Return"))
      {
        return "return";
      }
      
      return "";
    }
};

MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::valve_state
