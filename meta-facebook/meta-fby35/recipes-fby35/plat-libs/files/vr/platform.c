#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <facebook/bic.h>
#include <facebook/bic_ipmi.h>
#include <facebook/bic_xfer.h>
#include <facebook/fby35_common.h>
#include <openbmc/kv.h>
#include "raa_gen3.h"
#include "xdpe12284c.h"
#include "xdpe152xx.h"
#include "tps53688.h"
#include "mp2856.h"

#define SB_VR_BUS 4
#define RBF_BIC_VR_BUS 9

#define HD_VR_REVID_MASK 0x30
#define HD_VR_RENESAS 0x0
#define HD_VR_INFINEON 0x1
#define HD_VR_MPS 0x2
#define HD_VR_TI 0x3

int hd_vr_addr_table[][3] = {
  {VDDCR_CPU0_ADDR,     VDDCR_CPU1_ADDR,     VDD11S3_ADDR},
  {VDDCR_CPU0_IFX_ADDR, VDDCR_CPU1_IFX_ADDR, VDD11S3_IFX_ADDR},
  {VDDCR_CPU0_MPS_ADDR, VDDCR_CPU1_MPS_ADDR, VDD11S3_MPS_ADDR},
  {VDDCR_CPU0_TI_ADDR,  VDDCR_CPU1_TI_ADDR,  VDD11S3_TI_ADDR}
};

static uint8_t slot_id = 0;
static char fru_name[64] = {0};  // prefix of vr version cache (kv)
static char fru_exp_name[64] = {0};  // prefix of expansion board vr version cache (kv)
static char lock_name[80] = {0};

enum {
  VR_CL_VCCIN = 0,
  VR_CL_VCCD,
  VR_CL_VCCINFAON,
  VR_GL_VCCIN,
  VR_GL_VCCD,
  VR_GL_VCCINF,
  VR_HD_VDDCR_CPU0,
  VR_HD_VDDCR_CPU1,
  VR_HD_VDD11S3,
  VR_RBF_A0V8,
  VR_RBF_VDDQAB,
  VR_RBF_VDDQCD,
};

static int
_fby35_vr_rdwr(uint8_t bus, uint8_t addr, uint8_t *txbuf, uint8_t txlen,
              uint8_t *rxbuf, uint8_t rxlen, uint8_t intf) {
  uint8_t tbuf[64] = {0};

  if (txbuf == NULL || rxbuf == NULL) {
    return -1;
  }

  if ((txlen + 3) > sizeof(tbuf)) {
    syslog(LOG_WARNING, "%s: invalid txlen %u", __func__, txlen);
    return -1;
  }

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = rxlen;
  memcpy(&tbuf[3], txbuf, txlen);
  return bic_data_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ,
                       tbuf, txlen+3, rxbuf, &rxlen, intf);
}

static int
fby35_vr_rdwr(uint8_t bus, uint8_t addr, uint8_t *txbuf, uint8_t txlen,
              uint8_t *rxbuf, uint8_t rxlen) {
  return _fby35_vr_rdwr(bus, addr, txbuf, txlen, rxbuf, rxlen, NONE_INTF);
};

static int
rbf_vr_rdwr(uint8_t bus, uint8_t addr, uint8_t *txbuf, uint8_t txlen,
              uint8_t *rxbuf, uint8_t rxlen) {
  return _fby35_vr_rdwr(bus, addr, txbuf, txlen, rxbuf, rxlen, FEXP_BIC_INTF);
};

static int
plat_xdpe152xx_fw_update(struct vr_info *info, void *args) {
  int ret;

  if (bic_set_vr_monitor_enable(slot_id, false, NONE_INTF) < 0) {
    return VR_STATUS_FAILURE;
  }
  msleep(10);
  if (xdpe152xx_unlock_reg(info) < 0) {
    return VR_STATUS_FAILURE;
  }
  ret = xdpe152xx_fw_update(info, args);
  xdpe152xx_lock_reg(info);
  bic_set_vr_monitor_enable(slot_id, true, NONE_INTF);

  return ret;
}

/*MP2856 must stop VR sensor polling while fw updating */
int
plat_mp2856_fw_update(struct vr_info *info, void *args) {
 int ret = 0;
  // Stop bic polling VR sensors
  if (bic_set_vr_monitor_enable(slot_id, false, NONE_INTF) < 0) {
    return VR_STATUS_FAILURE;
  }
  ret = mp2856_fw_update(info,args);

  // Restart bic polling VR sensors
  bic_set_vr_monitor_enable(slot_id, true, NONE_INTF) ;
  return ret;
}

