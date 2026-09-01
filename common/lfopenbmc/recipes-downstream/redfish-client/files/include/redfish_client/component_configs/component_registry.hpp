#pragma once
#include <redfish_client/component_configs/grace_cpu.hpp>
#include <redfish_client/component_configs/blackwell_gpu.hpp>
#include <redfish_client/component_configs/hgx_power_supply.hpp>
#include <redfish_client/component_configs/hgx_thermal.hpp>
#include <redfish_client/component_configs/hgx_leak_detector.hpp>
#include <redfish_client/component_configs/sensor_threshold.hpp>
#include <redfish_client/component_configs/instinct_gpu.hpp>
#include <redfish_client/component_configs/common_openbmc.hpp>
#include <redfish_client/component_configs/common_update.hpp>
#include <redfish_client/component_configs/common_environmental.hpp>
#include <redfish_client/core/config.hpp>
#include <string>
#include <stdexcept>

namespace redfish_client::component_config {

inline void registerComponent(const std::string& componentName,
                               const core::Config& config,
                               sdbusplus::async::context& ctx, 
                               const std::string& host) {
    if (componentName == "grace_cpu")
    {
        registerGraceCpuMappers();
    }
    else if (componentName == "blackwell_gpu")
    {
        registerBlackwellGpuMappers();
    }
    else if (componentName == "hgx_power_supply")
    {
        registerHgxPowerSupplyMappers();
    }
    else if (componentName == "hgx_thermal")
    {
        registerHgxThermalMappers();
    }
    else if (componentName == "hgx_leak_detector")
    {
        registerHgxLeakDetectorMappers();
    }
    else if (componentName == "sensor_threshold")
    {
        registerSensorThresholdMappers(config.sensorConfig);
    }
    else if (componentName == "instinct_gpu")
    {
        registerInstinctGpuMappers(ctx, host);
    }
    else if (componentName == "common_openbmc")
    {
        registerCommonOpenBmcMappers();
    }
    else if (componentName == "common_update")
    {
        registerCommonUpdateMappers();
    }
    else if (componentName == "common_environmental")
    {
        registerCommonEnvironmentalMappers();
    }
    else
    {
        throw std::runtime_error("Unknown component: " + componentName);
    }
}

} // namespace redfish_client::component_config
