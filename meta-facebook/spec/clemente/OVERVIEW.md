## Overview

Clemente is Meta's NVIDIA GB300 NVL72 compute tray. Each tray carries
"Bianca" GB300 modules with 2x Grace CPUs and 4x Blackwell B300/GB110
GPUs (PCI ID `10de:31c2`). An NVL72 pod is built from 18 Clemente trays
plus 9 NVSwitch trays in a single IT rack (18 trays x 4 GPUs = 72 GPUs).
Clemente is the single-rack NVL72 variant of GB300 (vs. the spread-rack
Catalina). The Aspeed AST2600 BMC runs LF OpenBMC and communicates with
the NVIDIA HMC over Redfish for GPU/CPU sensor polling, event log
forwarding, firmware inventory, and firmware updates. A redfish-client
daemon bridges the BMC to the HMC.

The HMC-polled sensor list and event-forwarding requirements common to
all NVIDIA HMC compute trays live in
[meta-facebook/spec/common/nvidia/features/nvidia/](../common/nvidia/features/nvidia/) — see
`hmc-sensors.md` and `hmc-events.md` there. Clemente has 4 GPUs per tray
(the `<N>` placeholder in those docs ranges over `0..3`).

The pod is managed at the rack level by an RMC (Rack Management
Controller; "Ventura" RMC v1+) and powered by an HPRv3 power rack.
Clemente supports both AALCv1.5 (Air-Assisted Liquid Cooling, 5-rack
pod layout) and FLC (Facility Liquid Cooling, 2-rack pod layout)
deployments.

### Key Components
- 2x NVIDIA "Bianca" GB300 modules per tray
  - 2x Grace CPUs (ARM Neoverse V2 aarch64, 72 cores each)
  - 4x Blackwell B300/GB110 GPUs (~277 GiB HBM3e per GPU, 1400W max)
- 960 GB LPDDR5X soldered to the Grace modules + 4x ~283 GB HBM3e
- NVLink5 GPU fabric, 956 GB/s per GPU into NVSwitch trays
- 34 NUMA nodes per tray
- 5x E1.S NVMe (~30 TB) for boot/data
- Front-end NIC: 2x ConnectX-7 (`15b3:1021`), into Minipack 3 FE
- Back-end NIC: 8x ConnectX-8 (`15b3:2100`), into NSF or DSF backend
- Disaggregated DDI: 4x ConnectX-8 (`15b3:2100`)
- Aspeed AST2600 BMC running LF OpenBMC
- NVIDIA HMC for GPU/CPU management (Redfish to BMC via redfish-client)
- Hybrid liquid cooling (AALCv1.5 or FLC); rack-level management via
  Ventura RMC v1+; HPRv3 power rack

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
