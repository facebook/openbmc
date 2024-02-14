#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"
#include "utils/string.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>

#include <optional>
#include <ranges>

namespace mfgtool::cmds::fan_speed
{

PHOSPHOR_LOG2_USING;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("fan-speed", "Manipulate the fan mode");
        cmd->add_option("-t,--target", arg_target, "Desired PWM percentage");
        cmd->add_option("-p,--position", arg_position, "Fan position");

        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        namespace control = dbuspath::control;
        namespace sensor = dbuspath::sensor;
        using utils::mapper::subtree_for_each;
        using utils::string::last_element;
        using utils::string::replace_substring;

        auto result = R"({})"_json;

        debug("Finding Control.FanPwm objects.");
        co_await subtree_for_each(ctx, "/", control::fan_pwm::interface,

                                  [&](const auto& path, const auto& service)
                                      -> sdbusplus::async::task<> {
            auto pwm_name = last_element(path);
            if (!pwm_name.contains("_PWM"))
            {
                debug("{PATH} doesn't contain _PWM", "PATH", path);
                co_return;
            }

            if (arg_position && !pwm_name.contains(*arg_position))
            {
                info("Skipping {PATH} due to position mismatch.", "PATH", path);
                co_return;
            }

            auto [fan_name, _] = split(last_element(path), "_PWM");

            auto pwm =
                control::fan_pwm::Proxy(ctx).service(service).path(path.str);

            auto& result_json = result[fan_name]["pwm"];
            if (arg_target)
            {
                uint64_t target = 255.0 * (*arg_target / 100.0);
                co_await pwm.target(target);
                result_json["target"] = target;
            }
            else
            {
                uint64_t target = co_await pwm.target();
                result_json["target"] = double(target) * 100.0 / 255.0;
            }

            // This is a hack, but how dbus-sensor's FanSensor currently works.
            // They don't have any association but they at least use the same
            // final element name.
            auto sensor_path = replace_substring(path, "control/fanpwm",
                                                 "sensors/fan_pwm");
            auto sensor = sensor::Proxy(ctx).service(service).path(sensor_path);

            result_json["current"] = co_await sensor.value();
        });

        debug("Finding Sensor objects in Tach namespace.");
        auto fan_tach_subpath = std::string(sensor::ns_path) + "/" +
                                sensor::Proxy::namespace_path::fan_tach;
        co_await subtree_for_each(ctx, fan_tach_subpath, sensor::interface,

                                  [&](const auto& path, const auto& service)
                                      -> sdbusplus::async::task<> {
            auto tach_name = last_element(path);
            if (!tach_name.contains("_TACH"))
            {
                debug("{PATH} doesn't contain _TACH", "PATH", path);
                co_return;
            }

            if (arg_position && !tach_name.contains(*arg_position))
            {
                info("Skipping {PATH} due to position mismatch.", "PATH", path);
                co_return;
            }

            auto [fan_name, tach_shortname] = split(tach_name, "_TACH");

            // We need to strip off the _ from _TACH and make it lowercase for a
            // key.
            tach_shortname = tach_shortname.substr(1);
            std::ranges::transform(tach_shortname, tach_shortname.begin(),
                                   [](auto c) { return std::tolower(c); });

            auto sensor = sensor::Proxy(ctx).service(service).path(path.str);
            result[fan_name][tach_shortname] = co_await sensor.value();
        });

        json::display(result);
    }

    auto split(const auto& s, const auto& substr)
    {
        auto pos = s.find(substr);
        return std::make_tuple(s.substr(0, pos), s.substr(pos));
    }

    std::optional<double> arg_target = std::nullopt;
    std::optional<std::string> arg_position = std::nullopt;
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::fan_speed
