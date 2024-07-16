## Overview

Minerva is the chassis housing compute and network cards which are independently
managed by their respective BMCs. The Minerva chassis contains a chassis
manager card responsible for liquid cooling leakage management and potentially
additional debug capabilities to the various compute/network cards in the
Minerva chassis.

For all intensive purposes, when documentation/code specifies a "minerva BMC",
it is explicitly talking about the BMC in the chassis-manager of the Minerva
chassis. This is not to be confused with the "Harma BMC" which provides the
main (disaggregated) management of the compute cards.

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
