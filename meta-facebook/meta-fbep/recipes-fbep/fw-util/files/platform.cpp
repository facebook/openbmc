#include "usbdbg.h"
#include "vr_fw.h"
#include <openbmc/pal.h>
#include "switch.h"

UsbDbgComponent usbdbg("ocpdbg", "mcu", "JG7", 13, 0x60, false);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 13, 0x60, 0x02);  // target ID of bootloader = 0x02

class ClassConfig {
  public:
    ClassConfig() {
        if(pal_check_switch_config()) {
            static PAXComponent pex0_flash("mb", "pex0-flash", 0, "SEL_FLASH_PAX0");
            static PAXComponent pex1_flash("mb", "pex1-flash", 1, "SEL_FLASH_PAX1");
            static PAXComponent pex2_flash("mb", "pex2-flash", 2, "SEL_FLASH_PAX2");
            static PAXComponent pex3_flash("mb", "pex3-flash", 3, "SEL_FLASH_PAX3");

            static VrComponent vr_p0v9_vdd0("vr", "p0v9_vdd0", "VR_P0V9_VDD0");
            static VrComponent vr_p0v9_vdd1("vr", "p0v9_vdd1", "VR_P0V9_VDD1");
            static VrComponent vr_p0v9_vdd2("vr", "p0v9_vdd2", "VR_P0V9_VDD2");
            static VrComponent vr_p0v9_vdd3("vr", "p0v9_vdd3", "VR_P0V9_VDD3");
            static VrComponent vr_p1v8_avd0("vr", "p1v8_avd0", "VR_P1V8_AVD0");
            static VrComponent vr_p1v8_avd1("vr", "p1v8_avd1", "VR_P1V8_AVD1");
            static VrComponent vr_p1v8_avd2("vr", "p1v8_avd2", "VR_P1V8_AVD2");
            static VrComponent vr_p1v8_avd3("vr", "p1v8_avd3", "VR_P1V8_AVD3");
        } else {
            static PAXComponent pax0_fw("mb", "pax0-bl2", 0, "");
            static PAXComponent pax0_bl("mb", "pax0-img", 0, "");
            static PAXComponent pax0_cfg("mb", "pax0-cfg", 0, "");
            static PAXComponent pax0_flash("mb", "pax0-flash", 0, "SEL_FLASH_PAX0");
            static PAXComponent pax1_fw("mb", "pax1-bl2", 1, "");
            static PAXComponent pax1_bl("mb", "pax1-img", 1, "");
            static PAXComponent pax1_cfg("mb", "pax1-cfg", 1, "");
            static PAXComponent pax1_flash("mb", "pax1-flash", 1, "SEL_FLASH_PAX1");
            static PAXComponent pax2_fw("mb", "pax2-bl2", 2, "");
            static PAXComponent pax2_bl("mb", "pax2-img", 2, "");
            static PAXComponent pax2_cfg("mb", "pax2-cfg", 2, "");
            static PAXComponent pax2_flash("mb", "pax2-flash", 2, "SEL_FLASH_PAX2");
            static PAXComponent pax3_fw("mb", "pax3-bl2", 3, "");
            static PAXComponent pax3_bl("mb", "pax3-img", 3, "");
            static PAXComponent pax3_cfg("mb", "pax3-cfg", 3, "");
            static PAXComponent pax3_flash("mb", "pax3-flash", 3, "SEL_FLASH_PAX3");

            static VrComponent vr_p0v8_vdd0("vr", "p0v8_vdd0", "VR_P0V8_VDD0");
            static VrComponent vr_p0v8_vdd1("vr", "p0v8_vdd1", "VR_P0V8_VDD1");
            static VrComponent vr_p0v8_vdd2("vr", "p0v8_vdd2", "VR_P0V8_VDD2");
            static VrComponent vr_p0v8_vdd3("vr", "p0v8_vdd3", "VR_P0V8_VDD3");
            static VrComponent vr_p1v0_avd0("vr", "p1v0_avd0", "VR_P1V0_AVD0");
            static VrComponent vr_p1v0_avd1("vr", "p1v0_avd1", "VR_P1V0_AVD1");
            static VrComponent vr_p1v0_avd2("vr", "p1v0_avd2", "VR_P1V0_AVD2");
            static VrComponent vr_p1v0_avd3("vr", "p1v0_avd3", "VR_P1V0_AVD3");
        }
    }
};

ClassConfig platform_config;