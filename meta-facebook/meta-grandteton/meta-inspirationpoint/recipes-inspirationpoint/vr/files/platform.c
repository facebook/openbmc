#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <openbmc/pal.h>
#include <openbmc/pal_common.h>
#include <libpldm/pldm.h>
#include <libpldm/platform.h>
#include <libpldm-oem/pldm.h>
#include "raa_gen3.h"
#include "xdpe12284c.h"
#include "xdpe152xx.h"
#include "mp2856.h"
#include <openbmc/kv.h>

#define MB_VR_BUS_ID   (20)
#define SWB_VR_BUS_ID  (3)
#define MB_EEPROM_BUS  33
#define MB_EEPROM_ADDR 0x51
#define VPDB_VR_BUS_ID (38)
#define FRU_EEPROM "/sys/class/i2c-dev/i2c-%d/device/%d-00%x/eeprom"
#define MB_EEPROM_BASE_OFFSET 0x1810

enum {
  VR_MB_CPU0_VCORE0   = 0,
  VR_MB_CPU0_VCORE1   = 1,
  VR_MB_CPU0_PVDD11   = 2,
  VR_MB_CPU1_VCORE0   = 3,
  VR_MB_CPU1_VCORE1   = 4,
  VR_MB_CPU1_PVDD11   = 5,
  MB_VR_CNT           = 6,
};

enum {
  VR_MB_CPU0_RT_P0V9   = 0,
  VR_MB_CPU1_RT_P0V9   = 1,
  MB_RT_VR_CNT   = 2,
};

enum {
  VR_SWB_PEX01_VCC    = 0,
  VR_SWB_PEX23_VCC,
  SWB_VR_CNT,
};

enum ARTEMIS_ACB_VR {
  VR_ACB_PESW_VCC     = 0,
  ACB_VR_CNT,
};

enum ARTEMIS_CXL_VR {
  JCN1_VR_CXL_A0V8_9,
  JCN1_VR_CXL_VDDQ_AB,
  JCN1_VR_CXL_VDDQ_CD,
  JCN2_VR_CXL_A0V8_9,
  JCN2_VR_CXL_VDDQ_AB,
  JCN2_VR_CXL_VDDQ_CD,
  JCN3_VR_CXL_A0V8_9,
  JCN3_VR_CXL_VDDQ_AB,
  JCN3_VR_CXL_VDDQ_CD,
  JCN4_VR_CXL_A0V8_9,
  JCN4_VR_CXL_VDDQ_AB,
  JCN4_VR_CXL_VDDQ_CD,
  JCN9_VR_CXL_A0V8_9,
  JCN9_VR_CXL_VDDQ_AB,
  JCN9_VR_CXL_VDDQ_CD,
  JCN10_VR_CXL_A0V8_9,
  JCN10_VR_CXL_VDDQ_AB,
  JCN10_VR_CXL_VDDQ_CD,
  JCN11_VR_CXL_A0V8_9,
  JCN11_VR_CXL_VDDQ_AB,
  JCN11_VR_CXL_VDDQ_CD,
  JCN12_VR_CXL_A0V8_9,
  JCN12_VR_CXL_VDDQ_AB,
  JCN12_VR_CXL_VDDQ_CD,
  VR_VPDB_DISCRETE,
  CXL_VR_CNT,
};

enum {
  ADDR_SWB_VR_PEX01 = 0xC0,
  ADDR_SWB_VR_PEX23 = 0xC4,
};

enum ARTEMIS_ACB_VR_ADDRESS {
  ADDR_ACB_VR_PESW  = 0xC8,
};

enum ARTEMIS_CXL_VR_ADDRESS {
  ADDR_CXL_VR_A0V8_9  = 0xC8,
  AADR_CXL_VR_VDDQ_AB = 0xB0,
  ADDR_CXL_VR_ADDQ_CD = 0xB4,
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

enum {
  ADDR_CPU0_RT_P0V9 = 0xC0,
  ADDR_CPU1_RT_P0V9 = 0xEC,
};

enum {
  ADDR_VPDB_DISCRETE_VR = 0xA8,
};

#define MAX_VR_CNT (MB_VR_CNT + ACB_VR_CNT + SWB_VR_CNT + CXL_VR_CNT + MB_RT_VR_CNT)

struct vr_info vr_list[MAX_VR_CNT] = {0};

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
  ret = oem_pldm_ipmi_send_recv(bus, SWB_BIC_EID,
                                NETFN_OEM_1S_REQ, CMD_OEM_1S_BIC_BRIDGE,
                                tbuf, tlen,
                                rxbuf, &rlen,
                                true);
  return ret;
}

