#include <redfish_client/core/config.hpp>

namespace redfish_client::core
{

void from_json(const nlohmann::json& json, Config& config)
{
    json.at("host").get_to(config.host);
    json.at("compatible").get_to(config.compatible);
    if (auto it = json.find("sensorConfig"); it != json.end())
    {
        config.sensorConfig = it->template get<SensorConfig>();
    }
    if (auto it = json.find("logServiceConfig"); it != json.end())
    {
        config.logServiceConfig = it->template get<LogServiceConfig>();
    }
    if (auto it = json.find("updateServiceConfig"); it != json.end())
    {
        config.updateServiceConfig = it->template get<UpdateServiceConfig>();
    }
}

Config Config::parse(const std::string& configJson)
{
    return nlohmann::json::parse(configJson).template get<Config>();
}

} // namespace redfish_client::core
