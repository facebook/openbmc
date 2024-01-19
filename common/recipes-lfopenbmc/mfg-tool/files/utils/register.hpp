#pragma once

#include <CLI/CLI.hpp>

#include <functional>
#include <memory>

namespace mfgtool
{

template <typename T>
using command_t = std::unique_ptr<T>;

#define MFGTOOL_REGISTER(type, ...)                                            \
    static auto command_registration_##type =                                  \
        mfgtool::details::register_command<type>(__VA_ARGS__)

namespace details
{
void register_command(std::function<void(CLI::App&)>);

template <typename T>
concept has_init_with_app = requires(T& t, CLI::App& app) { t.init(app); };

template <typename T, typename... Args>
concept has_valid_constructor = requires(Args... args) { T(args...); };

template <typename T, typename... Args>
auto register_command(Args... args) -> command_t<T>
    requires has_init_with_app<T> && has_valid_constructor<T, Args...>
{
    auto c = std::make_unique<T>(args...);

    details::register_command(
        [command = c.get()](auto& app) { command->init(app); });

    return c;
}

} // namespace details

void init_commands(CLI::App&);

} // namespace mfgtool
