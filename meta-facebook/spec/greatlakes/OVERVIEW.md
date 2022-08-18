## Overview

Great Lakes is a chassis hosting four (4) slots in a chassis based on the
Yosemite v3.5 (YV3.5) program.  Within the chassis is a baseboard containing
an AST2620 as a BMC to provide system management for the entire Great Lakes
unit.  Each slot contains a Bridge IC (BIC) which is used to manage the
individual slots.  The plan is to enable MCTP over I3C as the communications
interface between the BMC and BIC.

The current plan is to enable a second source BMC for Great Lakes using the
Nuvoton Arbel BMC.  The support for Nuvoton Arbel is expected approximately
one quarter after the initial support is provided via the AST2620 BMC.

### BMC Software

The Great Lakes BMC is planned to run both the Facebook OpenBMC and the
[Linux Foundation OpenBMC](https://github.com/openbmc/openbmc) software.

The Facebook OpenBMC code is based on the fby35 (Yosemite v3.5) codebase
and is kept in the TBD meta-layer in the FB OpenBMC source tree.

The LF-oBMC software is developed and delivered upstream first and then
integrated into the [Facebook OpenBMC](https://github.com/facebook/openbmc)
tree after it has been accepted upstream.

The primary remote system management interface (API) for the Great Lakes BMC
is Redfish.  Command-line APIs may be available, via SSH, for development and
debug purposes but it is not expected that any command-line APIs are used in
production.  As such, all end-to-end feature testing should be based on
Redfish and automated using the upstream [openbmc-test-automation](https://github.com/openbmc/openbmc-test-automation) repository.
