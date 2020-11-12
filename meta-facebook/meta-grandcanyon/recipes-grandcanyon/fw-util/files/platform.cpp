#include <openbmc/pal.h>
#include <facebook/fbgc_common.h>
#include "nic_ext.h"
#include "bmc_fpga.h"

// Register NIC
NicExtComponent nic_fw("nic", "nic", "nic_fw_ver", FRU_NIC, 0x00);
BmcFpgaComponent fpga_fw("uic", "fpga", MAX10_10M25, I2C_UIC_FPGA_BUS, 0x40);
