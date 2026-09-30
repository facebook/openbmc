# Advanced Platform Management Link (APML) Library
# (formerly known as EPYC™ System Management Interface (E-SMI) Out-of-band Library)

Thank you for using AMD APML Library.

# Changes Notes
## Highlights of minor release v5.4.1

* Add support for run time post package repair
* Bug fixes:
  * Fix doxygen comments
  * API Parameter Correction: Fixed incorrect parameter passed to
    get_df_pstate_range() API call
  * Mailbox Command Update: Removed defeatured tool option for
    mailbox command ID 73h
  * PCIe Link Training: Updated PCIe Link training command to align
    with latest PPR specifications
  * Code Formatting: Corrected tool indentation for consistency
  * Power Efficiency Mode Input Parameter Validation

## Highlights of major release v5.3.0

* Add new platform support, Family:0x1A, Model:0x50~0x57
  * New mailbox command support
    - SET_DIMM_SPD (0x71)
    - PC6_ENABLE (0x75)
    - GET_DRAM_THROTTLE_CHANNELS (0x78)
    - SET_PCIE_LINK_TRAINING (0x79)
    - GET_CCD_POW_CONSUMPTION (0x7A)
    - GET_T_DELTA (0x7B)
    - GET_SVI3_VR_CONTROLLER_TEMP_BY_RAIL (0x7C)
    - ENABLED_HSMP_COMMANDS (0x7D)
    - GET_SET_APML_FLOOR_LIMIT (0x7E)
    - GET_SET_SOC_DIMM_POWER_LIMIT (0x7F)
  * New RMI registers APIs to control functionality
* Bug fixes and cosmetic changes

## Highlights of major release v4.0.1

* Add support for new mailbox messages:
   - PCIE config write(0x68)
   - xGMI_PSTATE RANGE(0x73),
   - CPU_RAIL_ISO_FREQ_POLICY(0x74),
   - DFC_ENABLE(0x76),
   - Read DRAM Throttle enhanced version (0x78)
   - Get SMU firmware version (0x1C)
   - Get Public Serial number (0x22)
   - DIMM spd register data (0x70)
   - DIMM spd serial number
* Add support for the new APML features on family 19h and model 90h ~ 9Fh
   - New SBRMI mailbox messages including
       - GetCurrentXGMIPState (0x85)
       - MaxOperatingTemperature (0xA2)
       - GetSlowdownTemperature (0xA3)
       - HBMDeviceInformation (0xB7)
       - GetPCIeStats (0XBA)
* New mailbox error codes, 0x5 and 0xB
* New Platform Support:
   - Family:0x19, Model:0xA0~0xAF
   - Family:0x1A, Model:0x10~0x1F

## Highlights of major release v3.2.0

* Add support for new APML features on family 1Ah and model 00h - 0Fh
   - New SBRMI mailbox messages adding support for Runtime errors
       - Runtime error validity check & Runtime error info
       - RAS set threshold
       - RAS set/get OOB config
       - RTC timer
    - Bug fixes & Cosmetic changes

## Highlights of major release v3.1.0
* Add support for the new APML features on family 19h amd model 90h - 9Fh
   - New SBRMI mailbox messages including, [80h and later]
       - GPU Telemetry
       - HBM Telemetry
       - BIST result on basis of DIE-IDs
       - Number of sockets in system/node
   - New SBTSI Registers
       - HBM cofiguration
       - HBM Temperature High and Low Threshold
   - APML tool, new improved TSI summary
* Add support for the following mailbox messages
    - Read microcode revision
    - Read CCLK Frequency Limit
    - Read socket C0 residency
    - Read PPIN fuse
    - BMC RAS DF Error validity check
    - BMC RAS DF Error Dump
    - Using revision instead of cpuid for messages 8h and 9h
    - Changes as per apml modules change to name misc device on basis
      of device static address for RMI and TSI(sbrmi0 -> sbrmi-3c)
    - Update alertmask and alert status to support all the threads
    - APML socket recovery mechanism
    - Bug fixes & Cosmetic changes

## Highlights of minor release v2.1

* Update library/tool based on APML spec from PPR for AMD Family 19h Model 11h B1
* Introduced a module in apml_tool to provide cpuid information
* Bug fix (display temperature)

## Highlights of major release v2.0
* Rename ESMI_OOB library to APML Library
* Rework the library to use APML modules (apml_sbrmi and apml_sbtsi)
    - This helps us acheive better locking across the protocols
    - APML modules takes care of the probing the APML devices and provding interfaces
* Add features supported on Family 19h Model 10h-1Fh
    - Support APML over I2C and APML over I3C
    - Handle thread count >127 per socket
    - CPUID and MSR read over I3C

## Highlights of minor release v1.1

* Single command to create doxygen based PDF document
* Improved the esmi tool

## Highlights of major release v1.0
APIs to Monitor and control the following feature
* Power
    * Get current power consumed
    * Get and set cap/limit
    * Get max power cap/limit
* Performance
    * Get/Set boostlimit
    * Get DDR bandwidth
    * Set DRAM throttle
* Temeprature
    * Get CPU temperature
    * Set High/Low temperature threshold
    * Set Temp offset
    * Set alert threshold sample & alert config
    * Set readorder

# Supported Processors
* Family 17h model 31h
* Family 19h model 0h~0Fh, 10h~1Fh, 90h - 9Fh
* Family 1Ah model 0h~0Fh

# Supported BMCs
Compile this library for any BMC running linux. Please use the [APML Library/tool Support](https://github.com/amd/esmi_oob_library/issues) to provide your feedback.

# Supported Operating Systems
APML Library is tested on OpenBMC for Aspeed and RPI based BMCs with the following "System requirements"

# System Requirements
## Hardware requirements
BMC with I2C/I3C controller as master, I2C/I3C master adapter channel (SCL and SDA lines) connected to the "Supported Processors"

## Software requirements

In order to build the APML library, the following components are required. Note that the software versions listed are what is being used in development. Earlier versions are not guaranteed to work:

### Compilation requirements
* CMake (v3.5.0)
* APML library upto v1.1 depends on libi2c-dev
* Doxygen (1.8.13)
* latex (pdfTeX 3.14159265-2.6-1.40.18)

# Dependencies
APML library upto v1.1 depends on libi2c-dev
Later versions depends on the apml_modules (hosted github.com/amd/apml_modules)

# Support
To provide your feedback, bug reports, support and feature requests.

Please use [APML Library Support](https://github.com/amd/esmi_oob_library/issues).