/*XDPE12284C must stop VR sensor polling while fw updating */
int
plat_xdpe12284c_fw_update(struct vr_info *info, void *args) {
 int ret = 0;
  // Stop bic polling VR sensors
  if (bic_set_vr_monitor_enable(slot_id, false, FEXP_BIC_INTF) < 0) {
    return VR_STATUS_FAILURE;
  }
  ret = xdpe_fw_update(info,args);

  // Restart bic polling VR sensors
  bic_set_vr_monitor_enable(slot_id, true, FEXP_BIC_INTF) ;
  return ret;
}

int sb_vr_remaining_wr(uint8_t addr, uint16_t *remain, bool is_update) {
  int ret;
  if(is_update) {
    ret = bic_set_vr_remaining_wr(slot_id, addr, *remain);
  } else {
    ret = bic_get_vr_remaining_wr(slot_id, addr, remain);
  }
  return ret;
}

int
sb_vr_polling_ctrl(bool enable) {
  bic_set_vr_monitor_enable(slot_id, enable, NONE_INTF);
  if (!enable) {
    msleep(10);
  }
  return 0;
}

int
exp_1ou_vr_polling_ctrl(bool enable) {
  return bic_set_vr_monitor_enable(slot_id, enable, FEXP_BIC_INTF);
}

struct vr_ops rns_ops = {
  .get_fw_ver = get_raa_ver,
  .parse_file = raa_parse_file,
  .validate_file = NULL,
  .fw_update = raa_fw_update,
  .fw_verify = NULL,
};

struct vr_ops ifx_ops = {
  .get_fw_ver = get_xdpe152xx_ver,
  .parse_file = xdpe152xx_parse_file,
  .validate_file = NULL,
  .fw_update = plat_xdpe152xx_fw_update,
  .fw_verify = NULL,
  .unlock_reg = xdpe152xx_unlock_reg,
  .lock_reg = xdpe152xx_lock_reg,
};

struct vr_ops ti_ops = {
  .get_fw_ver = get_tps_ver,
  .parse_file = tps_parse_file,
  .validate_file = NULL,
  .fw_update = tps_fw_update,
  .fw_verify = NULL,
};

struct vr_ops mps_ops = {
  .get_fw_ver = get_mp2856_ver,
  .parse_file = mp2856_parse_file,
  .validate_file = NULL,
  .fw_update = plat_mp2856_fw_update,
  .fw_verify = NULL,
};

struct vr_ops mps_mp2985h_ops = {
  .get_fw_ver = get_mp2985h_ver,
  .parse_file = mp2856_parse_file,
  .validate_file = NULL,
  .fw_update = plat_mp2856_fw_update,
  .fw_verify = NULL,
};

struct vr_ops ifx_xdpe12284_ops = {
  .get_fw_ver = get_xdpe_ver,
  .parse_file = xdpe_parse_file,
  .validate_file = NULL,
  .fw_update = plat_xdpe12284c_fw_update,
  .fw_verify = NULL,
};

struct vr_ops none_ops = {
  .get_fw_ver = NULL,
  .parse_file = NULL,
  .validate_file = NULL,
  .fw_update = NULL,
  .fw_verify = NULL,
};

