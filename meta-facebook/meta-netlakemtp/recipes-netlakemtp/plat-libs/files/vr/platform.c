#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <facebook/netlakemtp_common.h>
#include <openbmc/obmc-i2c.h>
#include "xdpe152xx.h"
#include "tda38640.h"
#include "mp2856.h"

enum {
  VR_1V05_STBY = 0,
  VR_VNN_PCH = 1,
  VR_VCCIN_1V8_STBY = 2,
  VR_VCCANA_CPU = 3,
  VR_VDDQ = 4,
};

//tda38640's i2c address is different from pmbus address
enum {
  TDA38640_VNN_PCH_ADDR = 0x20,
  TDA38640_P1V05_STBY_ADDR = 0x24,
  TDA38640_VDDQ_ADDR = 0x2A,
  TDA38640_VCCANA_CPU_ADDR = 0x2C,
};

static int
netlakemtp_vr_rdwr(uint8_t bus, uint8_t addr, uint8_t *txbuf, uint8_t txlen,
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

struct vr_ops ifx_ops = {
  .get_fw_ver = get_xdpe152xx_ver,
  .parse_file = xdpe152xx_parse_file,
  .validate_file = NULL,
  .fw_update = xdpe152xx_fw_update,
  .fw_verify = NULL,
};

struct vr_ops iftda_ops = {
  .get_fw_ver = get_tda38640_ver,
  .parse_file = tda38640_parse_file,
  .validate_file = NULL,
  .fw_update = tda38640_fw_update,
  .fw_verify = NULL,
};

struct vr_ops mps_ops = {
  .get_fw_ver = get_mp2856_ver,
  .parse_file = mp2856_parse_file,
  .validate_file = NULL,
  .fw_update = mp2856_fw_update,
  .fw_verify = NULL,
};

struct vr_ops mps2993_ops = {
  .get_fw_ver = get_mp2993_ver,
  .parse_file = mp2856_parse_file,
  .validate_file = NULL,
  .fw_update = mp2856_fw_update,
  .fw_verify = NULL,
};

struct vr_info netlakemtp_vr_list[] = {
  [VR_1V05_STBY] = {
    .bus = VR_BUS,
    .addr = TDA38640_P1V05_STBY_ADDR,
    .dev_name = "VR_1V05_STBY",
    .ops = &iftda_ops,
    .private_data = "server",
    .xfer = &netlakemtp_vr_rdwr,
  },
  [VR_VNN_PCH] = {
    .bus = VR_BUS,
    .addr = TDA38640_VNN_PCH_ADDR,
    .dev_name = "VR_VNN_PCH",
    .ops = &iftda_ops,
    .private_data = "server",
    .xfer = &netlakemtp_vr_rdwr,
  },
  [VR_VCCIN_1V8_STBY] = {
    .bus = VR_BUS,
    .addr = VR_PVCCIN_ADDR,
    .dev_name = "VR_VCCIN/VR_1V8_STBY",
    .ops = &ifx_ops,
    .private_data = "server",
    .xfer = &netlakemtp_vr_rdwr,
  },
  [VR_VCCANA_CPU] = {
    .bus = VR_BUS,
    .addr = TDA38640_VCCANA_CPU_ADDR,
    .dev_name = "VR_VCCANA_CPU",
    .ops = &iftda_ops,
    .private_data = "server",
    .xfer = &netlakemtp_vr_rdwr,
  },
  [VR_VDDQ] = {
    .bus = VR_BUS,
    .addr = TDA38640_VDDQ_ADDR,
    .dev_name = "VR_VDDQ",
    .ops = &iftda_ops,
    .private_data = "server",
    .xfer = &netlakemtp_vr_rdwr,
  },
};

struct vr_info netlakemtp_vr_second_list[] = {
  [VR_1V05_STBY] = {
    .bus = VR_BUS,
    .addr = VR_P1V05_STBY_ADDR,
    .dev_name = "VR_1V05_STBY",
    .ops = &mps_ops,
    .private_data = "server",
    .xfer = &netlakemtp_vr_rdwr,
  },
  [VR_VNN_PCH] = {
    .bus = VR_BUS,
    .addr = VR_PVNN_PCH_ADDR,
    .dev_name = "VR_VNN_PCH",
    .ops = &mps_ops,
    .private_data = "server",
    .xfer = &netlakemtp_vr_rdwr,
  },
  [VR_VCCIN_1V8_STBY] = {
    .bus = VR_BUS,
    .addr = VR_PVCCIN_ADDR,
    .dev_name = "VR_VCCIN/VR_1V8_STBY",
    .ops = &mps2993_ops,
    .private_data = "server",
    .xfer = &netlakemtp_vr_rdwr,
  },
  [VR_VCCANA_CPU] = {
    .bus = VR_BUS,
    .addr = VR_PVCCANCPU_ADDR,
    .dev_name = "VR_VCCANA_CPU",
    .ops = &mps_ops,
    .private_data = "server",
    .xfer = &netlakemtp_vr_rdwr,
  },
  [VR_VDDQ] = {
    .bus = VR_BUS,
    .addr = VR_PVDDQ_ABC_CPU_ADDR,
    .dev_name = "VR_VDDQ",
    .ops = &mps_ops,
    .private_data = "server",
    .xfer = &netlakemtp_vr_rdwr,
  },
};

int plat_vr_init(void) {
  int ret;
  int i2cfd = 0;
  uint8_t rev_id;
  uint8_t tlen = 1;
  uint8_t rev_id_reg = CPLD_REV_ID_REG;
  int vr_cnt;

  i2cfd = i2c_cdev_slave_open(CPLD_BUS, CPLD_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_ERR, "Failed to open CPLD, fd=%x\n", i2cfd);
    return -1;
  }

  ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDR, &rev_id_reg, tlen, &rev_id, sizeof(rev_id));
  close(i2cfd);
  if (ret < 0) {
    syslog(LOG_ERR, "Failed to get board revision form fpga\n");
    ret = -1;
  }

  int sku = ((int)rev_id & 0x08) >> 3;
  if (sku == 0) {
    vr_cnt = sizeof(netlakemtp_vr_list)/sizeof(netlakemtp_vr_list[0]);
    ret = vr_device_register(netlakemtp_vr_list, vr_cnt );
  } else if (sku == 1) {
    vr_cnt = sizeof(netlakemtp_vr_second_list)/sizeof(netlakemtp_vr_second_list[0]);
    ret = vr_device_register(netlakemtp_vr_second_list, vr_cnt );
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
