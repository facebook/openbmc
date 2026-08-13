# Advanced Platform Management Link (APML) Library
# (formerly known as EPYC™ System Management Interface (E-SMI) Out-of-band Library)

The Advanced Platform Management Link (APML) Library library, is part of the EPYC™ System Management Out-of-band software stack. It is a C library for Linux that provides a user space interface to monitor and control the CPU's Systems Management features.

# Important note about Versioning and Backward Compatibility
The APML library is currently under development, and therefore subject to change at the API level. The intention is to keep the API as stable as possible while in development, but in some cases we may need to break backwards compatibility in order to achieve future stability and usability. Following Semantic Versioning rules, while the APML library is in a high state of change, the major version will remain 0, and achieving backward compatibility may not be possible.

Once new development has leveled off, the major version will become greater than 0, and backward compatibility will be enforced between major versions.

# Building APML Library

#### Additional Required software for building
In order to build the APML library, the following components are required. Note that the software versions listed are what is being used in development. Earlier versions are not guaranteed to work:
* CMake (v3.5.0)
* latex (pdfTeX 3.14159265-2.6-1.40.18)
* apml modules (apml_sbrmi and apml_sbtsi)
   + available at https://github.com/amd/apml_modules/

#### Dowloading the source
The source code for APML library is available on [Github](https://github.com/amd/esmi_oob_library).

#### Directory stucture of the source
Once the APML library source has been cloned to a local Linux machine, the directory structure of source is as below:
* `$ docs/` Contains Doxygen configuration files and Library descriptions
* `$ tool/` Contains apml_tool  based on the APML library
* `$ include/esmi_oob` Contains the header files used by the APML library
* `$ src/esmi_oob` Contains library APML source

#### Building the library is achieved by following the typical CMake build sequence for native build, as follows.
##### ```$ mkdir -p build```
##### ```$ mkdir -p install```
##### ```$ cd build```
##### ```$ cmake -DCMAKE_INSTALL_PREFIX=${PWD}/install <location of root of APML library CMakeLists.txt>```
##### ```$ make```
The built library will appear in the `build` folder.

#### Cross compile the library for Target systems

Before installing the cross compiler verfiy the target architecture
##### ```$ uname -m```

Eg: To cross compile for ARM32 processor:
##### ```$ sudo apt-get install gcc-arm-linux-gnueabihf```

Eg: To cross compile for AARCH64 processor: use 
##### ```$ sudo apt-get install gcc-aarch64-linux-gnu```

NOTE: For cross compilation, cross-$ARCH.cmake file is provided for below Architectures:
 - armhf
 - aarch64

Compilation steps
##### ```$ mkdir -p build```
##### ```$ cd build```
##### ```$ cmake -DCMAKE_TOOLCHAIN_FILE=../cross-[arch..].cmake <location of root of APML library CMakeLists.txt>```
##### ```$ make```
The built library will appear in the `build` folder.
Copy the required binaries and the dynamic linked library to target board(BMC).
##### ```$ scp libapml64.so.0 root@10.x.x.x:/usr/lib```
##### ```$ scp apml_tool root@10.x.x.x:/usr/bin```

#### Disclaimer
 - Input arguments passed by the user are not validated. It might result in unreliable system behavior

#### Building the Documentation
The documentation PDF file can be built with the following steps (continued from the steps above):
##### ```$ make doc```
The reference manual (APML_Library_Manual.pdf), release notes (APML_Library_Release_Notes.pdf) upon a successful build.

# Usage Basics
Most of the APIs need socket index as the first argument. Refer tools/apml_tool.c

# Usage
## Tool Usage
APML tool is a C program based on the APML Library, the executable "apml_tool" will be generated
in the build/ folder. This tool provides options to monitor and control System Management functionality.

In execution platform, user can cross-verfiy "apml_sbrmi" and apml_rmi" modules are loaded.
The apml modules are open-sourced at https://github.com/amd/apml_modules.git

For detailed usage information, use -h or --help flag:
```

$ ./apml_tool -h

================================= APML System Management Interface ====================================
Usage: ./apml_tool [soc_num] [Option<s> / [--help] [module-name]
Where:  soc_num : socket number 0 or 1
Description:
./apml_tool -v                          - Displays tool version
./apml_tool [SOC_NUM] --showdependency  - Displays module dependency
./apml_tool --help <MODULE>             - Displays help on the options for the specified module
./apml_tool <option/s>                  - Runs the specified option/s.
Usage: ./apml_tool [soc_num] [Option] params

        MODULES:
        1. mailbox
        2. sbrmi
        3. sbtsi
        4. reg-access
        5. cpuid
        6. recovery

========================================== End of APML SMI ============================================

$ ./apml_tool -v

```
================================= APML System Management Interface ====================================

APML_tool version : X.Y.Z

========================================== End of APML SMI ============================================

```

Below is a sample usage to get the individual library functionality API's over I2C.
User can pass arguments either any of the ways "./apml_tool [socket_num] -p" or "./apml_tool [socket_num] --showpower"
```
	1. $ ./apml_tool 0 -p

		================================= APML System Management Interface ====================================

		---------------------------------------------
		| Power (Watts)          | 65.029           |
		| PowerLimit (Watts)     | 210.000          |
		| PowerLimitMax (Watts)  | 400.000          |
		---------------------------------------------

		========================================== End of APML SMI ============================================

	2. $ ./apml_tool 1 --setpowerlimit 200000

		================================= APML System Management Interface ====================================


		Set power_limit :          200.000 Watts successfully

		========================================== End of APML SMI ============================================

	3. $ ./apml_tool 0 --showtsiregisters

		================================= APML System Management Interface ====================================
		----------------------------------------------------------------------------------

		                 *** SB-TSI REGISTER SUMMARY ***
		-----------------------------------------------------------------------------------
		 FUNCTION/Reg Name      | Reg offset    | Hexa(0x)      | Value [Units]
		-------------------------------------------------------------------------------------------
		_PROCTEMP               |               |               | 55.125 °C
		        PROC_INT        | 0x1           | 0x37          | 55 °C
		        PROC_DEC        | 0x10          | 0x1           | 0.125 °C
		_STATUS                 | 0x2           |               |
		        PROC Temp Alert |               |               | PROC No Temp Alert
		        Mem Temp Alert  |               |               | HBM High Temp Alert
		_CONFIG                 | 0x3           |               |
		        ALERT_L pin     |               |               | Enabled
		        Runstop         |               |               | Comparison Enabled
		        Atomic Rd order |               |               | Integer latches Decimal
		_TSI_UPDATERATE         | 0x4           |               | 16.000 Hz
		_HIGH_THRESHOLD_TEMP    |               |               | 70.000 °C
		        HIGH_INT        | 0x7           | 0x46          | 70 °C
		        HIGH_DEC        | 0x13          | 0x0           | 0.000 °C
		_LOW_THRESHOLD_TEMP     |               |               | 0.000 °C
		        LOW_INT         | 0x8           | 0x0           | 0 °C
		        LOW_DEC         | 0x14          | 0x0           | 0.000 °C
		_HBM_HIGH_THRESHOLD_TEMP|               |               | 0.000 °C
		        HIGH_INT        | 0x40          | 0x0           | 0 °C
		        HIGH_DEC        | 0x44          | 0x0           | 0.000 °C
		_HBM_LOW_THRESHOLD_TEMP |               |               | 0.000 °C
		        LOW_INT         | 0x48          | 0x0           | 0 °C
		        LOW_DEC         | 0x4c          | 0x0           | 0.000 °C
		_HBM_MAX_TEMP           |               |               | 49.000 °C
		        MAX_INT         | 0x50          | 0x31          | 49 °C
		        MAX_DEC         | 0x54          | 0x0           | 0.000 °C
		_HBM_TEMP               |               |               | 47.000 °C
		        HBM_INT         | 0x5c          | 0x2f          | 47 °C
		        HBM_DEC         | 0x60          | 0x0           | 0.000 °C
		_TEMP_OFFSET            |               |               | 0.000 °C
		        OFF_INT         | 0x11          | 0x0           | 0 °C
		        OFF_DEC         | 0x12          | 0x0           | 0.000 °C
		_THRESHOLD_SAMPLE       | 0x32          |               |
		        PROC Alert TH   |               |               | 1
		        HBM Alert TH    |               |               | 1
		_TSI_ALERT_CONFIG       | 0xbf          |               |
		        PROC Alert CFG  |               |               | Enabled
		        HBM Alert CFG   |               |               | Disabled
		_TSI_MANUFACTURE_ID     | 0xfe          |               | 0
		_TSI_REVISION           | 0xff          |               | 0x4
		-----------------------------------------------------------------------------------

		========================================== End of APML SMI ============================================
```
