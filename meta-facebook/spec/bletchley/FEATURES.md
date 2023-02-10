## Features

### Overview

#### Priority Legend
* P0 - Before EVT Build (soft) to Mid EVT
* P1 - Before DVT build
* P2 - Before PVT Build
* P3 - PVT exit gating
* P4 - Optional

#### Infrastructure
- [X] P0 **Yocto**
    - Enable machine in meta-layer.
- [X] P0 **U-boot Porting**
    - Port platform specific u-boot changes.
- [X] P0 **Kernel Porting**
    - Port platform specific Kernel changes.
- [ ] P1 **Flash Layout**
    - Implement the required flash layout.
- [X] P0 **I2C device tree**
    - Create device tree entries for i2c devices (and enable associated
      drivers).
- [X] P0 **GPIO device tree**
    - Create device tree entries for gpio pins.
- [X] P0 **PWM/Tach device tree**
    - Create device tree entries for PWM/Tach devices.
- [X] P0 **ADC device tree**
    - Create device tree entries for ADC devices.
- [X] P0 **SPI device tree**
    - Create device tree entries for all SPI devices.
- [X] P1 **Qemu**
    - Create Qemu machine config for BMC.  Should boot OpenBMC without any
      service failures.

#### User Interface
- [X] P0 **OOB/SSH**
    - Provide interface for Host user to interact from a remote machine via
      Network.
- [ ] P1 **Redfish**
    - Provide redfish interface for Host user to interact from a remote machine
      via Network.  Redfish APIs should be used as a demo and test vehicle for
      all system features.
- [ ] P2 **Redfish w/mTLS**
    - Enable mTLS authentication for Redfish.

#### Network
- [X] P0 **DHCPv6**
    - BMC shall be able to get IPv6 address using standard DHCPv6 protocol
      including retries and timeouts.

#### Power Control
- [X] P0 **on**
    - Power ON the Host(s).
- [X] P0 **off**
    - Power OFF the Host(s).
- [X] P0 **cycle**
    - Power cycle the Host(s).
- [X] P0 **reset**
    - TODO: facebookexternal/openbmc.quanta#2616.
    - Reset the host(s).
- [X] P0 **recovery mode**
    - Put the host(s) into recovery mode.
- [X] P0 **chassis-cycle**
    - TODO: facebookexternal/openbmc.quanta#2617.
    - Power cycle entire chassis.
- [X] P1 **phosphor-state-manager**
    - Manage host(s) state using LF state management provided by
      phosphor-state-manager.
- ~~[ ] P2 **Power Failure Detection**~~
    - ~~TODO: facebookexternal/openbmc.quanta#2618.~~
    - ~~Report host power failure reasoning using information from various GPIO~~
      ~~signals and/or CPLD registers.~~

#### FRU Information
- [X] P0 **Entity Manager**
    - Add entity manager configuration for system.
