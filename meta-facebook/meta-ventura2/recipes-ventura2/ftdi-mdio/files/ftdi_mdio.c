#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libftdi1/ftdi.h>
#include <libusb-1.0/libusb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <termio.h>
#include <unistd.h>

typedef enum { LOG_ERROR = 0, LOG_WARN, LOG_INFO, LOG_DEBUG } LOG_LEVEL;

static LOG_LEVEL g_log_level = LOG_INFO;

void log_print(LOG_LEVEL level, const char* fmt, ...) {
  if (level > g_log_level)
    return;

  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

#define FTDI_VID 0x0403
#define FTDI_FT232H_PID 0x6014
#define FTDI_FT2232H_PID 0x6010

#define DEFAULT_DIVISOR 2

#define DEVICEID_MAX 0xFF
#define REGADDR_MAX 0xFFFF
#define WRITE_VALUE_MAX 0xFFFF

typedef enum {
  FTDI_MDIO_OPEN = 1,
  FTDI_GPIO_GET,
  FTDI_GPIO_SET,
  FTDI_MDIO_READ,
  FTDI_MDIO_WRITE,
  FTDI_MDIO_READ_C45,
  FTDI_MDIO_WRITE_C45,
  FTDI_MDIO_SET,
  FTDI_MDIO_CLOSE,
  FTDI_MDIO_LIST,
  FTDI_MDIO_LISTALL,
  FTDI_MDIO_EEPROM,

  END_OF_FUNCLIST
} FTDI_MDIO_ACTION;

struct ftdi_data {
  FTDI_MDIO_ACTION action;
  unsigned int device;
  unsigned int interface;
  unsigned int phyid;
  unsigned int devad;
  unsigned int regaddr;
  unsigned int value;
  unsigned int listall_flag;
  unsigned int gpio_num;
  unsigned int gpio_dir;
  unsigned int clk_div;
};

struct ftdi_context ftdic;
struct ftdi_device_list* ftdi_list = NULL;

typedef struct {
  uint16_t dir;
  uint16_t level;
} ftdi_gpio_s;
static ftdi_gpio_s gpio_s;

static int
send_buf(struct ftdi_context* ftdi, const unsigned char* buf, int size) {
  int r;
  r = ftdi_write_data(ftdi, (unsigned char*)buf, size);
  if (r < 0) {
    log_print(
        LOG_DEBUG, "ftdi_write_data: %d, %s\n", r, ftdi_get_error_string(ftdi));
    return 1;
  }
  return 0;
}

static int
get_buf(struct ftdi_context* ftdi, const unsigned char* buf, int size) {
  int r, i;

  while (size > 0) {
    r = ftdi_read_data(ftdi, (unsigned char*)buf, size);
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
int mdio_read_c22(uint8_t phyid, uint8_t phyreg, volatile uint8_t* value) {
  int status, i;
  unsigned char buf[12];
  uint32_t reg_val = 0;
  uint16_t data = 0xFFFF;

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

  data = (buf[6] << 8) | buf[7];
  log_print(LOG_INFO, "read phy 0x%04x [%04x]=%04x\n", phyid, phyreg, data);

  printf("0x%04X\n", data);
  return 0;
}

int mdio_write_c22(uint8_t phyid, uint8_t phyreg, volatile uint8_t* value) {
  int status, i;
  unsigned char buf[12];
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
  buf[9] = value[1];
  buf[10] = value[0];
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

  log_print(
      LOG_INFO,
      "write phy 0x%04x [%04x]=%04x\n",
      phyid,
      phyreg,
      (value[1] << 8) | value[0]);
  return 0;
}

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
    volatile uint8_t* value) {
  int status, i;
  unsigned char buf[20];
  uint32_t reg_val = 0;
  uint16_t data = 0xFFFF;

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

  data = (buf[14] << 8) | buf[15];
  log_print(LOG_INFO, "read [%04x]=%04x\n", phyreg, data);

  printf("0x%04X\n", data);
  return 0;
}

int mdio_write_c45(
    uint8_t phyid,
    uint8_t devad,
    uint16_t phyreg,
    volatile uint8_t* value) {
  int status, i;
  unsigned char buf[20];
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
  buf[17] = value[1];
  buf[18] = value[0];
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

int clk_divider_set(uint16_t clk_div)
{
  uint8_t buf[3] = {0};
  log_print(LOG_DEBUG, "Send TCK frequency commands\n");
  log_print(LOG_DEBUG, "Set clock divider to %d\n", clk_div);
  float clk = 30.0 / (1+clk_div);
  log_print(LOG_DEBUG, "TCK = 30MHz/%d = %.03fMHz\n", 1+clk_div, clk);
  buf[0] = 0x86;
  buf[1] = clk_div & 0xFF;
  buf[2] = (clk_div >> 8) & 0xFF;
  if (send_buf(&ftdic, buf, 3) < 0) {
    return 1;
  }
  usleep(5000);
  return 0;
}

/*******************************************************************************
 PURPOSE: Function to connect to the board through USB
 COMMENT:
*******************************************************************************/
int device_open(int device, int intf_id) {
  int f;
  int retval = 0;
  unsigned char buf[512];
  enum ftdi_interface ft2232_interface = intf_id;
  // unsigned int dwClockDivisor = 0x05DB;
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

  // Open ftdi_tmp for find all ftdi device
  if (ftdi_init(&ftdi_tmp) < 0) {
    log_print(LOG_ERROR, "ftdi_init failed\n");
    return -1;
  }

  // find all ftdi device and put libusb_device to ftdi_list
  result = ftdi_usb_find_all(&ftdi_tmp, &ftdi_list, 0, 0);
  memset(&dsc, 0, sizeof(dsc));

  // if search all ftdi fail
  if (result < 0) {
    log_print(
        LOG_ERROR,
        "Error occur when searching ftdi (Error : %d , %s)\n",
        result,
        ftdi_get_error_string(&ftdi_tmp));
    return -1;
  } else {
    log_print(LOG_DEBUG, "Found %d FTDI Device\n", result);
  }

  ftdi_count = ftdi_list;
  while (ftdi_count) {
    result = libusb_get_device_descriptor(ftdi_count->dev, &dsc);
    if (result != 0) {
      log_print(
          LOG_ERROR,
          "\n Get Device Descriptor fail with FTDI_%d Device (%d)!\n",
          ftdi_num + 1,
          result);
    } else {
      if (libusb_get_device_address(ftdi_count->dev) == device) {
        log_print(
            LOG_DEBUG,
            "Get ftdi device with device address %03d succeeded.\n",
            device);
        log_print(LOG_DEBUG, "\nGet FTDI Device info :\n");
        log_print(
            LOG_DEBUG,
            "BUS number : %d , Port number : %d , Device Address : 0x%x\n",
            libusb_get_bus_number(ftdi_count->dev),
            libusb_get_port_number(ftdi_count->dev),
            libusb_get_device_address(ftdi_count->dev));
        log_print(
            LOG_DEBUG,
            "idVendor : 0x%04x , idProduct : 0x%04x\n",
            dsc.idVendor,
            dsc.idProduct);

        f = ftdi_usb_open_dev(&ftdic, ftdi_count->dev);
        break;
      }
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
    retval = 1;
    return -1;
  }
  log_print(LOG_DEBUG, "\nftdi open succeeded: %d\n", f);
  if (ftdi_usb_reset(&ftdic) < 0) {
    log_print(
        LOG_ERROR,
        "Unable to reset FTDI device (%s).\n",
        ftdi_get_error_string(&ftdic));
  }

  if (ftdi_set_latency_timer(&ftdic, 2) < 0) {
    log_print(
        LOG_ERROR,
        "Unable to set latency timer (%s).\n",
        ftdi_get_error_string(&ftdic));
  }

  // Set timeout
  log_print(
      LOG_DEBUG,
      "ftdi original timeout: read:%d write:%d\n",
      ftdic.usb_read_timeout,
      ftdic.usb_write_timeout);
  ftdic.usb_read_timeout = 100;
  ftdic.usb_write_timeout = 5000;
  log_print(
      LOG_DEBUG,
      "Set timeout to read_timeout:%d write_timout:%d\n",
      ftdic.usb_read_timeout,
      ftdic.usb_write_timeout);

  log_print(LOG_DEBUG, "enabling bitbang mode\n");
  if (ftdi_set_bitmode(&ftdic, 0x0, 0) < 0) {
    log_print(LOG_ERROR, "Reset MPSSE bitmode failed!!\n");
    retval = -1;
    goto error_close;
  }
  if (ftdi_set_bitmode(&ftdic, 0x0b, BITMODE_MPSSE) < 0) {
    log_print(LOG_ERROR, "Set MPSSE bitmode failed!!\n");
    retval = -1;
    goto error_close;
  }

  log_print(LOG_DEBUG, "Send clock settings commands\n");
  buf[0] =
      0x8a; // Disables the clk divide by 5 to allow for a 60MHz master clock
  buf[1] = 0x97; // Disable adaptive clocking
  buf[2] = 0x8d; // Disables 3 phase data clocking
  if (send_buf(&ftdic, buf, 3) < 0) {
    retval = -6;
    goto error_close;
  }
  usleep(5000);

  clk_divider_set(59);  // 0.5 MHz

  log_print(
      LOG_DEBUG,
      " Setup the direction of the first 8 lines and force a value\n");
  buf[0] = 0x80; // Setup the direction of the first 8 lines and force a value
  buf[1] = 0x01; // pin value setting - low (0)/high (1)
  buf[2] = 0x03; // pin direction setting - input (0)/output (1)
  if (send_buf(&ftdic, buf, 3) < 0) {
    retval = -6;
    goto error_close;
  }
  usleep(5000);

  log_print(
      LOG_DEBUG,
      " Setup the direction of the high 8 lines and force a value\n");
  buf[0] = 0x82; // Setup the direction of the high 8 lines and force a value
  buf[1] = 0x00; // pin value setting - low (0)/high (1)
  buf[2] = 0x00; // pin direction setting - input (0)/output (1)
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
  log_print(LOG_ERROR, "error close!!!\n");
  ftdi_usb_close(&ftdic);
  ftdi_list_free(&ftdi_list);
  return retval;
}

/*******************************************************************************
 PURPOSE: Function to Search FTDI Device on the usb bus
                    Default Search 0x403:0x6001
                                                 0x403:0x6010
                                                 0x403:0x6011
                                                 0x403:0x6014
                                                 0x403:0x6015
 COMMENT:
*******************************************************************************/
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
        "Error occur when searching ftdi (Error : %d , %s)\n",
        result,
        ftdi_get_error_string(&ftdi_value));
    return -1;
  } else {
    log_print(LOG_INFO, "\nFound %d FTDI Device\n", result);
    if (!flag) {
      log_print(LOG_INFO, "\nNo.\tbus\tport\tdevaddr\n");
    }
  }

  count = ftdi_list;
  while (count) {
    result = libusb_get_device_descriptor(count->dev, &dsc);
    if (result != 0) {
      log_print(LOG_ERROR, "\n Get Device Descriptor fild (%d)\n", result);
    } else {
      if (flag) {
        log_print(LOG_INFO, "\nGet FTDI_%d Device info :\n", num);
        log_print(
            LOG_INFO, "BUS number : %d\n", libusb_get_bus_number(count->dev));
        log_print(
            LOG_INFO, "Port number : %d\n", libusb_get_port_number(count->dev));
        log_print(
            LOG_INFO,
            "Device Address : 0x%x\n",
            libusb_get_device_address(count->dev));
        log_print(
            LOG_INFO,
            "bcdUSB : 0x%04x , bDeviceClass : 0x%02x\n",
            dsc.bcdUSB,
            dsc.bDeviceClass);
        log_print(
            LOG_INFO,
            "idVendor : 0x%04x , idProduct : 0x%04x\n",
            dsc.idVendor,
            dsc.idProduct);
        log_print(
            LOG_INFO,
            "bcdDevice : 0x%04x , iManufacturer : 0x%02x\n",
            dsc.bcdDevice,
            dsc.iManufacturer);
        log_print(
            LOG_INFO,
            "bDeviceSubClass : 0x%02x , bNumConfigurations : 0x%02x\n",
            dsc.bDeviceSubClass,
            dsc.bNumConfigurations);
        log_print(LOG_INFO, "iSerialNumber : 0x%02x\n\n", dsc.iSerialNumber);
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

int gpio_get(uint8_t gpio_num_in) {
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
  gpio_s.level = ((rdata[1] << 8) | rdata[0]);

  log_print(
      LOG_INFO,
      "\n\n======================    GPIO Get Value    ======================\n\n");
  log_print(
      LOG_INFO,
      "GPIO value 0x%04x (gpio_s.level 0x%x, gpio_s.dir 0x%x)\n",
      ((rdata[1] << 8) | rdata[0]),
      gpio_s.level,
      gpio_s.dir);
  log_print(
      LOG_INFO,
      "==============================================================\n\n");

  return 0;
}

int gpio_set(uint8_t gpio_num, uint8_t dir, uint8_t value) {
  uint8_t cmd[3];
  log_print(LOG_ERROR, "GPIO_num: %d\n", gpio_num);
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
      "======================    GPIO Set Value    ======================\n\n");
  log_print(
      LOG_INFO,
      "GPIO set %d dir %s value %s (0x%x, 0x%x)\n\n",
      gpio_num,
      dir ? "out" : "in",
      value ? "High" : "Low",
      gpio_s.level,
      gpio_s.dir);
  log_print(
      LOG_INFO,
      "==============================================================\n\n");

  return send_buf(&ftdic, cmd, 3);
}

/*******************************************************************************
 PURPOSE: Function to set MDIO configurations
 COMMENT:
*******************************************************************************/
int device_settings(void) {
  return 0;
}

/*******************************************************************************
 PURPOSE: Function to close the USB handlers
 COMMENT:
*******************************************************************************/
int device_close(void) {
  // log_print(LOG_ERROR, "disabling bitbang mode\n");
  // ftdi_disable_bitbang(&ftdic);

  ftdi_usb_close(&ftdic);
  return 0;
}

static void showusage(int cmd) {
  switch (cmd) {
    case FTDI_GPIO_GET:
      printf("Usage:\n");
      printf("\tftdi_mdio gpioget -d <0-255> -i <1|2> -n <0-15>\n\n");
      printf("\t    -d : USB device address (0–255)\n");
      printf(
          "\t             This is the 'devaddr' value from    \"ftdi_mdio list\".\n");
      printf("\t    -i : interface (1 ~ 4)\n");
      printf("\t    -n : GPIO number\n");
      break;
    case FTDI_GPIO_SET:
      printf("Usage:\n");
      printf(
          "\tftdi_mdio gpioset -d <0-255> -i <1|2> -n <0-15> -w <0|1> -l <0|1>    \n\n");
      printf("\t    -d : USB device address (0–255)\n");
      printf(
          "\t             This is the 'devaddr' value from    \"ftdi_mdio list\".\n");
      printf("\t    -i : interface (1 ~ 4)\n");
      printf("\t    -n : GPIO number\n");
      printf("\t    -w : direction (0:input; 1:output)\n");
      printf("\t    -l : level (0:low; 1:high)\n\n");
      break;
    case FTDI_MDIO_READ:
      printf("Usage:\n");
      printf(
          "\tftdi_mdio read -d <0-255> -i <1-4> -p <0-31> -r <0x0000-0xFFFF>    \n\n");
      printf("\t    -d : USB device address (0–255)\n");
      printf(
          "\t             This is the 'devaddr' value from    \"ftdi_mdio list\".\n");
      printf("\t    -i : FTDI interface (1 ~ 4)\n");
      printf("\t    -p : PHY ID (0 - 31)\n");
      printf("\t    -r : regaddr (0x0000-0xFFFF)\n");
      printf("\t    --clkdiv: value (0x0000-0xFFFF\n");
      printf("\t          TCK period = 30MHz / ( 1 + clkdiv)\n");
      break;
    case FTDI_MDIO_WRITE:
      printf("Usage:\n");
      printf(
          "\tftdi_mdio write -d <0-255> -i <1-4> -p <0-31> -r <0x0000-0xFFFF> -v <0x0000-0xFFFF>\n");
      printf("\t    -d : USB device address (0–255)\n");
      printf(
          "\t             This is the 'devaddr' value from    \"ftdi_mdio list\".\n");
      printf("\t    -i : FTDI interface (1 ~ 4)\n");
      printf("\t    -p : PHY ID (0 - 31)\n");
      printf("\t    -r : regaddr (0x0000-0xFFFF)\n");
      printf("\t    -v : value (0x0000-0xFFFF)\n\n");
      printf("\t    --clkdiv: value (0x0000-0xFFFF\n");
      printf("\t          TCK period = 30MHz / ( 1 + clkdiv)\n");
      break;
    case FTDI_MDIO_READ_C45:
      printf("Usage:\n");
      printf(
          "\tftdi_mdio read_c45 -d <0-255> -i <1-4> -p <0-31> -da <0-31> -r <0x0000-0xFFFF>    \n\n");
      printf("\t    -d : USB device address (0–255)\n");
      printf(
          "\t             This is the 'devaddr' value from    \"ftdi_mdio list\".\n");
      printf("\t    -i : FTDI interface (1 ~ 4)\n");
      printf("\t    -p : PHY ID (0 - 31)\n");
      printf("\t    -da : DEVAD (0 - 31)\n");
      printf("\t    -r : regaddr (0x0000-0xFFFF)\n");
      printf("\t    --clkdiv: value (0x0000-0xFFFF\n");
      printf("\t          TCK period = 30MHz / ( 1 + clkdiv)\n");
      break;
    case FTDI_MDIO_WRITE_C45:
      printf("Usage:\n");
      printf(
          "\tftdi_mdio write_c45 -d <0-255> -i <1-4> -p <0-31> -da <0-31> -r <0x0000-0xFFFF> -v <0x0000-0xFFFF>\n");
      printf("\t    -d : USB device address (0–255)\n");
      printf(
          "\t             This is the 'devaddr' value from    \"ftdi_mdio list\".\n");
      printf("\t    -i : FTDI interface (1 ~ 4)\n");
      printf("\t    -p : PHY ID (0 - 31)\n");
      printf("\t    -da : DEVAD (0 - 31)\n");
      printf("\t    -r : regaddr (0x0000-0xFFFF)\n");
      printf("\t    -v : value (0x0000-0xFFFF)\n\n");
      printf("\t    --clkdiv: value (0x0000-0xFFFF\n");
      printf("\t          TCK period = 30MHz / ( 1 + clkdiv)\n");
      break;
    case FTDI_MDIO_EEPROM:
      printf("\tftdi_mdio eeprom -d <0-255> -i <1|2>\n");
      printf("\t    -d : USB device address (0–255)\n");
      printf(
          "\t             This is the 'devaddr' value from    \"ftdi_mdio list\".\n");
      printf("\t    -i : interface (1 or 2)\n");
      break;
    case FTDI_MDIO_LIST:
    // case FTDI_MDIO_CLOSE:
    case FTDI_MDIO_SET:
    default:
      printf("\nUsage:\n");
      printf("\tftdi_mdio COMMAND <options>\n");
      printf("\ncommand has:\n");
      // printf("\topen       - Open FTDI device\n");
      printf("\tset         - Set MDIO configurations\n");
      printf("\tread        - MDIO C22 read with FTDI device\n");
      printf("\twrite       - MDIO C22 write with FTDI device\n");
      printf("\tread_c45    - MDIO C45 read with FTDI device\n");
      printf("\twrite_c45   - MDIO C45 write with FTDI device\n");
      // printf("\tclose     - Close FTDI device\n");
      printf("\tlist        - List FTDI device information\n");
      printf("\tlistall     - List all FTDI device detail information\n");
      printf("\tgpioget     - Get GPIO value\n");
      printf("\tgpioset     - Set GPIO value\n");
      printf("\teeprom      - Show EEPROM Info\n");
      printf("\noptions:\n");
      printf("\t--debug     - Enable detailed debug output\n");
      printf("\t--quiet     - Suppress non-error messages\n");
      break;
  }
}

#define DEBUG 1
#define CMD_STR_OPEN "open"
#define CMD_STR_CLOSE "close"
#define CMD_STR_SET "setting"

static int
process_arguments(int argc, char** argv, struct ftdi_data* ctrl_data) {
  int i = 1;
  char buf[32];
  long devid, phyid, devad, regaddr, value;

  memset(buf, 0, sizeof(buf));

  while (i < argc) {
    if (strcmp(argv[i], "open") == 0) {
      ctrl_data->action = FTDI_MDIO_OPEN;
    } else if (strcmp(argv[i], "gpioget") == 0) {
      if (argc < 8) {
        showusage(FTDI_GPIO_GET);
        exit(0);
      }
      ctrl_data->action = FTDI_GPIO_GET;
    } else if (strcmp(argv[i], "gpioset") == 0) {
      if (argc < 12) {
        showusage(FTDI_GPIO_SET);
        exit(0);
      }
      ctrl_data->action = FTDI_GPIO_SET;
    } else if (strcmp(argv[i], "read") == 0) {
      if (argc < 10) {
        showusage(FTDI_MDIO_READ);
        exit(0);
      }
      ctrl_data->action = FTDI_MDIO_READ;
    } else if (strcmp(argv[i], "write") == 0) {
      if (argc < 12) {
        showusage(FTDI_MDIO_WRITE);
        exit(0);
      }
      ctrl_data->action = FTDI_MDIO_WRITE;
    } else if (strcmp(argv[i], "read_c45") == 0) {
      if (argc < 10) {
        showusage(FTDI_MDIO_READ_C45);
        exit(0);
      }
      ctrl_data->action = FTDI_MDIO_READ_C45;
    } else if (strcmp(argv[i], "write_c45") == 0) {
      if (argc < 12) {
        showusage(FTDI_MDIO_WRITE_C45);
        exit(0);
      }
      ctrl_data->action = FTDI_MDIO_WRITE_C45;
    } else if (strcmp(argv[i], "set") == 0) {
      ctrl_data->action = FTDI_MDIO_SET;
    }
#if 0
    else if( strcmp( argv[ i ], "close" ) == 0 )
    {
            ctrl_data->action = FTDI_MDIO_CLOSE;
    }
#endif
    else if (strcmp(argv[i], "list") == 0) {
      ctrl_data->action = FTDI_MDIO_LIST;
    } else if (strcmp(argv[i], "listall") == 0) {
      ctrl_data->action = FTDI_MDIO_LIST;
      ctrl_data->listall_flag = 1;
    } else if (strcmp(argv[i], "eeprom") == 0) {
      if (argc < 6) {
        showusage(FTDI_MDIO_EEPROM);
        exit(0);
      }
      ctrl_data->action = FTDI_MDIO_EEPROM;
    } else if (strcmp(argv[i], "-d") == 0) {
      i++;
      devid = strtol(argv[i], NULL, 0);
      if (devid > DEVICEID_MAX || devid < 0) {
        log_print(LOG_ERROR, "Error: USB device address is out of range\n");
        return -1;
      }
      ctrl_data->device = (unsigned short)devid;
    } else if (strcmp(argv[i], "-i") == 0) {
      i++;
      ctrl_data->interface = (unsigned short)strtol(argv[i], NULL, 0);
      if (ctrl_data->interface > INTERFACE_D || ctrl_data->interface < INTERFACE_A) {
        log_print(LOG_ERROR, "Error: interface is out of range\n");
        return -1;
      }
    } else if (strcmp(argv[i], "-p") == 0) {
      i++;
      phyid = strtol(argv[i], NULL, 0);
      if (phyid < 0 || phyid > 31) {
        log_print(LOG_ERROR, "\n Wrong PHY id, please set with (0 - 31) \n");
        return -1;
      }
      ctrl_data->phyid = (unsigned short)phyid;
    } else if (strcmp(argv[i], "-da") == 0) {
      i++;
      devad = strtol(argv[i], NULL, 0);
      if (devad < 0 || devad > 31) {
        log_print(LOG_ERROR, "\n Wrong DEVAD, please set with (0 - 31) \n");
        return -1;
      }
      ctrl_data->devad = (unsigned short)devad;
    } else if (strcmp(argv[i], "-r") == 0) {
      i++;
      regaddr = strtol(argv[i], NULL, 0);
      if (regaddr > REGADDR_MAX || regaddr < 0) {
        log_print(LOG_ERROR, "Error: regaddr is out of range\n");
        return -1;
      }
      ctrl_data->regaddr = (unsigned short)regaddr;

    } else if (strcmp(argv[i], "-v") == 0) {
      i++;
      value = strtol(argv[i], NULL, 0);
      if (value > WRITE_VALUE_MAX || value < 0) {
        log_print(LOG_ERROR, "Error: writting value is out of range\n");
        return -1;
      }
      ctrl_data->value = (unsigned short)value;

    } else if (strcmp(argv[i], "-w") == 0) {
      i++;
      if (strtol(argv[i], NULL, 10) > 1 || strtol(argv[i], NULL, 10) < 0) {
        log_print(LOG_ERROR, "Wrong direction, please set 0 or 1\n");
        return -1;
      }
      ctrl_data->gpio_dir = strtol(argv[i], NULL, 10);

    } else if (strcmp(argv[i], "-n") == 0) {
      i++;
      if (strtol(argv[i], NULL, 10) > 15 || strtol(argv[i], NULL, 10) < 0) {
        log_print(LOG_ERROR, "Wrong GPIO number, please set 0-15\n");
        return -1;
      }
      ctrl_data->regaddr = strtol(argv[i], NULL, 10);
    } else if (strcmp(argv[i], "-l") == 0) {
      i++;
      if (strtol(argv[i], NULL, 10) > 1 || strtol(argv[i], NULL, 10) < 0) {
        log_print(LOG_ERROR, "Wrong GPIO level, please set 0 or 1\n");
        return -1;
      }
      ctrl_data->value = strtol(argv[i], NULL, 10);
    } else if (strcmp(argv[i], "--clkdiv") == 0) {
      i++;
      if (strtol(argv[i], NULL, 10) > 0xFFFF || strtol(argv[i], NULL, 10) < 0) {
        log_print(LOG_ERROR, "Wrong clock divider, please set 0 - 0xFFFF\n");
        return -1;
      }
      ctrl_data->clk_div = strtol(argv[i], NULL, 10);

    } else if (strcmp(argv[i], "--debug") == 0) {
      g_log_level = LOG_DEBUG;
    } else if (strcmp(argv[i], "--quiet") == 0) {
      g_log_level = LOG_ERROR;
    }

    else {
      return -1;
    }
    i++;
  }

  return 0;
}

int main(int argc, char* argv[]) {
  unsigned char buf[32];
  int open_ret = -1;

  int ret = -1;
  struct ftdi_data ctrl_data;
  memset(&ctrl_data, 0, sizeof(struct ftdi_data));
  memset(&gpio_s, 0, sizeof(ftdi_gpio_s));
  memset(buf, 0, 32);
  ctrl_data.clk_div = 0xFFFFFFFF;

  if (argc < 2) {
    showusage(0);
    exit(0);
  }

  ret = process_arguments(argc, argv, &ctrl_data);
  if ((END_OF_FUNCLIST == ctrl_data.action) || ret < 0) {
    showusage(END_OF_FUNCLIST);
    exit(0);
  }

  switch (ctrl_data.action) {
    case FTDI_MDIO_READ:
    case FTDI_MDIO_WRITE:
    case FTDI_MDIO_READ_C45:
    case FTDI_MDIO_WRITE_C45:
      open_ret = device_open(ctrl_data.device, ctrl_data.interface);
      if (open_ret < 0) {
        log_print(LOG_ERROR, "Error: device_open failed!    \n\n");
        exit(0);
      }
      if(ctrl_data.clk_div!=0xFFFFFFFF){
        clk_divider_set(ctrl_data.clk_div);
      }
    break;
    default:
    break;
  }

  switch (ctrl_data.action) {
    case FTDI_MDIO_LIST:
      device_search(ctrl_data.listall_flag);
      break;
#if 0
    case FTDI_MDIO_CLOSE:
            log_print(LOG_ERROR, "\n\n Close FTDI Device....\n");
            device_close();
            break;
#endif
    case FTDI_GPIO_GET:
      open_ret = device_open(ctrl_data.device, ctrl_data.interface);
      if (open_ret < 0) {
        log_print(LOG_ERROR, "Error: device_open failed!    \n\n");
        exit(0);
      }
      gpio_get(ctrl_data.regaddr);
      device_close();
      break;
    case FTDI_GPIO_SET:
      open_ret = device_open(ctrl_data.device, ctrl_data.interface);
      if (open_ret < 0) {
        log_print(LOG_ERROR, "Error: device_open failed!    \n\n");
        exit(0);
      }
      gpio_get(ctrl_data.regaddr);
      gpio_set(ctrl_data.regaddr, ctrl_data.gpio_dir, ctrl_data.value);
      gpio_get(ctrl_data.regaddr);
      device_close();
      break;
    case FTDI_MDIO_SET:
      log_print(LOG_ERROR, "\n\n Set MDIO configurations....\n");
      device_settings();
      break;
    case FTDI_MDIO_READ:
      mdio_read_c22(ctrl_data.phyid, ctrl_data.regaddr, buf);
      device_close();
      break;
    case FTDI_MDIO_WRITE:
      mdio_write_c22(
          ctrl_data.phyid,
          ctrl_data.regaddr,
          (volatile uint8_t*)&ctrl_data.value);
      device_close();
      break;
    case FTDI_MDIO_READ_C45:
      mdio_read_c45(ctrl_data.phyid, ctrl_data.devad, ctrl_data.regaddr, buf);
      device_close();
      break;
    case FTDI_MDIO_WRITE_C45:
      mdio_write_c45(
          ctrl_data.phyid,
          ctrl_data.devad,
          ctrl_data.regaddr,
          (volatile uint8_t*)&ctrl_data.value);
      device_close();
      break;
    case FTDI_MDIO_EEPROM:
      open_ret = device_open(ctrl_data.device, ctrl_data.interface);
      if (open_ret < 0) {
        log_print(LOG_ERROR, "Error: device_open failed!    \n\n");
        exit(0);
      }
      ftdi_read_eeprom(&ftdic);
      ftdi_eeprom_decode(&ftdic, 1);

      device_close();
      break;

    default:
      showusage(0);
      exit(0);
  }
  exit(0);
}
