#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openbmc/pal.h>
#include <openbmc/pal_common.h>
#include <libpldm/pldm.h>
#include <libpldm/platform.h>
#include <libpldm-oem/pldm.h>
#include "raa_gen3.h"
#include "xdpe12284c.h"
#include "xdpe152xx.h"

#define MB_VR_BUS_ID   (20)
#define SWB_VR_BUS_ID  (3)
#define MB_VR_CNT      (6)
#define SWB_VR_CNT     (2)


enum {
  VR_MB_CPU0_VCORE0   = 0,
  VR_MB_CPU0_VCORE1   = 1,
  VR_MB_CPU0_PVDD11   = 2,
  VR_MB_CPU1_VCORE0   = 3,
  VR_MB_CPU1_VCORE1   = 4,
  VR_MB_CPU1_PVDD11   = 5,
  VR_SWB_PEX01_VCC    = 6,
  VR_SWB_PEX23_VCC    = 7,
  VR_CNT,
};

enum {
  ADDR_SWB_VR_PEX01 = 0xC0,
  ADDR_SWB_VR_PEX23 = 0xC4,
};

enum {
  ADDR_CPU0_VCORE0 = 0xC2,
  ADDR_CPU0_VCORE1 = 0xC4,
  ADDR_CPU0_PVDD11 = 0xC6,
  ADDR_CPU1_VCORE0 = 0xE4,
  ADDR_CPU1_VCORE1 = 0xE8,
  ADDR_CPU1_PVDD11 = 0xEA,
};

enum {
  ADDR_INF_CPU0_VCORE0  = 0x9E,
  ADDR_INF_CPU0_VCORE1  = 0x9C,
  ADDR_INF_CPU0_PVDD11  = 0x92,
  ADDR_INF_CPU1_VCORE0  = 0x9A,
  ADDR_INF_CPU1_VCORE1  = 0x98,
  ADDR_INF_CPU1_PVDD11  = 0x94,
};

int swb_vr_map_id(uint8_t addr, uint8_t* id) {
  switch (addr) {
  case ADDR_SWB_VR_PEX01:
    *id = VR0_COMP;
    return 0;

  case ADDR_SWB_VR_PEX23:
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
  int ret;

  ret = swb_vr_map_id(addr, &vr_id);
  if (ret)
    return -1;

  tbuf[0] = vr_id;
  tbuf[1] = rxlen;
  memcpy(tbuf+2, txbuf, txlen);
  tlen = txlen + 2;

  size_t rlen = 0;
  ret = pldm_oem_ipmi_send_recv(bus, SWB_BIC_EID,
                               NETFN_OEM_1S_REQ, CMD_OEM_1S_BIC_BRIDGE,
                               tbuf, tlen,
                               rxbuf, &rlen);
  return ret;
}


struct vr_ops raa_gen2_3_ops = {
  .get_fw_ver = get_raa_ver,
  .parse_file = raa_parse_file,
  .validate_file = NULL,
  .fw_update = raa_fw_update,
  .fw_verify = NULL,
};

//INFINEON
struct vr_ops xdpe152xx_ops = {
  .get_fw_ver = get_xdpe152xx_ver,
  .parse_file = xdpe152xx_parse_file,
  .validate_file = NULL,
  .fw_update = xdpe152xx_fw_update,
  .fw_verify = NULL,
};

struct vr_info vr_list[] = {
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
  [VR_SWB_PEX01_VCC] = {
    .bus = SWB_VR_BUS_ID,
    .addr = ADDR_SWB_VR_PEX01,
    .dev_name = "VR_PEX01_VCC",
    .ops = &raa_gen2_3_ops,
    .private_data = "swb",
    .xfer = vr_pldm_wr,
  },
  [VR_SWB_PEX23_VCC] = {
    .bus = SWB_VR_BUS_ID,
    .addr = ADDR_SWB_VR_PEX23,
    .dev_name = "VR_PEX23_VCC",
    .ops = &raa_gen2_3_ops,
    .private_data = "swb",
    .xfer = vr_pldm_wr,
  },
};

struct vr_info mb_inf_vr_list[] = {
  [VR_MB_CPU0_VCORE0] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_INF_CPU0_VCORE0,
    .dev_name = "VR_CPU0_VCORE0/SOC",
    .ops = &xdpe152xx_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU0_VCORE1] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_INF_CPU0_VCORE1,
    .dev_name = "VR_CPU0_VCORE1/PVDDIO",
    .ops = &xdpe152xx_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU0_PVDD11] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_INF_CPU0_PVDD11,
    .dev_name = "VR_CPU0_PVDD11",
    .ops = &xdpe152xx_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU1_VCORE0] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_INF_CPU1_VCORE0,
    .dev_name = "VR_CPU1_VCORE0/SOC",
    .ops = &xdpe152xx_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU1_VCORE1] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_INF_CPU1_VCORE1,
    .dev_name = "VR_CPU1_VCORE1/PVDDIO",
    .ops = &xdpe152xx_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU1_PVDD11] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_INF_CPU1_PVDD11,
    .dev_name = "VR_CPU1_PVDD11",
    .ops = &xdpe152xx_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
};

//SWB
//INFINEON
struct vr_ops xdpe12284c_ops = {
  .get_fw_ver = get_xdpe_ver,
  .parse_file = xdpe_parse_file,
  .validate_file = NULL,
  .fw_update = xdpe_fw_update,
  .fw_verify = NULL,
};


int plat_vr_init(void) {
  int ret, i, vr_cnt = sizeof(vr_list)/sizeof(vr_list[0]);
  uint8_t mb_sku_id = 0;
  uint8_t inf_devid[3] = { 0x02, 0x79, 0x02 };
  uint8_t tbuf[8], rbuf[8];

  pal_get_platform_id(&mb_sku_id);
  mb_sku_id = mb_sku_id & 0x07;

//MB
  if (mb_sku_id == 4) {
    for (i = 0; i < MB_VR_CNT; i++) {
      vr_list[i].ops =  &xdpe152xx_ops;
      vr_list[i].addr = mb_inf_vr_list[i].addr;
    }
  }

//SWB
  if (swb_presence()) {
    tbuf[0] = 0xAD;
    vr_pldm_wr(SWB_VR_BUS_ID, ADDR_SWB_VR_PEX01, tbuf, 1, rbuf, 3);
    if(!memcmp(rbuf, inf_devid, 3)) {
      for (i = 0; i < SWB_VR_CNT; i++) {
        vr_list[i+MB_VR_CNT].ops = &xdpe12284c_ops;
      }
    }
  }
  ret = vr_device_register(vr_list, vr_cnt);
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
