# Facebook OpenBMC Features for Waimea Canyon

This document tracks the features planned for platform Waimea Canyon.

## Priority Legend
* P0 - Before EVT Build (soft) to Mid EVT
* P1 - Before DVT build
* P2 - Before PVT Build
* P3 - PVT exit gating
* P4 - Optional

## Infrastructure
- [X] P0 **U-boot Porting**
    - Port platform specific u-boot (v2019.04) changes.
- [X] P0 **Kernel Porting**
    - Port platform specific Kernel (5.15) changes.
- [X] P0 **GPIO driver porting**
    - Port GPIO configuration to ensure we are using gpiocli.
- [X] P0 **IPMB support**
    - Port ipmbd to communicate with BIC and Expander.

## User Interface
- [X] P0 **In-Band**
    - Provide interface for Host user to interact with BMC via BIC. (Host --eSPI-> BIC --IPMB-> BMC)
- [X] P0 **In-Band/Mezz**
    - Provide interface to Host user over Ethernet interface ethX.
- [X] P0 **OOB/SSH**
    - Provide interface for Host user to interact from a remote machine via Network.
- [ ] P2 **OOB/REST**
    - Provide interface for Host user to interact from a remote machine via Network.
- [ ] P2 **Redfish**
    - Provide redfish interface for Host user to interact from a remote machine via Network. Redfish APIs should be used as a demo and test vehicle for all system features.
- [ ] P2 **Host2bmc USB**
    - This ether-over-usb recovery channel will only be used during BMC recovery scenario, the design shall support on-demand setup and tear-off this channel by host s/w. i.e use OEM IPMI command to on-demand setup and tear-off.

## Network
- [X] P0 **DHCPv4**
    - BMC shall be able to get IPv4 address using standard DHCPv4 protocol including retries and timeouts.
- [X] P0 **DHCPv6**
    - BMC shall be able to get IPv6 address using standard DHCPv6 protocol including retries and timeouts.
- [ ] P0 **OCP Mezz 3.0 Support**
    - Need to support Broadcom and Mellanox NIC OCP 3.0.
- [ ] P1 **NIC Monitoring and upgrade**
    - Capability to upgrade NIC via I2C or RMII.

## Power Control
- [ ] P0 **on**
    - Power ON the Host.
- [ ] P0 **off**
    - Power OFF the Host.
- [ ] P0 **cycle**
    - Power cycle the Host.
- [ ] P0 **reset**
    - Reset the host.
- [ ] P0 **graceful-shutdown**
    - Shutdown the Host by using ACPI gracefully either by user command or when user press the front button for < 4 seconds.
- [ ] P0 **12V-on**
    - Turn ON 12V of the Host slot.
- [ ] P0 **12V-off**
    - Turn OFF 12V of the Host slot.
- [ ] P0 **12V-cycle**
    - Cycle 12V of the Host slot.
- [ ] P0 **sled-cycle**
    - Power cycle entire chassis.

## Serial-Over-LAN (SoL)
- [X] P0 **Console**
    - Allow user to interact with Host serial console directly.
- [ ] P1 **Console History**
    - Allow user to access the history of the Host's serial console activity e.g to debug an issue that prevented Host boot process.

## FRU Information
- [ ] P0 **FRUID read**
    - Read and print FRUID (FRUID EEPROM for mb/iom/scb/scm/bsm/hdbp/pdb/fan).
- [ ] P0 **FRUID update**
    - Update FRUID with user given file assuming IPMI FRUID Format.

## Sensor Information
- [ ] P0 **sensor read**
    - Read the current status of all sensors of a given FRU.
        - mb/iom/scb/scm/hdbp/pdb.
- [ ] P1 **Display threshold value**
    - When requested, display the threshold values for each sensor (`lnr->unr`).
- [ ] P1 **Display OK status**
    - Display a sensor is OK or not.
- [ ] P3 **Sensor History**
    - Store historical min/average/max over several periods:
        -  3sec, 1minute, 10minutes, 1hour etc.
