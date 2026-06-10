#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <facebook/netlakenext_common.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-pal.h>
#include "mp29608a.h"
#include "xdpe152xx.h"

enum {
  VR_VDDCR_VDDCR_SOC = 0,
  VR_VDD_MISC = 1,
};

static int
netlakenext_vr_rdwr(uint8_t bus, uint8_t addr, uint8_t *txbuf, uint8_t txlen,
              uint8_t *rxbuf, uint8_t rxlen) {
  if ((txbuf == NULL) || ((rxlen != 0) && (rxbuf == NULL))) {
    return -1;
  }

  int ret = 0, fd = 0;
  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_ERR, "Failed to open vr, addr: 0x%x\n", addr);
    return -1;
  }

  ret = i2c_rdwr_msg_transfer(fd, addr, txbuf, txlen, rxbuf, rxlen);
  close(fd);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to do i2c rdwr %x-%x", __func__,
           bus, addr);
    return -1;
  }

  return ret;
}

static int
bmc_vr_polling_waiting(bool enable) {
  if (!enable) {
    pal_set_fw_update_ongoing(FRU_SERVER, 30);
    //wait 3 seconds to wait sensord stop switching the VR pages
    sleep(3);
  } else {
    pal_set_fw_update_ongoing(FRU_SERVER, 0);
  }
}

struct vr_ops mp29608b_ops = {
  .get_fw_ver = get_mp29608a_ver,
  .parse_file = mp29608a_parse_file,
  .validate_file = NULL,
  .fw_update = mp29608a_fw_update,
  .fw_verify = NULL,
};

struct vr_ops xpde19283d_ops = {
  .get_fw_ver = get_xdpe152xx_ver,
  .parse_file = xdpe152xx_parse_file,
  .validate_file = NULL,
  .fw_update = xdpe152xx_fw_update,
  .fw_verify = NULL,
};

struct vr_info netlakenext_vr_list[] = {
  [VR_VDDCR_VDDCR_SOC] = {
    .bus = VR_BUS,
    .addr = VR_PVDDCR_ADDR,
    .dev_name = "VR_VDDCR/VR_VDDCR_SOC",
    .ops = &mp29608b_ops,
    .private_data = "server",
    .xfer = &netlakenext_vr_rdwr,
    .sensor_polling_ctrl = bmc_vr_polling_waiting,
  },
  [VR_VDD_MISC] = {
    .bus = VR_BUS,
    .addr = VR_PVDD_MISC_ADDR,
    .dev_name = "VR_VDD_MISC",
    .ops = &mp29608b_ops,
    .private_data = "server",
    .xfer = &netlakenext_vr_rdwr,
    .sensor_polling_ctrl = bmc_vr_polling_waiting,
  },
};

struct vr_info netlakenext_vr_second_list[] = {
  [VR_VDDCR_VDDCR_SOC] = {
    .bus = VR_BUS,
    .addr = VR_PVDDCR_ADDR,
    .dev_name = "VR_VDDCR/VR_VDDCR_SOC",
    .ops = &xpde19283d_ops,
    .private_data = "server",
    .xfer = &netlakenext_vr_rdwr,
  },
  [VR_VDD_MISC] = {
    .bus = VR_BUS,
    .addr = VR_PVDD_MISC_ADDR,
    .dev_name = "VR_VDD_MISC",
    .ops = &xpde19283d_ops,
    .private_data = "server",
    .xfer = &netlakenext_vr_rdwr,
  },
};

void vr_change_bus(struct vr_info *vr_list, int vr_cnt, enum board_rev stage) {
  if (stage >= EVT2) {
    for (int i = 0; i < vr_cnt; i++) {
      if (vr_list[i].addr == VR_PVDDCR_ADDR) {
        vr_list[i].bus = VR_PVDDCR_BUS;
      }
      else if (vr_list[i].addr == VR_PVDDCR_SOC_ADDR) {
        vr_list[i].bus = VR_PVDDCR_SOC_BUS;
      }
      else if (vr_list[i].addr == VR_PVDD_MISC_ADDR) {
        vr_list[i].bus = VR_PVDD_MISC_BUS;
      }
    }
  }
}

int plat_vr_init(void) {
  int ret;
  int i2cfd = 0;
  uint8_t rev_id = 0;
  uint8_t tlen = 1;
  uint8_t rev_id_reg = CPLD_REV_ID_REG;
  int vr_cnt;

  ret = netlakenext_common_get_board_rev_backup(&rev_id);
  if (ret < 0) {
    syslog(LOG_ERR, "%s(): failed to get board revision. rev_id=%02X\n", __func__, rev_id);
  }

  int sku = ((int)rev_id & 0x08) >> 3;
  enum board_rev stage = (enum board_rev)(rev_id & 0x07);
  if (sku == 0) {
    vr_cnt = sizeof(netlakenext_vr_list)/sizeof(netlakenext_vr_list[0]);
    vr_change_bus(netlakenext_vr_list, vr_cnt, stage);
    ret = vr_device_register(netlakenext_vr_list, vr_cnt );
  } else if (sku == 1) {
    vr_cnt = sizeof(netlakenext_vr_second_list)/sizeof(netlakenext_vr_second_list[0]);
    vr_change_bus(netlakenext_vr_second_list, vr_cnt, stage);
    ret = vr_device_register(netlakenext_vr_second_list, vr_cnt );
  } else {
    syslog(LOG_ERR, "Invalid board revision got from fpga.");
  }

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
