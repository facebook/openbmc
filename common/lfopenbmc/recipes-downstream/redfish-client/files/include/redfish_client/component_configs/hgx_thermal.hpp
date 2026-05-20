#pragma once
#include <redfish_client/core/log_entry_mapper_registry.hpp>
#include <redfish_client/core/hgx_thermal_mapper.hpp>

namespace redfish_client::component_config
{

inline void registerHgxThermalMappers()
{
    auto& registry = core::LogEntryMapperRegistry::instance();
    registry.registerMapper(std::make_unique<core::HgxThermalMapper>(), 105);
}

} // namespace redfish_client::component_config