#pragma once

#include <sdbusplus/async.hpp>

#include <string>

namespace event_emulator
{

auto runGenerate(sdbusplus::async::context& ctx, const std::string& device,
                 const std::string& eventType) -> sdbusplus::async::task<>;

auto runResolve(sdbusplus::async::context& ctx, const std::string& device,
                const std::string& eventType) -> sdbusplus::async::task<>;

} // namespace event_emulator
