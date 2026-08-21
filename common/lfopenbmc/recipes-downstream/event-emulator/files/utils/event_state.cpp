#include "event_state.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

namespace event_emulator
{

auto loadState() -> EventStateMap
{
    EventStateMap state;
    std::ifstream file(stateFile);
    if (!file.is_open())
    {
        return state;
    }

    try
    {
        auto j = nlohmann::json::parse(file);
        for (auto& [key, value] : j.items())
        {
            state[key] = value.get<std::string>();
        }
    }
    catch (...)
    {}

    return state;
}

void saveState(const EventStateMap& state)
{
    std::filesystem::create_directories(stateDir);
    std::ofstream file(stateFile);
    nlohmann::json j(state);
    file << j.dump(4) << "\n";
}

auto makeKey(const std::string& device, const std::string& eventType,
             const std::string& eventName) -> std::string
{
    return device + ":" + eventType + ":" + eventName;
}

} // namespace event_emulator
