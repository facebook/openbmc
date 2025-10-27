#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace redfish_client::core
{

struct SensorMapper
{
    std::string fromUrl;
    std::string toNamespace;
    std::string toId;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SensorMapper, fromUrl, toNamespace, toId)
};

struct SensorConfig
{
    std::string associationPath;
    std::vector<SensorMapper> mappers;
    size_t intervalMilliseconds;
    size_t maxRetries;
    size_t retryIntervalMilliseconds;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SensorConfig, associationPath, mappers,
                                   intervalMilliseconds, maxRetries,
                                   retryIntervalMilliseconds)
};

struct LogServiceConfig
{
    std::vector<std::string> urls;
    size_t intervalMilliseconds;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LogServiceConfig, urls, intervalMilliseconds)
};

struct UpdateServiceMapper
{
    std::string fromId;
    std::string toId;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(UpdateServiceMapper, fromId, toId)
};

struct UpdateServiceConfig
{
    std::vector<UpdateServiceMapper> firmwareMappers;
    std::vector<UpdateServiceMapper> softwareMappers;
    size_t intervalMilliseconds;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(UpdateServiceConfig, firmwareMappers,
                                   softwareMappers, intervalMilliseconds)
};

struct Config
{
    std::string host;
    std::string compatible;
    std::optional<SensorConfig> sensorConfig;
    std::optional<LogServiceConfig> logServiceConfig;
    std::optional<UpdateServiceConfig> updateServiceConfig;

    static Config parse(const std::string& configJson);
};

void from_json(const nlohmann::json& json, Config& config);

} // namespace redfish_client::core