- [ ] P3 **Sensor History - Automatic**
    - Automatically store a copy of sensor history when catastrophic events are detected.

## Thermal Management
- [ ] P0 **Fan control (Manual)**
    - Implement fan utility to set fan-speed.
- [ ] P2 **Fan Speed Control (Automatic)**
    - Implement FSC algorithm with user given JSON configuration file with various linear/pid profiles for sensors across platform.
- [ ] P2 **FSC configuration update**
    - Allow user to update the fan configuration.  Allow to try out a new configuration and be able to restart the fan control daemon.
- [ ] P2 **Airflow Sensor**
    - Calculate Airflow based on current configuration.

## Firmware Versions
Support reading firmware versions of attached components including self.

- [X] P0 **OpenBMC**
    - Read f/w versions for all the components.
- [X] P0 **BIC**
    - Brige IC.
- [X] P0 **CPLD**
    - System and auxiliary CPLDs.
- [X] P0 **BIOS**
    - Host Firmware version.
- [X] P0 **ME**
    - Management Engine.
- [X] P0 **VR**
    - All attached Voltage regulators.
- [ ] P2 **NIC Mezzanine**
    - Display NIC firmware version.
- [ ] P2 **Expander**
    - Display Expander firmware version.
- [ ] P2 **SAS IOC**
    - Display SAS IOC firmware version.

## Firmware Update
- [X] P0 **OpenBMC**
    - Upgrade BMC Image from within the BMC.
- [X] P0 **BIC**
    - Brige IC.
- [X] P0 **CPLD**
    - Upgrade all system and auxiliary CPLDs.
- [ ] P0 **BIOS**
    - Upgrade host Firmware version.
- [X] P0 **VR**
    - Upgrade all attached Voltage regulators.
- [ ] P2 **NIC Mezzanine**
    - Upgrade NIC firmware using PLDM/NC-SI or PLDM/MCTP-SMBUS.
- [ ] P2 **Expander**
    - Upgrade Expander firmware.
- [ ] P2 **SAS IOC**
    - Upgrade SAS IOC firmware.

## Time Sync
- [ ] P1 **RTC**
    - Sync up BMC RTC with system's RTC.
- [ ] P1 **NTP Sync**
    - Sync up BMC RTC with NTP server.
- [ ] P1 **Time Zone**
    - Show PST/PDT time always.

## Configuration
- [ ] P0 **Set Power Policy**
    - Change the policy to one of the following state: Last Power State, Always ON, and Always OFF with default as 'Last Power State'.
- [ ] P0 **Set Boot Order Sequence**
    - Boot Order sequence to specify which devices to try to boot from: IPv4, IPv6, HDD etc.
- [ ] P0 **CMOS Restore (OEM/0x52)**
    - Restore BIOS settings like BIOS variables to the default state using the OEM/0x52 command.
- [ ] P1 **CMOS Restore (Physical reset)**
    - Reset CMOS using physical GPIO/I2C.

## Diagnostics
- [ ] P1 **crashdump -manual**
    - crash dump tool to generate log file with various CPU internal registers; User can generate on-demand using this utility.
- [ ] P1 **crashdump - auto**
    - crash dump automatically when IERR is detected on CATERR/MSMI GPIO pin.
- [ ] P2 **crashdump-prevent power off**
    - Prevent host power changes while crash dump is ongoing.
- [ ] P2 **Memory Post Package Repair**
    - A procedure to remediate hard failing rows within a memory device that takes place during system POST. It uses the spare rows to replace the broken row. The repair is permanent and cannot be undone.
- [ ] P2 **At Scale Debug**
    - Replace ITP to debug CPU with BMC as a bridge for Network->JTAG debug path.

## BMC Reset/Recovery
- [X] P0 **Remote Reset - SSH**
    - Reset any BMC from remote system using the `reboot` command.
- [ ] P2 **Remote Reset - Redfish**
    - Reset any BMC from remote system using Redfish APIs.