static int
plat_mp2856_fw_update(struct vr_info *info, void *args) {
  int ret;

  mb_vr_polling_ctrl(false);
  ret = mp2856_fw_update(info, args);
  mb_vr_polling_ctrl(true);

  return ret;
}

static int
acb_vr_pldm_wr(uint8_t bus, uint8_t addr,
           uint8_t *txbuf, uint8_t txlen,
           uint8_t *rxbuf, uint8_t rxlen) {
  int ret = 0;
  size_t rlen = 0;
  uint8_t tbuf[MAX_TXBUF_SIZE] = {0};
  uint8_t tlen=0;

  tbuf[0] = bus;
  tbuf[1] = addr;
  tbuf[2] = rxlen;
  tlen += 3;
  memcpy(tbuf + tlen, txbuf, txlen);
  tlen = txlen + tlen;

  ret = oem_pldm_ipmi_send_recv(ACB_BIC_BUS, ACB_BIC_EID,
                               NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ,
                               tbuf, tlen,
                               rxbuf, &rlen,
			       true);
  return ret;
}

static int
cxl_vr_pldm_wr(uint8_t bus, uint8_t addr,
           uint8_t *txbuf, uint8_t txlen,
           uint8_t *rxbuf, uint8_t rxlen) {
  int ret = 0;
  size_t rlen = 0;
  uint8_t tbuf[MAX_TXBUF_SIZE] = {0};
  uint8_t tlen=0;

  tbuf[0] = bus;
  tbuf[1] = addr;
  tbuf[2] = rxlen;
  tlen += 3;
  memcpy(tbuf + tlen, txbuf, txlen);
  tlen = txlen + tlen;

  ret = oem_pldm_ipmi_send_recv(MEB_BIC_BUS, MEB_BIC_EID,
                               NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ,
                               tbuf, tlen,
                               rxbuf, &rlen,
			       true);
  return ret;
}

//Renesas VR
struct vr_ops raa_gen2_3_ops = {
  .get_fw_ver = get_raa_ver,
  .parse_file = raa_parse_file,
  .validate_file = NULL,
  .fw_update = raa_fw_update,
  .fw_verify = NULL,
};

//INFINEON VR
struct vr_ops xdpe152xx_ops = {
  .get_fw_ver = get_xdpe152xx_ver,
  .parse_file = xdpe152xx_parse_file,
  .validate_file = NULL,
  .fw_update = xdpe152xx_fw_update,
  .fw_verify = NULL,
};

struct vr_ops xdpe12284c_ops = {
  .get_fw_ver = get_xdpe_ver,
  .parse_file = xdpe_parse_file,
  .validate_file = NULL,
  .fw_update = xdpe_fw_update,
  .fw_verify = NULL,
};

//MPS VR
struct vr_ops mp2856_ops = {
  .get_fw_ver = get_mp2856_ver,
  .parse_file = mp2856_parse_file,
  .validate_file = NULL,
  .fw_update = plat_mp2856_fw_update,
  .fw_verify = NULL,
};

// MB VR Support list
struct vr_info mb_vr_list[] = {
  [VR_MB_CPU0_VCORE0] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU0_VCORE0,
    .dev_name = "VR_CPU0_VCORE0/SOC",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
    .sensor_polling_ctrl = NULL,
  },
  [VR_MB_CPU0_VCORE1] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU0_VCORE1,
    .dev_name = "VR_CPU0_VCORE1/PVDDIO",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
    .sensor_polling_ctrl = NULL,
  },
  [VR_MB_CPU0_PVDD11] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU0_PVDD11,
    .dev_name = "VR_CPU0_PVDD11",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
    .sensor_polling_ctrl = NULL,
  },
  [VR_MB_CPU1_VCORE0] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU1_VCORE0,
    .dev_name = "VR_CPU1_VCORE0/SOC",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
    .sensor_polling_ctrl = NULL,
  },
  [VR_MB_CPU1_VCORE1] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU1_VCORE1,
    .dev_name = "VR_CPU1_VCORE1/PVDDIO",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
    .sensor_polling_ctrl = NULL,
  },
  [VR_MB_CPU1_PVDD11] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU1_PVDD11,
    .dev_name = "VR_CPU1_PVDD11",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
    .sensor_polling_ctrl = NULL,
  },
};

struct vr_info mb_rt_vr_list[] = {
  [VR_MB_CPU0_RT_P0V9] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU0_RT_P0V9,
    .dev_name = "VR_CPU0_RETIMER_P0V9",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
  [VR_MB_CPU1_RT_P0V9] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU1_RT_P0V9,
    .dev_name = "VR_CPU1_RETIMER_P0V9",
    .ops = &raa_gen2_3_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
};

