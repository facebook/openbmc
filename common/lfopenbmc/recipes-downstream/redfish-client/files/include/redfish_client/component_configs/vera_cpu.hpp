#pragma once
#include <redfish_client/core/log_entry_mapper_registry.hpp>
#include <redfish_client/core/cper_mapper.hpp>

namespace redfish_client::component_config {

inline void registerVeraCpuMappers() {
    auto& registry = core::LogEntryMapperRegistry::instance();

    registry.registerMapper(std::make_unique<redfish_client::core::CperMapper>(), 100);
}

} // namespace redfish_client::component_config
