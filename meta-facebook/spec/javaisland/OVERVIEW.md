## Overview

Java Island is a chassis hosting four (4) slots based on the Yosemite v3.5
(YV3.5) program.  Within the chassis is a baseboard containing AST2620 as BMC
to provide system management for the entire Java Island unit. Each slot
contains a Bridge IC (BIC) which is used to manage the individual slots. The
motherboard is all a FRU with the CPU and LPDDR memory soldered to it. Java
Island will use PLDM over MCTP over I3C as the communication interface between
the BMC and BIC.

### BMC Software

The Java Island BMC is planned to run the Facebook OpenBMC software.

The Facebook OpenBMC code is based on the fby35 (Yosemite v3.5) codebase and
is kept in the meta-fby35/meta-javaisland meta-layer in the FB OpenBMC source
tree.

openBMC software will use the same utilities and checks as in prior platforms
such as Halfdome and Craterlake.
