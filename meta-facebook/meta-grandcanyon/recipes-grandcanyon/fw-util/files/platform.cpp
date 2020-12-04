#include <openbmc/pal.h>
#include <facebook/fbgc_common.h>
#include "nic_ext.h"
#include "bmc_fpga.h"

#include "bic_vr.h"
#include "bic_bios.h"

// Register NIC
NicExtComponent nic_fw("nic", "nic", "nic_fw_ver", FRU_NIC, 0x00);
BmcFpgaComponent uic_fpga_fw("uic", "fpga", MAX10_10M25, I2C_UIC_FPGA_BUS, 0x40, UIC_FPGA_LOCATION);
BmcFpgaComponent bs_fpga_fw("server", "fpga", MAX10_10M25, I2C_BS_FPGA_BUS, 0x40, BS_FPGA_LOCATION);
BiosComponent bios_fw("server", "bios", FW_BIOS);
VrComponent vr_fw("server", "vr");
