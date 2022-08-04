#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openbmc/pal.h>
#include <libpldm/pldm.h>
#include <libpldm/platform.h>
#include <libpldm-oem/pldm.h>
#include "raa_gen3.h"

#define MB_VR_BUS_ID   (20)
#define SWB_VR_BUS_ID  (3)

enum {
  VR_MB_CPU0_VCORE0   = 0,
  VR_MB_CPU0_VCORE1   = 1,
  VR_MB_CPU0_PVDD11   = 2,
  VR_MB_CPU1_VCORE0   = 3,
  VR_MB_CPU1_VCORE1   = 4,
  VR_MB_CPU1_PVDD11   = 5,
  VR_SWB_PXE0_VCC     = 6,
  VR_SWB_PXE1_VCC     = 7,
};

enum {
  ADDR_SWB_VR_PXE0 = 0xC0,
  ADDR_CPU0_VCORE0 = 0xC2,
  ADDR_CPU0_VCORE1 = 0xC4,
  ADDR_SWB_VR_PXE1 = 0xC4,
  ADDR_CPU0_PVDD11 = 0xC6,
  ADDR_CPU1_VCORE0 = 0xE4,
  ADDR_CPU1_VCORE1 = 0xE8,
  ADDR_CPU1_PVDD11 = 0xEA,
};

int swb_vr_map_id(uint8_t addr, uint8_t* id) {
  switch (addr) {
  case ADDR_SWB_VR_PXE0:
    *id = VR0_COMP;
    return 0;

  case ADDR_SWB_VR_PXE1:
    *id = VR1_COMP;
    return 0;
  }
  return -1;
}

static int
vr_pldm_wr(uint8_t bus, uint8_t addr,
           uint8_t *txbuf, uint8_t txlen,
           uint8_t *rxbuf, uint8_t rxlen) {
  uint8_t tbuf[255] = {0};
  uint8_t tlen=0;
  uint8_t vr_id;
  int rc;
  int ret;

  ret = swb_vr_map_id(addr, &vr_id);
  if (ret)
    return -1;

  tbuf[0] = vr_id;
  tbuf[1] = rxlen;
  memcpy(tbuf+2, txbuf, txlen);
  tlen = txlen + 2;

  size_t rlen = 0;
  rc = pldm_oem_ipmi_send_recv(bus, SWB_BIC_EID,
                               NETFN_OEM_1S_REQ, CMD_OEM_1S_BIC_BRIDGE,
                               tbuf, tlen,
                               rxbuf, &rlen);
  return rc;
}



struct vr_ops raa_gen2_3_ops = {
  .get_fw_ver = get_raa_ver,
  .parse_file = raa_parse_file,
  .validate_file = NULL,
  .fw_update = raa_fw_update,
  .fw_verify = NULL,
};

struct vr_info fbgt_vr_list[] = {
  [VR_MB_CPU0_VCORE0] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU0_VCORE0,
    .dev_name = "VR_CPU0_VCORE0/SOC",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU0_VCORE1] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU0_VCORE1,
    .dev_name = "VR_CPU0_VCORE1/PVDDIO",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU0_PVDD11] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU0_PVDD11,
    .dev_name = "VR_CPU0_PVDD11",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU1_VCORE0] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU1_VCORE0,
    .dev_name = "VR_CPU1_VCORE0/SOC",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU1_VCORE1] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU1_VCORE1,
    .dev_name = "VR_CPU1_VCORE1/PVDDIO",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU1_PVDD11] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU1_PVDD11,
    .dev_name = "VR_CPU1_PVDD11",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_SWB_PXE0_VCC] = {
    .bus = SWB_VR_BUS_ID,
    .addr = ADDR_SWB_VR_PXE0,
    .dev_name = "VR_PEX0_VCC",
    .ops = &raa_gen2_3_ops,
    .private_data = "swb",
    .xfer = vr_pldm_wr,
  },
  [VR_SWB_PXE1_VCC] = {
    .bus = SWB_VR_BUS_ID,
    .addr = ADDR_SWB_VR_PXE1,
    .dev_name = "VR_PEX1_VCC",
    .ops = &raa_gen2_3_ops,
    .private_data = "swb",
    .xfer = vr_pldm_wr,
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
