# Facebook OpenBMC Feature Plan Template

This document tracks the features planned for platform <PLATFORM>

## Priority Legend
* P0 - Before EVT Build (soft) to Mid EVT
* P1 - Before DVT build
* P2 - Before PVT Build
* P3 - PVT exit gating
* P4 - Optional

## Infrastructure
- [ ] P0 **U-boot Porting** Port platform specific u-boot changes
- [ ] P0 **Kernel Porting** Port platform specific Kernel changes
- [ ] P0 **I2C device tree** Create device tree entries for i2c devices (and enable associated drivers)
- [ ] P0 **GPIO device tree** Create device tree entries for gpio pins
- [ ] P0 **PWM/Tach device tree** Create device tree entries for PWM/Tach devices
- [ ] P0 **ADC device tree** Create device tree entries for ADC devices.
- [ ] P0 **eMMC device tree** Create device tree entries for eMMC
- [ ] P0 **Power Supply** i2c/PMBus driver support for power supplies.
- [ ] P0 **Battery** i2c/smart-battery driver for the batteries.
- [ ] P0 **USB-UART** verify driver for USB RS-485 chip.

## User Interface
- [ ] P0 **OOB/SSH** Provide interface for Host user to interact from a remote machine via Network.
- [ ] P1 **OOB/REST** Provide interface for Host user to interact from a remote machine via Network.
- [ ] P2 **Redfish** Provide redfish interface for Host user to interact from a remote machine via Network.

## Network
- [ ] P0 **U-boot / Kernel PHY support** Add support for the RJ45 network PHY.
- [ ] P0 **DHCPv6** BMC shall be able to get IPv6 address using standard DHCPv6 protocol including retries and timeouts

