## Overview

Yosemite-v4 is a chassis hosting up to eight (8) compute cards. Within the
chassis is a management card containing the BMC, which is connected to a central
board and then further connected to the 8 card slots. Each slot is populated
with a compute card that has a "Bridge-IC" (BIC) that performs management of the
local resources.

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