- [ ] P1 **GPIO reset**
    - Recover hung BMC by sending GPIO reset from Host server.

## Front Panel Control
- [ ] P0 **Debug Card Console**
    - Support debug console when it is present.
- [ ] P0 **Debug Card Reset Button**
    - Power Reset Host by using the reset button on Debug Card.
- [ ] P0 **Console Selection Switch**
    - Toggle the serial console to BMC/Host/BIC/Expander.
- [ ] P0 **Identify LED/Power-ON**
    - Chassis Identification by blinking LED: User shall be able to turn ON/OFF the identification.

## Security
- [X] P0 **Verified Boot (dual-flash)**
    - Support updating both flashes images and support read-only image to continue boot into RW flash.
- [ ] P1 **Verified Boot (image verification)**
    - Validating the RW image and provide an utility to report on errors of validation/verification.
- [ ] P1 **Verified Boot (recovery)**
    - If RW image is empty or does not contain a valid image, boot into recovery.
- [ ] P2 **Verified Boot (SW enforcement)**
    - Boot into recovery image if signature verification of RW image fails.
- [ ] P2 **Verified Boot (HW enforcement)**
    - Do not allow modification of recovery image in hardware.
- [X] P0 **TPM 2.0 Test**
    - Test TPM access via SPI.
- [ ] P2 **TPM 2.0 Boot Measurements**
    - Use TPM for storing secret keys and measurement info.
- [ ] P3 **attest-util**
    - Utility to send SPDM messages via MCTP for supported devices like SAS IOC.

## Postcode
- [ ] P1 **POST code history**
    - Read previous POST codes e.g. last 256 bytes remotely.

## PLDM (Over NC-SI or SMBUS-MCTP)
- [ ] P2 **Firmware Update**
    - Use PLDM based f/w update for supported devices like NIC.

## LCD Debug Card
- [ ] P2 **GPIO Description**
    - Need to create a look up table and respond with descriptions for various GPIO signals connected to USB-Debug card.
- [ ] P2 **POST Description**
    - Need to create a look up table and respond with descriptions for various POST codes based on BIOS spec.
- [ ] P2 **Critical Sensors Frame**
    - Create a frame (with multiple pages) for various sensors in critical stage (UCR/LCR/ etc.).
- [ ] P2 **Critical Logs Frame**
    - Create a frame (with multiple pages) for various logs to show on LCD panel.
- [ ] P2 **Memory Loop Code**
    - When BMC check post code loop pattern "00 XX XX XX", the first code is 00. It is memory loop code fail, the code 2 is dimm location. Ex: 00 A0 30 1C, show "DIMMA0 initial fails".
- [ ] P2 **User Settings**
    - Need to allow user to change some configuration.

## BMC Health Monitoring
- [X] P0 **WDT**
    - This feature is useful to detect if BMC user space application is not running for some catastrophic reason. WDT Expire should trigger BMC reset.
- [ ] P1 **ME Health**
    - Sometimes, ME might be in a bad state. To detect this condition, BMC can send IPMI command to ME and if it does not respond, log appropriate event.
- [ ] P1 **Memory Usage**
    - When BMC's free memory goes below a threshold, BMC can reboot to function properly.
- [ ] P1 **CPU Usage**
    - When CPU utilization goes above certain threshold, print a warning.

## Miscellaneous
- [ ] P1 **JSON Format**
    - Allow the output of SSH-based utilites to be in JSON format (e.g sensor-util, fw-util, log-util etc..).
- [ ] P0 **Reset Default factory settings**
    - Allow user to clear all configurations.
- [ ] P1 **FRB 2/3 Monitoring**
    - Support BIOS Fault Resilient boot.
- [X] P0 **guid-util**
    - Utility to program Device GUID and System GUID to FRU EEPROM.
- [X] P0 **ipmi-util**
    - Utility to interact with OpenBMC's IPMI stack using raw commands.
- [ ] P2 **enclosure-util**
    - Utility to display E1.S SSD drive status.
