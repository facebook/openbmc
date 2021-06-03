#include <openbmc/pal.h>
#include <facebook/fbgc_common.h>
#include "nic_ext.h"
#include "bmc_fpga.h"

#include "bic_vr.h"
#include "bic_bios.h"
#include "scc_exp.h"
#include "bic_me.h"
#include "ioc.h"

// Register NIC
NicExtComponent nic_fw("nic", "nic", "nic_fw_ver", FRU_NIC, 0x00);
BmcFpgaComponent uic_fpga_fw("uic", "fpga", MAX10_10M25, I2C_UIC_FPGA_BUS, 0x40, UIC_FPGA_LOCATION);
BmcFpgaComponent bs_fpga_fw("server", "fpga", MAX10_10M25, I2C_BS_FPGA_BUS, 0x40, BS_FPGA_LOCATION);
BiosComponent bios_fw("server", "bios", FW_BIOS);
VrComponent vr_fw("server", "vr");
ExpanderComponent exp_fw("scc", "exp");
// Register the SCC IOC component
IOCComponent scc_ioc_fw("scc", "ioc");
// Register the ME component
MeComponent me_fw("server", "me", FRU_SERVER);

class ClassConfig {
  public:
    ClassConfig() {
      uint8_t chassis_type = 0;
      if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
        printf("Failed to initialize the fw-util because failed to get chassis type\n");
        exit(EXIT_FAILURE);
      }
      if (chassis_type == CHASSIS_TYPE7) {
        // Register the IOCM IOC component
        static IOCComponent iocm_ioc_fw("iocm", "ioc");
      }
    }
};

ClassConfig platform_config;

