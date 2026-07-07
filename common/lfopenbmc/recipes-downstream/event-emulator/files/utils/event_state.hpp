#pragma once

#include <map>
#include <string>

namespace event_emulator
{

static constexpr auto stateDir = "/tmp/event-emulator";
static constexpr auto stateFile = "/tmp/event-emulator/events.json";

// Key: "device:event-type", Value: event object path
using EventStateMap = std::map<std::string, std::string>;

auto loadState() -> EventStateMap;
void saveState(const EventStateMap& state);

auto makeKey(const std::string& device, const std::string& eventType)
    -> std::string;

} // namespace event_emulator
