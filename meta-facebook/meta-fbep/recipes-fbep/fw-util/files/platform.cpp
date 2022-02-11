#include "usbdbg.h"
#include "vr_fw.h"

UsbDbgComponent usbdbg("ocpdbg", "mcu", "JG7", 13, 0x60, false);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 13, 0x60, 0x02);  // target ID of bootloader = 0x02

VrComponent vr_p0v8_vdd0("vr", "p0v8_vdd0", "VR_P0V8_VDD0");
VrComponent vr_p0v8_vdd1("vr", "p0v8_vdd1", "VR_P0V8_VDD1");
VrComponent vr_p0v8_vdd2("vr", "p0v8_vdd2", "VR_P0V8_VDD2");
VrComponent vr_p0v8_vdd3("vr", "p0v8_vdd3", "VR_P0V8_VDD3");
VrComponent vr_p1v0_avd0("vr", "p1v0_avd0", "VR_P1V0_AVD0");
VrComponent vr_p1v0_avd1("vr", "p1v0_avd1", "VR_P1V0_AVD1");
VrComponent vr_p1v0_avd2("vr", "p1v0_avd2", "VR_P1V0_AVD2");
VrComponent vr_p1v0_avd3("vr", "p1v0_avd3", "VR_P1V0_AVD3");