struct vr_info fby35_vr_list[] = {
  {
    .bus = SB_VR_BUS,
    .addr = VCCIN_ADDR,
    .dev_name = "VCCIN/VCCFA_EHV_FIVRA",
    .ops = &rns_ops,
    .private_data = fru_name,
    .xfer = &fby35_vr_rdwr,
  },
  {
    .bus = SB_VR_BUS,
    .addr = VCCD_ADDR,
    .dev_name = "VCCD",
    .ops = &rns_ops,
    .private_data = fru_name,
    .xfer = &fby35_vr_rdwr,
  },
  {
    .bus = SB_VR_BUS,
    .addr = VCCINFAON_ADDR,
    .dev_name = "VCCINFAON/VCCFA_EHV",
    .ops = &rns_ops,
    .private_data = fru_name,
    .xfer = &fby35_vr_rdwr,
  },
  {
    .bus = SB_VR_BUS,
    .addr = GL_VCCIN_ADDR,
    .dev_name = "VCCIN/VCCFA_EHV",
    .ops = &rns_ops,
    .private_data = fru_name,
    .xfer = &fby35_vr_rdwr,
    .sensor_polling_ctrl = sb_vr_polling_ctrl,
  },
  {
    .bus = SB_VR_BUS,
    .addr = GL_VCCD_ADDR,
    .dev_name = "VCCD_HV",
    .ops = &rns_ops,
    .private_data = fru_name,
    .xfer = &fby35_vr_rdwr,
    .sensor_polling_ctrl = sb_vr_polling_ctrl,
  },
  {
    .bus = SB_VR_BUS,
    .addr = GL_VCCINF_ADDR,
    .dev_name = "VCCINF/VCCFA_EHV_FIVRA",
    .ops = &rns_ops,
    .private_data = fru_name,
    .xfer = &fby35_vr_rdwr,
    .sensor_polling_ctrl = sb_vr_polling_ctrl,
  },
  {
    .bus = SB_VR_BUS,
    .addr = VDDCR_CPU0_ADDR,
    .dev_name = "VDDCR_CPU0/VDDCR_SOC",
    .ops = &rns_ops,
    .private_data = fru_name,
    .xfer = &fby35_vr_rdwr,
    .sensor_polling_ctrl = sb_vr_polling_ctrl,
  },
  {
    .bus = SB_VR_BUS,
    .addr = VDDCR_CPU1_ADDR,
    .dev_name = "VDDCR_CPU1/VDDIO",
    .ops = &rns_ops,
    .private_data = fru_name,
    .xfer = &fby35_vr_rdwr,
    .sensor_polling_ctrl = sb_vr_polling_ctrl,
  },
  {
    .bus = SB_VR_BUS,
    .addr = VDD11S3_ADDR,
    .dev_name = "VDD11_S3",
    .ops = &rns_ops,
    .private_data = fru_name,
    .xfer = &fby35_vr_rdwr,
    .sensor_polling_ctrl = sb_vr_polling_ctrl,
  },
  {
    .bus = RBF_BIC_VR_BUS,
    .addr = VR_1OU_V9_ASICA_ADDR,
    .dev_name = "1OU_VR_P0V9/P0V8_ASICA",
    .ops = &rns_ops,
    .private_data = fru_exp_name,
    .xfer = &rbf_vr_rdwr,
    .sensor_polling_ctrl = exp_1ou_vr_polling_ctrl,
  },
  {
    .bus = RBF_BIC_VR_BUS,
    .addr = VR_1OU_VDDQAB_ADDR,
    .dev_name = "1OU_VR_VDDQAB/D0V8",
    .ops = &rns_ops,
    .private_data = fru_exp_name,
    .xfer = &rbf_vr_rdwr,
    .sensor_polling_ctrl = exp_1ou_vr_polling_ctrl,
  },
  {
    .bus = RBF_BIC_VR_BUS,
    .addr = VR_1OU_VDDQCD_ADDR,
    .dev_name = "1OU_VR_VDDQCD",
    .ops = &rns_ops,
    .private_data = fru_exp_name,
    .xfer = &rbf_vr_rdwr,
    .sensor_polling_ctrl = exp_1ou_vr_polling_ctrl,
  },
  {
    .bus = VPDB_VR_BUS,
    .addr = VPDB_VR_ADDR,
    .dev_name = "VPDB_VR",
    .ops = &rns_ops,
    .private_data = fru_name,
  },
};

void plat_vr_preinit(uint8_t slot, const char *name) {
  slot_id = slot;

  if (name) {
    snprintf(fru_name, sizeof(fru_name), "%s", name);
    snprintf(fru_exp_name, sizeof(fru_exp_name), "%s_1ou", name);
  } else {
    fru_name[0] = 0;
    fru_exp_name[0] = 0;
  }
}

void fby35_vr_device_check(void) {
  uint8_t rbuf[16], rlen;
  int i = 0;
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};

  for (i = VR_CL_VCCIN; i <= VR_CL_VCCINFAON; i++) {
    snprintf(key, sizeof(key), "slot%d_vr_device_id", slot_id);
    rlen = sizeof(rbuf);
    if (kv_get(key, value, NULL, 0) < 0) {
      if (bic_get_vr_device_id(slot_id, rbuf, &rlen, fby35_vr_list[i].bus,
                               fby35_vr_list[i].addr, NONE_INTF) < 0) {
        continue;
      }
      snprintf(value, sizeof(value), "%d", rlen);
      kv_set(key, value, 0, 0);
    } else {
      rlen = atoi(value);
    }

    switch (rlen) {
      case VR_INFINEON:
        fby35_vr_list[i].ops = &ifx_ops;
        fby35_vr_list[i].sensor_polling_ctrl = sb_vr_polling_ctrl;

        // VRs on a MB share one lock as it doesn't support individually
        // stop/resume VR monitoring
        snprintf(lock_name, sizeof(lock_name), "%s_vr", fru_name);
        fby35_vr_list[i].lock_name = lock_name;
        break;
      case VR_TI:
        fby35_vr_list[i].ops = &ti_ops;
        break;
      default:
        fby35_vr_list[i].ops = &rns_ops;
        break;
    }
  }
}

