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
- [ ] P0 **6.x Kernel Porting** Port platform specific Kernel changes
- [ ] P0 **I2C Driver porting** Port latest i2cdriver to latest kernel version
- [ ] P0 **GPIO driver porting** Port GPIO configuration to ensure we are using gpiocli
- [ ] P0 **PWM/Tach driver porting** Port to use the latest kernel interface of PWM/Tach
- [ ] P0 **ADC Driver porting** Port latest ADC settings on the latest kernel interface
- [ ] P0 **NCSI driver potting** Port NCSI driver to the latest kernel. Port NCSID to new driver interface
- [ ] P0 **EMMC Driver verification** Test emmc driver on the latest kernel
- [ ] P0 **LPC Driver verification** Test LPC/KCS Driver and port kcsd to new driver on the latest kernel
- [ ] P0 **IPMB support** Port ipmbd to new driver
- [ ] P0 **PECI support** Port PECI driver to latest kernel.

## User Interface
- [ ] P0 **In-Band/LPC** Provide interface for Host user to interact via LPC
- [ ] P2 **In-Band/Mezz** Provide interface to Host user over Ethernet interface ethX
- [ ] P0 **OOB/SSH** Provide interface for Host user to interact from a remote machine via Network.
- [ ] P2 **OOB/REST** Provide interface for Host user to interact from a remote machine via Network.
- [ ] P4 **Redfish** Provide redfish interface for Host user to interact from a remote machine via Network.

## Network
- [ ] P0 **DHCPv6** BMC shall be able to get IPv6 address using standard DHCPv6 protocol including retries and timeouts
- [ ] P0 **Broadcom OCP Mezz 3.0 Support** Need to support Broadcom and Mellanox 100G NIC OCP 3.0.
- [ ] P0 **Mellanox OCP Mezz 3.0 Support** NeedNeed to support Broadcom and Mellanox 100G NIC OCP 3.0.
- [ ] P1 **host2bmc (MLNX)** Need to access OpenBMC from Host via NIC Mezzanine Card
- [ ] P1 **host2bmc (BRCM)** Need to access OpenBMC from Host via NIC Mezzanine Card
- [ ] P2 **NIC reset from BMC** Capability to reset the OCP NIC from the BMC.
- [ ] P2 **NIC Monitoring and upgrade via I2C** Capability to upgrade both NIC on the mother board via I2C or RMII
- [ ] P2 **NIC Failure Detection** Monitor NIC Mezzanine card periodically(e.g. NCSI) and detect any failures (eg. reboots) and reinitialize network connectivity

## Power Control
- [ ] P0 **on** Power ON the Host
- [ ] P0 **off** Power OFF the Host
- [ ] P0 **cycle** Power cycle the Host
- [ ] P0 **reset** Reset the host
- [ ] P2 **graceful-shutdown** Shutdown the Host by using ACPI gracefully either by user command or when user press the front button for < 4 seconds
- [ ] P0 **sled-cycle** Power cycle entire chassis
- [ ] P2 **Power Failure Detection** Report host power failure reasoning using information from various GPIO signals and/or CPLD registers

## Serial-Over-LAN (SoL)
- [ ] P0 **Console** Allow user to interact with Host serial console directly
- [ ] P1 **Console History** Allow user to access the history of the Host's serial console activity e.g to debug an issue that prevented Host boot process

