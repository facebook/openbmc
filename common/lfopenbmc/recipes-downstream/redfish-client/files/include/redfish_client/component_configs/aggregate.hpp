#pragma once
#include <redfish_client/core/log_entry_mapper_registry.hpp>
#include <redfish_client/core/aggregate_mapper.hpp>

namespace redfish_client::component_config {

inline void registerAggregateMappers() {
    auto& registry = core::LogEntryMapperRegistry::instance();

    registry.registerMapper(std::make_unique<redfish_client::core::AggregateMapper>(), 100);
}

} // namespace redfish_client::component_config
