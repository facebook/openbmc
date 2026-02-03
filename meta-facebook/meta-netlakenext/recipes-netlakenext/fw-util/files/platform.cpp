#include "bmc_cpld.h"
#include "vr_fw.h"
#include <syslog.h>
#include <facebook/netlakenext_common.h>
#include "nic.h"

BmcCpldComponent  cpld_server("server", "cpld", "LFMXO5-25", "3", "0x40");

NicComponent  nic("nic", "nic");

VrComponent vr_vddcr("server", "vr_vddcr", "VR_VDDCR/VR_VDDCR_SOC");
VrComponent vr_vdd_misc("server", "vr_vdd_misc", "VR_VDD_MISC");

