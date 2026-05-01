# Meta OpenBMC Sensor List for NVIDIA HMC Compute Trays

This document lists the HMC-polled sensor requirements common to Meta
OpenBMC compute trays managed by an NVIDIA HMC over Redfish — currently
Catalina (GB200, 2 GPUs/tray) and Clemente (GB300, 4 GPUs/tray), and
extending to future HMC-managed platforms.

These sensors are read from the NVIDIA HMC via Redfish and exposed on the
BMC dbus under `/xyz/openbmc_project/inventory/system/board/NVIDIA_HMC`.

Polling interval: 2000ms

Per-platform GPU count is denoted by `<N>` below, where `N = 0..(NumGPUs - 1)`.
The platform OVERVIEW.md states the actual GPU count per tray.

## HGX Board - Power
| Sensor Name | Unit | Source |
| ----------- | ---- | ------ |
| HGX_TOTALGPU_PWR_W | Watts | HGX_Chassis_0/Sensors/HGX_Chassis_0_TotalGPU_Power_0 |

## HGX Board - GPU Voltage
One sensor per GPU.

| Sensor Name | Unit | Source |
| ----------- | ---- | ------ |
| HGX_GPU\<N\>_VOLT_V | Volts | HGX_GPU_\<N\>/Sensors/HGX_GPU_\<N\>_Voltage_0 |

## HGX Board - Altitude Pressure
One sensor per processor module (2 modules per tray).

| Sensor Name | Unit | Source |
| ----------- | ---- | ------ |
| HGX_HPM0_ALTITUDE_PRESSURE_PA | Pa | HGX_ProcessorModule_0/Sensors/HGX_ProcessorModule_0_AltitudePressure_0 |
| HGX_HPM1_ALTITUDE_PRESSURE_PA | Pa | HGX_ProcessorModule_1/Sensors/HGX_ProcessorModule_1_AltitudePressure_0 |

## HGX Board - CPU Core Utilization
72 cores per Grace CPU x 2 CPUs = 144 sensors per tray.

| Sensor Name | Unit | Source |
| ----------- | ---- | ------ |
| HGX_CPU0_CORE0_UTIL_PCT ... HGX_CPU0_CORE71_UTIL_PCT | % | ProcessorModule_0_CPU_0_CoreUtil_0 ... 71 |
| HGX_CPU1_CORE0_UTIL_PCT ... HGX_CPU1_CORE71_UTIL_PCT | % | ProcessorModule_1_CPU_0_CoreUtil_0 ... 71 |

## HGX Board - Firmware Inventory
The following firmware versions are tracked via HMC Redfish UpdateService.
GPU and GPU-ERoT entries scale with the platform's GPU count (`<N>`).

| BMC ID | HMC Redfish ID | Description |
| ------ | -------------- | ----------- |
| HGX_bmc | HGX_FW_BMC_0 | HMC BMC firmware |
| HGX_cpld_0 | HGX_FW_CPLD_0 | Baseboard CPLD |
| HGX_cpu_0 | HGX_FW_CPU_0 | Grace CPU 0 firmware |
| HGX_cpu_1 | HGX_FW_CPU_1 | Grace CPU 1 firmware |
| HGX_fpga_0 | HGX_FW_FPGA_0 | FPGA 0 firmware |
| HGX_fpga_1 | HGX_FW_FPGA_1 | FPGA 1 firmware |
| HGX_gpu_\<N\> | HGX_FW_GPU_\<N\> | Blackwell GPU \<N\> firmware (one per GPU) |
| HGX_erot_bmc | HGX_FW_ERoT_BMC_0 | BMC ERoT firmware |
| HGX_erot_cpu_0 | HGX_FW_ERoT_CPU_0 | CPU 0 ERoT firmware |
| HGX_erot_cpu_1 | HGX_FW_ERoT_CPU_1 | CPU 1 ERoT firmware |
| HGX_erot_fpga_0 | HGX_FW_ERoT_FPGA_0 | FPGA 0 ERoT firmware |
| HGX_erot_fpga_1 | HGX_FW_ERoT_FPGA_1 | FPGA 1 ERoT firmware |
| HGX_inforom_gpu_\<N\> | HGX_InfoROM_GPU_\<N\> | GPU \<N\> InfoROM (one per GPU) |
| HGX_pcieswitchconfig_0 | HGX_PCIeSwitchConfig_0 | PCIe Switch configuration |
