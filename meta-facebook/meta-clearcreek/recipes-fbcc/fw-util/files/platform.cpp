#include "usbdbg.h"
#include "vr_fw.h"
#include "nic_mctp.h"
#include <openbmc/pal.h>
#include "switch.h"

UsbDbgComponent usbdbg("ocpdbg", "mcu", "F0CE", 8, 0x60, false);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 8, 0x60, 0x02);  // target ID of bootloader = 0x02

MCTPOverSMBusNicComponent nic0("nic", "nic0", "nic0_fw_ver", 0x0, 1);
MCTPOverSMBusNicComponent nic1("nic", "nic1", "nic1_fw_ver", 0x0, 9);
MCTPOverSMBusNicComponent nic2("nic", "nic2", "nic2_fw_ver", 0x0, 2);
MCTPOverSMBusNicComponent nic3("nic", "nic3", "nic3_fw_ver", 0x0, 10);
MCTPOverSMBusNicComponent nic4("nic", "nic4", "nic4_fw_ver", 0x0, 4);
MCTPOverSMBusNicComponent nic5("nic", "nic5", "nic5_fw_ver", 0x0, 11);
MCTPOverSMBusNicComponent nic6("nic", "nic6", "nic6_fw_ver", 0x0, 7);
MCTPOverSMBusNicComponent nic7("nic", "nic7", "nic7_fw_ver", 0x0, 13);

class ClassConfig {
  public:
    ClassConfig() {
       uint8_t board_type = 0;
       pal_get_platform_id(&board_type);

        if((board_type & 0x07) == 0x3) {
          static VrComponent vr_p0v9_vdd0("vr", "p0v9_vdd_pcie0", "VR_P0V9_VDD0");
          static VrComponent vr_p0v9_vdd1("vr", "p0v9_vdd_pcie1", "VR_P0V9_VDD1");
          static VrComponent vr_p0v9_vdd2("vr", "p0v9_vdd_pcie2", "VR_P0V9_VDD2");
          static VrComponent vr_p0v9_vdd3("vr", "p0v9_vdd_pcie3", "VR_P0V9_VDD3");
          static VrComponent vr_p1v8_avd0("vr", "p1v8_avd0_1", "VR_P1V8_AVD0_1");
          static VrComponent vr_p1v8_avd1("vr", "p1v8_avd2_3", "VR_P1V8_AVD2_3");

          static PAXComponent pex0_flash("mb", "pex0-flash", 0, "SEL_FLASH_PAX0");
          static PAXComponent pex1_flash("mb", "pex1-flash", 1, "SEL_FLASH_PAX1");
          static PAXComponent pex2_flash("mb", "pex2-flash", 2, "SEL_FLASH_PAX2");
          static PAXComponent pex3_flash("mb", "pex3-flash", 3, "SEL_FLASH_PAX3");
        } else {
          static VrComponent vr_p0v8_vdd0("vr", "p0v8_vdd0", "P0V8_VDD0");
          static VrComponent vr_p0v8_vdd1("vr", "p0v8_vdd1", "P0V8_VDD1");
          static VrComponent vr_p0v8_vdd2("vr", "p0v8_vdd2", "P0V8_VDD2");
          static VrComponent vr_p0v8_vdd3("vr", "p0v8_vdd3", "P0V8_VDD3");
          static VrComponent vr_p0v8_avd0("vr", "p0v8_avd_pcie0", "P0V8_AVD_PCIE0");
          static VrComponent vr_p0v8_avd1("vr", "p0v8_avd_pcie1", "P0V8_AVD_PCIE1");
          static VrComponent vr_p0v8_avd2("vr", "p0v8_avd_pcie2", "P0V8_AVD_PCIE2");
          static VrComponent vr_p0v8_avd3("vr", "p0v8_avd_pcie3", "P0V8_AVD_PCIE3");

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
        }
    }
};

ClassConfig platform_config;