- [X] P0 **FRUID read**
    - Read and print FRUID (FRUID EEPROM for Motherboard supported on each BMC
      to read its local MB's fruid).
- [X] P0 **FRUID update**
    - Update FRUID with user given file assuming IPMI FRUID Format.

#### Sensor Information
- [X] P0 **sensor read**
    - Read the current status of all sensors of a given FRU.
        - sensors on motherboard, Mezzanine card, etc.
- [X] P1 **Display threshold value**
    - When requested, display the threshold values for each sensor (`lnr->unr`).
- [X] P1 **Display OK status**
    - Display a sensor is OK or not.
- [ ] P3 **Sensor History**
    - Store historical min/average/max over several periods:
        -  3sec, 1minute, 10minutes, 1hour etc.
- [ ] P3 **Sensor History - Automatic**
    - Automatically store a copy of sensor history when catastrophic events are
      detected.

#### Thermal Management
- [X] P0 **Fan control (Manual)**
    - Implement fan utility to set fan-speed.
- [X] P0 **Fan Speed Control (Automatic)**
    - Implement FSC algorithm with user given JSON configuration file with
      various linear/pid profiles for sensors across platform.
- [X] P2 **FSC configuration update**
    - Allow user to update the fan configuration.  Allow to try out a new
      configuration and be able to restart the fan control daemon.
- [ ] P3 **Persistent FSC configuration**
    - Allow user to update the fan configuration to be persistent across BMC
      reboots.
- [X] P2 **Sensor calibration**
    - The sensor value needs to be adjusted/corrected based on the current PWM
      value.
- [ ] P2 **Conditional Sensor calibration**
    - Allow for sensor calibration which can change based on runtime detected
      configuration changes.
- [X] P0 **Airflow Sensor**
    - Calculate Airflow based on current configuration e.g. use BIOS OEM command
      to get information on number of DIMMs populated etc.

#### Firmware Versions
Support reading firmware versions of attached components including self.

- [X] P0 **OpenBMC**
    - Read f/w versions for all the components.
- ~~[ ] P0 **VR**~~
    - ~~TODO: facebookexternal/openbmc.quanta#2619.~~
    - ~~All attached Voltage regulators.~~
- ~~[ ] P2 **Ethernet Switch**~~
    - ~~Display Ethernet Switch firmware version.~~
- [ ] P4 **USB3 Bridge**
    - Display USB3 Bridge firmware version.

#### Firmware Update
- [X] P0 **OpenBMC**
    - Upgrade BMC Image from within the BMC.
- [ ] P4 **Flashy**
    - _Facebook_ to develop.
    - Support system in `flashy`.
- [ ] P4 **Wipe Workflows**
    - _Facebook_ to develop.
    - Enable device wipe workflows for decomm.
- ~~[ ] P0 **VR**~~
    ~~- TODO: facebookexternal/openbmc.quanta#2619.~~
    ~~- Upgrade all attached Voltage regulators.~~
- ~~[ ] P0 **Ethernet Switch**~~
    - ~~Upgrade Ethernet Switch manually using flashcp.~~
- ~~[ ] P2 **Ethernet Switch**~~
    - ~~Upgrade Ethernet Switch using production (dbus+Redfish) methods.~~
- [X] P0 **USB3 Bridge**
    - Upgrade USB3 Bridge manually using flashcp.
- [ ] P4 **USB3 Bridge**
    - Upgrade USB3 Bridge using production (dbus+Redfish) methods.

#### Time Sync
- [X] P0 **RTC**
    - Enable local RTC device and ensure synchronization on BMC reboot.
- [X] P2 **NTP Sync**
    - Sync up BMC RTC with NTP server.
- [X] P3 **Time Zone**
    - Show PST/PDT time always.

#### Configuration
- [X] P0 **Set Power Policy**
    - Change the policy to one of the following state: Last Power State, Always
      ON, and Always OFF with default as 'Last Power State'.

#### BMC Reset/Recovery
- [X] P0 **Remote Reset - SSH**
    - Reset any BMC from remote system using the `reboot` command.
- [X] P1 **Remote Reset - Redfish**
    - Reset any BMC from remote system using Redfish APIs.

#### Front Panel Control
- [X] P0 **Power Button**
    - Power ON/OFF the Host by using this button.
- ~~[ ] P0 **Reset Button**~~
    - ~~PowerReset Host by using this button. Same as power-button.~~
- [X] P0 **Debug Card Console**
    - Support debug console when it is present.
- ~~[ ] P0 **Debug Card Reset Button**~~
    - ~~PowerReset Reset Host by using the reset button on Debug Card.~~
- [ ] P0 **Identify LED/Power-ON**
    - Chassis Identification by blinking LED: User shall be able to turn ON/OFF
      the identification; And specify the blinking rate to 1/5/10 seconds.
- [X] P0 **Heartbeat LED**
    - TODO: facebookexternal/openbmc.quanta#2623.
    - Allow configuration and use of heartbeat LED.

#### Security
- [ ] P4 **Verified Boot (dual-flash)**
    - Support updating both flashes images and support read-only image to
      continue boot into RW flash.
- [ ] P4 **Verified Boot (image verification)**
    - Validating the RW image and provide an utility to report on errors of
      validation/verification.
- [ ] P4 **Verified Boot (recovery)**
    - If RW image is empty or does not contain a valid image, boot into
      recovery.
- [ ] P4 **Verified Boot (SW enforcement)**
    - Boot into recovery image if signature verification of RW image fails.
- [ ] P4 **Verified Boot (HW enforcement)**
    - Do not allow modification of recovery image in hardware.
- [ ] P4 **Verified Boot (Image signing service)**
    - Setup a service for BMC image signing.
- [X] P1 **TPM 2.0 Test**
    - Test TPM access via SPI.
- [ ] P4 **TPM 2.0 Boot Measurements**
    - Use TPM for storing secret keys and measurement info.

#### Host Sensor Information
- [ ] P4 **Uptime**
    - Host to report uptime hourly to BMC.

#### LCD Debug Card
- [X] P2 **GPIO Description**
    - Need to create a look up table and respond with descriptions for various
      GPIO signals connected to USB-Debug card.
- [X] P2 **Critical Sensors Frame**
    - Create a frame (with multiple pages) for various sensors in critical stage
      (UCR/LCR/ etc.).
- [ ] P4 **Critical Logs Frame**
    - Create a frame (with multiple pages) for various logs to show on LCD
      panel.
- [ ] P4 **User Settings**
    - Need to allow user to change some configuration from host.

#### BMC Health Monitoring
- [X] P0 **WDT**
    - This feature is useful to detect if BMC user space application is not
      running for some catastrophic reason. WDT Expire should trigger BMC reset.
- [X] P1 **Memory Usage**
    - When BMC's free memory goes below a threshold, BMC can reboot to function
      properly.
- [X] P1 **CPU Usage**
    - When CPU utilization goes above certain threshold, print a warning.
- [X] P1 **Healthd configuration**
    - Allow configuration on whether healthd should reboot the BMC upon issues.

#### Device Specific Features
- [ ] P1 **USB-PD Emulation**
    - Emulate USB-PD to allow software communication with managed host(s).
- [X] P2 **Switch diagnostics**
    - TODO: facebookexternal/openbmc.quanta#2621.
    - Diagnostics, state, and sensors from 88E6191X MDIO interface.
- ~~[ ] P2 **USB-C diagnostics**~~
    - ~~TODO: facebookexternal/openbmc.quanta#2620.~~
    - ~~Diagnostics, state, and sensors from JHL7440 i2c interface.~~
- [X] P1 **Automated Recovery**
    - Initiate automated recovery of managed host(s) over USB.


#### Miscellaneous
- [ ] P4 **ODS/Scuba integration**
    - _Facebook_ to develop.
    - BMC to log various data in to ODS directly. Allow configuration of remote
      service.
- [ ] P4 **ODS sensor names**
    - _Facebook_ to develop.
    - Sensor naming should be standardized across various server platforms
      using different flavors of BMC(e.g. ODM, OEM or OpenBMC).
- [X] P2 **Reset Default factory settings**
    - Allow user to clear all configurations.
- ~~[ ] P1 **guid-util**~~
    - ~~TODO: facebookexternal/openbmc.quanta#2622.~~
    - ~~Utility to program Device GUID and System GUID to FRU EEPROM.~~
- [ ] P2 **openbmc-test-automation**
    - Testing enabled with upstream openbmc-test-automation package.
