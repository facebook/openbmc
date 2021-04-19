#include "bic_fw_ext.h"
#include "bic_cpld_ext.h"
#include "bmc_cpld.h"
#include "nic_ext.h"
#include "bic_me.h"
#include "bic_bios.h"
#include "bic_vr.h"
#include "bic_capsule.h"
#include "bmc_cpld_capsule.h"
#include "bic_pcie_sw.h"
#include "bic_m2_dev.h"
#include <syslog.h>
#include <facebook/fby3_common.h>
#include "usbdbg.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

NicExtComponent nic_fw("nic", "nic", "nic_fw_ver", FRU_NIC, 0x00);
MeComponent     me_fw1("slot1", "me", FRU_SLOT1);

//slot1 2ou bic/bicbl/cpld
BicFwExtComponent     bic_2ou_fw1("slot1", "2ou_bic"  , FRU_SLOT1, "2ou", FW_2OU_BIC);
BicFwExtBlComponent bicbl_2ou_fw1("slot1", "2ou_bicbl", FRU_SLOT1, "2ou", FW_2OU_BIC_BOOTLOADER);

//slot1 sb bic/bicbl/cpld/bios/vr
BicFwExtComponent     bic_fw1("slot1", "bic"  , FRU_SLOT1, "sb", FW_BIC);
BicFwExtBlComponent bicbl_fw1("slot1", "bicbl", FRU_SLOT1, "sb", FW_BIC_BOOTLOADER);
BmcCpldComponent     cpld_fw1("slot1", "cpld", MAX10_10M25, 3 + FRU_SLOT1, 0x40);
BiosComponent        bios_fw1("slot1", "bios" , FRU_SLOT1, FW_BIOS);
VrComponent            vr_fw1("slot1", "vr"   , FRU_SLOT1, FW_VR);

class ClassConfig {
  public:
    ClassConfig() {
      uint8_t bmc_location = 0;
      uint8_t board_type = 0;
      if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
        printf("Failed to initialize the fw-util\n");
        exit(EXIT_FAILURE);
      }

