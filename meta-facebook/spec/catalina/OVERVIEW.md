## Overview

Catalina is Meta's NVIDIA GB200 NVL72 compute tray. Each tray carries 2x
"Ariel" GB200 modules (each module = 1x Grace CPU + 1x Blackwell B100 GPU,
PCI ID `10de:2941`), giving 2 Grace CPUs and 2 Blackwell GPUs per tray.
The Aspeed AST2600 BMC runs LF OpenBMC and communicates with the NVIDIA
HMC (Hardware Management Controller) over Redfish for GPU/CPU sensor
polling, event log forwarding, firmware inventory, and firmware updates.
A redfish-client daemon bridges the BMC to the HMC.

The HMC-polled sensor list and event-forwarding requirements common to
all NVIDIA HMC compute trays live in
[meta-facebook/spec/common/nvidia/features/nvidia/](../common/nvidia/features/nvidia/) — see
`hmc-sensors.md` and `hmc-events.md` there. Catalina has 2 GPUs per tray
(the `<N>` placeholder in those docs ranges over `0..1`).

An NVL72 pod is built from 18 Catalina compute trays. Catalina is the
"NVL72-spread-across-2-IT-racks" variant of GB200 (18 trays per IT rack
x 2 racks = 36 GPUs/rack, 72 GPUs/pod), supported by NVSwitch trays in a
co-located switch rack and a separate AALC liquid-cooling rack. The pod
is managed at the rack level by an RMC (Rack Management Controller).
ODM partners are Quanta and Ingrasys.

### Key Components
- 2x NVIDIA "Ariel" GB200 modules per tray
  - 2x Grace CPUs (ARM Neoverse V2 aarch64, 72 cores, ~300W each)
  - 2x Blackwell B100 GPUs (~185 GiB HBM3e per GPU)
- 960 GB LPDDR5X soldered to the Grace modules (no DRAM DIMMs)
- NVLink5 GPU fabric, 956 GB/s per GPU into NVSwitch trays
- 18 NUMA nodes per tray
- 2x Samsung E1.S NVMe (~3.6 TB) for boot/data
- Front-end NIC: 2x ConnectX-7 (`15b3:1021`), 1x per Grace, into Wedge400 FE
- Back-end NIC: 4x ConnectX-7 (`15b3:1021`), 2x per Blackwell, into DSF
  backend (4x 400G)
- Aspeed AST2600 BMC running LF OpenBMC
- NVIDIA HMC for GPU/CPU management (Redfish to BMC via redfish-client)
- PDB with 3 Renesas VRs
- SCM, HDD, and PDB CPLDs
- Liquid cooling via co-located AALC rack; rack-level management via RMC

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
