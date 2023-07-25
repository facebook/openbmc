#include <facebook/fby35_common.h>
#include <facebook/bic.h>
#include <facebook/bic_fwupdate.h>
#include <facebook/bic_ipmi.h>
#include "bic_fw.h"
#include "bic_cpld.h"
#include "bmc_cpld.h"
#include "nic_ext.h"
#include "bic_me.h"
#include "bic_bios.h"
#include "bic_vr.h"
#include "vpdb_vr.h"
#include "bic_cxl.h"
#include "bic_retimer.h"
#include "usbdbg.h"
#include "mp5990.h"
#include "bic_prot.hpp"
#include <openbmc/obmc-i2c.h>
#include <openbmc/kv.hpp>

#define RNS_PDB_ADDR 0xB4

NicExtComponent nic_fw("nic", "nic", "nic_fw_ver", FRU_NIC, 0x00);

//slot1 sb bic/bios
BicFwComponent  bic_fw1("slot1", "bic", "sb", FW_SB_BIC);
BicFwComponent  bic_rcvy_fw1("slot1", "bic_rcvy", "sb", FW_BIC_RCVY);
BiosComponent   bios_fw1("slot1", "bios", FRU_SLOT1, FW_BIOS);

class ClassConfig {
  public:
    ClassConfig() {
      uint8_t bmc_location = 0, prsnt;
      uint8_t hsc_type = HSC_UNKNOWN;
      uint8_t card_type = TYPE_1OU_UNKNOWN;
      uint8_t pdb_bus = 11;

      int config_status;

      if (fby35_common_get_bmc_location(&bmc_location) < 0) {
        printf("Failed to initialize the fw-util\n");
        exit(EXIT_FAILURE);
      }

      if (bmc_location == NIC_BMC) {
        static BmcCpldComponent  cpld_bmc("bmc", "cpld", MAX10_10M04, 9, 0x40);

        //slot1 bb bic/cpld
        static BicFwComponent    bic_bb_fw1("slot1", "bb_bic", "bb", FW_BB_BIC);
        static CpldComponent     cpld_bb_fw1("slot1", "bb_cpld", "bb", FW_BB_CPLD, 0, 0);

        //slot1 2ou bic/cpld
        static BicFwComponent    bic_2ou_fw1("slot1", "2ou_bic", "2ou", FW_2OU_BIC);
        static CpldComponent     cpld_2ou_fw1("slot1", "2ou_cpld", "2ou", FW_2OU_CPLD, 0, 0);
      } else {
        // Register USB Debug Card components
        static UsbDbgComponent   usbdbg("ocpdbg", "mcu", "FBY35", 9, 0x60, false);
        static UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 9, 0x60, 0x02);  // target ID of bootloader = 0x02

        static BmcCpldComponent  cpld_bmc("bmc", "cpld", MAX10_10M04, 12, 0x40);

        if (fby35_common_get_bb_hsc_type(&hsc_type) == 0) {
          if (hsc_type == HSC_MP5990) {
            static MP5990Component hsc_bb("bmc", "hsc", FRU_BMC, 11, 0x40);
          }
        }

        if (is_48v_medusa() && (i2c_detect_device(pdb_bus, RNS_PDB_ADDR >> 1) == 0)) {
          static VpdbVrComponent vpdb_vr("bmc", "vpdb_vr", FW_VPDB_VR);
        }

        //slot1 1ou bic
        static BicFwComponent    bic_1ou_fw1("slot1", "1ou_bic", "1ou", FW_1OU_BIC);
        static BicFwComponent    bic_1ou_rcvy_fw1("slot1", "1ou_bic_rcvy", "1ou", FW_1OU_BIC_RCVY);

        if (fby35_common_is_fru_prsnt(FRU_SLOT1, &prsnt)) {
          prsnt = 0;
        }
        if (prsnt) {
          //slot1 cpld/me/vr
          switch (fby35_common_get_slot_type(FRU_SLOT1)) {
            case SERVER_TYPE_CL: {
              static CpldComponent cpld_fw1(
                  "slot1", "cpld", "sb", FW_CPLD, LCMXO3_4300C, 0x40);
              static MeComponent me_fw1("slot1", "me", FRU_SLOT1);
              static VrComponent vr_vccin_fw1("slot1", "vr_vccin", FW_VR_VCCIN);
              static VrComponent vr_vccd_fw1("slot1", "vr_vccd", FW_VR_VCCD);
              static VrComponent vr_vccinfaon_fw1(
                  "slot1", "vr_vccinfaon", FW_VR_VCCINFAON);
              if (isSbMpsHsc(FRU_SLOT1)) {
                static MP5990Component hsc_fw1(
                    "slot1", "hsc", FRU_SLOT1, 1, 0x16);
              }
              break;
            }
            case SERVER_TYPE_HD: {
              static CpldComponent cpld_fw1(
                  "slot1", "cpld", "sb", FW_CPLD, LCMXO3_4300C, 0x44);
              static VrComponent vr_vddcpu0_fw1(
                  "slot1", "vr_vddcrcpu0", FW_VR_VDDCRCPU0);
              static VrComponent vr_vddcpu1_fw1(
                  "slot1", "vr_vddcrcpu1", FW_VR_VDDCRCPU1);
              static VrComponent vr_vdd11s3_fw1(
                  "slot1", "vr_vdd11s3", FW_VR_VDD11S3);
              if (fby35_common_is_prot_card_prsnt(FRU_SLOT1)) {
                static ProtComponent prot_fw1("slot1", "prot", FW_PROT);
                static BiosComponent bios_fw1_a(
                    "slot1", "bios_spia", FRU_SLOT1, FW_BIOS, true);
                static BiosComponent bios_fw1_b(
                    "slot1", "bios_spib", FRU_SLOT1, FW_BIOS_SPIB, true);
              }
              if (isSbMpsHsc(FRU_SLOT1)) {
                static MP5990Component hsc_fw1(
                    "slot1", "hsc", FRU_SLOT1, 4, 0x40);
              }
              break;
            }
            case SERVER_TYPE_GL: {
              static CpldComponent cpld_fw1(
                  "slot1", "cpld", "sb", FW_CPLD, LCMXO3_4300C, 0x40);
              static VrComponent vr_vccin_fw1(
                  "slot1", "vr_vccin", FW_VR_VCCIN_EHV);
              static VrComponent vr_vccd_fw1("slot1", "vr_vccd", FW_VR_VCCD_HV);
              static VrComponent vr_vccinf_fw1(
                  "slot1", "vr_vccinf", FW_VR_VCCINF);
              if (isSbMpsHsc(FRU_SLOT1)) {
                static MP5990Component hsc_fw1(
                    "slot1", "hsc", FRU_SLOT1, 1, 0x16);
              }
              break;
            }
            default:
              syslog(LOG_WARNING, "%s() Cannot indentify the slot type", __func__);
              static CpldComponent cpld_fw1("slot1", "cpld", "sb", FW_CPLD, LCMXO3_4300C, 0x40);
              break;
          }

          static VrComponent     vr_fw1("slot1", "vr", FW_VR);

          //slot1 1ou vr
          if ((config_status = bic_is_exp_prsnt(FRU_SLOT1)) < 0) {
            config_status = 0;
          }
          if ((config_status & PRESENT_1OU) == PRESENT_1OU) {
            if (bic_get_1ou_type(FRU_SLOT1, &card_type) < 0) {
              card_type = TYPE_1OU_UNKNOWN;
            }
            if (card_type == TYPE_1OU_RAINBOW_FALLS) {
              static VrComponent  vr_1ou_fw1("slot1", "1ou_vr", FW_1OU_VR);
              static VrComponent  vr_1ou_va0v8_fw1("slot1", "1ou_vr_v9asica", FW_1OU_VR_V9_ASICA);
              static VrComponent  vr_1ou_vddqab_fw1("slot1", "1ou_vr_vddqab", FW_1OU_VR_VDDQAB);
              static VrComponent  vr_1ou_vddqcd_fw1("slot1", "1ou_vr_vddqcd", FW_1OU_VR_VDDQCD);
              static CxlComponent cxl_1ou_fw1("slot1", "cxl", "1ou", FW_1OU_CXL);
            } else if (card_type == TYPE_1OU_OLMSTEAD_POINT) {
              static BicFwComponent bic_2ou_fw1("slot1", "2ou_bic", "2ou", FW_2OU_BIC);
              static BicFwComponent bic_3ou_fw1("slot1", "3ou_bic", "3ou", FW_3OU_BIC);
              static BicFwComponent bic_4ou_fw1("slot1", "4ou_bic", "4ou", FW_4OU_BIC);
              static RetimerFwComponent retimer_1ou_fw1("slot1", "1ou_retimer", "1ou", FW_1OU_RETIMER);
              static RetimerFwComponent retimer_3ou_fw1("slot1", "3ou_retimer", "3ou", FW_3OU_RETIMER);
              static BicFwComponent bic_2ou_rcvy_fw1("slot1", "2ou_bic_rcvy", "2ou", FW_2OU_BIC_RCVY);
              static BicFwComponent bic_3ou_rcvy_fw1("slot1", "3ou_bic_rcvy", "3ou", FW_3OU_BIC_RCVY);
              static BicFwComponent bic_4ou_rcvy_fw1("slot1", "4ou_bic_rcvy", "4ou", FW_4OU_BIC_RCVY);
            }
          }
        } else {
          static CpldComponent   cpld_fw1("slot1", "cpld", "sb", FW_CPLD, LCMXO3_4300C, 0x40);
        }
        if (card_type == TYPE_1OU_OLMSTEAD_POINT)
          return;

        //slot2 sb bic/cpld/bios
        static BicFwComponent    bic_fw2("slot2", "bic", "sb", FW_SB_BIC);
        static BicFwComponent    bic_rcvy_fw2("slot2", "bic_rcvy", "sb", FW_BIC_RCVY);
        static CpldComponent     cpld_fw2("slot2", "cpld", "sb", FW_CPLD, LCMXO3_4300C, 0x40);
        static BiosComponent     bios_fw2("slot2", "bios", FRU_SLOT2, FW_BIOS);

        //slot2 1ou bic
        static BicFwComponent    bic_1ou_fw2("slot2", "1ou_bic", "1ou", FW_1OU_BIC);
        static BicFwComponent    bic_1ou_rcvy_fw2("slot2", "1ou_bic_rcvy", "1ou", FW_1OU_BIC_RCVY);

        if (fby35_common_is_fru_prsnt(FRU_SLOT2, &prsnt)) {
          prsnt = 0;
        }
        if (prsnt) {
          if(isSbMpsHsc(FRU_SLOT2)) {
            static MP5990Component hsc_fw2("slot2", "hsc", FRU_SLOT2, 1, 0x16);
          }

          //slot2 me/vr
          static MeComponent     me_fw2("slot2", "me", FRU_SLOT2);
          static VrComponent     vr_vccin_fw2("slot2", "vr_vccin", FW_VR_VCCIN);
          static VrComponent     vr_vccd_fw2("slot2", "vr_vccd", FW_VR_VCCD);
          static VrComponent     vr_vccinfaon_fw2("slot2", "vr_vccinfaon", FW_VR_VCCINFAON);
          static VrComponent     vr_fw2("slot2", "vr", FW_VR);
        }

        //slot3 sb bic/bios
        static BicFwComponent    bic_fw3("slot3", "bic", "sb", FW_SB_BIC);
        static BicFwComponent    bic_rcvy_fw3("slot3", "bic_rcvy", "sb", FW_BIC_RCVY);
        static BiosComponent     bios_fw3("slot3", "bios", FRU_SLOT3, FW_BIOS);

        //slot3 1ou bic
        static BicFwComponent    bic_1ou_fw3("slot3", "1ou_bic", "1ou", FW_1OU_BIC);
        static BicFwComponent    bic_1ou_rcvy_fw3("slot3", "1ou_bic_rcvy", "1ou", FW_1OU_BIC_RCVY);

        if (fby35_common_is_fru_prsnt(FRU_SLOT3, &prsnt)) {
          prsnt = 0;
        }
        if (prsnt) {
          // slot3 cpld/me/vr
          switch (fby35_common_get_slot_type(FRU_SLOT3)) {
            case SERVER_TYPE_CL: {
              static CpldComponent cpld_fw3(
                  "slot3", "cpld", "sb", FW_CPLD, LCMXO3_4300C, 0x40);
              static MeComponent me_fw3("slot3", "me", FRU_SLOT3);
              static VrComponent vr_vccin_fw3("slot3", "vr_vccin", FW_VR_VCCIN);
              static VrComponent vr_vccd_fw3("slot3", "vr_vccd", FW_VR_VCCD);
              static VrComponent vr_vccinfaon_fw3(
                  "slot3", "vr_vccinfaon", FW_VR_VCCINFAON);
              if (isSbMpsHsc(FRU_SLOT3)) {
                static MP5990Component hsc_fw3(
                    "slot3", "hsc", FRU_SLOT3, 1, 0x16);
              }
              break;
            }
            case SERVER_TYPE_HD: {
              static CpldComponent cpld_fw3(
                  "slot3", "cpld", "sb", FW_CPLD, LCMXO3_4300C, 0x44);
              static VrComponent vr_vddcpu0_fw3(
                  "slot3", "vr_vddcrcpu0", FW_VR_VDDCRCPU0);
              static VrComponent vr_vddcpu1_fw3(
                  "slot3", "vr_vddcrcpu1", FW_VR_VDDCRCPU1);
              static VrComponent vr_vdd11s3_fw3(
                  "slot3", "vr_vdd11s3", FW_VR_VDD11S3);
              if (fby35_common_is_prot_card_prsnt(FRU_SLOT3)) {
                static ProtComponent prot_fw3("slot3", "prot", FW_PROT);
                static BiosComponent bios_fw3_a(
                    "slot3", "bios_spia", FRU_SLOT3, FW_BIOS, true);
                static BiosComponent bios_fw3_b(
                    "slot3", "bios_spib", FRU_SLOT3, FW_BIOS_SPIB, true);
              }
              if (isSbMpsHsc(FRU_SLOT3)) {
                static MP5990Component hsc_fw3(
                    "slot3", "hsc", FRU_SLOT3, 4, 0x40);
              }
              break;
            }
            case SERVER_TYPE_GL: {
              static CpldComponent cpld_fw3(
                  "slot3", "cpld", "sb", FW_CPLD, LCMXO3_4300C, 0x40);
              static VrComponent vr_vccin_fw3(
                  "slot3", "vr_vccin", FW_VR_VCCIN_EHV);
              static VrComponent vr_vccd_fw3("slot3", "vr_vccd", FW_VR_VCCD_HV);
              static VrComponent vr_vccinf_fw3(
                  "slot3", "vr_vccinf", FW_VR_VCCINF);
              if (isSbMpsHsc(FRU_SLOT3)) {
                static MP5990Component hsc_fw3(
                    "slot3", "hsc", FRU_SLOT3, 1, 0x16);
              }
              break;
            }
            default:
              syslog(LOG_WARNING, "%s() Cannot indentify the slot type", __func__);
              static CpldComponent cpld_fw3("slot3", "cpld", "sb", FW_CPLD, LCMXO3_4300C, 0x40);
              break;
          }

          static VrComponent     vr_fw3("slot3", "vr", FW_VR);

          //slot3 1ou vr
          if ((config_status = bic_is_exp_prsnt(FRU_SLOT3)) < 0) {
            config_status = 0;
          }
          if ((config_status & PRESENT_1OU) == PRESENT_1OU) {
            if (isRainbowFalls(FRU_SLOT3)) {
              static VrComponent  vr_1ou_fw3("slot3", "1ou_vr", FW_1OU_VR);
              static VrComponent  vr_1ou_va0v8_fw3("slot3", "1ou_vr_v9asica", FW_1OU_VR_V9_ASICA);
              static VrComponent  vr_1ou_vddqab_fw3("slot3", "1ou_vr_vddqab", FW_1OU_VR_VDDQAB);
              static VrComponent  vr_1ou_vddqcd_fw3("slot3", "1ou_vr_vddqcd", FW_1OU_VR_VDDQCD);
              static CxlComponent cxl_1ou_fw3("slot3", "cxl", "1ou", FW_1OU_CXL);
            }
          }
        } else {
          static CpldComponent   cpld_fw3("slot3", "cpld", "sb", FW_CPLD, LCMXO3_4300C, 0x40);
        }

        //slot4 sb bic/cpld/bios
        static BicFwComponent    bic_fw4("slot4", "bic", "sb", FW_SB_BIC);
        static BicFwComponent    bic_rcvy_fw4("slot4", "bic_rcvy", "sb", FW_BIC_RCVY);
        static CpldComponent     cpld_fw4("slot4", "cpld", "sb", FW_CPLD, LCMXO3_4300C, 0x40);
        static BiosComponent     bios_fw4("slot4", "bios", FRU_SLOT4, FW_BIOS);

        //slot4 1ou bic
        static BicFwComponent    bic_1ou_fw4("slot4", "1ou_bic", "1ou", FW_1OU_BIC);
        static BicFwComponent    bic_1ou_rcvy_fw4("slot4", "1ou_bic_rcvy", "1ou", FW_1OU_BIC_RCVY);

        if (fby35_common_is_fru_prsnt(FRU_SLOT4, &prsnt)) {
          prsnt = 0;
        }
        if (prsnt) {
          if(isSbMpsHsc(FRU_SLOT4)) {
            static MP5990Component hsc_fw4("slot4", "hsc", FRU_SLOT4, 1, 0x16);
          }

          //slot4 me/vr
          static MeComponent     me_fw4("slot4", "me", FRU_SLOT4);
          static VrComponent     vr_vccin_fw4("slot4", "vr_vccin", FW_VR_VCCIN);
          static VrComponent     vr_vccd_fw4("slot4", "vr_vccd", FW_VR_VCCD);
          static VrComponent     vr_vccinfaon_fw4("slot4", "vr_vccinfaon", FW_VR_VCCINFAON);
          static VrComponent     vr_fw4("slot4", "vr", FW_VR);
        }
      }
  }

  static bool isRainbowFalls(uint8_t slot_id) {
    int ret;
    uint8_t card_type = TYPE_1OU_UNKNOWN;

    ret = bic_get_1ou_type(slot_id, &card_type);
    if (!ret && card_type == TYPE_1OU_RAINBOW_FALLS) {
      return true;
    }
    return false;
  }

  static bool isSbMpsHsc(uint8_t slot_id) {
    uint8_t board_rev = UNKNOWN_REV;
    uint8_t hsc_type = 0;
    bic_gpio_t gpio = {0};

    if (fby35_common_get_slot_type(slot_id) == SERVER_TYPE_HD) {
      if (!bic_get_gpio(slot_id, &gpio, NONE_INTF)) {
        hsc_type = (BIT_VALUE(gpio, HD_HSC_TYPE_1)) << 1 | BIT_VALUE(gpio, HD_HSC_TYPE_0);
        return hsc_type == HSC_MP5990;
      }
    } else if (fby35_common_get_slot_type(slot_id) == SERVER_TYPE_CL) {
      if (get_board_rev(slot_id, BOARD_ID_SB, &board_rev) == 0) {
        return IS_BOARD_REV_MPS(board_rev);
      }
    }
    return false;
  }

  static bool is_48v_medusa() {
    try {
      auto value = kv::get("medusa_hsc_conf", kv::region::persist);
      if (value == "ltc4282") {
        return false;
      }
    } catch (const std::exception&) {
      syslog(LOG_WARNING, "%s() Cannot get the key medusa_hsc_conf", __func__);
    }

    return true;
  }
};

ClassConfig platform_config;
