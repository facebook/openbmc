## Overview

Yosemite-v5 (codename "Big Dome") is a 3-blade chassis. Each blade is an
independent single-host server with its own DC-SCM (AST2700-based BMC), its
own BSM, its own NIC, and its own boot/data NVMe E1.S drives. Unlike the
prior Yosemite-v3/v4 generations, there is no shared chassis BMC and no
Bridge-IC; the per-blade BMC manages its host CPU and the local Wailua
Falls CXL expansion board directly. The chassis-level Medusa PDB and fan
control board are shared mechanical infrastructure across the 3 blades.

Note: "Yosemite-v5" is a platform family. This spec covers the AMD Venice
"Big Dome" (T2) variant; a separate ARM Phoenix "T11" variant exists under
the same family name and is not covered here.

### Key Components
- AMD Venice EPYC CPUs with APML interface (dual-socket per blade, 256 cores)
- Single-host blade architecture (3 independent blades per chassis)
- DC-SCM v2 carrying the per-blade BMC
- 16x DDR5 DIMMs per blade (64GB or 96GB), 1 DPC across 16 channels
- 12x CXL DIMMs per blade via Wailua Falls expansion board (3 NUMA nodes total)
- 1x boot E1.S 25mm PCIe Gen5 NVMe + 2x data E1.S 25mm PCIe Gen5 NVMe per blade
- 200G OCP3.0 TSFF NIC, single-host, PCIe Gen5 x16 (per blade)
- BSM (BMC Storage Module) per blade
- MAX31790 fan controllers
- Medusa PDB with MP5998 power monitors and IO expanders (chassis-level)
- Debug card support
- Aspeed AST2700 BMC (production target; AST2600 used as EVT contingency)

### BMC Software

The BMC will run the
[Linux Foundation OpenBMC](https://github.com/openbmc/openbmc) software. The
software is developed and delivered upstream first and then integrated into the
[Facebook OpenBMC](https://github.com/facebook/openbmc) tree after it has been
accepted upstream.

The primary remote system management interface (API) for the BMC is Redfish.
Command-line APIs may be available, via SSH, for development and debug purposes
but it is not expected that any command-line APIs are used in production. As
such, all end-to-end feature testing should be based on Redfish and automated
using the upstream
[openbmc-test-automation](https://github.com/openbmc/openbmc-test-automation)
repository.