## FRU Information
- [ ] P0 **FRUID read** Read and print FRUID (FRUID EEPROM  for Mother board. supported on each BMC to read its local MB's fruid.)
- [ ] P0 **FRUID update** Update FRUID with user given file assuming IPMI FRUID Format

## Sensor Information
- [ ] P0 **sensor read** Read the current status of all sensors of a given FRU:- sensors on motherboard, Mezzanine card, etc.
- [ ] P1 **Display threshold value** When requested, display the threshold values for each sensor(lnr→unr)
- [ ] P1 **Display OK status** Display a sensor is OK or not
- [ ] P1 **Sensor Caching** Cache the sensors in local DB to reduce hardware polling
- [ ] P3 **Sensor History** Store historical min/average/max over several periods:-  3sec, 1minute, 10minutes, 1hour etc.
- [ ] P3 **Sensor History - Automatic** Automatically store a copy of sensor history when CATERR(MSMI) happens

## Thermal Management
- [ ] P0 **Fan Speed Control (OCP-Like)** Implement FSC algorithm with user given JSON configuration file with various linear/pid profiles for sensors across platform
- [ ] P0 **Fan control (Manual)** Implement fan utility to set fan-speed
- [ ] P1 **FSC configuration update** Allow user to update the fan configuration:- Allow to try out a new configuration and be able to restart the fscd
- [ ] P3 **Persistent FSC configuration** Allow user to update the fan configuration to be persistent across BMC reboots
- [ ] P2 **Sensor calibration** The sensor value needs to be adjusted/corrected based on the current PWM value.
- [ ] P2 **Conditional Sensor calibration** Allow for sensor calibration which can change based on runtime detected configuration changes
- [ ] P2 **System COnfiguration** Allow for BIOS to send the current system configuration to the BMC which can be used to determine various thermal calibrations.
- [ ] P0 **Airflow Sensor** Calculate Airflow based on current configuration e.g. use BIOS OEM command to get information on number of DIMMs populated etc.

## Firmware Versions
Support reading firmware versions of attached components including self.

- [ ] P0 **OpenBMC** Read f/w versions for all the components
- [ ] P0 **CPLD** System and auxiliary CPLDs
- [ ] P0 **BIOS** Host Firmware version
- [ ] P0 **ME** Management Engine
- [ ] P0 **VR** All attached Voltage regulators
- [ ] P2 **NIC Mezzanine** Display NIC firmware version

## Firmware Update
- [ ] P0 **OpenBMC** Upgrade BMC Image from within the BMC
- [ ] P0 **CPLD** Upgrade all system and auxiliary CPLDs
- [ ] P0 **BIOS** Upgrade host Firmware version
- [ ] P0 **VR** Upgrade all attached Voltage regulators
- [ ] P2 **NIC Mezzanine** Upgrade NIC firmware version using PLDM/NC-SI or PLDM/MCTP-SMBUS.

## Time Sync
- [ ] P2 **No-NTP** Sync up BMC RTC with Host System's RTC
- [ ] P2 **NTP Sync** Sync up BMC RTC with NTP server
- [ ] P3 **Time Zone** Show PST/PDT time always

## Configuration
- [ ] P0 **Set Power Policy** Change the policy to one of the following state: Last Power State, Always ON, and Always OFF with default as 'Last Power State'
- [ ] P0 **Set Boot Order Sequence** Boot Order sequence to specify which devices to try to boot from: IPv4, IPv6, HDD etc.
- [ ] P0 **CMOS Restore (OEM/0x52)** Restore BIOS settings like BIOS variables to the default state using the OEM/0x52 command
- [ ] P1 **CMOS Restore (Using Set Boot Options)** OptionsRestore BIOS settings like BIOS variables to the default state
- [ ] P0 **CMOS Restore (Physical reset)** Reset CMOS using physical GPIO/I2C
- [ ] P0 **Enable Fast PROCHOT** Allow user to enable or disable Fast PROCHOT functionality
- [ ] P0 **HSC Switch Control** Enable accessing to HSC (either BMC or ME);Default BMC to access, 1

## Diagnostics
- [ ] P0 **crashdump -manual** crash dump tool to generate log file with various CPU internal registers;User can generate on-demand using this utility
- [ ] P1 **crashdump - auto** crash dump automatically when IERR is detected on CATERR/MSMI GPIO pin
- [ ] P2 **crashdump-prevent power off** Prevent host power changes while crash dump is ongoing
- [ ] P2 **Memory Post Package Repair** A procedure to remediate hard failing rows within a memory device that takes place during system POST. It uses the spare rows to replace the broken row. The repair is permanent and cannot be undone.
- [ ] P1 **PECI Interface** Support BMC->CPU communication via PECI interface
- [ ] P2 **At Scale Debug** Replace ITP to debug CPU with BMC as a bridge for Network->JTAG debug path.
- [ ] P3 **Physical Disable of At Scale Debug** Ensure we can physically disable ASD using either a jumper/switch and the default MP configuration is OFF.

## BMC Reset/Recovery
- [ ] P0 **Remote Reset -SSH** Reset any BMC from remote system using the `reboot` command
- [ ] P4 **Remote Reset -REST API** Reset any BMC from remote system using a POST action=reboot to the BMC endpoint (Disabled if client is not authenticated)
- [ ] P0 **GPIO reset** Recover hung BMC by sending GPIO reset from Host server

## Front Panel Control
- [ ] P0 **Power Button** Power ON/OFF the Host by using this button.
- [ ] P0 **Reset Button** PowerReset Host by using this button. Same as power-button.
- [ ] P0 **Debug Card Console** Support debug console when it is present
- [ ] P0 **Debug Card Reset Button** PowerResetReset Host by using the reset button on Debug Card
- [ ] P0 **Console Selection Switch** Toggle the serial console to either BMC or Host
- [ ] P0 **Identify LED/Power-ON** Chassis Identification by blinking LED: User shall be able to turn ON/OFF the identification; And specify the blinking rate to 1/5/10 seconds
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
- [ ] P2 **PFR** Support verified boot using PFR.

## Chassis Power Management
- [ ] P2 **Power Capping** Support Multiple Power Policies at the same time

## Postcode
- [ ] P0 **Send to two 7-segment LED** Send POST code to 8-segment LED display on Debug Card
- [ ] P2 **POST code history** Read previous POST codes e.g. last 256 bytes remotely

## Host Sensor Information
- [ ] P4 **Uptime** Host to report uptime hourly to BMC
- [ ] P4 **Load Average** Host to report load average hourly to BMC
- [ ] P2 **Boot-drive temperature** Host to report boot-drive temperature to the BMC

## PLDM (Over NC-SI or SMBUS-MCTP)
- [ ] P4 **Sensors** Use PLDM to talk to NIC to get various sensor value/status e.g. temperature, cable temp etc.
- [ ] P4 **Firmware Update** Use PLDM based f/w update for supported devices
- [ ] P4 **Notifications** Register and handle for various notifications from PLDM capable MCs

## LCD Debug Card
- [ ] P2 **GPIO Description** Need to create a look up table and respond with descriptions for various GPIO signals connected to USB-Debug card
- [ ] P1 **POST Description** Need to create a look up table and respond with descriptions for various POST codes based on BIOS spec
- [ ] P2 **Critical Sensors Frame** Create a frame (with multiple pages) for various sensors in critical stage (UCR/LCR/ etc.)
- [ ] P2 **Critical Logs Frame** Create a frame (with multiple pages) for various logs to show on LCD panel
- [ ] P2 **Memory Loop Code** When BMC check post code loop pattern "00 XX XX XX", the first code is 00. It is memory loop code fail, the code 2 is dimm location. Ex: 00 A0 30 1C, show "DIMMA0 initial fails".
- [ ] P4 **User Settings** Need to allow user to change some configuration from host

## BMC Health Monitoring
- [ ] P0 **WDT** This feature is useful to detect if BMC user space application is not running for some catastrophic reason. WDT Expire should trigger BMC reset.
- [ ] P2 **ME Health** Sometimes, ME might be in a bad state. To detect this condition, BMC can send IPMI command to ME and if it does not respond, log appropriate event. Disabled on Slave BMCs
- [ ] P1 **Memory Usage** When BMC's free memory goes below a threshold, BMC can reboot to function properly
- [ ] P1 **CPU Usage** When CPU utilization goes above certain threshold, print a warning.
- [ ] P1 **Healthd configuration** Allow configuration on whether healthd should reboot the BMC upon issues.

## Miscellaneous
- [ ] P3 **JSON Format** Allow the output of SSH-based utilites to be in JSON format (e.g sensor-util, power-util, fw-util, log-util etc..)
- [ ] P4 **ODS/Scuba integration** BMC to log various data in to ODS directly. Allow configuration of remote service.
- [ ] P4 **ODS sensor names** Sensor naming should be standardized across various server  platforms using different flavors of BMC(e.g. ODM, OEM or OpenBMC)
- [ ] P2 **Reset Default factory settings** Allow user to clear all configurations
- [ ] P1 **FRB 2/3 Monitoring** Support BIOS Fault Resilient boot.
- [ ] P1 **guid-util** Utility to program Device GUID and System GUID to FRU EEPROM
- [ ] P1 **ipmi-util** Utility to interact with OpenBMC's IPMI stack using raw commands
