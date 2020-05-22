#include "bic_fw_ext.h"
#include "bic_cpld_ext.h"
#include "bmc_cpld.h"
#include "nic_ext.h"
#include "bic_me.h"
#include "bic_bios.h"
#include "bic_vr.h"
#include "bic_capsule.h"
#include "bmc_cpld_capsule.h"
#include <facebook/fby3_common.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

NicExtComponent nic_fw("nic", "nic", "nic_fw_ver", FRU_NIC, 0x00);
MeComponent     me_fw1("slot1", "me", FRU_SLOT1);

//slot1 2ou bic/bicbl/cpld
BicFwExtComponent     bic_2ou_fw1("slot1", "2ou_bic"  , FRU_SLOT1, "2ou", FW_2OU_BIC);
BicFwExtBlComponent bicbl_2ou_fw1("slot1", "2ou_bicbl", FRU_SLOT1, "2ou", FW_2OU_BIC_BOOTLOADER);
CpldExtComponent     cpld_2ou_fw1("slot1", "2ou_cpld" , FRU_SLOT1, "2ou", FW_2OU_CPLD);

//slot1 sb bic/bicbl/cpld/bios/vr
BicFwExtComponent     bic_fw1("slot1", "bic"  , FRU_SLOT1, "sb", FW_BIC);
BicFwExtBlComponent bicbl_fw1("slot1", "bicbl", FRU_SLOT1, "sb", FW_BIC_BOOTLOADER);
BmcCpldComponent     cpld_fw1("slot1", "cpld", MAX10_10M25, 3 + FRU_SLOT1, 0x40);
BiosComponent        bios_fw1("slot1", "bios" , FRU_SLOT1, FW_BIOS);
VrComponent            vr_fw1("slot1", "vr"   , FRU_SLOT1, FW_VR);
CapsuleComponent bios_cap_fw1("slot1", "bios_cap" , FRU_SLOT1, FW_BIOS_CAPSULE);
CapsuleComponent cpld_cap_fw1("slot1", "cpld_cap" , FRU_SLOT1, FW_CPLD_CAPSULE);
CapsuleComponent bios_cap_rcvy_fw1("slot1", "bios_cap_rcvy" , FRU_SLOT1, FW_BIOS_RCVY_CAPSULE);
CapsuleComponent cpld_cap_rcvy_fw1("slot1", "cpld_cap_rcvy" , FRU_SLOT1, FW_CPLD_RCVY_CAPSULE);

class ClassConfig {
  public:
    ClassConfig() {
      uint8_t bmc_location = 0;
      if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
        printf("Failed to initialize the fw-util\n");
        exit(EXIT_FAILURE);
      }

