#pragma once
#include <redfish_client/component_configs/grace_cpu.hpp>
#include <redfish_client/component_configs/blackwell_gpu.hpp>
#include <string>
#include <stdexcept>

namespace redfish_client::component_config {

inline void registerComponent(const std::string& componentName) {
    if (componentName == "grace_cpu")
    {
        registerGraceCpuMappers();
    }
    else if (componentName == "blackwell_gpu")
    {
        registerBlackwellGpuMappers();
    }
    else
    {
        throw std::runtime_error("Unknown component: " + componentName);
    }
}

} // namespace redfish_client::component_config
