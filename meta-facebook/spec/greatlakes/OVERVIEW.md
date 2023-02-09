## Overview

Great Lakes is a chassis hosting four (4) slots in a chassis based on the
Yosemite v3.5 (YV3.5) program.  Within the chassis is a baseboard containing
an AST2620 as a BMC to provide system management for the entire Great Lakes
unit.  Each slot contains a Bridge IC (BIC) which is used to manage the
individual slots.  The plan is to enable MCTP over I3C as the communications
interface between the BMC and BIC.

### BMC Software

The Great Lakes BMC is planned to run the Facebook OpenBMC software.

The Facebook OpenBMC code is based on the fby35 (Yosemite v3.5) codebase
and is kept in the meta-fby35/meta-greatlakes meta-layer in the FB OpenBMC
source tree.

The primary remote system management interface (API) for the Great Lakes BMC
is Redfish.  Command-line APIs will also be available, via SSH for use in
production.  As such, all end-to-end feature testing should be based on
Redfish and automated using the upstream [openbmc-test-automation](https://github.com/openbmc/openbmc-test-automation) repository.