void halfdome_vr_device_check(void){
  uint8_t board_rev = 0;
  int ret = 0;
  struct vr_ops* hd_vr_ops_list[] = {&rns_ops, &ifx_ops, &mps_ops, &ti_ops};

  ret = get_board_rev(slot_id, BOARD_ID_SB, &board_rev);
  if (ret < 0) {
      printf("Failed to get board revision ID, slot=%d\n", slot_id);
  }
  board_rev = (board_rev & HD_VR_REVID_MASK) >> 4;

  for (int i = VR_HD_VDDCR_CPU0; i <= VR_HD_VDD11S3; i++) {
    fby35_vr_list[i].addr = hd_vr_addr_table[board_rev][i - VR_HD_VDDCR_CPU0] ;
    fby35_vr_list[i].ops = hd_vr_ops_list[board_rev];
    // VRs on a MB share one lock as it doesn't support individually
    // stop/resume VR monitoring
    if (board_rev == HD_VR_MPS || board_rev == HD_VR_INFINEON) {
      snprintf(lock_name, sizeof(lock_name), "%s_vr", fru_name);
      fby35_vr_list[i].lock_name = lock_name;
    }
  }
}

void rbf_vr_device_check(void){
  uint8_t rbuf[16], rlen;
  int i;
  for (i = VR_RBF_A0V8; i <= VR_RBF_VDDQCD; i++) {
    rlen = sizeof(rbuf);
    if (bic_get_vr_device_id(slot_id, rbuf, &rlen, fby35_vr_list[i].bus,
                             fby35_vr_list[i].addr, FEXP_BIC_INTF) < 0) {
      continue;
    }

    switch (rlen) {
      case 2:
        fby35_vr_list[i].ops = &ifx_xdpe12284_ops;
        break;
      default:
        fby35_vr_list[i].ops = &rns_ops;
        break;
    }
  }
}

void greatlakes_vr_device_check(void) {
  uint8_t rbuf[16], rlen;
  int i;

  for (i = VR_GL_VCCIN; i <= VR_GL_VCCINF; i++) {
    switch (fby35_common_get_sb_rev(slot_id)) {
    case FW_REV_EVT:
      switch (fby35_vr_list[i].addr) {
      case GL_VCCIN_ADDR:
        fby35_vr_list[i].addr = GL_EVT_VCCIN_ADDR;
        break;
      case GL_VCCD_ADDR:
        fby35_vr_list[i].addr = GL_EVT_VCCD_ADDR;
        break;
      case GL_VCCINF_ADDR:
        fby35_vr_list[i].addr = GL_EVT_VCCINF_ADDR;
        break;
      }
    default:
      break;
    }

    rlen = sizeof(rbuf);
    if (bic_get_vr_device_id(slot_id, rbuf, &rlen, fby35_vr_list[i].bus,
                             fby35_vr_list[i].addr, NONE_INTF) < 0) {
      continue;
    }

    switch (rlen) {
    case 1:
      fby35_vr_list[i].ops = &mps_mp2985h_ops;
      break;
    case 2:
      fby35_vr_list[i].ops = &ifx_ops;
      break;
    case 4:
      fby35_vr_list[i].ops = &rns_ops;
      break;
    case 6:
      fby35_vr_list[i].ops = &ti_ops;
      break;
    default:
      syslog(LOG_WARNING, "Failed to initialize VR operation. Device ID's rlen: %d", rlen);
      fby35_vr_list[i].ops = &none_ops;
      break;
    }
  }
}

int plat_vr_init(void) {
  int config_status = 0;
  int vr_cnt = sizeof(fby35_vr_list)/sizeof(fby35_vr_list[0]);
  uint8_t type_1ou = TYPE_1OU_UNKNOWN;
  int server_type = SERVER_TYPE_NONE;

  if (slot_id != FRU_BMC) {
    server_type = fby35_common_get_slot_type(slot_id);
    if (server_type < 0) {
      syslog(LOG_WARNING, "%s() Error while getting slot%d type: %d", __func__, slot_id, server_type);
    }

    if (server_type == SERVER_TYPE_HD) {
      halfdome_vr_device_check();
    } else if (server_type == SERVER_TYPE_GL) {
      greatlakes_vr_device_check();
    } else {
      fby35_vr_device_check();
    }

    config_status = bic_is_exp_prsnt(slot_id);
    if (config_status < 0) config_status = 0;
    if (((config_status & PRESENT_1OU) == PRESENT_1OU) &&
      (bic_get_1ou_type(slot_id, &type_1ou) == 0)) {
      if (type_1ou == TYPE_1OU_RAINBOW_FALLS) {
        rbf_vr_device_check();
      }
    }
  }

  for (int i = 0; i < vr_cnt; i++) {
    fby35_vr_list[i].remaining_wr_op = sb_vr_remaining_wr;
  }

  return vr_device_register(fby35_vr_list, vr_cnt);
}

void plat_vr_exit(void) {
  if (plat_configs) {
    free(plat_configs);
  }

  return;
}
