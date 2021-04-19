#include "usbdbg.h"
#include "vr_fw.h"
#include "nic_mctp.h"

UsbDbgComponent usbdbg("ocpdbg", "mcu", "F0CE", 8, 0x60, false);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 8, 0x60, 0x02);  // target ID of bootloader = 0x02

VrComponent vr_p0v8_vdd0("vr", "p0v8_vdd0", "P0V8_VDD0");
VrComponent vr_p0v8_vdd1("vr", "p0v8_vdd1", "P0V8_VDD1");
VrComponent vr_p0v8_vdd2("vr", "p0v8_vdd2", "P0V8_VDD2");
VrComponent vr_p0v8_vdd3("vr", "p0v8_vdd3", "P0V8_VDD3");
VrComponent vr_p0v8_avd0("vr", "p0v8_avd_pcie0", "P0V8_AVD_PCIE0");
VrComponent vr_p0v8_avd1("vr", "p0v8_avd_pcie1", "P0V8_AVD_PCIE1");
VrComponent vr_p0v8_avd2("vr", "p0v8_avd_pcie2", "P0V8_AVD_PCIE2");
VrComponent vr_p0v8_avd3("vr", "p0v8_avd_pcie3", "P0V8_AVD_PCIE3");
MCTPOverSMBusNicComponent nic0("nic", "nic0", "nic0_fw_ver", 0x0, 1);
MCTPOverSMBusNicComponent nic1("nic", "nic1", "nic1_fw_ver", 0x0, 9);
MCTPOverSMBusNicComponent nic2("nic", "nic2", "nic2_fw_ver", 0x0, 2);
MCTPOverSMBusNicComponent nic3("nic", "nic3", "nic3_fw_ver", 0x0, 10);
MCTPOverSMBusNicComponent nic4("nic", "nic4", "nic4_fw_ver", 0x0, 4);
MCTPOverSMBusNicComponent nic5("nic", "nic5", "nic5_fw_ver", 0x0, 11);
MCTPOverSMBusNicComponent nic6("nic", "nic6", "nic6_fw_ver", 0x0, 7);
MCTPOverSMBusNicComponent nic7("nic", "nic7", "nic7_fw_ver", 0x0, 13);
