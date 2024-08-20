#include <stdlib.h>
#include "raa_gen3.h"

#define PDB_VR_BUS_ID   (19)

enum {
  PDB_P12V_N1_VR = 0,
  PDB_P12V_N2_VR = 1,
  PDB_P12V_AUX_VR = 2,
  PDB_VR_CNT = 3,
};

//PDB VR Address
enum {
  ADDR_PDB_P12V_N1_VR = 0xC0,
  ADDR_PDB_P12V_N2_VR = 0xC2,
  ADDR_PDB_P12V_AUX_VR = 0xC4,
};

// PDB VR OPS
struct vr_ops raa_gen2_3_ops = {
  .get_fw_ver = get_raa_ver,
  .parse_file = raa_parse_file,
  .validate_file = NULL,
  .fw_update = raa_fw_update,
  .fw_verify = NULL,
};

// PDB VR Support list
struct vr_info pdb_vr_list[] = {
  [PDB_P12V_N1_VR] = {
    .bus = PDB_VR_BUS_ID,
    .addr = ADDR_PDB_P12V_N1_VR,
    .dev_name = "PDB_P12V_N1_VR",
    .ops = &raa_gen2_3_ops,
    .private_data = "pdb",
    .xfer = NULL,
    .sensor_polling_ctrl = NULL,
  },
  [PDB_P12V_N2_VR] = {
    .bus = PDB_VR_BUS_ID,
    .addr = ADDR_PDB_P12V_N2_VR,
    .dev_name = "PDB_P12V_N2_VR",
    .ops = &raa_gen2_3_ops,
    .private_data = "pdb",
    .xfer = NULL,
    .sensor_polling_ctrl = NULL,
  },
  [PDB_P12V_AUX_VR] = {
    .bus = PDB_VR_BUS_ID,
    .addr = ADDR_PDB_P12V_AUX_VR,
    .dev_name = "PDB_P12V_AUX_VR",
    .ops = &raa_gen2_3_ops,
    .private_data = "pdb",
    .xfer = NULL,
    .sensor_polling_ctrl = NULL,
  },
};

int plat_vr_init(void) {
  int ret = 0;

  ret = vr_device_register(pdb_vr_list, PDB_VR_CNT);
  if (ret < 0) {
    vr_device_unregister();
  }

  return ret;
}

void plat_vr_exit(void) {
  if (plat_configs) {
    free(plat_configs);
  }
  return;
}
