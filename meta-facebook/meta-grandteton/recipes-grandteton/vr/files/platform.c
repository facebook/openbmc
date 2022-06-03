#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openbmc/pal.h>
#include "raa_gen3.h"

#define VR_MB_BUS_ID 20
#define VR_SWB_BUS_ID 3

enum {
  VR_MB_CPU0_VCCIN   = 0,
  VR_MB_CPU0_VCCFA   = 1,
  VR_MB_CPU0_VCCD    = 2,
  VR_MB_CPU1_VCCIN   = 3,
  VR_MB_CPU1_VCCFA   = 4,
  VR_MB_CPU1_VCCD    = 5,
  VR_SWB_PXE0_VCC    = 6,
  VR_SWB_PXE1_VCC    = 7,
};

enum {
  ADDR_SWB_VR_PXE0 = 0xC0,
  ADDR_CPU0_VCCIN  = 0xC0,
  ADDR_CPU0_VCCFA  = 0xC2,
  ADDR_SWB_VR_PXE1 = 0xC4,
  ADDR_CPU0_VCCD   = 0xC6,
  ADDR_CPU1_VCCIN  = 0xE4,
  ADDR_CPU1_VCCFA  = 0xE8,
  ADDR_CPU1_VCCD   = 0xEC,
};

struct vr_ops raa_gen2_3_ops = {
  .get_fw_ver = get_raa_ver,
  .parse_file = raa_parse_file,
  .validate_file = NULL,
  .fw_update = raa_fw_update,
  .fw_verify = raa_fw_verify,
};

struct vr_info fbgt_vr_list[] = {
  [VR_MB_CPU0_VCCIN] = {
    .bus = VR_MB_BUS_ID,
    .addr = ADDR_CPU0_VCCIN,
    .dev_name = "VR_CPU0_VCCIN/VCCFA_FIVRA",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU0_VCCFA] = {
    .bus = VR_MB_BUS_ID,
    .addr = ADDR_CPU0_VCCFA,
    .dev_name = "VR_CPU0_VCCFAEHV/FAON",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU0_VCCD] = {
    .bus = VR_MB_BUS_ID,
    .addr = ADDR_CPU0_VCCD,
    .dev_name = "VR_CPU0_VCCD",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU1_VCCIN] = {
    .bus = VR_MB_BUS_ID,
    .addr = ADDR_CPU1_VCCIN,
    .dev_name = "VR_CPU1_VCCIN/VCCFA_FIRVA",
    .ops = &raa_gen2_3_ops,
    .xfer = NULL,
  },
  [VR_MB_CPU1_VCCFA] = {
    .bus = VR_MB_BUS_ID,
    .addr = ADDR_CPU1_VCCFA,
    .dev_name = "VR_CPU1_VCCFAEHV/FAON",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU1_VCCD] = {
    .bus = VR_MB_BUS_ID,
    .addr = ADDR_CPU1_VCCD,
    .dev_name = "VR_CPU1_VCCD",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
};

int plat_vr_init(void) {
  int ret;
  int vr_cnt = sizeof(fbgt_vr_list)/sizeof(fbgt_vr_list[0]);

  ret = vr_device_register(fbgt_vr_list, vr_cnt );
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
