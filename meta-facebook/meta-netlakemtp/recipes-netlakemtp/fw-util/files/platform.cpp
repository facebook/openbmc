#include "bmc_cpld.h"
#include "vr_fw.h"
#include <syslog.h>
#include <facebook/netlakemtp_common.h>
#include "nic.h"
#include <openbmc/cpld.h>

BmcCpldComponent  cpld_server("server", "cpld", MAX10_10M08, 3, 0x40);

NicComponent  nic("nic", "nic");

VrComponent vr_1v05_stby("server", "vr_1v05_stby", "VR_1V05_STBY");
VrComponent vr_vnn_pch("server", "vr_vnn_pch", "VR_VNN_PCH");
VrComponent vr_vccin("server", "vr_vccin", "VR_VCCIN/VR_1V8_STBY");
VrComponent vr_vccna_cpu("server", "vr_vccna_cpu", "VR_VCCANA_CPU");
VrComponent vr_vddq("server", "vr_vddq", "VR_VDDQ");

