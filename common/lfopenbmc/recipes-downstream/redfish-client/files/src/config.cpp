#include <redfish_client/core/config.hpp>

namespace redfish_client::core
{

void from_json(const nlohmann::json& json, UpdateServiceMapper& mapper)
{
    json.at("fromId").get_to(mapper.fromId);
    json.at("toId").get_to(mapper.toId);
    if (auto it = json.find("updateParametersTargetsOverride");
        it != json.end())
    {
        mapper.updateParametersTargetsOverride =
            it->template get<std::vector<std::string>>();
    }
}


void from_json(const nlohmann::json& json, LogServiceConfig& config)
{
    json.at("urls").get_to(config.urls);
    json.at("intervalMilliseconds").get_to(config.intervalMilliseconds);
    if (auto it = json.find("skipHistoricalEntriesThresholdSeconds");
        it != json.end())
    {
        if (it->is_null())
        {
            config.skipHistoricalEntriesThresholdSeconds = std::nullopt;
        }
        else
        {
            it->get_to(config.skipHistoricalEntriesThresholdSeconds.emplace());
        }
    }
}

void from_json(const nlohmann::json& json, Config& config)
{
    json.at("host").get_to(config.host);
    json.at("compatible").get_to(config.compatible);
    if (auto it = json.find("components"); it != json.end())
    {
        config.components = it->template get<std::vector<std::string>>();
    }
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
