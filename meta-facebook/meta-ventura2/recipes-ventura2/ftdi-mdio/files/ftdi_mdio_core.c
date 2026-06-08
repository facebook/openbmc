/*
 * ftdi_mdio_core.c
 *
 * Core FTDI MDIO/GPIO implementation.
 * Handles USB device open/close, MDIO Clause 22/45 read/write,
 * GPIO get/set, and clock divider configuration.
 */

#include "ftdi_mdio_core.h"

#include <arpa/inet.h>
#include <libftdi1/ftdi.h>
#include <libusb-1.0/libusb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ── globals ── */
LOG_LEVEL g_log_level = LOG_INFO;

struct ftdi_context ftdic;
struct ftdi_device_list* ftdi_list = NULL;
ftdi_gpio_s gpio_s;

/* ── log ── */
void log_print(LOG_LEVEL level, const char* fmt, ...) {
  if (level > g_log_level)
    return;
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

/* ── internal helpers ── */
static int send_buf(struct ftdi_context* ftdi, const uint8_t* buf, int size) {
  int r;
  r = ftdi_write_data(ftdi, (uint8_t*)buf, size);
  if (r < 0) {
    log_print(
        LOG_DEBUG, "ftdi_write_data: %d, %s\n", r, ftdi_get_error_string(ftdi));
    return 1;
  }
  return 0;
}

static int get_buf(struct ftdi_context* ftdi, const uint8_t* buf, int size) {
  int r, i;

  while (size > 0) {
    r = ftdi_read_data(ftdi, (uint8_t*)buf, size);
    if (r < 0) {
      log_print(
          LOG_DEBUG,
          "ftdi_read_data: %d, %s\n",
          r,
          ftdi_get_error_string(ftdi));
      return 1;
    }
    log_print(LOG_DEBUG, "ftdi_read_data retval = %d\n", r);
    if (r > 0) {
      for (i = 0; i < r; i++) {
        log_print(LOG_DEBUG, "buf[%02d]=0x%02x\n", i, buf[i]);
      }
    }
    buf += r;
    size -= r;
  }
  return 0;
}

/* ── MDIO frame bit definitions ── */
/* START */
#define START_BIT_SHIFT 30
#define START_BIT_MASK 0x3
#define START_C45_OPCODE 0x0
#define START_C22_OPCODE 0x1

/* OP */
#define OP_BIT_SHIFT 28
#define OP_BIT_MASK 0x3
#define MDIO_OP_ADDR 0x0
#define MDIO_OP_WRITE 0x1
#define MDIO_OP_READ 0x3
#define MDIO_OP_INC_ADDR 0x2
#define MDIO_OP_READ_C22 0x2

/* PORTID */
#define PORTAD_BIT_SHIFT 23
#define PORTAD_BIT_MASK 0x1F

/* DEVAD */
#define DEVAD_BIT_SHIFT 18
#define DEVAD_BIT_MASK 0x1F

/* REG ADDR */
#define REG_ADDR_C22_BIT_SHIFT 18
#define REG_ADDR_C22_BIT_MASK 0x1F

/* TA */
#define TA_BIT_SHIFT 16
#define TA_BIT_MASK 0x3
#define TA_READ 0x0
#define TA_WRITE 0x2

/* ── MDIO C22 ── */
// clang-format off
/*
*  MDIO Frame Format (Clause 22)
*  ┌──────┬────────────────┬────┬────┬─────────┬─────────┬────┬──────────────┐
*  │ IDLE │   PREAMBLE     │ STR│ OP │  PHYAD  │  REGAD  │ TA │     DATA     │
*  ├──────┼────────────────┼────┼────┼─────────┼─────────┼────┼──────────────┤
*  │      │       32       │  2 │  2 │    5    │    5    │  2 │      16      │
*  └──────┴────────────────┴────┴────┴─────────┴─────────┴────┴──────────────┘
*/
// clang-format on
int mdio_read_c22(uint8_t phyid, uint8_t phyreg, uint16_t* data) {
  int status, i;
  uint8_t buf[12];
  uint32_t reg_val = 0;
  memset(buf, 0xff, sizeof(buf) / sizeof(buf[0]));

  /* MDIO Address write format. */
  reg_val |= ((START_C22_OPCODE & START_BIT_MASK) << START_BIT_SHIFT);
  reg_val |= ((MDIO_OP_READ_C22 & OP_BIT_MASK) << OP_BIT_SHIFT);
  reg_val |= ((phyid & PORTAD_BIT_MASK) << PORTAD_BIT_SHIFT);
  reg_val |= ((phyreg & REG_ADDR_C22_BIT_MASK) << REG_ADDR_C22_BIT_SHIFT);
  reg_val |= ((TA_READ & TA_BIT_MASK) << TA_BIT_SHIFT);

  buf[0] = 0x34;
  buf[1] = 8 - 1; // Length L
  buf[2] = 0x00; // Length H
  buf[3] = 0xff;
  buf[4] = 0xff;
  buf[5] = 0xff;
  buf[6] = 0xff;
  buf[7] = (reg_val >> 24) & 0xff;
  buf[8] = (reg_val >> 16) & 0xff;
  buf[9] = 0xff;
  buf[10] = 0xff;
  buf[11] = 0x87; // Send Immediate

  for (i = 0; i < sizeof(buf) / sizeof(buf[0]); i += 2) {
    log_print(
        LOG_DEBUG,
        "         buf[%02d]=0x%02x buf[%02d]=0x%02x \n",
        i,
        buf[i],
        i + 1,
        buf[i + 1]);
  }

  status = send_buf(&ftdic, buf, sizeof(buf) / sizeof(buf[0]));
  if (status != 0) {
    log_print(LOG_ERROR, "unable to FT_Write\n");
    return -1;
  }

  status = get_buf(&ftdic, &buf[0], 8);
  if (status < 0) {
    log_print(LOG_ERROR, "FT_Read returned\n");
    return -1;
  }

  *data = (buf[6] << 8) | buf[7];
  log_print(LOG_INFO, "read phy 0x%04x [%04x]=%04x\n", phyid, phyreg, *data);
  return 0;
}

int mdio_write_c22(uint8_t phyid, uint8_t phyreg, uint16_t data) {
  int status, i;
  uint8_t buf[12];
  uint32_t reg_val = 0;

  memset(buf, 0xff, sizeof(buf) / sizeof(buf[0]));

  /* MDIO Address write format. */
  reg_val |= ((START_C22_OPCODE & START_BIT_MASK) << START_BIT_SHIFT);
  reg_val |= ((MDIO_OP_WRITE & OP_BIT_MASK) << OP_BIT_SHIFT);
  reg_val |= ((phyid & PORTAD_BIT_MASK) << PORTAD_BIT_SHIFT);
  reg_val |= ((phyreg & REG_ADDR_C22_BIT_MASK) << REG_ADDR_C22_BIT_SHIFT);
  reg_val |= ((TA_WRITE & TA_BIT_MASK) << TA_BIT_SHIFT);

  buf[0] = 0x10;
  buf[1] = 8 - 1; // Length L
  buf[2] = 0x00; // Length H
  buf[3] = 0xff;
  buf[4] = 0xff;
  buf[5] = 0xff;
  buf[6] = 0xff;
  buf[7] = (reg_val >> 24) & 0xff;
  buf[8] = (reg_val >> 16) & 0xff;
  buf[9] = data >> 8;
  buf[10] = data & 0xFF;
  buf[11] = 0x87; // Send Immediate

  for (i = 0; i < sizeof(buf) / sizeof(buf[0]); i += 2) {
    log_print(
        LOG_DEBUG,
        "         buf[%02d]=0x%02x buf[%d]=0x%02x \n",
        i,
        buf[i],
        i + 1,
        buf[i + 1]);
  }

  status = send_buf(&ftdic, buf, sizeof(buf) / sizeof(buf[0]));
  if (status != 0) {
    log_print(LOG_ERROR, "unable to FT_Write\n");
    return -1;
  }

  log_print(LOG_INFO, "write phy 0x%04x [%04x]=%04x\n", phyid, phyreg, data);
  return 0;
}

/* ── MDIO C45 ── */
// clang-format off
/*
*  MDIO Frame Format (Clause 45)
*  ┌──────┬────────────────┬────┬────┬─────────┬─────────┬────┬────────────────┐
*  │ IDLE │   PREAMBLE     │ STR│ OP │  PORTID │  DEVAD  │ TA │  DATA / ADDR   │
*  ├──────┼────────────────┼────┼────┼─────────┼─────────┼────┼────────────────┤
*  │      │       32       │  2 │  2 │    5    │    5    │  2 │        16      │
*  └──────┴────────────────┴────┴────┴─────────┴─────────┴────┴────────────────┘
*/
// clang-format on
int mdio_read_c45(
    uint8_t phyid,
    uint8_t devad,
    uint16_t phyreg,
    uint16_t* data) {
  int status, i;
  uint8_t buf[20];
  uint32_t reg_val = 0;

  memset(buf, 0xff, sizeof(buf) / sizeof(buf[0]));

  /* MDIO Address write format. */
  reg_val |= ((START_C45_OPCODE & START_BIT_MASK) << START_BIT_SHIFT);
  reg_val |= ((MDIO_OP_ADDR & OP_BIT_MASK) << OP_BIT_SHIFT);
  reg_val |= ((phyid & PORTAD_BIT_MASK) << PORTAD_BIT_SHIFT);
  reg_val |= ((devad & DEVAD_BIT_MASK) << DEVAD_BIT_SHIFT);
  reg_val |= ((TA_WRITE & TA_BIT_MASK) << TA_BIT_SHIFT);

  buf[0] = 0x34;
  buf[1] = 16 - 1; // Length L
  buf[2] = 0x00; // Length H
  buf[3] = 0xff;
  buf[4] = 0xff;
  buf[5] = 0xff;
  buf[6] = 0xff;
  buf[7] = (reg_val >> 24) & 0xff;
  buf[8] = (reg_val >> 16) & 0xff;
  buf[9] = (phyreg >> 8) & 0xff;
  buf[10] = phyreg & 0xff;

  /* MDIO read format. */
  reg_val &= ~(OP_BIT_MASK << OP_BIT_SHIFT);
  reg_val |= ((MDIO_OP_READ & OP_BIT_MASK) << OP_BIT_SHIFT);
  reg_val &= ~(TA_BIT_MASK << TA_BIT_SHIFT);
  reg_val |= ((TA_READ & TA_BIT_MASK) << TA_BIT_SHIFT);

  buf[11] = 0xff;
  buf[12] = 0xff;
  buf[13] = 0xff;
  buf[14] = 0xff;
  buf[15] = (reg_val >> 24) & 0xff;
  buf[16] = (reg_val >> 16) & 0xff;
  buf[17] = 0xff;
  buf[18] = 0xff;
  buf[19] = 0x87; // Send Immediate

  for (i = 0; i < sizeof(buf) / sizeof(buf[0]); i += 2) {
    log_print(
        LOG_DEBUG,
        "         buf[%02d]=0x%02x buf[%d]=0x%02x \n",
        i,
        buf[i],
        i + 1,
        buf[i + 1]);
  }

  status = send_buf(&ftdic, buf, sizeof(buf) / sizeof(buf[0]));
  if (status != 0) {
    log_print(LOG_ERROR, "unable to FT_Write\n");
    return -1;
  }

  status = get_buf(&ftdic, &buf[0], 16);
  if (status < 0) {
    log_print(LOG_ERROR, "FT_Read returned\n");
    return -1;
  }

  *data = (buf[14] << 8) | buf[15];
  log_print(LOG_INFO, "read [%04x]=%04x\n", phyreg, *data);
  return 0;
}

int mdio_write_c45(
    uint8_t phyid,
    uint8_t devad,
    uint16_t phyreg,
    uint16_t data) {
  int status, i;
  uint8_t buf[20];
  uint32_t reg_val = 0;

  memset(buf, 0xff, sizeof(buf) / sizeof(buf[0]));

  /* MDIO Address write format. */
  reg_val |= ((START_C45_OPCODE & START_BIT_MASK) << START_BIT_SHIFT);
  reg_val |= ((MDIO_OP_ADDR & OP_BIT_MASK) << OP_BIT_SHIFT);
  reg_val |= ((phyid & PORTAD_BIT_MASK) << PORTAD_BIT_SHIFT);
  reg_val |= ((devad & DEVAD_BIT_MASK) << DEVAD_BIT_SHIFT);
  reg_val |= ((TA_WRITE & TA_BIT_MASK) << TA_BIT_SHIFT);

  buf[0] = 0x10;
  buf[1] = 16 - 1; // Length L
  buf[2] = 0x00; // Length H
  buf[3] = 0xff;
  buf[4] = 0xff;
  buf[5] = 0xff;
  buf[6] = 0xff;
  buf[7] = (reg_val >> 24) & 0xff;
  buf[8] = (reg_val >> 16) & 0xff;
  buf[9] = (phyreg >> 8) & 0xff;
  buf[10] = phyreg & 0xff;

  /* MDIO write format. */
  reg_val &= ~(OP_BIT_MASK << OP_BIT_SHIFT);
  reg_val |= ((MDIO_OP_WRITE & OP_BIT_MASK) << OP_BIT_SHIFT);

  buf[11] = 0xff;
  buf[12] = 0xff;
  buf[13] = 0xff;
  buf[14] = 0xff;
  buf[15] = (reg_val >> 24) & 0xff;
  buf[16] = (reg_val >> 16) & 0xff;
  buf[17] = data >> 8;
  buf[18] = data & 0xFF;
  buf[19] = 0x87; // Send Immediate

  for (i = 0; i < sizeof(buf) / sizeof(buf[0]); i += 2) {
    log_print(
        LOG_DEBUG,
        "         buf[%02d]=0x%02x buf[%d]=0x%02x \n",
        i,
        buf[i],
        i + 1,
        buf[i + 1]);
  }

  status = send_buf(&ftdic, buf, sizeof(buf) / sizeof(buf[0]));
  if (status != 0) {
    log_print(LOG_ERROR, "unable to FT_Write\n");
    return -1;
  }

  return 0;
}

/* ── FTDI clock ── */
int clk_divider_set(uint16_t clk_div) {
  uint8_t buf[3] = {0};
  float clk = 30.0f / (1 + clk_div);
  log_print(
      LOG_DEBUG, "Set clock divider to %d (TCK=%.03fMHz)\n", clk_div, clk);
  buf[0] = 0x86;
  buf[1] = clk_div & 0xFF;
  buf[2] = (clk_div >> 8) & 0xFF;
  if (send_buf(&ftdic, buf, 3) < 0)
    return 1;
  usleep(5000);
  return 0;
}

/* ── FTDI device open / close / search ── */
int device_open(int device, int intf_id) {
  int f;
  int retval = 0;
  unsigned char buf[512];
  enum ftdi_interface ft2232_interface = intf_id;
  struct libusb_device_descriptor dsc;
  struct ftdi_device_list* ftdi_count;
  int ftdi_num = 0, result;
  struct ftdi_context ftdi_tmp;

  if (ftdi_init(&ftdic) < 0) {
    log_print(LOG_ERROR, "ftdi_init failed\n");
    return -1;
  }
  if (ftdi_set_interface(&ftdic, ft2232_interface) < 0) {
    log_print(
        LOG_ERROR,
        "Unable to select channel (%s).\n",
        ftdi_get_error_string(&ftdic));
  }
  if (ftdi_init(&ftdi_tmp) < 0) {
    log_print(LOG_ERROR, "ftdi_init failed\n");
    return -1;
  }

  result = ftdi_usb_find_all(&ftdi_tmp, &ftdi_list, 0, 0);
  memset(&dsc, 0, sizeof(dsc));
  if (result < 0) {
    log_print(
        LOG_ERROR,
        "Error searching ftdi (%d, %s)\n",
        result,
        ftdi_get_error_string(&ftdi_tmp));
    return -1;
  }
  log_print(LOG_DEBUG, "Found %d FTDI Device\n", result);

  ftdi_count = ftdi_list;
  while (ftdi_count) {
    result = libusb_get_device_descriptor(ftdi_count->dev, &dsc);
    if (result == 0 &&
        libusb_get_device_address(ftdi_count->dev) == (uint8_t)device) {
      log_print(
          LOG_DEBUG,
          "BUS:%d PORT:%d ADDR:0x%x VID:0x%04x PID:0x%04x\n",
          libusb_get_bus_number(ftdi_count->dev),
          libusb_get_port_number(ftdi_count->dev),
          libusb_get_device_address(ftdi_count->dev),
          dsc.idVendor,
          dsc.idProduct);
      f = ftdi_usb_open_dev(&ftdic, ftdi_count->dev);
      break;
    }
    ftdi_count = ftdi_count->next;
    ftdi_num++;
  }

  if (f < 0 && f != -5) {
    log_print(
        LOG_ERROR,
        "unable to open ftdi device: %d (%s)\n",
        f,
        ftdi_get_error_string(&ftdic));
    return -1;
  }

  ftdi_usb_reset(&ftdic);
  ftdi_set_latency_timer(&ftdic, 2);
  ftdic.usb_read_timeout = 100;
  ftdic.usb_write_timeout = 5000;

  if (ftdi_set_bitmode(&ftdic, 0x0, 0) < 0) {
    log_print(LOG_ERROR, "Reset MPSSE bitmode failed\n");
    retval = -1;
    goto error_close;
  }
  if (ftdi_set_bitmode(&ftdic, 0x0b, BITMODE_MPSSE) < 0) {
    log_print(LOG_ERROR, "Set MPSSE bitmode failed\n");
    retval = -1;
    goto error_close;
  }

  buf[0] = 0x8a;
  buf[1] = 0x97;
  buf[2] = 0x8d;
  if (send_buf(&ftdic, buf, 3) < 0) {
    retval = -6;
    goto error_close;
  }
  usleep(5000);

  clk_divider_set(59); /* 0.5 MHz default */

  buf[0] = 0x80;
  buf[1] = 0x01;
  buf[2] = 0x03;
  if (send_buf(&ftdic, buf, 3) < 0) {
    retval = -6;
    goto error_close;
  }
  usleep(5000);

  buf[0] = 0x82;
  buf[1] = 0x00;
  buf[2] = 0x00;
  if (send_buf(&ftdic, buf, 3) < 0) {
    retval = -6;
    goto error_close;
  }
  usleep(5000);

  gpio_s.level = 0x0001;
  gpio_s.dir = 0x0003;
  ftdi_list_free(&ftdi_list);
  return 0;

error_close:
  ftdi_usb_close(&ftdic);
  ftdi_list_free(&ftdi_list);
  return retval;
}

int device_close(void) {
  ftdi_usb_close(&ftdic);
  return 0;
}

int device_search(int flag) {
  int num = 1;
  struct ftdi_context ftdi_value;
  struct ftdi_device_list* count;
  struct libusb_device_descriptor dsc;

  if (ftdi_init(&ftdi_value) < 0) {
    log_print(LOG_ERROR, "ftdi_init failed\n");
    return -1;
  }

  int result = ftdi_usb_find_all(&ftdi_value, &ftdi_list, 0, 0);
  memset(&dsc, 0, sizeof(dsc));

  if (result < 0) {
    log_print(
        LOG_ERROR,
        "Error searching ftdi (%d, %s)\n",
        result,
        ftdi_get_error_string(&ftdi_value));
    return -1;
  }
  log_print(LOG_INFO, "\nFound %d FTDI Device\n", result);
  if (!flag)
    log_print(LOG_INFO, "\nNo.\tbus\tport\tdevaddr\n");

  count = ftdi_list;
  while (count) {
    result = libusb_get_device_descriptor(count->dev, &dsc);
    if (result == 0) {
      if (flag) {
        log_print(
            LOG_INFO,
            "\nFTDI_%d: BUS=%d PORT=%d ADDR=0x%x "
            "VID=0x%04x PID=0x%04x\n",
            num,
            libusb_get_bus_number(count->dev),
            libusb_get_port_number(count->dev),
            libusb_get_device_address(count->dev),
            dsc.idVendor,
            dsc.idProduct);
      } else {
        log_print(
            LOG_INFO,
            "%d\t%d\t%d\t%d\n",
            num,
            libusb_get_bus_number(count->dev),
            libusb_get_port_number(count->dev),
            libusb_get_device_address(count->dev));
      }
    }
    count = count->next;
    num++;
  }
  return 0;
}

/* ── FTDI GPIO ── */
int gpio_get() {
  uint8_t cmd, rdata[2];
  memset(rdata, 0, 2);

  /* Read low byte */
  cmd = 0x81;
  if (send_buf(&ftdic, &cmd, 1) != 0) {
    log_print(LOG_ERROR, "send command failed!\n");
    return -1;
  }
  usleep(50000);
  if (get_buf(&ftdic, &rdata[0], 1) != 0) {
    log_print(LOG_ERROR, "get_buf failed!\n");
    return -1;
  }
  usleep(50000);
  /* Read high byte */
  cmd = 0x83;
  if (send_buf(&ftdic, &cmd, 1) != 0) {
    log_print(LOG_ERROR, "send command failed!\n");
    return -1;
  }
  usleep(50000);
  if (get_buf(&ftdic, &rdata[1], 1) != 0) {
    log_print(LOG_ERROR, "get_buf failed!\n");
    return -1;
  }
  gpio_s.level = (rdata[1] << 8) | rdata[0];
  log_print(
      LOG_INFO, "GPIO value 0x%04x (dir 0x%x)\n", gpio_s.level, gpio_s.dir);
  return 0;
}

int gpio_set(uint8_t gpio_num, uint8_t dir, uint8_t value) {
  uint8_t cmd[3];
  if (gpio_num > 15)
    return -1;

  gpio_s.level &= ~(1 << gpio_num);
  gpio_s.level |= (value << gpio_num);
  gpio_s.dir &= ~(1 << gpio_num);
  gpio_s.dir |= (dir << gpio_num);

  if (gpio_num > 7) {
    cmd[0] = 0x82;
    cmd[1] = (uint8_t)((gpio_s.level >> 8) & 0xff);
    cmd[2] = (uint8_t)((gpio_s.dir >> 8) & 0xff);
  } else {
    cmd[0] = 0x80;
    cmd[1] = (uint8_t)(gpio_s.level & 0xff);
    cmd[2] = (uint8_t)(gpio_s.dir & 0xff);
  }
  log_print(
      LOG_INFO,
      "GPIO set %d dir %s value %s (level=0x%x dir=0x%x)\n",
      gpio_num,
      dir ? "out" : "in",
      value ? "High" : "Low",
      gpio_s.level,
      gpio_s.dir);
  return send_buf(&ftdic, cmd, 3);
}
