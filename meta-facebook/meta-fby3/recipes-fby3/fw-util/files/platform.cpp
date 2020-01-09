#include "bic_fw.h"
#include "bic_me.h"
#include "bic_cpld.h"
#include "bic_vr.h"
#include "bmc_cpld.h"
#include "bios.h"
#include "fscd.h"
#include "nic_ext.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>
#include <facebook/fby3_common.h>

// Register BIC FW and BL components
/****************
 *param1 - string fru
 *param2 - string board
 *param3 - string comp
 *param4 - uint8_t slot_id
 *param5 - uint8_t intf
*****************/

/*The BIC of the server board*/
BicFwComponent bicfw1("slot1", "sb", "bic", FRU_SLOT1, NONE_INTF);
BicFwComponent bicfw2("slot2", "sb", "bic", FRU_SLOT2, NONE_INTF);
BicFwComponent bicfw3("slot3", "sb", "bic", FRU_SLOT3, NONE_INTF);
BicFwComponent bicfw4("slot4", "sb", "bic", FRU_SLOT4, NONE_INTF);

BicFwBlComponent bicfwbl1("slot1", "sb", "bicbl", FRU_SLOT1, NONE_INTF);
BicFwBlComponent bicfwbl2("slot2", "sb", "bicbl", FRU_SLOT2, NONE_INTF);
BicFwBlComponent bicfwbl3("slot3", "sb", "bicbl", FRU_SLOT3, NONE_INTF);
BicFwBlComponent bicfwbl4("slot4", "sb", "bicbl", FRU_SLOT4, NONE_INTF);

/*The BIC of the 2OU board*/
BicFwExtComponent bic_2ou_fw1("slot1", "2ou", "bic", FRU_SLOT1, REXP_BIC_INTF);
BicFwExtComponent bic_2ou_fw2("slot2", "2ou", "bic", FRU_SLOT2, REXP_BIC_INTF);
BicFwExtComponent bic_2ou_fw3("slot3", "2ou", "bic", FRU_SLOT3, REXP_BIC_INTF);
BicFwExtComponent bic_2ou_fw4("slot4", "2ou", "bic", FRU_SLOT4, REXP_BIC_INTF);

BicFwBlExtComponent bic_2ou_fwbl1("slot1", "2ou", "bicbl", FRU_SLOT1, REXP_BIC_INTF);
BicFwBlExtComponent bic_2ou_fwbl2("slot2", "2ou", "bicbl", FRU_SLOT2, REXP_BIC_INTF);
BicFwBlExtComponent bic_2ou_fwbl3("slot3", "2ou", "bicbl", FRU_SLOT3, REXP_BIC_INTF);
BicFwBlExtComponent bic_2ou_fwbl4("slot4", "2ou", "bicbl", FRU_SLOT4, REXP_BIC_INTF);

/*The ME of the serverboard*/
MeFwComponent me_fw1("slot1", "sb", "me", FRU_SLOT1, NONE_INTF);
MeFwComponent me_fw2("slot2", "sb", "me", FRU_SLOT2, NONE_INTF);
MeFwComponent me_fw3("slot3", "sb", "me", FRU_SLOT3, NONE_INTF);
MeFwComponent me_fw4("slot4", "sb", "me", FRU_SLOT4, NONE_INTF);

/*The CPLD of the server board*/
CpldComponent cpld_fw1("slot1", "sb", "cpld", FRU_SLOT1, 2, 0x80, NONE_INTF);
CpldComponent cpld_fw2("slot2", "sb", "cpld", FRU_SLOT2, 2, 0x80, NONE_INTF);
CpldComponent cpld_fw3("slot3", "sb", "cpld", FRU_SLOT3, 2, 0x80, NONE_INTF);
CpldComponent cpld_fw4("slot4", "sb", "cpld", FRU_SLOT4, 2, 0x80, NONE_INTF);

/*The CPLD of the 2OU board*/
CpldComponent cpld_2ou_fw1("slot1", "2ou", "cpld", FRU_SLOT1, 0, 0x80, REXP_BIC_INTF);
CpldComponent cpld_2ou_fw2("slot2", "2ou", "cpld", FRU_SLOT2, 0, 0x80, REXP_BIC_INTF);
CpldComponent cpld_2ou_fw3("slot3", "2ou", "cpld", FRU_SLOT3, 0, 0x80, REXP_BIC_INTF);
CpldComponent cpld_2ou_fw4("slot4", "2ou", "cpld", FRU_SLOT4, 0, 0x80, REXP_BIC_INTF);

