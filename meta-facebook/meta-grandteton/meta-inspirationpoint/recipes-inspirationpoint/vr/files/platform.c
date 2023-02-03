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
#include "mp2856.h"

#define MB_VR_BUS_ID   (20)
#define SWB_VR_BUS_ID  (3)

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

#define MAX_VR_CNT (MB_VR_CNT + ACB_VR_CNT + SWB_VR_CNT + CXL_VR_CNT)

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
  .fw_update = mp2856_fw_update,
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
    .private_data = "acb",
    .xfer = acb_vr_pldm_wr,
  },
};

/* Artemis CXL */
struct vr_info cxl_vr_list[] = {
  [JCN1_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MEB JCN1 VR_A0V8_9",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn1",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN1_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MEB JCN1 VR_VDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn1",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN1_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MEB JCN1 VR_VDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn1",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN2_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MEB JCN2 VR_A0V8_9",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn2",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN2_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MEB JCN2 VR_VDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn2",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN2_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MEB JCN2 VR_VDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn2",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN3_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MEB JCN3 VR_A0V8_9",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn3",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN3_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MEB JCN3 VR_VDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn3",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN3_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MEB JCN3 VR_VDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn3",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN4_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MEB JCN4 VR_A0V8_9",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn4",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN4_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MEB JCN4 VR_VDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn4",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN4_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MEB JCN4 VR_VDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn4",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN9_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MEB JCN9 VR_A0V8_9",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn9",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN9_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MEB JCN9 VR_VDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn9",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN9_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MEB JCN9 VR_VDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn9",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN10_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MEB JCN10 VR_A0V8_9",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn10",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN10_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MEB JCN10 VR_VDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn10",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN10_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MEB JCN10 VR_VDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn10",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN11_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MEB JCN11 VR_A0V8_9",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn11",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN11_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MEB JCN11 VR_VDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn11",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN11_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MEB JCN11 VR_VDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn11",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN12_VR_CXL_A0V8_9] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_A0V8_9,
    .dev_name = "MEB JCN12 VR_A0V8_9",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn12",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN12_VR_CXL_VDDQ_AB] = {
    .bus = CXL_VR_BUS_ID,
    .addr = AADR_CXL_VR_VDDQ_AB,
    .dev_name = "MEB JCN12 VR_VDDQ_AB",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn12",
    .xfer = cxl_vr_pldm_wr,
  },
  [JCN12_VR_CXL_VDDQ_CD] = {
    .bus = CXL_VR_BUS_ID,
    .addr = ADDR_CXL_VR_ADDQ_CD,
    .dev_name = "MEB JCN12 VR_VDDQ_CD",
    .ops = &xdpe12284c_ops,
    .private_data = "meb_jcn12",
    .xfer = cxl_vr_pldm_wr,
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
  uint8_t source = 0;

  get_comp_source(FRU_MB, MB_VR_SOURCE, &source);
  // TODO: GET MB source
  if (source == SECOND_SOURCE) {
    for (int i = 0; i < MB_VR_CNT; i++) {
      vr_list[i].ops =  &xdpe152xx_ops;
      vr_list[i].addr = mb_inf_vr_addr[i];
    }
  }
  // TODO: GET ACB source
}

int plat_vr_init(void) {
  int i = 0;
  int ret = 0;
  uint8_t vr_cnt = 0;
  uint8_t id = 0;
  uint8_t status = 0;

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
      }
    }
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
