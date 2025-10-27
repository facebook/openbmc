#include <redfish_client/core/persist_map.hpp>

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <fstream>

PHOSPHOR_LOG2_USING;

namespace redfish_client::core
{

bool PersistMap::update(const std::string& key, const std::string& value)
{
    auto it = map.find(key);
    if (it != map.end() && value == it->second)
    {
        return false;
    }
    if (it == map.end())
    {
        map.insert({key, value});
    }
    else
    {
        it->second = value;
    }
    flush();
    return true;
}

void PersistMap::load()
{
    if (path.empty())
    {
        return;
    }
    if (auto file = std::ifstream(path); file.is_open())
    {
        try
        {
            nlohmann::json::parse(file).get_to(map);
        }
        catch (const std::exception& e)
        {
            warning("Error parsing persist map from {PATH}: {EX}", "PATH", path,
                    "EX", e.what());
        }
        file.close();
    }
}

void PersistMap::flush()
{
    if (path.empty())
    {
        return;
    }
    std::ofstream file(path);
    if (!file.is_open())
    {
        warning("Unable to flush persist map to {PATH}: file is not opened",
                "PATH", path);
        return;
    }
    file << nlohmann::json(map);
    file.close();
}

} // namespace redfish_client::core
