## Overview

Harma is the compute card for the Minerva chassis containing the BMC which
has explicit connections to the various motherboard components. The BMC
is responsible for thermal, configuration and power management of the
locally connected server CPUs.

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
