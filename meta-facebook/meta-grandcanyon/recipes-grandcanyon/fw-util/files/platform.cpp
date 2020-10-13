#include <openbmc/pal.h>
#include "nic_ext.h"


// Register NIC
NicExtComponent nic_fw("nic", "nic", "nic_fw_ver", FRU_NIC, 0x00);