/*The BIOS of the server board*/
BiosComponent bios_fw1("slot1", "sb", "bios", FRU_SLOT1, NONE_INTF);
BiosComponent bios_fw2("slot2", "sb", "bios", FRU_SLOT2, NONE_INTF);
BiosComponent bios_fw3("slot3", "sb", "bios", FRU_SLOT3, NONE_INTF);
BiosComponent bios_fw4("slot4", "sb", "bios", FRU_SLOT4, NONE_INTF);

/*The fscd of BMC*/
FscdComponent fscd("bmc", "bmc", "fscd");

/*The VR of the server board*/
VrComponent vr_fw1("slot1", "sb", "vr", FRU_SLOT1, NONE_INTF);
VrComponent vr_fw2("slot2", "sb", "vr", FRU_SLOT2, NONE_INTF);
VrComponent vr_fw3("slot3", "sb", "vr", FRU_SLOT3, NONE_INTF);
VrComponent vr_fw4("slot4", "sb", "vr", FRU_SLOT4, NONE_INTF);

NicExtComponent nic_fw("nic", "nic", "nic", 0);

class ClassConfig {
  public:
    ClassConfig() {
      uint8_t bmc_location = 0;
      if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
        printf("Failed to initialize the fw-util\n");
        exit(EXIT_FAILURE);
      }

      //class2
      if ( bmc_location == NIC_BMC ) {
        static BmcCpldComponent cpld_bmc("bmc", "bmc", "cpld", MAX10_10M25, 9, 0x40);
        static BicFwExtComponent bic_bb_fw1("slot1", "bb", "bic", FRU_SLOT1, BB_BIC_INTF);
        static BicFwBlExtComponent bic_bb_fwbl1("slot1", "bb", "bicbl", FRU_SLOT1, BB_BIC_INTF);
      } else {
      //class1
        /*The BIC of the 1OU board*/
        static BicFwExtComponent bic_1ou_fw1("slot1", "1ou", "bic", FRU_SLOT1, FEXP_BIC_INTF);
        static BicFwExtComponent bic_1ou_fw2("slot2", "1ou", "bic", FRU_SLOT2, FEXP_BIC_INTF);
        static BicFwExtComponent bic_1ou_fw3("slot3", "1ou", "bic", FRU_SLOT3, FEXP_BIC_INTF);
        static BicFwExtComponent bic_1ou_fw4("slot4", "1ou", "bic", FRU_SLOT4, FEXP_BIC_INTF);

        static BicFwBlExtComponent bic_1ou_fwbl1("slot1", "1ou", "bicbl", FRU_SLOT1, FEXP_BIC_INTF);
        static BicFwBlExtComponent bic_1ou_fwbl2("slot2", "1ou", "bicbl", FRU_SLOT2, FEXP_BIC_INTF);
        static BicFwBlExtComponent bic_1ou_fwbl3("slot3", "1ou", "bicbl", FRU_SLOT3, FEXP_BIC_INTF);
        static BicFwBlExtComponent bic_1ou_fwbl4("slot4", "1ou", "bicbl", FRU_SLOT4, FEXP_BIC_INTF);

        /*The CPLD of the 1OU board*/
        static CpldComponent cpld_1ou_fw1("slot1", "1ou", "cpld", FRU_SLOT1, 0, 0x80, FEXP_BIC_INTF);
        static CpldComponent cpld_1ou_fw2("slot2", "1ou", "cpld", FRU_SLOT2, 0, 0x80, FEXP_BIC_INTF);
        static CpldComponent cpld_1ou_fw3("slot3", "1ou", "cpld", FRU_SLOT3, 0, 0x80, FEXP_BIC_INTF);
        static CpldComponent cpld_1ou_fw4("slot4", "1ou", "cpld", FRU_SLOT4, 0, 0x80, FEXP_BIC_INTF);

        /*The CPLD of BMC*/
        static BmcCpldComponent cpld_bmc("bmc", "bmc", "cpld", MAX10_10M25, 12, 0x40);
      }
    }
};

ClassConfig platform_config;

#endif
