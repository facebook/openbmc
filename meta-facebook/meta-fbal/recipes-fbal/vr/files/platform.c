#include <stdlib.h>
#include <openbmc/pal.h>
#include "tps53688.h"
#include "pxe1110c.h"
#include "xdpe12284c.h"

#define VR_BUS_ID 1

enum {
  VR_PCH_PVNN      = 0,
  VR_CPU0_VCCIN    = 1,
  VR_CPU0_VCCIO    = 2,
  VR_CPU0_VDDQ_ABC = 3,
  VR_CPU0_VDDQ_DEF = 4,
  VR_CPU1_VCCIN    = 5,
  VR_CPU1_VCCIO    = 6,
  VR_CPU1_VDDQ_ABC = 7,
  VR_CPU1_VDDQ_DEF = 8,
};

enum {
  ADDR_PCH_PVNN      = 0x94,
  ADDR_CPU0_VCCIN    = 0xC0,
  ADDR_CPU0_VCCIO    = 0xC4,
  ADDR_CPU0_VDDQ_ABC = 0xCC,
  ADDR_CPU0_VDDQ_DEF = 0xD0,
  ADDR_CPU1_VCCIN    = 0xE0,
  ADDR_CPU1_VCCIO    = 0xE4,
  ADDR_CPU1_VDDQ_ABC = 0xEC,
  ADDR_CPU1_VDDQ_DEF = 0xD8,
};

struct vr_ops pxe1110c_ops = {
  .get_fw_ver = get_pxe_ver,
  .parse_file = pxe_parse_file,
  .validate_file = NULL,
  .fw_update = pxe_fw_update,
  .fw_verify = pxe_fw_verify,
};

struct vr_ops tps53688_ops = {
  .get_fw_ver = get_tps_ver,
  .parse_file = tps_parse_file,
  .validate_file = NULL,
  .fw_update = tps_fw_update,
  .fw_verify = tps_fw_verify,
};

struct vr_ops xdpe12284c_ops = {
  .get_fw_ver = get_xdpe_ver,
  .parse_file = xdpe_parse_file,
  .validate_file = NULL,
  .fw_update = xdpe_fw_update,
  .fw_verify = xdpe_fw_verify,
};

struct vr_info fbal_vr_list[] = {
  [VR_PCH_PVNN] = {
    .bus = VR_BUS_ID,
    .addr = ADDR_PCH_PVNN,
    .dev_name = "VR_PCH_PVNN/P1V05",
    .ops = &pxe1110c_ops,
    .private_data = NULL,
    .xfer = NULL,
  },
  [VR_CPU0_VCCIN] = {
    .bus = VR_BUS_ID,
    .addr = ADDR_CPU0_VCCIN,
    .dev_name = "VR_CPU0_VCCIN/VCCSA",
    .ops = &tps53688_ops,
    .private_data = NULL,
    .xfer = NULL,
  },
  [VR_CPU0_VCCIO] = {
    .bus = VR_BUS_ID,
    .addr = ADDR_CPU0_VCCIO,
    .dev_name = "VR_CPU0_VCCIO",
    .ops = &tps53688_ops,
    .private_data = NULL,
    .xfer = NULL,
  },
  [VR_CPU0_VDDQ_ABC] = {
    .bus = VR_BUS_ID,
    .addr = ADDR_CPU0_VDDQ_ABC,
    .dev_name = "VR_CPU0_VDDQ_ABC",
    .ops = &tps53688_ops,
    .private_data = NULL,
    .xfer = NULL,
  },
  [VR_CPU0_VDDQ_DEF] = {
    .bus = VR_BUS_ID,
    .addr = ADDR_CPU0_VDDQ_DEF,
    .dev_name = "VR_CPU0_VDDQ_DEF",
    .ops = &tps53688_ops,
    .private_data = NULL,
    .xfer = NULL,
  },
  [VR_CPU1_VCCIN] = {
    .bus = VR_BUS_ID,
    .addr = ADDR_CPU1_VCCIN,
    .dev_name = "VR_CPU1_VCCIN/VCCSA",
    .ops = &tps53688_ops,
    .private_data = NULL,
    .xfer = NULL,
  },
  [VR_CPU1_VCCIO] = {
    .bus = VR_BUS_ID,
    .addr = ADDR_CPU1_VCCIO,
    .dev_name = "VR_CPU1_VCCIO",
    .ops = &tps53688_ops,
    .private_data = NULL,
    .xfer = NULL,
  },
  [VR_CPU1_VDDQ_ABC] = {
    .bus = VR_BUS_ID,
    .addr = ADDR_CPU1_VDDQ_ABC,
    .dev_name = "VR_CPU1_VDDQ_ABC",
    .ops = &tps53688_ops,
    .private_data = NULL,
    .xfer = NULL,
  },
  [VR_CPU1_VDDQ_DEF] = {
    .bus = VR_BUS_ID,
    .addr = ADDR_CPU1_VDDQ_DEF,
    .dev_name = "VR_CPU1_VDDQ_DEF",
    .ops = &tps53688_ops,
    .private_data = NULL,
    .xfer = NULL,
  }
};

int plat_vr_init(void) {
  int ret, i;
  int vr_cnt = sizeof(fbal_vr_list)/sizeof(fbal_vr_list[0]);
  uint8_t sku_id = 0xFF;

  if (pal_is_fw_update_ongoing(FRU_MB)) {
    msleep(500);  // wait the reading finish if going to do update
  }

  pal_get_platform_id(&sku_id);
  //DVT SKU_ID[2:1] = 00 (TI), 01 (INFINEON), TODO: 10 (3rd Source)
  if (sku_id & 0x2) {
    for (i = 1; i < vr_cnt; i++) {
      fbal_vr_list[i].ops = &xdpe12284c_ops;
    }
  }

  ret = vr_device_register(fbal_vr_list, vr_cnt );
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
