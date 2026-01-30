#pragma once
#include <redfish_client/core/log_entry_mapper_registry.hpp>
#include <redfish_client/core/hgx_ps_run_pwr_fault_mapper.hpp>

namespace redfish_client::component_config {

inline void registerHgxPowerSupplyMappers() {
    auto& registry = core::LogEntryMapperRegistry::instance();

    // Priority 110 to ensure it's checked before CperMapper (priority 100)
    registry.registerMapper(std::make_unique<redfish_client::core::HgxPsRunPwrFaultMapper>(), 110);
}

} // namespace redfish_client::component_config