struct vr_info swb_vr_list[] = {
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

/* Artemis ACB */
struct vr_info acb_vr_list[] = {
  [VR_ACB_PESW_VCC] = {
    .bus = ACB_VR_BUS_ID,
    .addr = ADDR_ACB_VR_PESW,
    .dev_name = "VR_PESW_VCC",
    .ops = &xdpe152xx_ops,
    .private_data = "cb",
    .xfer = acb_vr_pldm_wr,
  },
};

/* Artemis CXL */
struct vr_info cxl_vr_list[] = {
  [JCN1_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MC CXL8 VR_P0V89A",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl8",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN1_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MC CXL8 VR_P0V8D_PVDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl8",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN1_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MC CXL8 VR_PVDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl8",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN2_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MC CXL7 VR_P0V89A",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl7",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN2_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MC CXL7 VR_P0V8D_PVDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl7",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN2_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MC CXL7 VR_PVDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl7",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN3_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MC CXL6 VR_P0V89A",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl6",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN3_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MC CXL6 VR_P0V8D_PVDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl6",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN3_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MC CXL6 VR_PVDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl6",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN4_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MC CXL5 VR_P0V89A",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl5",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN4_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MC CXL5 VR_P0V8D_PVDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl5",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN4_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MC CXL5 VR_PVDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl5",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN9_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MC CXL3 VR_P0V89A",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl3",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN9_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MC CXL3 VR_P0V8D_PVDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl3",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN9_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MC CXL3 VR_PVDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl3",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN10_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MC CXL4 VR_P0V89A",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl4",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN10_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MC CXL4 VR_P0V8D_PVDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl4",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN10_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MC CXL4 VR_PVDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl4",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN11_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MC CXL1 VR_P0V89A",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl1",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN11_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MC CXL1 VR_P0V8D_PVDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl1",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN11_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MC CXL1 VR_PVDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl1",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN12_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MC CXL2 VR_P0V89A",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl2",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN12_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MC CXL2 VR_P0V8D_PVDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl2",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN12_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MC CXL2 VR_PVDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "mc_cxl2",
    .xfer = cxl_vr_pldm_wr,
  },
  [VR_VPDB_DISCRETE] = {
    .bus = VPDB_VR_BUS_ID,
    .addr = ADDR_VPDB_DISCRETE_VR,
    .dev_name = "VPDB_VR",
    .ops = &raa_gen2_3_ops,
    .private_data = "vpdb",
    .xfer = NULL,
    .sensor_polling_ctrl = NULL,
  },
};

uint8_t mb_inf_vr_addr[] = {
  ADDR_INF_CPU0_VCORE0,
  ADDR_INF_CPU0_VCORE1,
  ADDR_INF_CPU0_PVDD11,
  ADDR_INF_CPU1_VCORE0,
  ADDR_INF_CPU1_VCORE1,
  ADDR_INF_CPU1_PVDD11,
};

void
load_artemis_comp_source(void) {
  // TODO: GET ACB source
}

uint16_t
mb_get_mps_remaining_offset(uint8_t addr) {
  addr = (((addr + 2) & 0xf) | 0x40);
  return MB_EEPROM_BASE_OFFSET + addr; 
}

int
mb_set_vr_remaining_wr(uint8_t addr, uint16_t *remain) {
  int fd;
  ssize_t bytes_wr;
  errno = 0;
  char mb_path[128] = {0};
  uint16_t offset;

  sprintf(mb_path, FRU_EEPROM, MB_EEPROM_BUS, MB_EEPROM_BUS, MB_EEPROM_ADDR);

  // check for file presence
  if (access(mb_path, F_OK)) {
    syslog(LOG_ERR, "mb_set_vr_remaining_wr: unable to access %s: %s", mb_path, strerror(errno));
    return errno;
  }

  fd = open(mb_path, O_WRONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "mb_set_vr_remaining_wr: unable to open %s: %s", mb_path, strerror(errno));
    return errno;
  }

  offset = mb_get_mps_remaining_offset(addr);

  lseek(fd, offset, SEEK_SET);

  bytes_wr = write(fd, remain, sizeof(*remain));
  if (bytes_wr != sizeof(*remain)) {
    syslog(LOG_ERR, "mb_set_vr_remaining_wr: write to %s failed: %s", mb_path, strerror(errno));
  }

  close(fd);
  return errno;
}

