#include "bmc_cpld.h"
#include <syslog.h>
#include <facebook/netlakemtp_common.h>
#include <openbmc/cpld.h>

BmcCpldComponent  cpld_server("server", "cpld", MAX10_10M08, 3, 0x40);