      if ( bmc_location == NIC_BMC ) {
        if (fby3_common_get_2ou_board_type(FRU_SLOT1, &board_type) < 0) {
          syslog(LOG_WARNING, "Failed to get slot1 2ou board type\n");
        }
        static BmcCpldComponent        cpld_bmc("bmc", "cpld", MAX10_10M25, 9, 0x40);

        //slot1 bb bic/bicbl/cpld
        static BicFwExtComponent     bic_bb_fw1("slot1", "bb_bic"  , FRU_SLOT1, "bb", FW_BB_BIC);
        static BicFwExtBlComponent bicbl_bb_fw1("slot1", "bb_bicbl", FRU_SLOT1, "bb", FW_BB_BIC_BOOTLOADER);
        static CpldExtComponent     cpld_bb_fw1("slot1", "bb_cpld" , FRU_SLOT1, "bb", FW_BB_CPLD);

        if (board_type != E1S_BOARD) {
          static PCIESWComponent pciesw_2ou_fw1("slot1", "2ou_pciesw", FRU_SLOT1, "2ou", FW_2OU_PESW);
          static VrExtComponent  vr_2ou_vr_p3v3_1_fw1("slot1", "2ou_vr_stby1", FRU_SLOT1, "2ou", FW_2OU_3V3_VR1);
          static VrExtComponent  vr_2ou_vr_p3v3_2_fw1("slot1", "2ou_vr_stby2", FRU_SLOT1, "2ou", FW_2OU_3V3_VR2);
          static VrExtComponent  vr_2ou_vr_p3v3_3_fw1("slot1", "2ou_vr_stby3", FRU_SLOT1, "2ou", FW_2OU_3V3_VR3);
          static VrExtComponent  vr_2ou_vr_p1v8_fw1("slot1", "2ou_vr_p1v8", FRU_SLOT1, "2ou", FW_2OU_1V8_VR);
          static VrExtComponent  vr_2ou_fw1("slot1", "2ou_vr_pesw", FRU_SLOT1, "2ou", FW_2OU_PESW_VR);
          static M2DevComponent  m2_2ou_dev0("slot1", "2ou_dev0", FRU_SLOT1, "2ou", FW_2OU_M2_DEV0);
          static M2DevComponent  m2_2ou_dev1("slot1", "2ou_dev1", FRU_SLOT1, "2ou", FW_2OU_M2_DEV1);
          static M2DevComponent  m2_2ou_dev2("slot1", "2ou_dev2", FRU_SLOT1, "2ou", FW_2OU_M2_DEV2);
          static M2DevComponent  m2_2ou_dev3("slot1", "2ou_dev3", FRU_SLOT1, "2ou", FW_2OU_M2_DEV3);
          static M2DevComponent  m2_2ou_dev4("slot1", "2ou_dev4", FRU_SLOT1, "2ou", FW_2OU_M2_DEV4);
          static M2DevComponent  m2_2ou_dev5("slot1", "2ou_dev5", FRU_SLOT1, "2ou", FW_2OU_M2_DEV5);
          static M2DevComponent  m2_2ou_dev6("slot1", "2ou_dev6", FRU_SLOT1, "2ou", FW_2OU_M2_DEV6);
          static M2DevComponent  m2_2ou_dev7("slot1", "2ou_dev7", FRU_SLOT1, "2ou", FW_2OU_M2_DEV7);
          static M2DevComponent  m2_2ou_dev8("slot1", "2ou_dev8", FRU_SLOT1, "2ou", FW_2OU_M2_DEV8);
          static M2DevComponent  m2_2ou_dev9("slot1", "2ou_dev9", FRU_SLOT1, "2ou", FW_2OU_M2_DEV9);
          static M2DevComponent  m2_2ou_dev10("slot1", "2ou_dev10", FRU_SLOT1, "2ou", FW_2OU_M2_DEV10);
          static M2DevComponent  m2_2ou_dev11("slot1", "2ou_dev11", FRU_SLOT1, "2ou", FW_2OU_M2_DEV11);
          static CpldExtComponent cpld_2ou_fw1("slot1", "2ou_cpld" , FRU_SLOT1, "2ou", FW_2OU_CPLD);
        }
      } else {
        // Register USB Debug Card components
        static UsbDbgComponent usbdbg("ocpdbg", "mcu", "FBY3", 9, 0x60, false);
        static UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 9, 0x60, 0x02);  // target ID of bootloader = 0x02

        //slot1 1ou bic/bicbl/cpld
        static BicFwExtComponent     bic_1ou_fw1("slot1", "1ou_bic"  , FRU_SLOT1, "1ou", FW_1OU_BIC);
        static BicFwExtBlComponent bicbl_1ou_fw1("slot1", "1ou_bicbl", FRU_SLOT1, "1ou", FW_1OU_BIC_BOOTLOADER);
        static CpldExtComponent     cpld_1ou_fw1("slot1", "1ou_cpld" , FRU_SLOT1, "1ou", FW_1OU_CPLD);

        //slot2 sb bic/bicbl/cpld/bios/vr
        static BicFwExtComponent     bic_fw2("slot2", "bic"  , FRU_SLOT2, "sb", FW_BIC);
        static BicFwExtBlComponent bicbl_fw2("slot2", "bicbl", FRU_SLOT2, "sb", FW_BIC_BOOTLOADER);
        static BmcCpldComponent     cpld_fw2("slot2", "cpld", MAX10_10M25, 3 + FRU_SLOT2, 0x40);
        static BiosComponent        bios_fw2("slot2", "bios" , FRU_SLOT2, FW_BIOS);
        static VrComponent            vr_fw2("slot2", "vr"   , FRU_SLOT2, FW_VR);

        //slot2 1ou bic/bicbl/cpld
        static BicFwExtComponent     bic_1ou_fw2("slot2", "1ou_bic"  , FRU_SLOT2, "1ou", FW_1OU_BIC);
        static BicFwExtBlComponent bicbl_1ou_fw2("slot2", "1ou_bicbl", FRU_SLOT2, "1ou", FW_1OU_BIC_BOOTLOADER);
        static CpldExtComponent     cpld_1ou_fw2("slot2", "1ou_cpld" , FRU_SLOT2, "1ou", FW_1OU_CPLD);
        
