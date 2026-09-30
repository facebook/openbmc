#pragma once
#include <redfish_client/core/hgx_leak_detector_mapper.hpp>
#include <redfish_client/core/log_entry_mapper_registry.hpp>

namespace redfish_client::component_config
{

inline void registerHgxLeakDetectorMappers()
{
    auto& registry = core::LogEntryMapperRegistry::instance();

    registry.registerMapper(std::make_unique<core::HgxLeakDetectorMapper>(),
                            109);
}

} // namespace redfish_client::component_config