      if ( bmc_location == NIC_BMC ) {
        static BmcCpldComponent        cpld_bmc("bmc", "cpld", MAX10_10M25, 9, 0x40);
        static BmcCpldCapsuleComponent cpld_capsule("bmc", "cpld_cap", MAX10_10M25, 9, 0x40);
        static BmcCpldCapsuleComponent bmc_capsule("bmc", "bmc_cap", 0, 0, 0);
        static BmcCpldCapsuleComponent cpld_capsule_rcvy("bmc", "cpld_cap_rcvy", MAX10_10M25, 9, 0x40);
        static BmcCpldCapsuleComponent bmc_capsule_rcvy("bmc", "bmc_cap_rcvy", 0, 0, 0);

        //slot1 bb bic/bicbl/cpld
        static BicFwExtComponent     bic_bb_fw1("slot1", "bb_bic"  , FRU_SLOT1, "bb", FW_BB_BIC);
        static BicFwExtBlComponent bicbl_bb_fw1("slot1", "bb_bicbl", FRU_SLOT1, "bb", FW_BB_BIC_BOOTLOADER);
        static CpldExtComponent     cpld_bb_fw1("slot1", "bb_cpld" , FRU_SLOT1, "bb", FW_BB_CPLD);
      } else {
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
        static CapsuleComponent bios_cap_fw2("slot2", "bios_cap" , FRU_SLOT2, FW_BIOS_CAPSULE);
        static CapsuleComponent cpld_cap_fw2("slot2", "cpld_cap" , FRU_SLOT2, FW_CPLD_CAPSULE);
        static CapsuleComponent bios_cap_rcvy_fw2("slot2", "bios_cap_rcvy" , FRU_SLOT2, FW_BIOS_RCVY_CAPSULE);
        static CapsuleComponent cpld_cap_rcvy_fw2("slot2", "cpld_cap_rcvy" , FRU_SLOT2, FW_CPLD_RCVY_CAPSULE);

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
        static CapsuleComponent bios_cap_fw3("slot3", "bios_cap" , FRU_SLOT3, FW_BIOS_CAPSULE);
        static CapsuleComponent cpld_cap_fw3("slot3", "cpld_cap" , FRU_SLOT3, FW_CPLD_CAPSULE);
        static CapsuleComponent bios_cap_rcvy_fw3("slot3", "bios_cap_rcvy" , FRU_SLOT3, FW_BIOS_RCVY_CAPSULE);
        static CapsuleComponent cpld_cap_rcvy_fw3("slot3", "cpld_cap_rcvy" , FRU_SLOT3, FW_CPLD_RCVY_CAPSULE);

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
        static CapsuleComponent bios_cap_fw4("slot4", "bios_cap" , FRU_SLOT4, FW_BIOS_CAPSULE);
        static CapsuleComponent cpld_cap_fw4("slot4", "cpld_cap" , FRU_SLOT4, FW_CPLD_CAPSULE);
        static CapsuleComponent bios_cap_rcvy_fw4("slot4", "bios_cap_rcvy" , FRU_SLOT4, FW_BIOS_RCVY_CAPSULE);
        static CapsuleComponent cpld_cap_rcvy_fw4("slot4", "cpld_cap_rcvy" , FRU_SLOT4, FW_CPLD_RCVY_CAPSULE);

        //slot4 1ou bic/bicbl/cpld
        static BicFwExtComponent     bic_1ou_fw4("slot4", "1ou_bic"  , FRU_SLOT4, "1ou", FW_1OU_BIC);
        static BicFwExtBlComponent bicbl_1ou_fw4("slot4", "1ou_bicbl", FRU_SLOT4, "1ou", FW_1OU_BIC_BOOTLOADER);
        static CpldExtComponent     cpld_1ou_fw4("slot4", "1ou_cpld" , FRU_SLOT4, "1ou", FW_1OU_CPLD);

        //slot4 2ou bic/bicbl/cpld
        static BicFwExtComponent     bic_2ou_fw4("slot4", "2ou_bic"  , FRU_SLOT4, "2ou", FW_2OU_BIC);
        static BicFwExtBlComponent bicbl_2ou_fw4("slot4", "2ou_bicbl", FRU_SLOT4, "2ou", FW_2OU_BIC_BOOTLOADER);
        static CpldExtComponent     cpld_2ou_fw4("slot4", "2ou_cpld" , FRU_SLOT4, "2ou", FW_2OU_CPLD);

        static BmcCpldComponent cpld_bmc("bmc", "cpld", MAX10_10M25, 12, 0x40);
        static BmcCpldCapsuleComponent cpld_capsule("bmc", "cpld_cap", MAX10_10M25, 12, 0x40);
        static BmcCpldCapsuleComponent bmc_capsule("bmc", "bmc_cap", 0, 0, 0);
        static BmcCpldCapsuleComponent cpld_capsule_rcvy("bmc", "cpld_cap_rcvy", MAX10_10M25, 12, 0x40);
        static BmcCpldCapsuleComponent bmc_capsule_rcvy("bmc", "bmc_cap_rcvy", 0, 0, 0);
        static MeComponent me_fw2("slot2", "me", FRU_SLOT2);
        static MeComponent me_fw3("slot3", "me", FRU_SLOT3);
        static MeComponent me_fw4("slot4", "me", FRU_SLOT4);
      }
  }
};

ClassConfig platform_config;
#endif