        //slot2 2ou bic/bicbl/cpld
        static BicFwExtComponent     bic_2ou_fw2("slot2", "2ou_bic"  , FRU_SLOT2, "2ou", FW_2OU_BIC);
        static BicFwExtBlComponent bicbl_2ou_fw2("slot2", "2ou_bicbl", FRU_SLOT2, "2ou", FW_2OU_BIC_BOOTLOADER);
        static CpldExtComponent     cpld_2ou_fw2("slot2", "2ou_cpld" , FRU_SLOT2, "2ou", FW_2OU_CPLD);

        //slot3 sb bic/bicbl/cpld/bios/vr
        static BicFwExtComponent     bic_fw3("slot3", "bic"  , FRU_SLOT3, "sb", FW_BIC);
        static BicFwExtBlComponent bicbl_fw3("slot3", "bicbl", FRU_SLOT3, "sb", FW_BIC_BOOTLOADER);
        static BmcCpldComponent     cpld_fw3("slot3", "cpld", MAX10_10M25, 3 + FRU_SLOT3, 0x40);
        static BiosComponent        bios_fw3("slot3", "bios" , FRU_SLOT3, FW_BIOS);
        static VrComponent            vr_fw3("slot3", "vr"   , FRU_SLOT3, FW_VR);

        //slot3 1ou bic/bicbl/cpld
        static BicFwExtComponent     bic_1ou_fw3("slot3", "1ou_bic"  , FRU_SLOT3, "1ou", FW_1OU_BIC);
        static BicFwExtBlComponent bicbl_1ou_fw3("slot3", "1ou_bicbl", FRU_SLOT3, "1ou", FW_1OU_BIC_BOOTLOADER);
        static CpldExtComponent     cpld_1ou_fw3("slot3", "1ou_cpld" , FRU_SLOT3, "1ou", FW_1OU_CPLD);

        //slot3 2ou bic/bicbl/cpld
        static BicFwExtComponent     bic_2ou_fw3("slot3", "2ou_bic"  , FRU_SLOT3, "2ou", FW_2OU_BIC);
        static BicFwExtBlComponent bicbl_2ou_fw3("slot3", "2ou_bicbl", FRU_SLOT3, "2ou", FW_2OU_BIC_BOOTLOADER);
        static CpldExtComponent     cpld_2ou_fw3("slot3", "2ou_cpld" , FRU_SLOT3, "2ou", FW_2OU_CPLD);

        //slot4 sb bic/bicbl/cpld/bios/vr
        static BicFwExtComponent     bic_fw4("slot4", "bic"  , FRU_SLOT4, "sb", FW_BIC);
        static BicFwExtBlComponent bicbl_fw4("slot4", "bicbl", FRU_SLOT4, "sb", FW_BIC_BOOTLOADER);
        static BmcCpldComponent     cpld_fw4("slot4", "cpld", MAX10_10M25, 3 + FRU_SLOT4, 0x40);
        static BiosComponent        bios_fw4("slot4", "bios" , FRU_SLOT4, FW_BIOS);
        static VrComponent            vr_fw4("slot4", "vr"   , FRU_SLOT4, FW_VR);

        //slot4 1ou bic/bicbl/cpld
        static BicFwExtComponent     bic_1ou_fw4("slot4", "1ou_bic"  , FRU_SLOT4, "1ou", FW_1OU_BIC);
        static BicFwExtBlComponent bicbl_1ou_fw4("slot4", "1ou_bicbl", FRU_SLOT4, "1ou", FW_1OU_BIC_BOOTLOADER);
        static CpldExtComponent     cpld_1ou_fw4("slot4", "1ou_cpld" , FRU_SLOT4, "1ou", FW_1OU_CPLD);