## FRU Information
- [ ] P0 **FRUID read** Read and print FRUID (FRUID EEPROM  for Mother board. supported on each BMC to read its local MB's fruid.)
- [ ] P0 **FRUID update** Update FRUID with user given file assuming IPMI FRUID Format

## Sensor Information
- [ ] P0 **sensor read** Read the current status of all sensors of a given FRU:- sensors on motherboard, Mezzanine card, etc.
- [ ] P1 **Display threshold value** When requested, display the threshold values for each sensor(lnr→unr)
- [ ] P1 **Display OK status** Display a sensor is OK or not
- [ ] P1 **Sensor Caching** Cache the sensors in local DB to reduce hardware polling
- [ ] P2 **Sensor History** Store historical min/average/max over several periods:-  3sec, 1minute, 10minutes, 1hour etc.  Log to eMMC.
- [ ] P3 **Sensor History - Automatic** Automatically store a copy of sensor history when certain fatal errors occur.

## Thermal Management

### TODO: Unsure if there will be any fan on device.  Likely none.
- [ ] P0 **Fan Speed Control (OCP-Like)** Implement FSC algorithm with user given JSON configuration file with various linear/pid profiles for sensors across platform
- [ ] P0 **Fan control (Manual)** Implement fan utility to set fan-speed
- [ ] P1 **FSC configuration update** Allow user to update the fan configuration:- Allow to try out a new configuration and be able to restart the fscd
- [ ] P3 **Persistent FSC configuration** Allow user to update the fan configuration to be persistent across BMC reboots
- [ ] P2 **Sensor calibration** The sensor value needs to be adjusted/corrected based on the current PWM value.
- [ ] P2 **Conditional Sensor calibration** Allow for sensor calibration which can change based on runtime detected configuration changes
- [ ] P2 **System Configuration** Allow for BIOS to send the current system configuration to the BMC which can be used to determine various thermal calibrations.
- [ ] P0 **Airflow Sensor** Calculate Airflow based on current configuration e.g. use BIOS OEM command to get information on number of DIMMs populated etc.

## Firmware Versions
Support reading firmware versions of attached components including self.

- [ ] P0 **OpenBMC** Read f/w versions for all the components
- [ ] P0 **VR** All attached Voltage regulators
- [ ] P0 **PS** All attached power supplies
- [ ] P0 **Battery** All attached batteries
- [ ] P2 **Version via Redfish** Support all version operations via Redfish.

## Firmware Update
- [ ] P0 **OpenBMC** Upgrade BMC Image from within the BMC
- [ ] P0 **VR** Upgrade all attached Voltage regulators
- [ ] P0 **PS** Upgrade all attached power supplies
- [ ] P0 **Battery** Upgrade all attached batteries
- [ ] P2 **Update via Redfish** Support all update operations via Redfish.

## Time Sync
- [ ] P0 **RTC** Enable RTC hardware device
- [ ] P2 **NTP Sync** Sync up BMC RTC with NTP server
- [ ] P3 **Time Zone** Show PST/PDT time always

## Diagnostics

### TODO: Define necessary diagnostics.

## BMC Reset/Recovery
- [ ] P0 **Remote Reset -SSH** Reset any BMC from remote system using the `reboot` command
- [ ] P4 **Remote Reset -REST API** Reset any BMC from remote system using a POST action=reboot to the BMC endpoint (Disabled if client is not authenticated)
- [ ] P3 **Remote Reset - Redfish API** Reset any BMC from a remote system using Redfish.

## Front Panel Control
- [ ] P0 **Identify LED/Power-ON** Chassis Identification by blinking LED: User shall be able to turn ON/OFF the identification; And specify the blinking rate to 1/5/10 seconds
- [ ] P0 **Fault LED** Assert fault LED on critical errors.
- [ ] P0 **Heartbeat LED** Allow configuration and use of heartbeat LED

## Security
- [ ] P0 **Verified Boot (dual-flash)** Support updating both flashes images and support read-only image to continue boot into RW flash.
- [ ] P1 **Verified Boot (image verification)** Validating the RW image and provide an utility to report on errors of validation/verification
- [ ] P2 **Verified Boot (recovery)** If RW image is empty or does not contain a valid image, boot into recovery
- [ ] P2 **Verified Boot (SW enforement)** Boot into recovery image if signature verification of RW image fails.
- [ ] P2 **Verified Boot (HW enforcement)** Do not allow modification of recovery image in hardware.
- [ ] P2 **Verified Boot (Image signing service)** Setup a service for BMC image signing.
- [ ] P1 **TPM 2.0 Test** Test TPM access via SPI
- [ ] P2 **TPM 2.0 Boot Measurements** Use TPM for storing secret keys and measurement info

### TODO: Do we have a hardware ROT, like PFR?

- [ ] P2 **PFR** Support verified boot using PFR.

## LCD Debug Card

### TODO: Need to determine if the LCD debug card will be supported by hardware design.

- [ ] P2 **GPIO Description** Need to create a look up table and respond with descriptions for various GPIO signals connected to USB-Debug card
- [ ] P1 **POST Description** Need to create a look up table and respond with descriptions for various POST codes based on BIOS spec
- [ ] P2 **Critical Sensors Frame** Create a frame (with multiple pages) for various sensors in critical stage (UCR/LCR/ etc.)
- [ ] P2 **Critical Logs Frame** Create a frame (with multiple pages) for various logs to show on LCD panel

## BMC Health Monitoring
- [ ] P0 **WDT** This feature is useful to detect if BMC user space application is not running for some catastrophic reason. WDT Expire should trigger BMC reset.
- [ ] P1 **Memory Usage** When BMC's free memory goes below a threshold, BMC can reboot to function properly
- [ ] P1 **CPU Usage** When CPU utilization goes above certain threshold, print a warning.
- [ ] P1 **Healthd configuration** Allow configuration on whether healthd should reboot the BMC upon issues.
- [ ] P2 **eMMC Health** Monitoring for the eMMC device and filesystem.

## Device Specific Features

- [ ] P0 **PS Sensors** Voltage, current, temperature, fan speed.
- [ ] P0 **Battery Sensors** Voltage, current, capacity, temperature, fan speed.
- [ ] P1 **PS Failure handling** Black box data collection, fault signal handling.
- [ ] P1 **Battery Charge Rate** Interface to configure variable charge rate.
- [ ] P2 **High Res Sensor Mode** Enable mode to collect and record high resolution sensor data from power supplies.
- [ ] P3 **RS-485/Modbus** Support cooling units.

## Miscellaneous
- [ ] P3 **JSON Format** Allow the output of SSH-based utilites to be in JSON format (e.g sensor-util, power-util, fw-util, log-util etc..)
- [ ] P4 **ODS/Scuba integration** BMC to log various data in to ODS directly. Allow configuration of remote service.
- [ ] P4 **ODS sensor names** Sensor naming should be standardized across various server  platforms using different flavors of BMC(e.g. ODM, OEM or OpenBMC)
- [ ] P2 **Reset Default factory settings** Allow user to clear all configurations

### TODO: Is guid-util needed?
- [ ] P1 **guid-util** Utility to program Device GUID and System GUID to FRU EEPROM
