## Features

### Overview

#### Infrastructure
- [X] P0 **Yocto**
    - Enable machine in meta-layer.
- [X] P0 **U-boot Porting**
    - Port platform specific u-boot changes.
- [X] P0 **Kernel Porting**
    - Port platform specific Kernel changes.
- [ ] P2 **Flash Layout**
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
- [X] P2 **Qemu**
    - Create Qemu machine config for BMC.  Should boot OpenBMC without any
      service failures.

#### User Interface
- [X] P0 **OOB/SSH**
    - Provide interface for Host user to interact from a remote machine via
      Network.
- [ ] P2 **Redfish**
    - Provide redfish interface for Host user to interact from a remote machine
      via Network.  Redfish APIs should be used as a demo and test vehicle for
      all system features.
- [ ] P3 **Redfish w/mTLS**
    - Enable mTLS authentication for Redfish.

#### Network
- [X] P1 **DHCPv6**
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
- [X] P2 **phosphor-state-manager**
    - Manage host(s) state using LF state management provided by
      phosphor-state-manager.

#### FRU Information
- [X] P0 **Entity Manager**
    - Add entity manager configuration for system.
- [X] P0 **FRUID read**
    - Read and print FRUID (FRUID EEPROM for Motherboard supported on each BMC
      to read its local MB's fruid).
- [X] P1 **FRUID update**
    - Update FRUID with user given file assuming IPMI FRUID Format.

#### Sensor Information
- [X] P0 **sensor read**
    - Read the current status of all sensors of a given FRU.
        - sensors on motherboard, Mezzanine card, etc.
- [X] P2 **Display threshold value**
    - When requested, display the threshold values for each sensor (`lnr->unr`).
- [X] P2 **Display OK status**
    - Display a sensor is OK or not.
- [ ] P4 **Sensor History**
    - Store historical min/average/max over several periods:
        -  3sec, 1minute, 10minutes, 1hour etc.
- [ ] P4 **Sensor History - Automatic**
    - Automatically store a copy of sensor history when catastrophic events are
      detected.

#### Thermal Management
- [X] P0 **Fan control (Manual)**
    - Implement fan utility to set fan-speed.
- [X] P1 **Fan Speed Control (Automatic)**
    - Implement FSC algorithm with user given JSON configuration file with
      various linear/pid profiles for sensors across platform.
- [X] P3 **FSC configuration update**
    - Allow user to update the fan configuration.  Allow to try out a new
      configuration and be able to restart the fan control daemon.
- [ ] P4 **Persistent FSC configuration**
    - Allow user to update the fan configuration to be persistent across BMC
      reboots.
- [X] P3 **Sensor calibration**
    - The sensor value needs to be adjusted/corrected based on the current PWM
      value.
- [ ] P3 **Conditional Sensor calibration**
    - Allow for sensor calibration which can change based on runtime detected
      configuration changes.
- [X] P1 **Airflow Sensor**
    - Calculate Airflow based on current configuration e.g. use BIOS OEM command
      to get information on number of DIMMs populated etc.

#### Firmware Versions
Support reading firmware versions of attached components including self.

- [X] P0 **OpenBMC**
    - Read f/w versions for all the components.
- [ ] P5 **USB3 Bridge**
    - Display USB3 Bridge firmware version.

#### Firmware Update
- [X] P0 **OpenBMC**
    - Upgrade BMC Image from within the BMC.
- [ ] P5 **Flashy**
    - _Facebook_ to develop.
    - Support system in `flashy`.
- [ ] P5 **Wipe Workflows**
    - _Facebook_ to develop.
    - Enable device wipe workflows for decomm.
- [X] P0 **USB3 Bridge**
    - Upgrade USB3 Bridge manually using flashcp.
- [ ] P5 **USB3 Bridge**
    - Upgrade USB3 Bridge using production (dbus+Redfish) methods.

#### Time Sync
- [X] P0 **RTC**
    - Enable local RTC device and ensure synchronization on BMC reboot.
- [X] P3 **NTP Sync**
    - Sync up BMC RTC with NTP server.
- [X] P4 **Time Zone**
    - Show PST/PDT time always.

#### Configuration
- [X] P1 **Set Power Policy**
    - Change the policy to one of the following state: Last Power State, Always
      ON, and Always OFF with default as 'Last Power State'.

#### BMC Reset/Recovery
- [X] P0 **Remote Reset - SSH**
    - Reset any BMC from remote system using the `reboot` command.
- [X] P2 **Remote Reset - Redfish**
    - Reset any BMC from remote system using Redfish APIs.

#### Front Panel Control
- [X] P0 **Power Button**
    - Power ON/OFF the Host by using this button.
- [X] P0 **Debug Card Console**
    - Support debug console when it is present.
- [ ] P0 **Identify LED/Power-ON**
    - Chassis Identification by blinking LED: User shall be able to turn ON/OFF
      the identification; And specify the blinking rate to 1/5/10 seconds.
- [X] P0 **Heartbeat LED**
    - TODO: facebookexternal/openbmc.quanta#2623.
    - Allow configuration and use of heartbeat LED.

#### Security
- [ ] P5 **Verified Boot (dual-flash)**
    - Support updating both flashes images and support read-only image to
      continue boot into RW flash.
- [ ] P5 **Verified Boot (image verification)**
    - Validating the RW image and provide an utility to report on errors of
      validation/verification.
- [ ] P5 **Verified Boot (recovery)**
    - If RW image is empty or does not contain a valid image, boot into
      recovery.
- [ ] P5 **Verified Boot (SW enforcement)**
    - Boot into recovery image if signature verification of RW image fails.
- [ ] P5 **Verified Boot (HW enforcement)**
    - Do not allow modification of recovery image in hardware.
- [ ] P5 **Verified Boot (Image signing service)**
    - Setup a service for BMC image signing.
- [X] P2 **TPM 2.0 Test**
    - Test TPM access via SPI.
- [ ] P5 **TPM 2.0 Boot Measurements**
    - Use TPM for storing secret keys and measurement info.

#### Host Sensor Information
- [ ] P5 **Uptime**
    - Host to report uptime hourly to BMC.

#### LCD Debug Card
- [X] P3 **GPIO Description**
    - Need to create a look up table and respond with descriptions for various
      GPIO signals connected to USB-Debug card.
- [X] P3 **Critical Sensors Frame**
    - Create a frame (with multiple pages) for various sensors in critical stage
      (UCR/LCR/ etc.).
- [ ] P5 **Critical Logs Frame**
    - Create a frame (with multiple pages) for various logs to show on LCD
      panel.
- [ ] P5 **User Settings**
    - Need to allow user to change some configuration from host.

#### BMC Health Monitoring
- [X] P1 **WDT**
    - This feature is useful to detect if BMC user space application is not
      running for some catastrophic reason. WDT Expire should trigger BMC reset.
- [X] P2 **Memory Usage**
    - When BMC's free memory goes below a threshold, BMC can reboot to function
      properly.
- [X] P2 **CPU Usage**
    - When CPU utilization goes above certain threshold, print a warning.
- [X] P2 **Healthd configuration**
    - Allow configuration on whether healthd should reboot the BMC upon issues.

#### Device Specific Features
- [ ] P2 **USB-PD Emulation**
    - Emulate USB-PD to allow software communication with managed host(s).
- [X] P3 **Switch diagnostics**
    - TODO: facebookexternal/openbmc.quanta#2621.
    - Diagnostics, state, and sensors from 88E6191X MDIO interface.
- [X] P2 **Automated Recovery**
    - Initiate automated recovery of managed host(s) over USB.


#### Miscellaneous
- [ ] P5 **ODS/Scuba integration**
    - _Facebook_ to develop.
    - BMC to log various data in to ODS directly. Allow configuration of remote
      service.
- [ ] P5 **ODS sensor names**
    - _Facebook_ to develop.
    - Sensor naming should be standardized across various server platforms
      using different flavors of BMC(e.g. ODM, OEM or OpenBMC).
- [X] P3 **Reset Default factory settings**
    - Allow user to clear all configurations.
- [ ] P3 **openbmc-test-automation**
    - Testing enabled with upstream openbmc-test-automation package.