        //slot4 2ou bic/bicbl/cpld
        static BicFwExtComponent     bic_2ou_fw4("slot4", "2ou_bic"  , FRU_SLOT4, "2ou", FW_2OU_BIC);
        static BicFwExtBlComponent bicbl_2ou_fw4("slot4", "2ou_bicbl", FRU_SLOT4, "2ou", FW_2OU_BIC_BOOTLOADER);
        static CpldExtComponent     cpld_2ou_fw4("slot4", "2ou_cpld" , FRU_SLOT4, "2ou", FW_2OU_CPLD);

        static BmcCpldComponent cpld_bmc("bmc", "cpld", MAX10_10M25, 12, 0x40);
        static MeComponent me_fw2("slot2", "me", FRU_SLOT2);
        static MeComponent me_fw3("slot3", "me", FRU_SLOT3);
        static MeComponent me_fw4("slot4", "me", FRU_SLOT4);

        static CpldExtComponent cpld_2ou_fw1("slot1", "2ou_cpld" , FRU_SLOT1, "2ou", FW_2OU_CPLD);

        if ( (bic_is_m2_exp_prsnt(FRU_SLOT1) & PRESENT_2OU) == PRESENT_2OU ) {
          if ( fby3_common_get_2ou_board_type(FRU_SLOT1, &board_type) < 0) {
            syslog(LOG_WARNING, "Failed to get slot1 2ou board type\n");
          } else if ( board_type == GPV3_MCHP_BOARD || board_type == GPV3_BRCM_BOARD ) {
            static PCIESWComponent pciesw_2ou_fw1("slot1", "2ou_pciesw", FRU_SLOT1, "2ou", FW_2OU_PESW);
            static VrExtComponent  vr_2ou_vr_p3v3_1_fw1("slot1", "2ou_vr_stby1", FRU_SLOT1, "2ou", FW_2OU_3V3_VR1);
            static VrExtComponent  vr_2ou_vr_p3v3_2_fw1("slot1", "2ou_vr_stby2", FRU_SLOT1, "2ou", FW_2OU_3V3_VR2);
            static VrExtComponent  vr_2ou_vr_p3v3_3_fw1("slot1", "2ou_vr_stby3", FRU_SLOT1, "2ou", FW_2OU_3V3_VR3);
            static VrExtComponent  vr_2ou_vr_p1v8_fw1("slot1", "2ou_vr_p1v8", FRU_SLOT1, "2ou", FW_2OU_1V8_VR);
            static VrExtComponent  vr_2ou_fw1("slot1", "2ou_vr_pesw", FRU_SLOT1, "2ou", FW_2OU_PESW_VR);
            static M2DevComponent  m2_2ou_dev0_fw1("slot1", "2ou_dev0", FRU_SLOT1, "2ou", FW_2OU_M2_DEV0);
            static M2DevComponent  m2_2ou_dev1_fw1("slot1", "2ou_dev1", FRU_SLOT1, "2ou", FW_2OU_M2_DEV1);
            static M2DevComponent  m2_2ou_dev2_fw1("slot1", "2ou_dev2", FRU_SLOT1, "2ou", FW_2OU_M2_DEV2);
            static M2DevComponent  m2_2ou_dev3_fw1("slot1", "2ou_dev3", FRU_SLOT1, "2ou", FW_2OU_M2_DEV3);
            static M2DevComponent  m2_2ou_dev4_fw1("slot1", "2ou_dev4", FRU_SLOT1, "2ou", FW_2OU_M2_DEV4);
            static M2DevComponent  m2_2ou_dev5_fw1("slot1", "2ou_dev5", FRU_SLOT1, "2ou", FW_2OU_M2_DEV5);
            static M2DevComponent  m2_2ou_dev6_fw1("slot1", "2ou_dev6", FRU_SLOT1, "2ou", FW_2OU_M2_DEV6);
            static M2DevComponent  m2_2ou_dev7_fw1("slot1", "2ou_dev7", FRU_SLOT1, "2ou", FW_2OU_M2_DEV7);
            static M2DevComponent  m2_2ou_dev8_fw1("slot1", "2ou_dev8", FRU_SLOT1, "2ou", FW_2OU_M2_DEV8);
            static M2DevComponent  m2_2ou_dev9_fw1("slot1", "2ou_dev9", FRU_SLOT1, "2ou", FW_2OU_M2_DEV9);
            static M2DevComponent  m2_2ou_dev10_fw1("slot1", "2ou_dev10", FRU_SLOT1, "2ou", FW_2OU_M2_DEV10);
            static M2DevComponent  m2_2ou_dev11_fw1("slot1", "2ou_dev11", FRU_SLOT1, "2ou", FW_2OU_M2_DEV11);
          }
        }
        if ( (bic_is_m2_exp_prsnt(FRU_SLOT3) & PRESENT_2OU) == PRESENT_2OU ) {
          if ( fby3_common_get_2ou_board_type(FRU_SLOT3, &board_type) < 0) {
            syslog(LOG_WARNING, "Failed to get slot3 2ou board type\n");
          } else if ( board_type == GPV3_MCHP_BOARD || board_type == GPV3_BRCM_BOARD ) {
            static PCIESWComponent pciesw_2ou_fw3("slot3", "2ou_pciesw", FRU_SLOT3, "2ou", FW_2OU_PESW);
            static VrExtComponent  vr_2ou_vr_p3v3_1_fw3("slot3", "2ou_vr_stby1", FRU_SLOT3, "2ou", FW_2OU_3V3_VR1);
            static VrExtComponent  vr_2ou_vr_p3v3_2_fw3("slot3", "2ou_vr_stby2", FRU_SLOT3, "2ou", FW_2OU_3V3_VR2);
            static VrExtComponent  vr_2ou_vr_p3v3_3_fw3("slot3", "2ou_vr_stby3", FRU_SLOT3, "2ou", FW_2OU_3V3_VR3);
            static VrExtComponent  vr_2ou_vr_p1v8_fw3("slot3", "2ou_vr_p1v8", FRU_SLOT3, "2ou", FW_2OU_1V8_VR);
            static VrExtComponent  vr_2ou_fw3("slot3", "2ou_vr_pesw", FRU_SLOT3, "2ou", FW_2OU_PESW_VR);
            static M2DevComponent  m2_2ou_dev0_fw3("slot3", "2ou_dev0", FRU_SLOT3, "2ou", FW_2OU_M2_DEV0);
            static M2DevComponent  m2_2ou_dev1_fw3("slot3", "2ou_dev1", FRU_SLOT3, "2ou", FW_2OU_M2_DEV1);
            static M2DevComponent  m2_2ou_dev2_fw3("slot3", "2ou_dev2", FRU_SLOT3, "2ou", FW_2OU_M2_DEV2);
            static M2DevComponent  m2_2ou_dev3_fw3("slot3", "2ou_dev3", FRU_SLOT3, "2ou", FW_2OU_M2_DEV3);
            static M2DevComponent  m2_2ou_dev4_fw3("slot3", "2ou_dev4", FRU_SLOT3, "2ou", FW_2OU_M2_DEV4);
            static M2DevComponent  m2_2ou_dev5_fw3("slot3", "2ou_dev5", FRU_SLOT3, "2ou", FW_2OU_M2_DEV5);
            static M2DevComponent  m2_2ou_dev6_fw3("slot3", "2ou_dev6", FRU_SLOT3, "2ou", FW_2OU_M2_DEV6);
            static M2DevComponent  m2_2ou_dev7_fw3("slot3", "2ou_dev7", FRU_SLOT3, "2ou", FW_2OU_M2_DEV7);
            static M2DevComponent  m2_2ou_dev8_fw3("slot3", "2ou_dev8", FRU_SLOT3, "2ou", FW_2OU_M2_DEV8);
            static M2DevComponent  m2_2ou_dev9_fw3("slot3", "2ou_dev9", FRU_SLOT3, "2ou", FW_2OU_M2_DEV9);
            static M2DevComponent  m2_2ou_dev10_fw3("slot3", "2ou_dev10", FRU_SLOT3, "2ou", FW_2OU_M2_DEV10);
            static M2DevComponent  m2_2ou_dev11_fw3("slot3", "2ou_dev11", FRU_SLOT3, "2ou", FW_2OU_M2_DEV11);
          }
        }
      }
  }
};

ClassConfig platform_config;
#endif
