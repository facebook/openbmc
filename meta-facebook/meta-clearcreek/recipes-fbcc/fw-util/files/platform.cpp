#include "usbdbg.h"
#include "vr_fw.h"

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
