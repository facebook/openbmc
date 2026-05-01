## Overview

Anacapa is Meta's rack-scale AMD AI POD compute tray. Each tray pairs
2x AMD EPYC "Venice" SP7 host CPUs with 4x AMD Instinct MI455X GPUs
(HBM4 OAM modules). An Anacapa POD aggregates 18 trays (72 GPUs)
behind a Broadcom Tomahawk 6 (TH6) scale-up fabric distributed across
6 switch trays (12x TH6 ASICs total). The Aspeed AST2600 BMC runs LF
OpenBMC on a DC-SCM 2.1 module and manages the host CPUs via APML/I3C.
GPU and accelerator-domain management is handled by an on-tray AMC
(Accelerator Management Controller) that the BMC talks to over PLDM.

The POD is liquid-cooled with hybrid AALCv2 / Facility Liquid Cooling
(FLC) support and managed at the rack level by an RMCv2 (Rack
Management Controller). Each tray includes BBU and CBU (Capacitor
Backup Unit) support for ride-through during power events.

### Key Components
- 2x AMD EPYC "Venice" SP7 CPUs with APML/I3C interface
- 4x AMD Instinct MI455X GPUs (HBM4 OAM)
- 16x MRDIMMs per tray
- 5x E1.S NVMe drives per tray
- 1x OCP3.0 front-end NIC + 4x OCP3.0 back-end NICs per tray
- DC-SCM 2.1 carrying the Aspeed AST2600 BMC
- AMC (Accelerator Management Controller) for GPU domain, BMC<->AMC PLDM
- Lattice XO5 CPLD
- BBU + CBU (Capacitor Backup Unit) for power ride-through
- POD-level: 6x switch trays / 12x Broadcom TH6 ASICs (scale-up fabric)
- Hybrid liquid cooling: AALCv2 + Facility Liquid Cooling (FLC);
  rack-level management via RMCv2

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
