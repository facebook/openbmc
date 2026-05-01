## Overview

Santa Barbara is Meta's AMD Venice + MTIA Iris training/inference platform
in the Channel Islands family (config CI_SANTABARBARA_IRIS_T17_VENICE).
The host is a 2-socket AMD EPYC "Venice" SP7 server with ~1024 GB DRAM.
Four MTIA Iris accelerator modules (MTIA-I v1.5) are attached over a
PCIe Gen6 switch board (SWB), each presented to the host over PCIe Gen6
x16. The Aspeed AST2600 BMC runs LF OpenBMC and manages the host CPUs
via APML and the four MTIA modules via PLDM. The chassis is
liquid-cooled.

Each MTIA Iris module is a CoWoS-L stack containing 2x Medha compute
chiplets, 1x Hamsa I/O chiplet, and 2x Owl chiplets, paired with HBM3E
(192 GB or 288 GB per module). Each module exposes 12 NICs.

### Key Components
- 2x AMD EPYC "Venice" SP7 CPUs with APML interface
- 16x DDR5 DIMMs (~1024 GB host RAM)
- 4x MTIA Iris (MTIA-I v1.5) accelerator modules
  - 2x Medha + 1x Hamsa + 2x Owl chiplets per module (CoWoS-L)
  - HBM3E 192 GB or 288 GB per module
  - 12 NICs per module, PCIe Gen6 x16 to host via SWB
- Switch Board (SWB) with PCIe Gen6 switch
- Broadcom 800G OCP host-side NIC; ConnectX-7/8 fabric NICs on modules
- Aspeed AST2600 BMC running LF OpenBMC
- MB CPLD (Lattice)
- Extension boards with INA238 power monitors and MCP9600/TMP175 sensors
- Liquid cooling

### BMC Software

The BMC runs the
[Linux Foundation OpenBMC](https://github.com/openbmc/openbmc) software.
Features are developed and delivered upstream first, then integrated into
the [Facebook OpenBMC](https://github.com/facebook/openbmc) tree after
upstream acceptance.

The primary remote management interface is Redfish. Command-line APIs
may be available via SSH for development and debug, but production
management uses Redfish only. End-to-end feature testing should use the
upstream
[openbmc-test-automation](https://github.com/openbmc/openbmc-test-automation)
repository.