int
mb_get_vr_remaining_wr(uint8_t addr, uint16_t *remain) {
  int fd;
  ssize_t bytes_rd;
  errno = 0;
  char mb_path[128] = {0};
  uint16_t offset;

  sprintf(mb_path, FRU_EEPROM, MB_EEPROM_BUS, MB_EEPROM_BUS, MB_EEPROM_ADDR);

  // check for file presence
  if (access(mb_path, F_OK)) {
    syslog(LOG_ERR, "mb_get_vr_remaining_wr: unable to access %s: %s", mb_path, strerror(errno));
    return errno;
  }

  fd = open(mb_path, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "mb_get_vr_remaining_wr: unable to open %s: %s", mb_path, strerror(errno));
    return errno;
  }

  offset = mb_get_mps_remaining_offset(addr);

  lseek(fd, offset, SEEK_SET);

  bytes_rd = read(fd, remain, sizeof(*remain));
  if (bytes_rd != sizeof(*remain)) {
    syslog(LOG_ERR, "mb_get_vr_remaining_wr: read from %s failed: %s", mb_path, strerror(errno));
  }

  close(fd);
  return errno;
}

int mb_vr_remaining_wr(uint8_t addr, uint16_t *remain, bool is_update) {
  int ret;
  if(is_update) {
    ret = mb_set_vr_remaining_wr(addr, remain);
  } else {
    ret = mb_get_vr_remaining_wr(addr, remain);
  }
  return ret;
}

int plat_vr_init(void) {
  int i = 0;
  int ret = 0;
  uint8_t vr_cnt = 0;
  uint8_t id = 0;
  uint8_t status = 0;
  char rev_id[MAX_VALUE_LEN] = {0};
  char sku_id[MAX_VALUE_LEN] = {0};

  // Add MB VR
  memcpy(vr_list + vr_cnt, mb_vr_list, MB_VR_CNT*sizeof(struct vr_info));
  vr_cnt += MB_VR_CNT;

  if (pal_is_artemis()) {
    // Add ACB VR
    if (pal_is_fru_prsnt(FRU_ACB, &status) == 0 && (status == FRU_PRSNT)) {
      memcpy(vr_list + vr_cnt,  acb_vr_list, ACB_VR_CNT*sizeof(struct vr_info));
      vr_cnt += ACB_VR_CNT;
    }
    // Add MEB CXL VR
    if (pal_is_fru_prsnt(FRU_MEB, &status) == 0 && (status == FRU_PRSNT)) {
      memcpy(vr_list + vr_cnt,  cxl_vr_list, CXL_VR_CNT*sizeof(struct vr_info));
      vr_cnt += CXL_VR_CNT;
    }
    // get artemis VR source
    load_artemis_comp_source();
  } else {
    // Add SWB VR
    if (fru_presence(FRU_SWB, &status) && (status == FRU_PRSNT)) {
      memcpy(vr_list + vr_cnt, swb_vr_list, SWB_VR_CNT*sizeof(struct vr_info));
      vr_cnt += SWB_VR_CNT;
      // get SWB VR source
      get_comp_source(FRU_SWB, SWB_VR_SOURCE, &id);
      if(id == SECOND_SOURCE) {
        for (i = 0; i < SWB_VR_CNT; i++) {
          vr_list[i+MB_VR_CNT].ops = &xdpe12284c_ops;
        }
      } else if (id == THIRD_SOURCE) {
        for (i = 0; i < SWB_VR_CNT; i++) {
          vr_list[i+MB_VR_CNT].ops = &mp2856_ops;
        }
      }
    }
  }

  // get MB VR source
  get_comp_source(FRU_MB, MB_VR_SOURCE, &id);
  if (id == SECOND_SOURCE) {
    for (i = 0; i < MB_VR_CNT; i++) {
      vr_list[i].ops =  &xdpe152xx_ops;
      vr_list[i].addr = mb_inf_vr_addr[i];
    }
  } else if (id == THIRD_SOURCE) {
    for (i = 0; i < MB_VR_CNT; i++) {
      vr_list[i].ops =  &mp2856_ops;
      vr_list[i].addr = mb_inf_vr_addr[i];
      vr_list[i].sensor_polling_ctrl = &mb_vr_polling_ctrl;
      vr_list[i].remaining_wr_op = mb_vr_remaining_wr;
    }
  }

  //Add Retimer VR
  kv_get("mb_rev", rev_id, 0, 0);
  kv_get("mb_sku", sku_id, 0, 0);
  if (strcmp(rev_id, "2")) {
    if (   !strcmp(sku_id, "20") || !strcmp(sku_id, "25") 
        || !strcmp(sku_id, "28") || !strcmp(sku_id, "29")) {
      //3nd source
      mb_rt_vr_list[0].ops = &xdpe152xx_ops;
      mb_rt_vr_list[1].ops = &xdpe152xx_ops;
    }
    memcpy(vr_list + vr_cnt, mb_rt_vr_list, MB_RT_VR_CNT * sizeof(struct vr_info));
    vr_cnt += MB_RT_VR_CNT;
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
