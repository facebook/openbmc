/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <openbmc/obmc-sensor.h>
#include <openbmc/kv.h>
#include <facebook/bic.h>
#include "pal.h"

#define BIT(value, index) ((value >> index) & 1)

#define FBTTN_PLATFORM_NAME "FBTTN"
#define LAST_KEY "last_key"
#define MAX_NUM_SLOTS 1
#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define GPIO_DIR "/sys/class/gpio/gpio%d/direction"

/*For Triton with GPIO Table 0.5
 * BMC_TO_EXP_RESET     GPIOA2   2  //Reset EXP
 * PWRBTN_OUT_N         GPIOD2   26 //power button signal input
 * COMP_PWR_BTN_N       GPIOD3   27 //power button signal output
 * RSTBTN_OUT_N         GPIOD4   28  //input
 * SYS_RESET_N_OUT      GPIOD5   29  //output
 * COMP_RST_BTN_N       GPIOAA0  208 //Dedicate reset Mono Lake
 * SCC_STBY_PWR_EN      GPIOF4   44 //Enable SCC STBY pwr
 * SCC_LOC_FULL_PWR_EN  GPIOF0   40 //For control SCC Power sequence
 * SCC_RMT_FULL_PWR_EN  GPIOF1   41 //
 * COMP_POWER_FAIL_N    GPIOO6   118
 * COMP_PWR_EN          GPIOO7   119
 * P12V_A_PGOOD         GPIOF6   46 //Whole system stby pwr good
 * IOM_FULL_PWR_EN      GPIOAA7  215
 * IOM_FULL_PGOOD       GPIOAB1  217 // EVT: GPIOAB2(218); DVT: GPIOAB1(217)
 * BMC_LOC_HEARTBEAT    GPIOO1   113
 * UART_SEL_IN          GPIOS1   145 // input; 0: UART_SEL_SERVER; 1: UART_SEL_BMC
 * DB_PRSNT_BMC_N       GPIOQ6   134
 * SYS_PWR_LED          GPIOA3   3
 * ENCL_FAULT_LED       GPIOO3   115
 * BOARD_REV_0          GPIOJ0   72  // GPIOJ[0:2] = 000 is EVT
 * BOARD_REV_1          GPIOJ1   73  // GPIOJ[0:2] = 001 is DVT
 * BOARD_REV_2          GPIOJ2   74  // GPIOJ[0:2] = 010 is MP
 * IOM_TYPE0            GPIOJ4   76
 * IOM_TYPE1            GPIOJ5   77
 * IOM_TYPE2            GPIOJ6   78
 * IOM_TYPE3            GPIOJ7   79
 * DEBUG_PWR_BTN_N      GPIOP3   123
 * DEBUG_RST_BTN_N      GPIOZ0   200
 * */

//Update at 9/12/2016 for Triton
#define GPIO_PWR_BTN 26
#define GPIO_PWR_BTN_N 27
#define GPIO_RST_BTN 28
#define GPIO_SYS_RST_BTN 29   // active by low pulse
#define GPIO_COMP_PWR_EN 119  // computer power enable. GPO. 1: enable
#define GPIO_IOM_FULL_PWR_EN 215
#define GPIO_SCC_RMT_TYPE_0 47
#define GPIO_SLOTID_0 48
#define GPIO_SLOTID_1 49
#define GPIO_IOM_TYPE0 76
#define GPIO_IOM_TYPE1 77
#define GPIO_IOM_TYPE2 78
#define GPIO_IOM_TYPE3 79

#define GPIO_HB_LED 113
#define GPIO_PWR_LED 3
#define GPIO_ENCL_FAULT_LED 115
#define GPIO_BMC_ML_PWR_YELLOW_LED_N 201
#define GPIO_BMC_ML_PWR_BLUE_LED 202

#define BMC_EXT1_LED_Y_N 37
#define BMC_EXT2_LED_Y_N 39

#define UART_SEL_IN 145

#define GPIO_POSTCODE_0 56
#define GPIO_POSTCODE_1 57
#define GPIO_POSTCODE_2 58
#define GPIO_POSTCODE_3 59
#define GPIO_POSTCODE_4 60
#define GPIO_POSTCODE_5 61
#define GPIO_POSTCODE_6 62
#define GPIO_POSTCODE_7 63

#define GPIO_DBG_CARD_PRSNT 134
#define GPIO_SCC_A_INS 478        // 0: Present; 1: Absent
#define GPIO_SCC_B_INS 479        // 0: Present; 1: Absent

#define GPIO_BMC_READY_N    28
#define GPIO_BIC_READY_N    55   // GPIOG7: I2C_COMP_ALERT_N

#define GPIO_CHASSIS_INTRUSION  487
#define GPIO_PCIE_RESET 203

#define GPIO_BOARD_REV_0    72
#define GPIO_BOARD_REV_1    73
#define GPIO_BOARD_REV_2    74

#define GPIO_DEBUG_PWR_BTN_N 123
#define GPIO_DEBUG_RST_BTN_N 200

#define PAGE_SIZE  0x1000
#define AST_SCU_BASE 0x1e6e2000
#define PIN_CTRL1_OFFSET 0x80
#define PIN_CTRL2_OFFSET 0x84
#define SCU_RST_STS_OFFSET 0x3C   // SCU3C: System reset control/status

//#define UART1_TXD (1 << 22)

#define DELAY_GRACEFUL_SHUTDOWN 1
#define DELAY_POWER_OFF 6
#define DELAY_POWER_CYCLE 10
#define DELAY_12V_CYCLE 5

#ifdef CONFIG_FBTTN
#define DELAY_FULL_POWER_DOWN 3
#define RETRY_COUNT 5
#endif

#define CRASHDUMP_BIN       "/usr/local/bin/dump.sh"
#define CRASHDUMP_FILE      "/mnt/data/crashdump_"

#define LARGEST_DEVICE_NAME 120
#define PWM_DIR "/sys/devices/platform/ast_pwm_tacho.0"
#define PWM_UNIT_MAX 100

#define TACH_RPM "/sys/devices/platform/ast_pwm_tacho.0/tacho%d_rpm"
#define TACH_BMC_RMT_HB 0
#define TACH_SCC_LOC_HB 4
#define TACH_SCC_RMT_HB 5

#define PLATFORM_FILE "/tmp/system.bin"
#define ERR_CODE_FILE "/tmp/error_code.bin"
#define BOOT_LIST_FILE "/tmp/boot_list.bin"
#define FIXED_BOOT_DEVICE_FILE "/tmp/fixed_boot_device.bin"
#define BIOS_DEFAULT_SETTING_FILE "/tmp/bios_default_setting.bin"
#define LAST_BOOT_TIME "/tmp/last_boot_time.bin"

#define AST_POR_FLAG "/tmp/ast_por"
// SHIFT to 16
#define UART1_TXD 0

unsigned char g_err_code[ERROR_CODE_NUM];

 /* For fbttn Power Sequence (controlled by HW)
  * 1.       Input  : SCC_STBY_PWR_EN
  * 2. Check Input  : SCC_STBY_PWR_GOOD     // OVER GPIO_EXP
  * 3.       Input  : SCC_LOC_FULL_PWR_EN
  * 4. Check Input  : SCC_LOC_FULL_PWR_GOOD // OVER GPIO_EXP
  * 5.       Input  : IOM_FULL_PWR_EN
  * 6. Check Input  : IOM_FULL_PGOOD
  * After BMC ready (controlled by BMC)
  * 7.       Output : SCC_RMT_FULL_PWR_EN   // type 7 only
  * 8.       Output : COMP_PWR_EN
  * 9.       Output : COMP_PWR_BTN_N        // is_server_prsnt = true
*/
const static uint8_t gpio_rst_btn[] = { 0, GPIO_SYS_RST_BTN };
const static uint8_t gpio_led[] = { 0, GPIO_PWR_LED };      // System power LED (Blue color, on front panel)
const static uint8_t gpio_id_led[] = { 0,  GPIO_PWR_LED };  // Identify LED (System power LED)
//const static uint8_t gpio_prsnt[] = { 0, 61 };
//const static uint8_t gpio_bic_ready[] = { 0, 107 };
const static uint8_t gpio_power_btn[] = { 0, GPIO_PWR_BTN_N };
const static uint8_t gpio_12v[] = { 0, GPIO_COMP_PWR_EN };
const char pal_fru_list[] = "all, server, iom, dpb, scc, nic";
const char pal_fru_list_print[] = "all, server, iom, dpb, scc"; // Cannot read fruid from "nic"
const char pal_fru_list_rw[] = "server, iom"; // Cannot write fruid to "scc" and "dpb"
const char pal_server_list[] = "server";
const char pal_fru_list_sensor_history[] = "all, server, iom, nic"; // Cannot show sensor history to "scc" and "dpb"

size_t pal_pwm_cnt = 2;
size_t pal_tach_cnt = 8;
const char pal_pwm_list[] = "0, 1";
const char pal_tach_list[] = "0...7";
uint8_t fanid2pwmid_mapping[] = {1, 1, 0, 0, 0, 0, 1, 1};

static uint8_t bios_default_setting_timer_flag = 0;
static uint8_t otp_server_12v_off_flag = 0;


char * key_list[] = {
"pwr_server1_last_state",
"sysfw_ver_slot1",
"identify_slot1",
"timestamp_sled",
"slot1_por_cfg",
"slot1_sensor_health",
"iom_sensor_health",
"dpb_sensor_health",
"scc_sensor_health",
"nic_sensor_health",
"heartbeat_health",
"fru_prsnt_health",
"bmc_health",
"slot1_sel_error",
"slot1_boot_order",
"server_pcie_port_config",
/* Add more Keys here */
LAST_KEY /* This is the last key of the list */
};

char * def_val_list[] = {
  "on", /* pwr_server1_last_state */
  "0", /* sysfw_ver_slot */
  "off", /* identify_slot */
  "0", /* timestamp_sled */
  "lps", /* slot_por_cfg */
  "1", /* slot_sensor_health */
  "1", /* iom_sensor_health */
  "1", /* dpb_sensor_health */
  "1", /* scc_sensor_health */
  "1", /* nic_sensor_health */
  "1", /* heartbeat_health */
  "1", /* fru_prsnt_health */
  "1", /* bmc_health */
  "1", /* slot_sel_error */
  "0000000", /* slot1_boot_order */
  "0000", /* server_pcie_port_config */
  /* Add more def values for the correspoding keys*/
  LAST_KEY /* Same as last entry of the key_list */
};

char * cfg_support_key_list[] = {
  "slot1_por_cfg",
  LAST_KEY /* This is the last key of the list */
};

struct power_coeff {
  float ein;
  float coeff;
};

/* IPMI SEL: System Firmware Error string table */
struct system_fw_progress {
  uint8_t EventData1;
  char DecodeString[128];
};

struct system_fw_progress system_fw_error[] = {
  {0x00, "Unspecified"},
  {0x01, "No system memory is physically installed in the system"},
  {0x02, "No usable system memory, all installed memory has experienced an unrecoverable failure"},
  {0x03, "Unrecoverable hard-disk/ATAPI/IDE device failure"},
  {0x04, "Unrecoverable system-board failure"},
  {0x05, "Unrecoverable diskette subsystem failure"},
  {0x06, "Unrecoverable hard-disk controller failure"},
  {0x07, "Unrecoverable PS/2 or USB keyboard failure"},
  {0x08, "Removable boot media not found"},
  {0x09, "Unrecoverable video controller failure"},
  {0x0A, "No video device detected"},
  {0x0B, "Firmware (BIOS) ROM corruption detected"},
  {0x0C, "CPU voltage mismatch"},
  {0x0D, "CPU speed matching failure"},
};

struct system_fw_progress system_fw_hang_or_progress[] = {
  {0x00, "Unspecified"},
  {0x01, "Memory initialization"},
  {0x02, "Hard-disk initialization"},
  {0x03, "Secondary processor(s) initialization"},
  {0x04, "User authentication"},
  {0x05, "User-initiated system setup"},
  {0x06, "USB resource configuration"},
  {0x07, "PCI resource configuration"},
  {0x08, "Option ROM initialization"},
  {0x09, "Video initialization"},
  {0x0A, "Cache initialization"},
  {0x0B, "SM Bus initialization"},
  {0x0C, "Keyboard controller initialization"},
  {0x0D, "Embedded controller/management controller initialization"},
  {0x0E, "Docking station attachment"},
  {0x0F, "Enabling docking station"},
  {0x10, "Docking station ejection"},
  {0x11, "Disabling docking station"},
  {0x12, "Calling operating system wake-up vector"},
  {0x13, "Starting operating system boot process, e.g. calling Int 19h"},
  {0x14, "Baseboard or motherboard initialization"},
  {0x15, "reserved"},
  {0x16, "Floppy initialization"},
  {0x17, "Keyboard test"},
  {0x18, "Pointing device test"},
  {0x19, "Primary processor initialization"},
};

/* NVMe-MI SSD Status Flag bit mask */
#define NVME_SFLGS_MASK_BIT 0x28  //Just check bit 3,5
#define NVME_SFLGS_CHECK_VALUE 0x28 // normal - bit 3,5 = 1

/* NVMe-MI SSD SMART Critical Warning */
#define NVME_SMART_WARNING_MASK_BIT 0x1F // Check bit 0~4

#define MAX_SERIAL_NUM 20

// Helper Functions
int
read_device(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%d", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
read_device_hex(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return errno;
  }

  rc = fscanf(fp, "%x", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}


int
write_device(const char *device, const char *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device for write %s", device);
#endif
    return err;
  }

  rc = fputs(value, fp);
  fclose(fp);

  if (rc < 0) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to write device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
pal_key_check(char *key) {
  int i;

  i = 0;
  while(strcmp(key_list[i], LAST_KEY)) {

    // If Key is valid, return success
    if (!strcmp(key, key_list[i])) {
      return 0;
    }

    i++;
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_key_check: invalid key - %s", key);
#endif
  return -1;
}

// Check what keys can be set by cfg-util
int
pal_cfg_key_check(char *key) {
  int i;

  i = 0;
  while(strcmp(cfg_support_key_list[i], LAST_KEY)) {
    // If Key is valid and can be set, return success
    if (!strcmp(key, cfg_support_key_list[i])) {
      return 0;
    }
    i++;
  }

  // Check is key is defined and valid but cannot be set
  if (pal_key_check(key) == 0) {
    return 1;
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "%s: invalid key - %s", __func__, key);
#endif
    return -1;
  }
}

int
pal_get_key_value(char *key, char *value) {
  int i = 0;
  int ret = 0;

  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  //Retry for max RETRY_COUNT Times
  for (i = 0; i < RETRY_COUNT; i++) {
    ret = 0;
    ret = kv_get(key, value, NULL, KV_FPERSIST);
    if (ret != 0) {
      syslog(LOG_ERR, "%s, failed to read device (%s), ret: %d, retry: %d", __func__, key, ret, i);
    }
    else {
      break;
    }
    msleep(100);
  }

  return ret;
}

int
pal_set_key_value(char *key, char *value) {
  int i = 0;
  int ret = 0;

  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  //Retry for max RETRY_COUNT Times
  for (i = 0; i < RETRY_COUNT; i++) {
    ret = 0;
    ret = kv_set(key, value, 0, KV_FPERSIST);
    if (ret != 0) {
      syslog(LOG_ERR, "%s, failed to write device (%s), ret: %d, retry: %d", __func__, key, ret, i);
    }
    else {
      break;
    }
    msleep(100);
  }

  return ret;
}

static int
power_on_server_physically(uint8_t slot_id){
  char vpath[64] = {0};

  sprintf(vpath, GPIO_VAL, gpio_power_btn[slot_id]);
  if (write_device(vpath, "1")) {
    return -1;
  }

  if (write_device(vpath, "0")) {
    return -1;
  }

  msleep(200);

  if (write_device(vpath, "1")) {
    return -1;
  }

  return 0;
}

// Power On the server in a given slot
static int
server_power_on(uint8_t slot_id) {
  char vpath[64] = {0};
  int val = 0;
  int loop = 0;
  int max_retry = 5;

  // Check if another instance is running
  if (pal_powering_on_flag(slot_id) < 0) {
    syslog(LOG_WARNING, "%s(): Another instance is running for FRU: %d.\n", __func__, slot_id);
    //Make server_power_on exit code to "-2" when another instance is running
    return -2;
  }

  //Enable IOM full power via GPIO_IOM_FULL_PWR_EN
  sprintf(vpath, GPIO_VAL, GPIO_IOM_FULL_PWR_EN);
  for (loop = 0; loop < max_retry; loop++){
    write_device(vpath, "1");
    read_device(vpath, &val);
    if (val == 1)
      break;
    syslog(LOG_WARNING, "%s(): GPIO_IOM_FULL_PWR_EN status is %d. Try %d time(s).\n", __func__, val, loop);
    msleep(10);
    // Max retry case
    if (loop == (max_retry-1)) {
      syslog(LOG_CRIT, "%s(): Fail to enable GPIO_IOM_FULL_PWR_EN after %d tries.\n", __func__, max_retry);
      pal_rm_powering_on_flag(slot_id);
      return -1;
    }
  }

  // Power on server
  for (loop = 0; loop < max_retry; loop++){
    val = power_on_server_physically(slot_id);
    if (val == 0)
      break;
    syslog(LOG_WARNING, "%s(): Power on server failed for %d time(s).\n", __func__, loop);
    sleep(2);

    // Max retry case
    if (loop == (max_retry-1)) {
      pal_rm_powering_on_flag(slot_id);
      return -1;
    }
  }

  pal_rm_powering_on_flag(slot_id);
  return 0;
}

// Power Off the server in given slot
static int
server_power_off(uint8_t slot_id, bool gs_flag, bool cycle_flag) {
  char vpath[64] = {0};
  uint8_t status;
  int retry = 0;
  int iom_board_id = BOARD_MP;

  if (slot_id != FRU_SLOT1) {
    return -1;
  }

  sprintf(vpath, GPIO_VAL, gpio_power_btn[slot_id]);

  if (write_device(vpath, "1")) {
    return -1;
  }

  sleep(1);

  if (write_device(vpath, "0")) {
    return -1;
  }

  if (gs_flag) {
    sleep(DELAY_GRACEFUL_SHUTDOWN);
  } else {
    sleep(DELAY_POWER_OFF);
  }

  if (write_device(vpath, "1")) {
    return -1;
  }

// TODO: Workaround for EVT only. Remove after PVT.
#ifdef CONFIG_FBTTN
  iom_board_id = pal_get_iom_board_id();
  if(iom_board_id == BOARD_EVT) { // EVT only
  // When ML-CPU is made sure shutdown that is not power-cycle, we should power-off M.2/IOC by BMC.
  //if (cycle_flag == false) {
    do {
      if (pal_get_server_power(slot_id, &status) < 0) {
        #ifdef DEBUG
        syslog(LOG_WARNING, "server_power_off: pal_get_server_power status is %d\n", status);
        #endif
      }
      sleep(DELAY_FULL_POWER_DOWN);
      if (retry > RETRY_COUNT) {
        #ifdef DEBUG
        syslog(LOG_WARNING, "server_power_off: retry fail\n");
        #endif
        break;
      }
      else {
        retry++;
      }
    } while (status != SERVER_POWER_OFF);
    // M.2/IOC power-off
    sprintf(vpath, GPIO_VAL, GPIO_IOM_FULL_PWR_EN);
    if (write_device(vpath, "0")) {
      return -1;
    }
  //}
  }
#endif

  return 0;
}

int
server_power_reset(uint8_t slot_id) {
  int ret = 0;

  ret = pal_set_rst_btn(slot_id, GPIO_LOW);
  if (ret < 0)
    return ret;

  msleep(100);

  ret = pal_set_rst_btn(slot_id, GPIO_HIGH);
  if (ret < 0)
    return ret;

  return 0;
}

// Control 12V to the server in a given slot
int
server_12v_on(uint8_t slot_id) {

  if (slot_id != FRU_SLOT1) {
    return -1;
  }

  return set_gpio_value(gpio_12v[slot_id], CTRL_ENABLE);
}

// Turn off 12V for the server in given slot
int
server_12v_off(uint8_t slot_id) {
  char vpath[64] = {0};

  if (slot_id != FRU_SLOT1) {
    return -1;
  }

  sprintf(vpath, GPIO_VAL, gpio_12v[slot_id]);

  if (write_device(vpath, "0")) {
    return -1;
  }

  return 0;
}

// Display the given POST code using GPIO port
static int
pal_post_display(uint8_t status) {
  char path[64] = {0};
  int ret;
  char *val;

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_post_display: status is %d\n", status);
#endif

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_0);

  if (BIT(status, 0)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_1);
  if (BIT(status, 1)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_2);
  if (BIT(status, 2)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_3);
  if (BIT(status, 3)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_4);
  if (BIT(status, 4)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_5);
  if (BIT(status, 5)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_6);
  if (BIT(status, 6)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_7);
  if (BIT(status, 7)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

post_exit:
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "write_device failed for %s\n", path);
#endif
    return -1;
  } else {
    return 0;
  }
}

// Platform Abstraction Layer (PAL) Functions
int
pal_get_platform_name(char *name) {
  strcpy(name, FBTTN_PLATFORM_NAME);

  return 0;
}

int
pal_get_num_slots(uint8_t *num) {
  *num = MAX_NUM_SLOTS;

  return 0;
}

int
pal_is_scc_prsnt() {

  uint8_t val = 0;
  int ret = 0;
  int sku = 0;
  int gpio_num;

  sku = pal_get_iom_type();
  // IOM type: M.2 solution
  if (sku == IOM_M2) {
    if (pal_get_iom_location() == IOM_SIDEA) { // IOMA
      gpio_num = GPIO_SCC_A_INS;
    } else if (pal_get_iom_location() == IOM_SIDEB) { // IOMB
      gpio_num = GPIO_SCC_B_INS;
    } else {
      gpio_num = GPIO_SCC_A_INS;
    }

  // IOM type: IOC solution
  } else {
    gpio_num = GPIO_SCC_A_INS;
  }

  ret = get_gpio_value(gpio_num, &val);
  if (ret != 0) {
    return -1;
  }

  if (val == 0) {
    return 1; // Present
  } else {
    return 0; // Absent
  }
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {

  switch (fru) {
    case FRU_SLOT1:
      *status = is_server_prsnt(fru);
      break;
    // Need to check if local SCC is inserted.
    case FRU_SCC:
      *status = pal_is_scc_prsnt();
      break;
    case FRU_DPB:
    case FRU_IOM:
    case FRU_NIC:
      *status = 1;
      break;
    default:
      return -1;
  }

  return 0;
}

int
pal_is_slot_server(uint8_t fru) {
  if (fru == FRU_SLOT1)
    return 1;
  return 0;
}

int
pal_is_server_12v_on(uint8_t slot_id, uint8_t *status) {
  int ret = -1;
  uint8_t val = -1;

  if (slot_id != FRU_SLOT1) {
    return -1;
  }

  ret = get_gpio_value(gpio_12v[slot_id], &val);
  if (ret != 0)
    return -1;

  if (val == 0x1) {
    *status = CTRL_ENABLE;
  } else {
    *status = CTRL_DISABLE;
  }

  return 0;
}

int
pal_is_bic_ready(uint8_t slot_id, uint8_t *status) {

  uint8_t val = 0;
  int ret = 0;

  ret = get_gpio_value(GPIO_BIC_READY_N, &val);
  if (ret != 0) {
    return -1;
  }

  if (val == 0) {
    *status = BIC_READY;
  } else {
    *status = BIC_NOT_READY;
  }

  return 0;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  uint8_t ctrl_status, val;

  switch (fru) {
    case FRU_SLOT1:
      // Check if the server board is powered up
      if (!pal_is_server_12v_on(fru, &ctrl_status)) {

        // If the server is powered up, check if the BIC is ready.
        if (ctrl_status == CTRL_ENABLE) {

          if (!pal_is_bic_ready(fru, &val))
            *status = (val == BIC_READY)? 1 : 0; // BIC_READY: 0; BIC_NOT_READY: 1

        // ctrl_status == CTRL_DISABLE; the server is 12V-OFF
        } else {
          *status = 0;
        }
        break;

      // if pal_is_server_12v_on failed
      } else
        return -1;

    case FRU_DPB:
      // Only if local SCC is present, BMC can read the DPB sensors and fruid
      *status = pal_is_scc_prsnt();

      //TODO: is_SCC_ready needed to confirm is expander is ready
      break;

    case FRU_SCC:
      //TODO: is_SCC_ready needed to confirm is expander is ready
    case FRU_IOM:
    case FRU_NIC:
      *status = 1;
      break;

    default:
      return -1;
  }

  return 0;
}

int
pal_is_debug_card_prsnt(uint8_t *status) {
  int val;
  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_DBG_CARD_PRSNT);

  if (read_device(path, &val)) {
    return -1;
  }

  if (val == 0x0) {
    *status = 1;
  } else {
    *status = 0;
  }
  return 0;
}

int
pal_get_server_power(uint8_t slot_id, uint8_t *status) {
  int ret;
  int iom_board_id = BOARD_MP;
  uint8_t gpio_perst_val = 0;
  char value[MAX_VALUE_LEN] = { 0 };
  bic_gpio_t gpio;

  /* Check whether the system is 12V off or on */
  ret = pal_is_server_12v_on(slot_id, status);
  if (ret < 0) {
    syslog(LOG_ERR, "pal_get_server_power: pal_is_server_12v_on failed");
    return -1;
  }

  /* If 12V-off, return */
  if (*status == CTRL_DISABLE) {
    *status = SERVER_12V_OFF;
    return 0;
  }

  // In DVT systems, we added a new GPIO pin to detect power status
  iom_board_id = pal_get_iom_board_id();
  if (iom_board_id == BOARD_EVT) {
    // Get server power status via BIC
    /* If 12V-on, check if the CPU is turned on or not */
    ret = bic_get_gpio(slot_id, &gpio);
    if (ret) {
      // Check for if the BIC is irresponsive due to 12V_OFF or 12V_CYCLE
      syslog(LOG_INFO, "pal_get_server_power: bic_get_gpio returned error hence"
          "reading the kv_store for last power state  for fru %d", slot_id);
      pal_get_last_pwr_state(slot_id, value);
      if (!(strcmp(value, "off"))) {
        *status = SERVER_POWER_OFF;
      } else if (!(strcmp(value, "on"))) {
        *status = SERVER_POWER_ON;
      } else {
        return ret;
      }
      return 0;
    }

    if (gpio.pwrgood_cpu) {
      *status = SERVER_POWER_ON;
    } else {
      *status = SERVER_POWER_OFF;
    }
  } else {
    // Get server power status via GPIO PERST
    ret = get_gpio_value(GPIO_PCIE_RESET, &gpio_perst_val);
    if (ret != 0) {
      syslog(LOG_ERR, "%s: get GPIO_PCIE_RESET fail", __func__);
      return -1;
    }

    // Each time Server Power On and Reset, BIOS sends a 13~14ms PERST# low pulse to PCIe devices.
    // To filter-out the low pulse to prevent the false power status query,
    // we add recheck in 50ms (add some tolerance) if the power status is off.
    if (gpio_perst_val == 0) {
      msleep(50);
      ret = get_gpio_value(GPIO_PCIE_RESET, &gpio_perst_val);
      if (ret != 0) {
        syslog(LOG_ERR, "%s: get GPIO_PCIE_RESET fail", __func__);
        return -1;
      }
    }

    if (gpio_perst_val == 0) {
      *status = SERVER_POWER_OFF;
    } else {
      *status = SERVER_POWER_ON;
    }
  }

  return 0;
}

// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t slot_id, uint8_t cmd) {
  uint8_t status;
  bool gs_flag = false;
  bool cycle_flag = false;

  if (slot_id != FRU_SLOT1) {
    return -1;
  }

  if (pal_get_server_power(slot_id, &status) < 0) {
    return -1;
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if (status == SERVER_POWER_ON)
        return 1;
      else
        return server_power_on(slot_id);
      break;

    case SERVER_POWER_OFF:
      if (status == SERVER_POWER_OFF)
        return 1;
      else
        return server_power_off(slot_id, gs_flag, cycle_flag);
      break;

    case SERVER_POWER_CYCLE:
      cycle_flag = true;
      if (status == SERVER_POWER_ON) {
        if (server_power_off(slot_id, gs_flag, cycle_flag))
          return -1;

        sleep(DELAY_POWER_CYCLE);

        return server_power_on(slot_id);

      } else if (status == SERVER_POWER_OFF) {

        return server_power_on(slot_id);
      }
      break;
    case SERVER_POWER_RESET:
      if (status == SERVER_POWER_OFF) {
        syslog(LOG_WARNING, "%s: power reset has no effect while power is off", __func__);
        return -1;
      } else {
        return server_power_reset(slot_id);
      }
      break;
    case SERVER_GRACEFUL_SHUTDOWN:
      if (status == SERVER_POWER_OFF) {
        return 1;
      } else {
        gs_flag = true;
        return server_power_off(slot_id, gs_flag, cycle_flag);
      }
      break;

    case SERVER_12V_ON:
      if (status != SERVER_12V_OFF) // pal_get_server_power() doesn't provide status SERVER_12V_ON
        return 1;
      else
        return server_12v_on(slot_id);
      break;

    case SERVER_12V_OFF:
      if (status == SERVER_12V_OFF)
        return 1;
      else
        return server_12v_off(slot_id);
      break;

    case SERVER_12V_CYCLE:
      if (server_12v_off(slot_id)) {
        return -1;
      }

      sleep(DELAY_12V_CYCLE);

      return server_12v_on(slot_id);
    default:
      return -1;
  }

  return 0;
}

int
pal_sled_cycle(void) {
  pal_update_ts_sled();
  // Remove the adm1275 module as the HSC device is busy
  system("rmmod adm1275");

  // Send command to HSC power cycle
  system("i2cset -y 0 0x10 0xd9 c");

  return 0;
}

// Read  Debug Card UART Select and return the position
int
pal_get_uart_sel_pos(uint8_t *pos) {
  uint8_t uart_sel = HAND_SW_BMC;
  int ret = 0;

  // UART_SEL_IN: GPIOS1 (145)
  // 0: UART_SEL_SERVER; 1: UART_SEL_BMC
  ret = get_gpio_value(UART_SEL_IN, &uart_sel);
  if (ret != 0) {
    return -1;
  }

  *pos = uart_sel;

  return 0;
}

// Return the Front panel's debug card Power Button status
int
pal_get_dbg_pwr_btn(uint8_t *status) {
  uint8_t val = 0;
  int ret = 0;

  ret = get_gpio_value(GPIO_DEBUG_PWR_BTN_N, &val);
  if (ret != 0) {
    return -1;
  }

  if (val == 1) {
    *status = 0x0;
  } else {
    *status = 0x1;
  }

  return 0;
}

// Return the front panel's debug card Reset Button status
int
pal_get_dbg_rst_btn(uint8_t *status) {
  uint8_t val = 0;
  int ret = 0;

  ret = get_gpio_value(GPIO_DEBUG_RST_BTN_N, &val);
  if (ret != 0) {
    return -1;
  }

  if (val == 1) {
    *status = 0x0;
  } else {
    *status = 0x1;
  }

  return 0;
}

// Return the Front panel Power Button
int
pal_get_pwr_btn(uint8_t *status) {
  uint8_t val = 0;
  int ret = 0;

  ret = get_gpio_value(GPIO_PWR_BTN, &val);
  if (ret != 0) {
    return -1;
  }

  if (val) {
    *status = 0x0;
  } else {
    *status = 0x1;
  }

  return 0;
}

// Return the front panel's Reset Button status
int
pal_get_rst_btn(uint8_t *status) {
  uint8_t val = 0;
  int ret = 0;

  ret = get_gpio_value(GPIO_RST_BTN, &val);
  if (ret != 0) {
    return -1;
  }

  if (val) {
    *status = 0x0;
  } else {
    *status = 0x1;
  }

  return 0;
}

// Update the Reset button input to the server at given slot
int
pal_set_rst_btn(uint8_t slot, uint8_t status) {

  if (slot < 1 || slot > MAX_NUM_SLOTS) {
    return -1;
  }

  return set_gpio_value(gpio_rst_btn[slot], status);
}

// Update the LED for the given slot with the status
int
pal_set_led(uint8_t slot, uint8_t status) {
  char path[64] = {0};
  char *val;

  if (slot < 1 || slot > 4) {
    return -1;
  }

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  sprintf(path, GPIO_VAL, gpio_led[slot]);
  if (write_device(path, val)) {
    return -1;
  }

  return 0;
}

// Update Heartbeet LED
int
pal_set_hb_led(uint8_t status) {
  char path[64] = {0};
  char *val;

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  sprintf(path, GPIO_VAL, GPIO_HB_LED);
  if (write_device(path, val)) {
    return -1;
  }

  return 0;
}

// Update the Identification LED for the given slot with the status
int
pal_set_id_led(uint8_t slot, uint8_t status) {
  char path[64] = {0};
  char *val;

  if (slot < 1 || slot > 4) {
    return -1;
  }

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  sprintf(path, GPIO_VAL, gpio_id_led[slot]);
  if (write_device(path, val)) {
    return -1;
  }

  return 0;
}

// Update the USB Mux to the server at given slot
// In Triton, we don't need it
int
pal_switch_usb_mux(uint8_t slot) {

  return 0;
}

// Enable POST buffer for the server in given slot
int
pal_post_enable(uint8_t slot) {
  int ret;
  bic_config_t config = {0};
  bic_config_u *t = (bic_config_u *) &config;

  ret = bic_get_config(slot, &config);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "post_enable: bic_get_config failed for fru: %d\n", slot);
#endif
    return ret;
  }

  if (t->bits.post == 0) {
    t->bits.post = 1;
    ret = bic_set_config(slot, &config);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "post_enable: bic_set_config failed\n");
#endif
      return ret;
    }
  }
  return 0;
}

// Disable POST buffer for the server in given slot
int
pal_post_disable(uint8_t slot) {
  int ret;
  bic_config_t config = {0};
  bic_config_u *t = (bic_config_u *) &config;

  ret = bic_get_config(slot, &config);
  if (ret) {
    return ret;
  }

  t->bits.post = 0;

  ret = bic_set_config(slot, &config);
  if (ret) {
    return ret;
  }

  return 0;
}

// Get the last post code of the given slot
int
pal_post_get_last(uint8_t slot, uint8_t *status) {
  int ret;
  uint8_t buf[MAX_IPMB_RES_LEN] = {0x0};
  uint8_t len;

  ret = bic_get_post_buf(slot, buf, &len);
  if (ret) {
    return ret;
  }

  // The post buffer is LIFO and the first byte gives the latest post code
  *status = buf[0];

  return 0;
}

// Handle the received post code, for now display it on debug card
int
pal_post_handle(uint8_t slot, uint8_t status) {
  int ret;

  // Only allow front-paneld to control
  if ((slot == HAND_SW_SERVER) || (slot == HAND_SW_BMC)) {
    // Display the post code or error code in the 7-seg LED of debug card
    ret = pal_post_display(status);
    if (ret) {
      return ret;
    }
  }

  return 0;
}

int
pal_get_fru_list(char *list) {

  strcpy(list, pal_fru_list);
  return 0;
}

int
pal_get_fru_id(char *str, uint8_t *fru) {

  return fbttn_common_fru_id(str, fru);
}

int
pal_get_fru_name(uint8_t fru, char *name) {

  return fbttn_common_fru_name(fru, name);
}

int
pal_get_fru_sdr_path(uint8_t fru, char *path) {
  return fbttn_sensor_sdr_path(fru, path);
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {

  int sku = 0;
  int iom_board_id = BOARD_MP;


  switch(fru) {
    case FRU_SLOT1:
      *sensor_list = (uint8_t *) bic_sensor_list;
      *cnt = bic_sensor_cnt;
      break;
    case FRU_IOM:
      sku = pal_get_iom_type();
      if (sku == IOM_M2) {  // IOM type: M.2 solution
        // It's a transition period from EVT to DVT
        iom_board_id = pal_get_iom_board_id();
        if (iom_board_id == BOARD_EVT) {  // EVT
          *sensor_list = (uint8_t *) iom_sensor_list_type5;
          *cnt = iom_sensor_cnt_type5;
        } else {                          // DVT later
          *sensor_list = (uint8_t *) iom_sensor_list_type5_dvt;
          *cnt = iom_sensor_cnt_type5_dvt;
        }
      } else {              // IOM type: IOC solution
        *sensor_list = (uint8_t *) iom_sensor_list_type7;
        *cnt = iom_sensor_cnt_type7;
      }
      break;
    case FRU_DPB:
      *sensor_list = (uint8_t *) dpb_sensor_list;
      *cnt = dpb_sensor_cnt;
      break;
    case FRU_SCC:
      *sensor_list = (uint8_t *) scc_sensor_list;
      *cnt = scc_sensor_cnt;
      break;
    case FRU_NIC:
      *sensor_list = (uint8_t *) nic_sensor_list;
      *cnt = nic_sensor_cnt;
      break;
    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_get_fru_sensor_list: Wrong fru id %u", fru);
#endif
      return -1;
  }
    return 0;
}

int
pal_fruid_write(uint8_t fru, char *path) {
  return bic_write_fruid(fru, 0, path);
}

int
pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  uint8_t status;

  // SDR only supported for MonoLake
  switch(fru) {
    case FRU_SLOT1:
      pal_is_fru_prsnt(fru, &status);
      if (status)
        return fbttn_sensor_sdr_init(fru, sinfo);
    default:
      return -1;
  }
}

bool pal_sensor_is_cached(uint8_t fru, uint8_t sensor_num) {
  if (fru == FRU_DPB || fru == FRU_SCC) {
      return false;
  }
  return true;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {

  uint8_t status;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int ret;
  int sku = 0;
  int i;
  bool check_server_power_status = false;

  if(pal_is_fru_prsnt(fru, &status) < 0)
      return -1;

  if (!status)
      return -1;

  switch(fru) {
    case FRU_SLOT1:
      sprintf(key, "server_sensor%d", sensor_num);
      break;
    case FRU_IOM:
      sprintf(key, "iom_sensor%d", sensor_num);
      break;
    case FRU_DPB:
      pal_expander_sensor_check(fru, sensor_num);
      sprintf(key, "dpb_sensor%d", sensor_num);
      break;
    case FRU_SCC:
      pal_expander_sensor_check(fru, sensor_num);
      sprintf(key, "scc_sensor%d", sensor_num);
      break;
    case FRU_NIC:
      sprintf(key, "nic_sensor%d", sensor_num);
      break;
  }

  // Check for the power status
  ret = pal_get_server_power(FRU_SLOT1, &status);
  if (ret == 0) {
    ret = fbttn_sensor_read(fru, sensor_num, value, status, key);
    if (ret != 0) {
      if (ret < 0) {
        if (fru == FRU_IOM || fru == FRU_DPB || fru == FRU_SCC || fru == FRU_NIC) {
          ret = -1;
        } else if (pal_get_server_power(fru, &status) < 0) {
          ret = -1;
        } else if (status == SERVER_POWER_ON) {
          // This check helps interpret the IPMI packet loss scenario
          ret = -1;
        }

        strcpy(str, "NA");
      } else {
        // If ret = READING_SKIP, doesn't update sensor reading and keep the previous value
        return ret;
      }
    }
    else {
      // On successful sensor read
      sku = pal_get_iom_type();
      if (sku == IOM_M2) { // IOM type: M.2 solution
        for (i = 0; i < iom_t5_non_stby_sensor_cnt; i++) {
          if (sensor_num == iom_t5_non_stby_sensor_list[i]) {
            check_server_power_status = true;
            break;
          }
        }
      } else {            // IOM type: IOC solution
        for (i = 0; i < iom_t7_non_stby_sensor_cnt; i++) {
          if (sensor_num == iom_t7_non_stby_sensor_list[i]) {
            check_server_power_status = true;
            break;
          }
        }
      }
      if (check_server_power_status == true) {
        // If server is powered off, ignore the read and write NA
        if (status != SERVER_POWER_ON) {
          strcpy(str, "NA");
          ret = -1;
        }
        else {
          // double check power status, after reading the sensor
          pal_get_server_power(FRU_SLOT1, &status);

          // If server is powered off, ignore the read and write NA
          if (status != SERVER_POWER_ON) {
            strcpy(str, "NA");
            ret = -1;
          } else {
            sprintf(str, "%.2f",*((float*)value));
          }
        }
      } else {    // check_server_power_status == false
        sprintf(str, "%.2f",*((float*)value));
      }
    }

    if(kv_set(key, str, 0, 0) < 0) {
  #ifdef DEBUG
       syslog(LOG_WARNING, "pal_sensor_read_raw: cache_set key = %s, str = %s failed.", key, str);
  #endif
      return -1;
    }
    else {
      return ret;
    }
  } else {
    return -1;
  }
}

int
pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag) {

  switch(fru) {
    case FRU_SLOT1:
      if (snr_num == BIC_SENSOR_SOC_THERM_MARGIN)
        *flag = GETMASK(SENSOR_VALID) | GETMASK(UCR_THRESH);
      else if (snr_num == BIC_SENSOR_SOC_PACKAGE_PWR)
        *flag = GETMASK(SENSOR_VALID);
      else if (snr_num == BIC_SENSOR_SOC_TJMAX)
        *flag = GETMASK(SENSOR_VALID);
      break;
    case FRU_IOM:
    case FRU_DPB:
    case FRU_SCC:
    case FRU_NIC:
      break;
  }

  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  return fbttn_sensor_threshold(fru, sensor_num, thresh, value);
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  return fbttn_sensor_name(fru, sensor_num, name);
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  return fbttn_sensor_units(fru, sensor_num, units);
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  //Return -1 when ask for NIC fruid path
  if ( fru == FRU_NIC )
    return -1;
  else
    return fbttn_get_fruid_path(fru, path);
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  return fbttn_get_fruid_eeprom_path(fru, path);
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  return fbttn_get_fruid_name(fru, name);
}

int
pal_set_def_key_value() {

  int ret;
  int i;
  int fru;
  char key[MAX_KEY_LEN] = {0};

  for(i = 0; strcmp(key_list[i], LAST_KEY) != 0; i++) {
    if ((ret = kv_set(key_list[i], def_val_list[i], 0, KV_FPERSIST | KV_FCREATE)) < 0) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_set_def_key_value: kv_set failed. %d", ret);
#endif
    }
  }

  /* Actions to be taken on Power On Reset */
  if (pal_is_bmc_por()) {

    for (fru = 1; fru <= MAX_NUM_FRUS; fru++) {

      /* Clear all the SEL errors */
      memset(key, 0, MAX_KEY_LEN);

      switch(fru) {
        case FRU_SLOT1:
          sprintf(key, "slot%d_sel_error", fru);
        break;

        case FRU_IOM:
          continue;

        case FRU_DPB:
          continue;

        case FRU_SCC:
          continue;

        case FRU_NIC:
          continue;

        default:
          return -1;
      }

      /* Write the value "1" which means FRU_STATUS_GOOD */
      ret = pal_set_key_value(key, "1");

      /* Clear all the sensor health files*/
      memset(key, 0, MAX_KEY_LEN);

      switch(fru) {
        case FRU_SLOT1:
          sprintf(key, "slot%d_sensor_health", fru);
        break;

        case FRU_IOM:
          continue;

        case FRU_DPB:
          continue;

        case FRU_SCC:
          continue;

        case FRU_NIC:
          continue;

        default:
          return -1;
      }

      /* Write the value "1" which means FRU_STATUS_GOOD */
      ret = pal_set_key_value(key, "1");
    }
  }

  return 0;
}

int
pal_get_fru_devtty(uint8_t fru, char *devtty) {

  switch(fru) {
    case FRU_SLOT1:
      sprintf(devtty, "/dev/ttyS1");
      break;
    default:
#ifdef DEBUG
      syslog(LOG_CRIT, "pal_get_fru_devtty: Wrong fru id %u", fru);
#endif
      return -1;
  }
    return 0;
}

void
pal_dump_key_value(void) {
  int i = 0;

  char value[MAX_VALUE_LEN] = {0x0};

  while (strcmp(key_list[i], LAST_KEY)) {
    printf("%s:", key_list[i]);
    if (kv_get(key_list[i], value, NULL, KV_FPERSIST) < 0) {
      printf("\n");
    } else {
      printf("%s\n",  value);
    }
    i++;
    memset(value, 0, MAX_VALUE_LEN);
  }
}

int
pal_set_last_pwr_state(uint8_t fru, char *state) {

  int ret;
  char key[MAX_KEY_LEN] = {0};
  uint8_t last_boot_time[4] = {0};

  sprintf(key, "pwr_server%d_last_state", (int) fru);

  //If the OS state is "off", clear the last boot time to 0 0 0 0
  if(!strcmp(state, "off"))
    pal_set_last_boot_time(1, last_boot_time);

  ret = pal_set_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_set_last_pwr_state: pal_set_key_value failed for "
        "fru %u", fru);
#endif
  }
  return ret;
}

int
pal_get_last_pwr_state(uint8_t fru, char *state) {
  int ret;
  char key[MAX_KEY_LEN] = {0};

  switch(fru) {
    case FRU_SLOT1:

      sprintf(key, "pwr_server%d_last_state", (int) fru);

      ret = pal_get_key_value(key, state);
      if (ret < 0) {
#ifdef DEBUG
        syslog(LOG_WARNING, "pal_get_last_pwr_state: pal_get_key_value failed for "
            "fru %u", fru);
#endif
      }
      return ret;
    case FRU_IOM:
    case FRU_DPB:
    case FRU_SCC:
    case FRU_NIC:
      sprintf(state, "on");
      return 0;
  }
  return -1;
}

int
pal_get_sys_guid(uint8_t slot, char *guid) {

  return bic_get_sys_guid(slot, (uint8_t *)guid);
}

int
pal_set_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int i;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};

  sprintf(key, "sysfw_ver_slot%d", (int) slot);

  for (i = 0; i < SIZE_SYSFW_VER; i++) {
    sprintf(tstr, "%02x", ver[i]);
    strcat(str, tstr);
  }

  return pal_set_key_value(key, str);
}

int
pal_get_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  sprintf(key, "sysfw_ver_slot%d", (int) slot);

  ret = pal_get_key_value(key, str);
  if (ret) {
    return ret;
  }

  for (i = 0; i < 2*SIZE_SYSFW_VER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    ver[j++] = (msb << 4) | lsb;
  }

  return 0;
}

// Determine if BMC is AC on
int
pal_is_bmc_por(void) {
  FILE *fp;
  int por = 0;

  fp = fopen(AST_POR_FLAG, "r");
  if (fp != NULL) {
    fscanf(fp, "%d", &por);
    fclose(fp);
  }

  return (por == 1) ? 1 : 0;
}

int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {

  switch(fru) {
    case FRU_SLOT1:
      *sensor_list = (uint8_t *) bic_discrete_list;
      *cnt = bic_discrete_cnt;
      break;
    case FRU_DPB:
      *sensor_list = (uint8_t *) dpb_discrete_list;
      *cnt = dpb_discrete_cnt;
      break;
    case FRU_IOM:
    case FRU_SCC:
    case FRU_NIC:
      *sensor_list = NULL;
      *cnt = 0;
      break;
    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_get_fru_discrete_list: Wrong fru id %u", fru);
#endif
      break;
  }
    return 0;
}

static void
_print_sensor_discrete_log(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t val, char *event) {
  if (val) {
    syslog(LOG_CRIT, "ASSERT: %s discrete - raised - FRU: %d, num: 0x%X,"
        " snr: %-16s val: %d", event, fru, snr_num, snr_name, val);
  } else {
    syslog(LOG_CRIT, "DEASSERT: %s discrete - settled - FRU: %d, num: 0x%X,"
        " snr: %-16s val: %d", event, fru, snr_num, snr_name, val);
  }
  pal_update_ts_sled();
}

int
pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val) {

  char name[32];
  bool valid = false;
  uint8_t diff = o_val ^ n_val;

  if (GETBIT(diff, 0)) {
    switch(snr_num) {
      case BIC_SENSOR_SYSTEM_STATUS:
        sprintf(name, "SOC_Thermal_Trip");
        valid = true;
        break;
      case BIC_SENSOR_VR_HOT:
        sprintf(name, "SOC_VR_Hot");
        valid = true;
        break;
    }
    if (valid) {
      _print_sensor_discrete_log( fru, snr_num, snr_name, GETBIT(n_val, 0), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 1)) {
    switch(snr_num) {
      case BIC_SENSOR_SYSTEM_STATUS:
        sprintf(name, "SOC_FIVR_Fault");
        valid = true;
        break;
      case BIC_SENSOR_VR_HOT:
        sprintf(name, "SOC_DIMM_VR_Hot");
        valid = true;
        break;
      case BIC_SENSOR_CPU_DIMM_HOT:
        sprintf(name, "SOC_MEMHOT");
        valid = true;
        break;
    }
    if (valid) {
      _print_sensor_discrete_log( fru, snr_num, snr_name, GETBIT(n_val, 1), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 2)) {
    switch(snr_num) {
      case BIC_SENSOR_SYSTEM_STATUS:
        sprintf(name, "SOC_Throttle");
        valid = true;
        break;
    }
    if (valid) {
      _print_sensor_discrete_log( fru, snr_num, snr_name, GETBIT(n_val, 2), name);
      valid = false;
    }
  }
  return 0;
}

static int
pal_store_crashdump(uint8_t fru) {

  return fbttn_common_crashdump(fru);
}

int
pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data) {

  char key[MAX_KEY_LEN] = {0};

  /* For every SEL event received from the BIC, set the critical LED on */
  switch(fru) {
    case FRU_SLOT1:
      switch(snr_num) {
        case CATERR_B:
          pal_store_crashdump(fru);
        }
      sprintf(key, "slot%d_sel_error", fru);
      break;

    case FRU_IOM:
      return 0;

    case FRU_DPB:
      return 0;

    case FRU_SCC:
      return 0;

    case FRU_NIC:
      return 0;

    default:
      return -1;
  }

  /* Write the value "0" which means FRU_STATUS_BAD */
  return pal_set_key_value(key, "0");
}

int
pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name) {
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];

  switch (snr_type) {
    case OS_BOOT:
      // OS_BOOT used by OS
      sprintf(name, "OS");
      return 0;
  }

  switch (fru) {
    case FRU_SLOT1:
      pal_get_x86_event_sensor_name(fru, snr_num, name);
      break;
    case FRU_SCC:
      fbttn_sensor_name(fru, snr_num, name);
      fbttn_sensor_name(fru-1, snr_num, name); // the fru is always 4, so we have to minus 1 for DPB sensors, since scc and dpb sensor all come from Expander
      break;
  }
  return 0;
}

int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log) {
  bool parsed;
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  uint8_t *ed = &event_data[3];
  char temp_log[512] = {0};
  char err_str[512] = {0};
  uint8_t sen_type = event_data[0];
  uint8_t event_type = sel[12] & 0x7F;
  uint8_t event_dir = sel[12] & 0x80;
  uint8_t map_of_dimm_num;

  parsed = false;
  strcpy(error_log, "");

  // First, try checking if the SEL message is a platform-specific one (FBTTN)
  switch (fru) {
    case (FRU_SLOT1):
      switch (snr_num) {
        case POST_ERROR:
          parsed = true;
          if (((ed[0] >> 6) & 0x03) == 0x3) {
            // Reference from IPMI spec v2.0 Table 42-3, Sensor Type Codes
            switch (ed[0] & 0xF) {
              case 0x00:
                strcat(error_log, "System Firmware Error (POST Error), IPMI Post Code");
                if (ed[1] <  (sizeof(system_fw_error) / sizeof(system_fw_error[0]))) {
                  sprintf(temp_log, ", %s", system_fw_error[ed[1]].DecodeString);
                } else {
                  sprintf(temp_log, ", reserved");
                }
                break;
              case 0x01:
                strcat(error_log, "System Firmware Hang, IPMI Post Code");
              case 0x02:
                if (strcmp(error_log, "") == 0) {
                  strcat(error_log, "System Firmware Progress, IPMI Post Code");
                }
                if (ed[1] <  (sizeof(system_fw_hang_or_progress) / sizeof(system_fw_hang_or_progress[0]))) {
                  sprintf(temp_log, ", %s", system_fw_hang_or_progress[ed[1]].DecodeString);
                } else {
                  sprintf(temp_log, ", reserved");
                }
                break;
              default:
                sprintf(temp_log, "Unknown");
                break;
            }
            strcat(error_log, temp_log);
          } else if (((ed[0] >> 6) & 0x03) == 0x2) {
            if ((ed[0] & 0x0F) == 0x0)
              strcat(error_log, "System Firmware Error (POST Error)");
            else
              strcat(error_log, "Unknown");

            sprintf(temp_log, ", OEM Post Code, 0x%02X%02X", ed[2], ed[1]);
            strcat(error_log, temp_log);
            switch ((ed[2] << 8) | ed[1]) {
              case 0xA104:
                sprintf(temp_log, ", CMOS/NVRAM configuration cleared");
                strcat(error_log, temp_log);
                break;
              case 0xA105:
                sprintf(temp_log, ", BMC Failed (No Response)");
                strcat(error_log, temp_log);
                break;
              default:
                break;
            }
          } else {
            strcat(error_log, "Unknown");
          }
          break;
        case MEMORY_ECC_ERR:
          parsed = true;
          if (sen_type == 0x0C) {
            // SEL from MEMORY_ECC_ERR
            if ((ed[0] & 0x0F) == 0x0) {
              strcat(error_log, "Correctable");
              sprintf(temp_log, "DIMM%02X ECC err", ed[2]);
              pal_add_cri_sel(temp_log);
            } else if ((ed[0] & 0x0F) == 0x1) {
              strcat(error_log, "Uncorrectable");
              sprintf(temp_log, "DIMM%02X UECC err", ed[2]);
              pal_add_cri_sel(temp_log);
            } else if ((ed[0] & 0x0F) == 0x5) {
              strcat(error_log, "Correctable ECC error Logging Limit Reached");
            } else {
              strcat(error_log, "Sensor Type: 0x0C, Unknown");
            }
          } else if (sen_type == 0x10) {
            // SEL from MEMORY_ERR_LOG_DIS
            if ((ed[0] & 0x0F) == 0x5) {
              strcat(error_log, "Correctable Memory Error Logging Disabled");
            } else {
              strcat(error_log, "Sensor Type: 0x10, Unknown");
            }
          }

          // Common routine for both MEM_ECC_ERR and MEMORY_ERR_LOG_DIS
          sprintf(temp_log, " (DIMM %02X)", ed[2]);
          strcat(error_log, temp_log);

          sprintf(temp_log, " Logical Rank %d", ed[1] & 0x03);
          strcat(error_log, temp_log);

          // Bit[7:5]: CPU number     (Range: 0-7)
          // Bit[4:3]: Channel number (Range: 0-3)
          // Bit[2:0]: DIMM number    (Range: 0-7)
          if (((ed[1] & 0xC) >> 2) == 0x0) {
            /* All Info Valid */
            // uint8_t chn_num = (ed[2] & 0x18) >> 3;
            // uint8_t dimm_num = ed[2] & 0x7;
            map_of_dimm_num = ed[2];

            /* If critical SEL logging is available, do it */
            if (sen_type == 0x0C) {
              if ((ed[0] & 0x0F) == 0x0) {
                sprintf(err_str, "ECC err");
              } else if ((ed[0] & 0x0F) == 0x1) {
                sprintf(err_str, "UECC err");
              }              
              switch (map_of_dimm_num) {
                case 0x00:
                  sprintf(temp_log, "DIMMA0(%02X) %s, FRU: %u", map_of_dimm_num, err_str, fru);
                  break;
                case 0x01:
                  sprintf(temp_log, "DIMMA1(%02X) %s, FRU: %u", map_of_dimm_num, err_str, fru);
                  break;
                case 0x08:
                  sprintf(temp_log, "DIMMB0(%02X) %s, FRU: %u", map_of_dimm_num, err_str, fru);
                  break;
                case 0x09:
                  sprintf(temp_log, "DIMMB1(%02X) %s, FRU: %u", map_of_dimm_num, err_str, fru);
                  break;
              }
              pal_add_cri_sel(temp_log);
            }
            /* Then continue parse the error into a string. */
            /* All Info Valid                               */
            strcpy(temp_log, "");
            sprintf(temp_log, " (CPU# %d, CHN# %d, DIMM# %d)",
                (ed[2] & 0xE0) >> 5, (ed[2] & 0x18) >> 3, ed[2] & 0x7);
          } else if (((ed[1] & 0xC) >> 2) == 0x1) {
            /* DIMM info not valid */
            sprintf(temp_log, " (CPU# %d, CHN# %d)",
                (ed[2] & 0xE0) >> 5, (ed[2] & 0x18) >> 3);
          } else if (((ed[1] & 0xC) >> 2) == 0x2) {
            /* CHN info not valid */
            sprintf(temp_log, " (CPU# %d, DIMM# %d)",
                (ed[2] & 0xE0) >> 5, ed[2] & 0x7);
          } else if (((ed[1] & 0xC) >> 2) == 0x3) {
            /* CPU info not valid */
            sprintf(temp_log, " (CHN# %d, DIMM# %d)",
                (ed[2] & 0x18) >> 3, ed[2] & 0x7);
          }
          strcat(error_log, temp_log);
          break;
      }
      break;
    case (FRU_SCC):
      parsed = true;
      switch (event_type) {
        case SENSOR_SPECIFIC:
          switch (snr_type) {
            case DIGITAL_DISCRETE:
              switch (ed[0] & 0x0F) {
                //Sensor Type Code, Physical Security 0x5h, SENSOR_SPECIFIC Offset 0x0h General Chassis Intrusion
                case 0x0:
                  if (!event_dir)
                    sprintf(error_log, "Drawer be Pulled Out");
                  else
                    sprintf(error_log, "Drawer be Pushed Back");
                  break;
              }
              break;
          }
          break;

        case GENERIC:
          if (ed[0] & 0x0F)
            sprintf(error_log, "ASSERT, Limit Exceeded");
          else
            sprintf(error_log, "DEASSERT, Limit Not Exceeded");
          break;

        default:
          sprintf(error_log, "Unknown");
          break;
      }
      break;
    default:
      break;
  }

  // If this message was not a platform specific message, just run a
  // common SEL parsing routine
  if (!parsed)
    pal_parse_sel_helper(fru, sel, error_log);

  return 0;
}

int
pal_set_sensor_health(uint8_t fru, uint8_t value) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  switch(fru) {
    case FRU_SLOT1:
      sprintf(key, "slot%d_sensor_health", fru);
      break;
    case FRU_IOM:
      sprintf(key, "iom_sensor_health");
      break;
    case FRU_DPB:
      sprintf(key, "dpb_sensor_health");
      break;
    case FRU_SCC:
      sprintf(key, "scc_sensor_health");
      break;
    case FRU_NIC:
      sprintf(key, "nic_sensor_health");
      break;

    default:
      return -1;
  }

  sprintf(cvalue, (value > 0) ? "1": "0");

  return pal_set_key_value(key, cvalue);
}

int
pal_get_fru_health(uint8_t fru, uint8_t *value) {

  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret;

  switch(fru) {
    case FRU_SLOT1:
      sprintf(key, "slot%d_sensor_health", fru);
      break;
    case FRU_IOM:
      sprintf(key, "iom_sensor_health");
      break;
    case FRU_DPB:
      sprintf(key, "dpb_sensor_health");
      break;
    case FRU_SCC:
      sprintf(key, "scc_sensor_health");
      break;
    case FRU_NIC:
      sprintf(key, "nic_sensor_health");
      break;

    default:
      return -1;
  }

  ret = pal_get_key_value(key, cvalue);
  if (ret) {
    return ret;
  }

  *value = atoi(cvalue);

  memset(key, 0, MAX_KEY_LEN);
  memset(cvalue, 0, MAX_VALUE_LEN);

  switch(fru) {
    case FRU_SLOT1:
      sprintf(key, "slot%d_sel_error", fru);
      break;

    case FRU_IOM:
      return 0;

    case FRU_DPB:
      return 0;

    case FRU_SCC:
      return 0;

    case FRU_NIC:
      return 0;

    default:
      return -1;
  }

  ret = pal_get_key_value(key, cvalue);
  if (ret) {
    return ret;
  }

  *value = *value & atoi(cvalue);
  return 0;
}
// TBD
void
pal_inform_bic_mode(uint8_t fru, uint8_t mode) {
  switch(mode) {
  case BIC_MODE_NORMAL:
    // Bridge IC entered normal mode
    // Inform BIOS that BMC is ready
    //bic_set_gpio(fru, GPIO_BMC_READY_N, 0);
    break;
  case BIC_MODE_UPDATE:
    // Bridge IC entered update mode
    // TODO: Might need to handle in future
    break;
  default:
    break;
  }
}

int
pal_get_fan_name(uint8_t num, char *name) {

  switch(DPB_SENSOR_FAN1_FRONT+num) {

    case DPB_SENSOR_FAN1_FRONT:
      sprintf(name, "Fan 1 Front");
      break;

    case DPB_SENSOR_FAN1_REAR:
      sprintf(name, "Fan 1 Rear");
      break;

    case DPB_SENSOR_FAN2_FRONT:
      sprintf(name, "Fan 2 Front");
      break;

    case DPB_SENSOR_FAN2_REAR:
      sprintf(name, "Fan 2 Rear");
      break;

    case DPB_SENSOR_FAN3_FRONT:
      sprintf(name, "Fan 3 Front");
      break;

    case DPB_SENSOR_FAN3_REAR:
      sprintf(name, "Fan 3 Rear");
      break;

    case DPB_SENSOR_FAN4_FRONT:
      sprintf(name, "Fan 4 Front");
      break;

    case DPB_SENSOR_FAN4_REAR:
      sprintf(name, "Fan 4 Rear");
      break;

    default:
      return -1;
  }

  return 0;
}

static int
write_fan_value(const int fan, const char *device, const int value) {
  char full_name[LARGEST_DEVICE_NAME];
  char device_name[LARGEST_DEVICE_NAME];
  char output_value[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR, device_name);
  snprintf(output_value, LARGEST_DEVICE_NAME, "%d", value);
  return write_device(full_name, output_value);
}


int
pal_set_fan_speed(uint8_t fan, uint8_t pwm) {
  int unit;
  int ret;

  if (fan >= pal_pwm_cnt) {
    syslog(LOG_INFO, "pal_set_fan_speed: fan number is invalid - %d", fan);
    return -1;
  }

  // Convert the percentage to our 1/100th unit.
  unit = pwm * PWM_UNIT_MAX / 100;

  // For 0%, turn off the PWM entirely
  if (unit == 0) {
    ret = write_fan_value(fan, "pwm%d_en", 0);
    if (ret < 0) {
      syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
      return -1;
    }
    return 0;

  // For 100%, set falling and rising to the same value
  } else if (unit == PWM_UNIT_MAX) {
    unit = 0;
  }

  ret = write_fan_value(fan, "pwm%d_type", 0);
  if (ret < 0) {
    syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
    return -1;
  }

  ret = write_fan_value(fan, "pwm%d_rising", 0);
  if (ret < 0) {
    syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
    return -1;
  }

  ret = write_fan_value(fan, "pwm%d_falling", unit);
  if (ret < 0) {
    syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
    return -1;
  }

  ret = write_fan_value(fan, "pwm%d_en", 1);
  if (ret < 0) {
    syslog(LOG_INFO, "set_fan_speed: write_fan_value failed");
    return -1;
  }

  return 0;
}

int
pal_get_fan_speed(uint8_t fan, int *rpm) {
  int ret;
  float value;
  // Fan value have to be fetch real value from DPB, so have to use pal_sensor_read_raw to force updating cache value
  ret = pal_sensor_read_raw(FRU_DPB, DPB_SENSOR_FAN1_FRONT + fan , &value);

  if (ret == 0)
    *rpm = (int) value;

  return ret;
}

void
pal_update_ts_sled()
{
  char key[MAX_KEY_LEN] = {0};
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%ld", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

int
pal_handle_dcmi(uint8_t fru, uint8_t *request, uint8_t req_len, uint8_t *response, uint8_t *rlen) {
  return bic_me_xmit(fru, request, req_len, response, rlen);
}
//For Merge Yosemite and TP
int
pal_get_platform_id(uint8_t *id) {


  return 0;
}
int
pal_get_board_rev_id(uint8_t *id) {

  return 0;
}
int
pal_get_mb_slot_id(uint8_t *id) {

  return 0;
}
int
pal_get_slot_cfg_id(uint8_t *id) {

  return 0;
}

int
pal_get_boot_order(uint8_t slot, uint8_t *req_data, uint8_t *boot, uint8_t *res_len) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  sprintf(key, "slot1_boot_order");

  ret = pal_get_key_value(key, str);
  if (ret) {
    *res_len = 0;
    return ret;
  }

  for (i = 0; i < 2*SIZE_BOOT_ORDER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    boot[j++] = (msb << 4) | lsb;
  }

  *res_len = SIZE_BOOT_ORDER;

  return 0;
}

int
pal_set_boot_order(uint8_t slot, uint8_t *boot, uint8_t *res_data, uint8_t *res_len) {
  int i;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};

  *res_len = 0;

  sprintf(key, "slot1_boot_order");

  for (i = 0; i < SIZE_BOOT_ORDER; i++) {
    snprintf(tstr, 3, "%02x", boot[i]);
    strncat(str, tstr, 3);
  }

  return pal_set_key_value(key, str);
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {

  return 0;
}

void
pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {
   char str_server_por_cfg[64];
   char buff[MAX_VALUE_LEN];
   int policy = 3;
   uint8_t status, ret;
   unsigned char *data = res_data;

   // Platform Power Policy
   memset(str_server_por_cfg, 0 , sizeof(char) * 64);
   sprintf(str_server_por_cfg, "%s", "slot1_por_cfg");

   if (pal_get_key_value(str_server_por_cfg, buff) == 0)
   {
     if (!memcmp(buff, "off", strlen("off")))
       policy = POWER_CFG_OFF;
     else if (!memcmp(buff, "lps", strlen("lps")))
       policy = POWER_CFG_LPS;
     else if (!memcmp(buff, "on", strlen("on")))
       policy = POWER_CFG_ON;
     else
       policy = POWER_CFG_UKNOWN;
   }

  // Current Power State
  ret = pal_get_server_power(FRU_SLOT1, &status);
  if (ret >= 0) {
    switch (status) {
      case SERVER_POWER_ON:
        status = SERVER_POWER_ON;
        break;
      case SERVER_POWER_OFF:
      case SERVER_12V_OFF:
        status = SERVER_POWER_OFF;
        break;
      default:
        status = SERVER_POWER_OFF;
        break;
    }
    *data++ = status | (policy << 5);
  } else {
    // load default
    syslog(LOG_WARNING, "ipmid: pal_get_server_power failed for slot1\n");
    *data++ = 0x00 | (policy << 5);
  }
   *data++ = 0x00;   // Last Power Event
   *data++ = 0x40;   // Misc. Chassis Status
   *data++ = 0x00;   // Front Panel Button Disable
   *res_len = data - res_data;
}

void
pal_log_clear(char *fru) {
  if (!strcmp(fru, "server")) {
    pal_set_key_value("slot1_sensor_health", "1");
    pal_set_key_value("slot1_sel_error", "1");
  } else if (!strcmp(fru, "iom")) {
    pal_set_key_value("iom_sensor_health", "1");
  } else if (!strcmp(fru, "dpb")) {
    pal_set_key_value("dpb_sensor_health", "1");
  } else if (!strcmp(fru, "scc")) {
    pal_set_key_value("scc_sensor_health", "1");
  }  else if (!strcmp(fru, "nic")) {
    pal_set_key_value("nic_sensor_health", "1");
  } else if (!strcmp(fru, "all")) {
    pal_set_key_value("slot1_sensor_health", "1");
    pal_set_key_value("slot1_sel_error", "1");
    pal_set_key_value("iom_sensor_health", "1");
    pal_set_key_value("dpb_sensor_health", "1");
    pal_set_key_value("scc_sensor_health", "1");
    pal_set_key_value("nic_sensor_health", "1");
    pal_set_key_value("heartbeat_health", "1");
    pal_set_key_value("fru_prsnt_health", "1");
    pal_set_key_value("bmc_health", "1");
  }
}

// To get the platform sku
int pal_get_sku(void){
  int pal_sku = 0;

  // PAL_SKU[6:4] = {SCC_RMT_TYPE_0, SLOTID_0, SLOTID_1}
  // PAL_SKU[3:0] = {IOM_TYPE0, IOM_TYPE1, IOM_TYPE2, IOM_TYPE3}
  if (read_device(PLATFORM_FILE, &pal_sku)) {
    printf("Get platform SKU failed\n");
    return -1;
  }

  return pal_sku;
}
// To get the BMC location
int pal_get_iom_location(void){
  int pal_sku = 0, slotid = 0;

  // SLOTID_[0:1]: 01=IOM_A; 10=IOM_B
  pal_sku = pal_get_sku();
  slotid = ((pal_sku >> 4) & 0x03);
  return slotid;
}
// To get the IOM type
int pal_get_iom_type(void){
  int pal_sku = 0, iom_type = 0;

  // Type [0:3]: 0001=M.2 solution; 0010=IOC solution
  pal_sku = pal_get_sku();
  iom_type = (pal_sku & 0x0F);
  return iom_type;
}

int pal_is_scc_stb_pwrgood(void){
//To do: to get SCC STB Power good from IO Exp via I2C
return 0;
}
int pal_is_scc_full_pwrgood(void){
//To do: to get SCC STB Power good from IO Exp via I2C
return 0;
}
int pal_is_iom_full_pwrgood(void){
//To do: to get IOM PWR GOOD IOM_FULL_PGOOD    GPIOAB2
return 0;
}
int pal_en_scc_stb_pwr(void){
//To do: enable SCC STB PWR; SCC_STBY_PWR_EN  GPIOF4   44
return 0;
}
int pal_en_scc_full_pwr(void){
//To do: ENABLE SCC STB PWR; SCC_LOC_FULL_PWR_ENGPIOF0   40
return 0;
}
int pal_en_iom_full_pwr(void){
//To do: enable iom PWR  ;IOM_FULL_PWR_EN      GPIOAA7
return 0;
}

//For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7e
int pal_get_plat_sku_id(void){
  int sku = 0;
  int location = 0;
  uint8_t platform_info;

  sku = pal_get_iom_type();
  location = pal_get_iom_location();

  if(sku == IOM_M2) {
    if(location == IOM_SIDEA) {
      platform_info = PLAT_INFO_SKUID_TYPE5A;
    }
    else if(location == IOM_SIDEB) {
      platform_info = PLAT_INFO_SKUID_TYPE5B;
    }
    else
      return -1;
  }
  else if (sku == IOM_IOC) {
    platform_info = PLAT_INFO_SKUID_TYPE7SS;
  }
  else
    return -1;

  return platform_info;
}

//Use part of the function for OEM Command "CMD_OEM_GET_POSS_PCIE_CONFIG" 0xF4
int pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len){

  int sku = 0;
  uint8_t pcie_conf = 0x00;
  uint8_t completion_code = CC_UNSPECIFIED_ERROR;
  unsigned char *data = res_data;
  sku = pal_get_iom_type();

  if(sku == IOM_M2)
    pcie_conf = PICE_CONFIG_TYPE5;
  else if (sku == IOM_IOC)
    pcie_conf = PICE_CONFIG_TYPE7;
  else
    return completion_code;

  *data++ = pcie_conf;
  *res_len = data - res_data;
  completion_code = CC_SUCCESS;

  return completion_code;
}

int pal_minisas_led(uint8_t port, uint8_t operation) {
  //operation - 1: on, 0: off

  if (port == SAS_EXT_PORT_1) {
    if (operation == LED_ON)
      return set_gpio_value(BMC_EXT1_LED_Y_N, LED_N_ON);
    else
      return set_gpio_value(BMC_EXT1_LED_Y_N, LED_N_OFF);
  } else if (port == SAS_EXT_PORT_2) {
    if (operation == LED_ON)
      return set_gpio_value(BMC_EXT2_LED_Y_N, LED_N_ON);
    else
      return set_gpio_value(BMC_EXT2_LED_Y_N, LED_N_OFF);
  } else {
    syslog(LOG_WARNING, "%s(): Unexpected mini sas port %d", __func__, port+1);
  }

  return 0;
}

int
pal_get_pwm_value(uint8_t fan_num, uint8_t *value) {
  char path[64] = {0};
  char device_name[64] = {0};
  int val = 0;
  int pwm_enable = 0;

  snprintf(device_name, LARGEST_DEVICE_NAME, "pwm%d_en", fanid2pwmid_mapping[fan_num]);
  snprintf(path, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR, device_name);
  if (read_device(path, &pwm_enable)) {
    syslog(LOG_INFO, "pal_get_pwm_value: read %s failed", path);
    return -1;
  }

  // Check the PWM is enable or not
  if(pwm_enable) {
    // fan number should in this range
    if(fan_num >= 0 && fan_num <= 11)
      snprintf(device_name, LARGEST_DEVICE_NAME, "pwm%d_falling", fanid2pwmid_mapping[fan_num]);
    else {
      syslog(LOG_INFO, "pal_get_pwm_value: fan number is invalid - %d", fan_num);
      return -1;
    }

    snprintf(path, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR, device_name);

    if (read_device_hex(path, &val)) {
      syslog(LOG_INFO, "pal_get_pwm_value: read %s failed", path);
      return -1;
    }
    if(val)
      *value = (100 * val) / PWM_UNIT_MAX;
    else
      // 0 means duty cycle is 100%
      *value = 100;
  }
  else
    //PWM is disable
    *value = 0;


  return 0;
}

int
pal_fan_dead_handle(int fan_num) {

  // TODO: Add action in case of fan dead
  return 0;
}

int
pal_fan_recovered_handle(int fan_num) {

  // TODO: Add action in case of fan recovered
  return 0;
}

int pal_expander_sensor_check(uint8_t fru, uint8_t sensor_num) {
  int ret;
  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  int timestamp, timestamp_flag = 0, current_time, tolerance = 0;
  //clock_gettime parameters
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  int sensor_cnt = 1; //default is single call, so just 1 sensor
  uint8_t *sensor_list;

  switch(fru) {
    case FRU_DPB:
      sprintf(key, "dpb_sensor_timestamp");
      break;
    case FRU_SCC:
      sprintf(key, "scc_sensor_timestamp");
      break;
    default:
      return -1;
  }

  ret = pal_get_edb_value(key, cvalue);
    if (ret < 0) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_expander_sensor_check: pal_get_key_value failed for "
          "fru %u", fru);
#endif
      return ret;
    }

  timestamp = atoi(cvalue);

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%ld", ts.tv_sec);
  current_time = atoi(tstr);

  //set 1 sec tolerance for Firsr Sensor Number, to avoid updating all FRU sensor when interval around 4.9999 second
  if (sensor_num == DPB_FIRST_SENSOR_NUM || sensor_num == SCC_FIRST_SENSOR_NUM) {
    tolerance = 1;
    timestamp_flag = 1; //Update only after First sensor update
    //Get FRU sensor list for update all sensor
    ret = pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
      if (ret < 0) {
        return ret;
    }
  }

  //timeout: 5 second, 1 second tolerance only for First sensor
  if ( abs(current_time - timestamp) > (5 - tolerance) ) {
    //SCC
    switch(fru) {
      case FRU_SCC:
        ret = pal_exp_scc_read_sensor_wrapper(fru, sensor_list, sensor_cnt, sensor_num);
        if (ret < 0) {
          return ret;
        }
      break;
      case FRU_DPB:
      // DPB sensors are too much, needs twice ipmb commands
        ret = pal_exp_dpb_read_sensor_wrapper(fru, sensor_list, MAX_EXP_IPMB_SENSOR_COUNT, sensor_num, 0);
        if (ret < 0) {
          return ret;
        }
        //DO the Second transaction only when sensor number is the first in DPB sensor list
        if (sensor_num == DPB_FIRST_SENSOR_NUM) {
          ret = pal_exp_dpb_read_sensor_wrapper(fru, sensor_list, (sensor_cnt - MAX_EXP_IPMB_SENSOR_COUNT), sensor_num, 1);
          if (ret < 0) {
            return ret;
          }
        }
      break;
    }

    if (timestamp_flag) {
      //update timestamp after Updated Expander sensor
      clock_gettime(CLOCK_REALTIME, &ts);
      sprintf(tstr, "%ld", ts.tv_sec);
      pal_set_edb_value(key, tstr);
    }
  }
  return 0;
}

int
pal_exp_dpb_read_sensor_wrapper(uint8_t fru, uint8_t *sensor_list, int sensor_cnt, uint8_t sensor_num, int second_transaction) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  int ret, i = 0;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  float value;
  char units[64];
  int offset = 0; //sensor overload offset

  if (second_transaction)
    offset = MAX_EXP_IPMB_SENSOR_COUNT;

  //Fill up sensor number
  if (sensor_num == DPB_FIRST_SENSOR_NUM) {
    tbuf[0] = sensor_cnt;
     for( i = 0 ; i < sensor_cnt; i++) {
      tbuf[i+1] = sensor_list[i + offset];  //feed sensor number to tbuf
    }
    tlen = sensor_cnt + 1;
  }
  else {
    sensor_cnt = 1;
    tbuf[0] = sensor_cnt;
    tbuf[1] = sensor_num;
    tlen = 2;
  }

  //send tbuf with sensor count and numbers to get spcific sensor data from exp
  ret = expander_ipmb_wrapper(NETFN_OEM_REQ, CMD_EXP_GET_SENSOR_READING, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    #ifdef DEBUG
      syslog(LOG_WARNING, "%s: expander_ipmb_wrapper failed.", __func__);
    #endif

    //if expander doesn't respond, set all sensors value to NA and save to cache
    for(i = 0; i < sensor_cnt; i++) {
      sprintf(key, "dpb_sensor%d", tbuf[i+1]);
      sprintf(str, "NA");

      if(kv_set(key, str, 0, 0) < 0) {
        #ifdef DEBUG
          syslog(LOG_WARNING, "%s: cache_set key = %s, str = %s failed.", __func__ , key, str);
        #endif
      }
    }
    return ret;
  }

  for(i = 0; i < sensor_cnt; i++) {
    if (rbuf[5*i+4] != 0) {
      //if sensor status byte is not 0, means sensor reading is unavailable
      sprintf(str, "NA");
    }
    else {
      // search the corresponding sensor table to fill up the raw data and status
      // rbuf[5*i+1] sensor number
      // rbuf[5*i+2] sensor raw data1
      // rbuf[5*i+3] sensor raw data2
      // rbuf[5*i+4] sensor status
      // rbuf[5*i+5] reserved
      fbttn_sensor_units(fru, rbuf[5*i+1], units);
      if( strcmp(units,"C") == 0 ) {
        value = rbuf[5*i+2];
      }
      else if( rbuf[5*i+1] >= DPB_SENSOR_FAN1_FRONT && rbuf[5*i+1] <= DPB_SENSOR_FAN4_REAR ) {
        value =  (((rbuf[5*i+2] << 8) + rbuf[5*i+3]));
        value = value * 10;
      }
      else if( rbuf[5*i+1] == DPB_SENSOR_HSC_POWER || rbuf[5*i+1] == DPB_SENSOR_12V_POWER_CLIP || (rbuf[5*i+1] == AIRFLOW) ) {
        value =  (((rbuf[5*i+2] << 8) + rbuf[5*i+3]));
      }
      else {
        value =  (((rbuf[5*i+2] << 8) + rbuf[5*i+3]));
        value = value/100;
      }

      sprintf(str, "%.2f",(float)value);
    }

    //cache sensor reading
    sprintf(key, "dpb_sensor%d", rbuf[5*i+1]);
    if(kv_set(key, str, 0, 0) < 0) {
      #ifdef DEBUG
        syslog(LOG_WARNING, "%s: cache_set key = %s, str = %s failed.", __func__ , key, str);
      #endif
    }
  }

  return 0;
}

int
pal_exp_scc_read_sensor_wrapper(uint8_t fru, uint8_t *sensor_list, int sensor_cnt, uint8_t sensor_num) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  int ret, i;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  float value;
  char units[64];
  uint8_t status;

  tbuf[0] = sensor_cnt; //sensor_count
  //Fill up sensor number
  if (sensor_num ==  SCC_FIRST_SENSOR_NUM) {
    for( i = 0 ; i < sensor_cnt; i++) {
      tbuf[i+1] = sensor_list[i];  //feed sensor number to tbuf
    }
  }
  else {
    tbuf[1] = sensor_num;
  }

  tlen = sensor_cnt + 1;

  //send tbuf with sensor count and numbers to get spcific sensor data from exp
  ret = expander_ipmb_wrapper(NETFN_OEM_REQ, CMD_EXP_GET_SENSOR_READING, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    #ifdef DEBUG
      syslog(LOG_WARNING, "%s: expander_ipmb_wrapper failed.", __func__);
    #endif

    //if expander doesn't respond, set all sensors value to NA and save to cache
    for(i = 0; i < sensor_cnt; i++) {
      sprintf(key, "scc_sensor%d", tbuf[i+1]);
      sprintf(str, "NA");

      if(kv_set(key, str, 0, 0) < 0) {
        #ifdef DEBUG
          syslog(LOG_WARNING, "%s: cache_set key = %s, str = %s failed.", __func__ , key, str);
        #endif
      }
    }
    return ret;
  }

  for(i = 0; i < sensor_cnt; i++) {
    if (rbuf[5*i+4] != 0) {
      //if sensor status byte is not 0, means sensor reading is unavailable
      sprintf(str, "NA");
    }
    else {
      // search the corresponding sensor table to fill up the raw data and status
      // rbuf[5*i+1] sensor number
      // rbuf[5*i+2] sensor raw data1
      // rbuf[5*i+3] sensor raw data2
      // rbuf[5*i+4] sensor status
      // rbuf[5*i+5] reserved
      fbttn_sensor_units(fru, rbuf[5*i+1], units);

      if( strcmp(units,"C") == 0 ) {
        value = rbuf[5*i+2];
      }
      else if( strcmp(units,"Watts") == 0 ) {
        value = (((rbuf[5*i+2] << 8) + rbuf[5*i+3]));
      }
      else {
        value = (((rbuf[5*i+2] << 8) + rbuf[5*i+3]));
        value = value/100;
      }

      sprintf(str, "%.2f",(float)value);

      // SCC_IOC have to check if the server is on, if not shows "NA"
      if (rbuf[5*i+1] == SCC_SENSOR_IOC_TEMP) {
        pal_get_server_power(FRU_SLOT1, &status);
        if (status != SERVER_POWER_ON) {
          strcpy(str, "NA");
        }
      }
    }

    //cache sensor reading
    sprintf(key, "scc_sensor%d", rbuf[5*i+1]);
    if(kv_set(key, str, 0, 0) < 0) {
      #ifdef DEBUG
        syslog(LOG_WARNING, "%s: cache_set key = %s, str = %s failed.", __func__, key, str);
      #endif
    }
  }

  return 0;
}

int  pal_get_bmc_rmt_hb(void) {
  int bmc_rmt_hb = 0;
  char path[64] = {0};

  sprintf(path, TACH_RPM, TACH_BMC_RMT_HB);

  if (read_device(path, &bmc_rmt_hb)) {
    return -1;
  }

  return bmc_rmt_hb;
}

int  pal_get_scc_loc_hb(void) {
  int scc_loc_hb = 0;
  char path[64] = {0};

  sprintf(path, TACH_RPM, TACH_SCC_LOC_HB);

  if (read_device(path, &scc_loc_hb)) {
    return -1;
  }

  return scc_loc_hb;
}

int  pal_get_scc_rmt_hb(void) {
  int scc_rmt_hb = 0;
  char path[64] = {0};

  sprintf(path, TACH_RPM, TACH_SCC_RMT_HB);

  if (read_device(path, &scc_rmt_hb)) {
    return -1;
  }

  return scc_rmt_hb;
}

void pal_err_code_enable(unsigned char num) {
  error_code errorCode = {0, 0};

  if(num < 100) {  // It's used for expander (0~99).
    return;
  }

  errorCode.code = num;
  if (errorCode.code < (ERROR_CODE_NUM * 8)) {
    errorCode.status = 1;
    pal_write_error_code_file(&errorCode);
  } else {
    syslog(LOG_WARNING, "%s(): wrong error code number", __func__);
  }
}

void pal_err_code_disable(unsigned char num) {
  error_code errorCode = {0, 0};

  if(num < 100) {
    return;
  }

  errorCode.code = num;
  if (errorCode.code < (ERROR_CODE_NUM * 8)) {
    errorCode.status = 0;
    pal_write_error_code_file(&errorCode);
  } else {
    syslog(LOG_WARNING, "%s(): wrong error code number", __func__);
  }
}

uint8_t
pal_read_error_code_file(uint8_t *error_code_array) {
  FILE *fp = NULL;
  uint8_t ret = 0;
  int i = 0;
  int tmp = 0;
  int retry_count = 0;

  if (access(ERR_CODE_FILE, F_OK) == -1) {
    memset(error_code_array, 0, ERROR_CODE_NUM);
    return 0;
  } else {
    fp = fopen(ERR_CODE_FILE, "r");
  }

  if (!fp) {
    return -1;
  }

  ret = flock(fileno(fp), LOCK_EX | LOCK_NB);
  while (ret && (retry_count < 3)) {
    retry_count++;
    msleep(100);
    ret = flock(fileno(fp), LOCK_EX | LOCK_NB);
  }
  if (ret) {
    int err = errno;
    syslog(LOG_WARNING, "%s(): failed to flock on %s, err %d", __func__, ERR_CODE_FILE, err);
    fclose(fp);
    return -1;
  }

  for (i = 0; fscanf(fp, "%X", &tmp) != EOF && i < ERROR_CODE_NUM; i++) {
    error_code_array[i] = (uint8_t) tmp;
  }

  flock(fileno(fp), LOCK_UN);
  fclose(fp);
  return 0;
}

uint8_t
pal_write_error_code_file(error_code *update) {
  FILE *fp = NULL;
  uint8_t ret = 0;
  int retry_count = 0;
  int i = 0;
  int stat = 0;
  int bit_stat = 0;
  uint8_t error_code_array[ERROR_CODE_NUM] = {0};

  if (pal_read_error_code_file(error_code_array) != 0) {
    syslog(LOG_ERR, "%s(): pal_read_error_code_file() failed", __func__);
    return -1;
  }

  if (access(ERR_CODE_FILE, F_OK) == -1) {
    fp = fopen(ERR_CODE_FILE, "w");
  } else {
    fp = fopen(ERR_CODE_FILE, "r+");
  }
  if (!fp) {
    syslog(LOG_ERR, "%s(): open %s failed. %s", __func__, ERR_CODE_FILE, strerror(errno));
    return -1;
  }

  ret = flock(fileno(fp), LOCK_EX | LOCK_NB);
  while (ret && (retry_count < 3)) {
    retry_count++;
    msleep(100);
    ret = flock(fileno(fp), LOCK_EX | LOCK_NB);
  }
  if (ret) {
    syslog(LOG_WARNING, "%s(): failed to flock on %s. %s", __func__, ERR_CODE_FILE, strerror(errno));
    fclose(fp);
    return -1;
  }

  stat = update->code / 8;
  bit_stat = update->code % 8;

  if (update->status) {
    error_code_array[stat] |= 1 << bit_stat;
  } else {
    error_code_array[stat] &= ~(1 << bit_stat);
  }

  for (i = 0; i < ERROR_CODE_NUM; i++) {
    fprintf(fp, "%X ", error_code_array[i]);
    if(error_code_array[i] != 0) {
      ret = 1;
    }
  }

  fprintf(fp, "\n");
  flock(fileno(fp), LOCK_UN);
  fclose(fp);

  return ret;
}

/*
 * Calculate the sum of error code
 * If Err happen, the sum does not equal to 0
 *
 */
unsigned char pal_sum_error_code(void) {
  uint8_t error_code_array[ERROR_CODE_NUM] = {0};
  int i;

  if (pal_read_error_code_file(error_code_array) != 0) {
    syslog(LOG_ERR, "%s(): pal_read_error_code_file() failed", __func__);
    return -1;
  }
  for (i = 0; i < ERROR_CODE_NUM; i++) {
    if (error_code_array[i] != 0) {
      return 1;
    }
  }

  return 0;
}
void
pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  uint8_t status;

  if ((snr_num == MEZZ_SENSOR_TEMP) && (thresh == UNR_THRESH)) {

    pal_get_server_power(FRU_SLOT1, &status);
    if (status != SERVER_12V_OFF) {
      pal_nic_otp(FRU_NIC, snr_num, nic_sensor_threshold[snr_num][UNR_THRESH]);
    }
  }
  return;
}

void
pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  uint8_t status;

  if ((snr_num == MEZZ_SENSOR_TEMP) && (thresh == UNC_THRESH)) {
    pal_get_server_power(FRU_SLOT1, &status);
    if ((status != SERVER_12V_ON) && (otp_server_12v_off_flag == 1)) {
      // power on Mono Lake 12V HSC
      syslog(LOG_CRIT, "Due to NIC temp UNC deassert. Power On Server 12V. (val = %.2f)", val);
      server_12v_on(FRU_SLOT1);
      otp_server_12v_off_flag = 0;
    }
  }
  return;
}

void
pal_post_end_chk(uint8_t *post_end_chk) {
  return;
}

int
pal_get_fw_info(uint8_t fru, unsigned char target, unsigned char* res, unsigned char* res_len) {
  if(target > TARGET_VR_PVCCSCUS_VER)
    return -1;
  if( target!= TARGET_BIOS_VER ) {
    bic_get_fw_ver(FRU_SLOT1, target, res);
    if( target == TARGET_BIC_VER)
      *res_len = 2;
    else
      *res_len = 4;
    return 0;
  }
  if( target == TARGET_BIOS_VER ) {
  pal_get_sysfw_ver(FRU_SLOT1, res);
      *res_len = 2 + res[2];
    return 0;
  }
  return -1;
}

int
pal_self_tray_location(uint8_t *value) {

  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_CHASSIS_INTRUSION);
  if (read_device(path, &val))
    return -1;

  *value = (uint8_t) val;

  return 0;
}

int
pal_get_iom_ioc_ver(uint8_t *ver) {
  return mctp_get_iom_ioc_ver(ver);
}

int
pal_oem_bitmap(uint8_t* in_error,uint8_t* data) {
  int ret = 0;
  int ii=0;
  int kk=0;
  int NUM = 0;
  if(data == NULL)
  {
    return 0;
  }
  for(ii = 0; ii < 32; ii++)
  {
    for(kk = 0; kk < 8; kk++)
    {
      if(((in_error[ii] >> kk)&0x01) == 1)
      {
        NUM = ii*8 + kk;
        *(data + ret) = NUM;
        ret++;
      }
    }
  }
  return ret;
}

int
pal_get_error_code(uint8_t data[MAX_ERROR_CODES], uint8_t* error_count) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = 0;
  FILE *fp;
  uint8_t exp_error[13];
  uint8_t error[32];
  int ret, count = 0;

 ret = expander_ipmb_wrapper(NETFN_OEM_REQ, EXPANDER_ERROR_CODE, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    printf("Error: Expander Error Code Query Fail...\n");
    #ifdef DEBUG
       syslog(LOG_WARNING, "enclosure-util: get_error_code, expander_ipmb_wrapper failed.");
    #endif
    memset(exp_error, 0, 13); //When Epander Fail, fill all data to 0
  }
  else
    memcpy(exp_error, rbuf, rlen);

  exp_error[0] &= ~(0x1); //Expander Error Code 1 is no Error, Ignore it.

  fp = fopen("/tmp/error_code.bin", "r");
  if (!fp) {
    printf("fopen Fail: %s,  Error Code: %d\n", strerror(errno), errno);
    #ifdef DEBUG
      syslog(LOG_WARNING, "enclosure-util get_error_code, BMC error code File open failed...\n");
    #endif
    memset(error, 0, 32); //When BMC Error Code file Open Fail, fill all data to 0
  }
  else {
    lockf(fileno(fp),F_LOCK,0L);
    while (fscanf(fp, "%hhX", error+count) != EOF && count!=32) {
      count++;
    }
    lockf(fileno(fp),F_ULOCK,0L);
    fclose(fp);
  }

  //Expander Error Code 0~99; BMC Error Code 100~255
  memcpy(error, exp_error, rlen - 1); //Not the last one (12th)
  error[12] = ((error[12] & 0xF0) + (exp_error[12] & 0xF));
  memset(data, 0, MAX_ERROR_CODES);
  *error_count = pal_oem_bitmap(error, data);
  return 0;
}

// Get post code buffer of the given slot from BIC
int
pal_post_get_buffer(uint8_t *buffer, uint8_t *buf_len) {
  int ret;
  uint8_t buf[MAX_IPMB_RES_LEN] = {0x0};
  uint8_t len;

  ret = bic_get_post_buf(FRU_SLOT1, buf, &len);
  if (ret)
    return ret;

  // The post buffer is LIFO and the first byte gives the latest post code
  memcpy(buffer, buf, len);
  *buf_len = len;

  return 0;
}

void
pal_add_cri_sel(char *str)
{
  char cmd[128];
  snprintf(cmd, 128, "logger -p local0.err \"%s\"",str);
  system(cmd);
}

void
pal_i2c_crash_assert_handle(int i2c_bus_num) {
  // I2C bus number: 0~13
  if (i2c_bus_num < I2C_BUS_MAX_NUMBER) {
    pal_err_code_enable(ERR_CODE_I2C_CRASH_BASE + i2c_bus_num);
  } else {
    syslog(LOG_WARNING, "%s(): wrong I2C bus number", __func__);
  }
}

void
pal_i2c_crash_deassert_handle(int i2c_bus_num) {
  // I2C bus number: 0~13
  if (i2c_bus_num < I2C_BUS_MAX_NUMBER) {
    pal_err_code_disable(ERR_CODE_I2C_CRASH_BASE + i2c_bus_num);
  } else {
    syslog(LOG_WARNING, "%s(): wrong I2C bus number", __func__);
  }
}

int
pal_set_bios_current_boot_list(uint8_t slot, uint8_t *boot_list, uint8_t list_length, uint8_t *cc) {
  FILE *fp;
  int i;

  if(*boot_list != 1) {
    *cc = CC_INVALID_PARAM;
    return -1;
  }

  fp = fopen(BOOT_LIST_FILE, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", BOOT_LIST_FILE);
#endif
    return err;
  }
  lockf(fileno(fp),F_LOCK,0L);

  for(i = 0; i < list_length; i++) {
    fprintf(fp, "%02X ", *(boot_list+i));
  }
  fprintf(fp, "\n");
  lockf(fileno(fp),F_ULOCK,0L);
  fclose(fp);
  return 0;
}

int
pal_get_bios_current_boot_list(uint8_t slot, uint8_t *boot_list, uint8_t *list_length) {
  FILE *fp;
  uint8_t count=0;

  fp = fopen(BOOT_LIST_FILE, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", BOOT_LIST_FILE);
#endif
    return err;
  }
  lockf(fileno(fp),F_LOCK,0L);

  while (fscanf(fp, "%hhX", boot_list+count) != EOF) {
      count++;
  }
  *list_length = count;
  lockf(fileno(fp),F_ULOCK,0L);
  fclose(fp);
  return 0;
}

int
pal_set_bios_fixed_boot_device(uint8_t slot, uint8_t *fixed_boot_device) {
  FILE *fp;

  fp = fopen(FIXED_BOOT_DEVICE_FILE, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", FIXED_BOOT_DEVICE_FILE);
#endif
    return err;
  }
  lockf(fileno(fp),F_LOCK,0L);

  fprintf(fp, "%02X ", *fixed_boot_device);
  fprintf(fp, "\n");

  lockf(fileno(fp),F_ULOCK,0L);
  fclose(fp);
  return 0;
}

void *pal_clear_bios_default_setting_timer_handler(void *unused){
  uint8_t default_setting;

  bios_default_setting_timer_flag = 0;

  sleep(200);

  pal_get_bios_restores_default_setting(1, &default_setting);
  if(default_setting != 0) {
    default_setting = 0;
    pal_set_bios_restores_default_setting(1, &default_setting);
  }
  return NULL;
}

int
pal_set_bios_restores_default_setting(uint8_t slot, uint8_t *default_setting) {
  FILE *fp;
  pthread_t tid_clear_bios_default_setting_timer;

  fp = fopen(BIOS_DEFAULT_SETTING_FILE, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", BIOS_DEFAULT_SETTING_FILE);
#endif
    return err;
  }
  lockf(fileno(fp),F_LOCK,0L);

  fprintf(fp, "%02X ", *default_setting);
  fprintf(fp, "\n");

  lockf(fileno(fp),F_ULOCK,0L);
  fclose(fp);

  //Hack a thread wait a certain time then clear the setting, when userA didn't reboot the System, and userB doesn't know about the setting.
  if(!bios_default_setting_timer_flag) {
    if (pthread_create(&tid_clear_bios_default_setting_timer, NULL, pal_clear_bios_default_setting_timer_handler, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for clear default setting timer error\n");
      exit(1);
    }
    else
      bios_default_setting_timer_flag = 1;
  }
  return 0;
}

int
pal_get_bios_restores_default_setting(uint8_t slot, uint8_t *default_setting) {
  FILE *fp;

  fp = fopen(BIOS_DEFAULT_SETTING_FILE, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", BIOS_DEFAULT_SETTING_FILE);
#endif
    return err;
  }
  lockf(fileno(fp),F_LOCK,0L);

  fscanf(fp, "%hhX", default_setting);

  lockf(fileno(fp),F_ULOCK,0L);
  fclose(fp);
  return 0;
}

int
pal_set_last_boot_time(uint8_t slot, uint8_t *last_boot_time) {
  FILE *fp;
  int i;

  fp = fopen(LAST_BOOT_TIME, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", LAST_BOOT_TIME);
#endif
    return err;
  }
  lockf(fileno(fp),F_LOCK,0L);

  for(i = 0; i < 4; i++) {
    fprintf(fp, "%02X ", *(last_boot_time+i));
  }
  fprintf(fp, "\n");

  lockf(fileno(fp),F_ULOCK,0L);
  fclose(fp);
  return 0;
}

int
pal_get_last_boot_time(uint8_t slot, uint8_t *last_boot_time) {
  FILE *fp;
  int i;

  fp = fopen(LAST_BOOT_TIME, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", LAST_BOOT_TIME);
#endif
    return err;
  }
  lockf(fileno(fp),F_LOCK,0L);

  for(i = 0; i < 4; i++) {
    fscanf(fp, "%hhX", last_boot_time+i);
  }

  lockf(fileno(fp),F_ULOCK,0L);
  fclose(fp);
  return 0;
}

int
pal_get_bios_fixed_boot_device(uint8_t slot, uint8_t *fixed_boot_device) {
  FILE *fp;

  fp = fopen(FIXED_BOOT_DEVICE_FILE, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", FIXED_BOOT_DEVICE_FILE);
#endif
    return err;
  }
  lockf(fileno(fp),F_LOCK,0L);

  fscanf(fp, "%hhX", fixed_boot_device);

  lockf(fileno(fp),F_ULOCK,0L);
  fclose(fp);
  return 0;
}

unsigned char option_offset[] = {0,1,2,3,4,6,11,20,37,164};
unsigned char option_size[]   = {1,1,1,1,2,5,9,17,127};

void
pal_save_boot_option(unsigned char* buff)
{
  int fp = 0;
  fp = open("/tmp/boot.in", O_WRONLY|O_CREAT);
  if(fp > 0 )
  {
	write(fp,buff,256);
    close(fp);
  }
}

int
pal_load_boot_option(unsigned char* buff)
{
  int fp = 0;
  fp = open("/tmp/boot.in", O_RDONLY);
  if(fp > 0 )
  {
    read(fp,buff,256);
    close(fp);
    return 0;
  }
  else
    return -1;
}

void
pal_set_boot_option(unsigned char para,unsigned char* pbuff)
{
  unsigned char buff[256] = { 0 };
  unsigned char offset = option_offset[para];
  unsigned char size   = option_size[para];
  pal_load_boot_option(buff);
  memcpy(buff + offset, pbuff, size);
  pal_save_boot_option(buff);
}


int
pal_get_boot_option(unsigned char para,unsigned char* pbuff)
{
  unsigned char buff[256] = { 0 };
  int ret = 0;
  unsigned char offset = option_offset[para];
  unsigned char size   = option_size[para];
  ret = pal_load_boot_option(buff);
  if (!ret){
    memcpy(pbuff,(buff + offset), size);
  } else
    memcpy(pbuff,buff,size);
  return size;
}

int pal_nic_otp(int fru, int snr_num, float thresh_val) {
  int retry = 0;
  int ret = 0;
  float curr_val = 0;

  while (retry < NIC_TEMP_RETRY) {
    ret = pal_sensor_read_raw(fru, snr_num, &curr_val);
    if (ret < 0) {
      return -1;
    }
    if (curr_val >= thresh_val) {
      retry++;
    } else {
      return 0;
    }
    msleep(200);
  }

  // power off Mono Lake 12V HSC
  syslog(LOG_CRIT, "Powering Off Server 12V due to NIC temp UNR reached. (val = %.2f)", curr_val);
  server_12v_off(FRU_SLOT1);
  otp_server_12v_off_flag = 1;
  return 0;
}

int
pal_bmc_err_enable(const char *error_item) {

  if (strcasestr(error_item, "CPU") != 0ULL) {
    pal_err_code_enable(ERR_CODE_CPU);
  } else if (strcasestr(error_item, "Memory") != 0ULL) {
    pal_err_code_enable(ERR_CODE_MEM);
  } else if (strcasestr(error_item, "ECC Unrecoverable") != 0ULL) {
    pal_err_code_enable(ERR_CODE_ECC_UNRECOVERABLE);
  } else if (strcasestr(error_item, "ECC Recoverable") != 0ULL) {
    pal_err_code_enable(ERR_CODE_ECC_RECOVERABLE);
  } else {
    syslog(LOG_WARNING, "%s: invalid bmc health item: %s", __func__, error_item);
    return -1;
  }
  return 0;
}

int
pal_bmc_err_disable(const char *error_item) {

  if (strcasestr(error_item, "CPU") != 0ULL) {
    pal_err_code_disable(ERR_CODE_CPU);
  } else if (strcasestr(error_item, "Memory") != 0ULL) {
    pal_err_code_disable(ERR_CODE_MEM);
  } else if (strcasestr(error_item, "ECC Unrecoverable") != 0ULL) {
    pal_err_code_disable(ERR_CODE_ECC_UNRECOVERABLE);
  } else if (strcasestr(error_item, "ECC Recoverable") != 0ULL) {
    pal_err_code_disable(ERR_CODE_ECC_RECOVERABLE);
  } else {
    syslog(LOG_WARNING, "%s: invalid bmc health item: %s", __func__, error_item);
    return -1;
  }
  return 0;
}

uint8_t
pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data) {

  uint8_t completion_code;
  completion_code = CC_SUCCESS;  // Fill response with default values
  unsigned char policy = *pwr_policy & 0x07;  // Power restore policy

  if (slot != FRU_SLOT1) {
    return -1;
  }

  switch (policy)
  {
      case 0:
        if (pal_set_key_value("slot1_por_cfg", "off") != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 1:
        if (pal_set_key_value("slot1_por_cfg", "lps") != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 2:
        if (pal_set_key_value("slot1_por_cfg", "on") != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 3:
        // no change (just get present policy support)
        break;
      default:
        completion_code = CC_PARAM_OUT_OF_RANGE;
        break;
  }
  return completion_code;
}

int
pal_drive_status(const char* i2c_bus) {
  ssd_data ssd;
  t_status_flags status_flag_decoding;
  t_smart_warning smart_warning_decoding;
  t_key_value_pair temp_decoding;
  t_key_value_pair pdlu_decoding;
  t_key_value_pair vendor_decoding;
  t_key_value_pair sn_decoding;

  if (nvme_vendor_read_decode(i2c_bus, &ssd.vendor, &vendor_decoding))
    printf("%s: Fail on reading Vendor ID\n", vendor_decoding.key);
  else
    printf("%s: %s\n", vendor_decoding.key, vendor_decoding.value);

  if (nvme_serial_num_read_decode(i2c_bus, ssd.serial_num, MAX_SERIAL_NUM, &sn_decoding))
    printf("%s: Fail on reading Serial Number\n", sn_decoding.key);
  else
    printf("%s: %s\n", sn_decoding.key, sn_decoding.value);

  if (nvme_temp_read_decode(i2c_bus, &ssd.temp, &temp_decoding))
    printf("%s: Fail on reading Composite Temperature\n", temp_decoding.key);
  else
    printf("%s: %s\n", temp_decoding.key, temp_decoding.value);

  if (nvme_pdlu_read_decode(i2c_bus, &ssd.pdlu, &pdlu_decoding))
    printf("%s: Fail on reading Percentage Drive Life Used\n", pdlu_decoding.key);
  else
    printf("%s: %s\n", pdlu_decoding.key, pdlu_decoding.value);

  if (nvme_sflgs_read_decode(i2c_bus, &ssd.sflgs, &status_flag_decoding))
    printf("%s: Fail on reading Status Flags\n", status_flag_decoding.self.key);
  else {
    printf("%s: %s\n", status_flag_decoding.self.key, status_flag_decoding.self.value);
    printf("    %s: %s\n", status_flag_decoding.read_complete.key, status_flag_decoding.read_complete.value);
    printf("    %s: %s\n", status_flag_decoding.ready.key, status_flag_decoding.ready.value);
    printf("    %s: %s\n", status_flag_decoding.functional.key, status_flag_decoding.functional.value);
    printf("    %s: %s\n", status_flag_decoding.reset_required.key, status_flag_decoding.reset_required.value);
    printf("    %s: %s\n", status_flag_decoding.port0_link.key, status_flag_decoding.port0_link.value);
    printf("    %s: %s\n", status_flag_decoding.port1_link.key, status_flag_decoding.port1_link.value);
  }

  if (nvme_smart_warning_read_decode(i2c_bus, &ssd.warning, &smart_warning_decoding))
    printf("%s: Fail on reading SMART Critical Warning\n", smart_warning_decoding.self.key);
  else {
    printf("%s: %s\n", smart_warning_decoding.self.key, smart_warning_decoding.self.value);
    printf("    %s: %s\n", smart_warning_decoding.spare_space.key, smart_warning_decoding.spare_space.value);
    printf("    %s: %s\n", smart_warning_decoding.temp_warning.key, smart_warning_decoding.temp_warning.value);
    printf("    %s: %s\n", smart_warning_decoding.reliability.key, smart_warning_decoding.reliability.value);
    printf("    %s: %s\n", smart_warning_decoding.media_status.key, smart_warning_decoding.media_status.value);
    printf("    %s: %s\n", smart_warning_decoding.backup_device.key, smart_warning_decoding.backup_device.value);
  }

  printf("\n");
  return 0;
}

int
pal_drive_health(const char* dev) {
  uint8_t sflgs;
  uint8_t warning;

  if (nvme_smart_warning_read(dev, &warning))
    return -1;
  else
    if ((warning & NVME_SMART_WARNING_MASK_BIT) != NVME_SMART_WARNING_MASK_BIT)
      return -1;

  if (nvme_sflgs_read(dev, &sflgs))
    return -1;
  else
    if ((sflgs & NVME_SFLGS_MASK_BIT) != NVME_SFLGS_CHECK_VALUE)
      return -1;

  return 0;
}

int
pal_open_fw_update_flag(void) {
    int fd;
    //if fd = 0 means open successfully.
    fd = open(FW_UPDATE_FLAG, O_CREAT);
    if (!fd) {
      close(fd);
    }
    return fd;
}

int
pal_remove_fw_update_flag(void) {
    int ret;
    //if ret = 0 means remove successfully.
    ret = remove(FW_UPDATE_FLAG);
    return ret;
}

int
pal_get_fw_update_flag(void) {
    int ret;
    //if ret = 0 means BMC is Updating a Device FW, -1 means BMC is not Updating a Device FW
    ret = access(FW_UPDATE_FLAG, F_OK);
    return !ret;
}

//To control iom led to yellow or blue color
uint8_t
pal_iom_led_control(uint8_t color) {

  switch(color) {
    case IOM_LED_OFF:
      if(set_gpio_value(GPIO_BMC_ML_PWR_YELLOW_LED_N, LED_N_OFF))
        break;
      if(set_gpio_value(GPIO_BMC_ML_PWR_BLUE_LED, LED_OFF))
        break;

      return 0;
    case IOM_LED_YELLOW:
      if(set_gpio_value(GPIO_BMC_ML_PWR_YELLOW_LED_N, LED_N_ON))
        break;
      if(set_gpio_value(GPIO_BMC_ML_PWR_BLUE_LED, LED_OFF))
        break;

      return 0;
    case IOM_LED_BLUE:
      if(set_gpio_value(GPIO_BMC_ML_PWR_YELLOW_LED_N, LED_N_OFF))
        break;
      if(set_gpio_value(GPIO_BMC_ML_PWR_BLUE_LED, LED_ON))
        break;

      return 0;
    default:
      break;
  }
  syslog(LOG_WARNING, "%s, Failed to Control IOM LED...", __func__);

  return -1;
}

// ex: set_gpio_value(GPIO_IOM_FULL_PWR_EN, 1);
int
set_gpio_value(int gpio_num, uint8_t value) {
  char vpath[64] = {0};

  sprintf(vpath, GPIO_VAL, gpio_num);

  if (value == 0)
    return write_device(vpath, "0");
  else if (value == 1)
    return write_device(vpath, "1");
  else
    return -1;
}

int
get_gpio_value(int gpio_num, uint8_t *value){
  char vpath[64] = {0};
  FILE *fp = NULL;
  int rc = 0;
  int ret = 0;
  int retry_cnt = 5;
  int i = 0;
  int tmp_value = -1;

  sprintf(vpath, GPIO_VAL, gpio_num);

  for (i = 0; i < retry_cnt; i++) {
    fp = fopen(vpath, "r");
    if (fp == NULL) {
      syslog(LOG_ERR, "%s(): failed to open device %s (%s)",
                       __func__, vpath, strerror(errno));
      if (i == (retry_cnt - 1)) {
        return errno;
      }
    } else {
      break;
    }
    msleep(100);
  }
  for (i = 0; i < retry_cnt; i++) {
    ret = 0;
    rc = fscanf(fp, "%d", &tmp_value);
    if (rc != 1) {
      syslog(LOG_ERR, "failed to read device %s (%s)", vpath, strerror(errno));
      if (i == (retry_cnt - 1)) {
        ret = errno;
      }
    } else {
      break;
    }
    msleep(100);
  }

  fclose(fp);

  *value = (uint8_t) tmp_value;

  return ret;
}

int
pal_get_iom_board_id (void) {
  uint8_t gpio_board_rev_val[3] = {0};
  int iom_board_id = BOARD_MP;
  int ret = 0;

  ret = get_gpio_value(GPIO_BOARD_REV_0, &gpio_board_rev_val[0]);
  if (ret != 0) {
    goto default_iom_board_id;
  }
  ret = get_gpio_value(GPIO_BOARD_REV_1, &gpio_board_rev_val[1]);
  if (ret != 0) {
    goto default_iom_board_id;
  }
  ret = get_gpio_value(GPIO_BOARD_REV_2, &gpio_board_rev_val[2]);
  if (ret != 0) {
    goto default_iom_board_id;
  }

  iom_board_id = (gpio_board_rev_val[0] << 2) | (gpio_board_rev_val[1] << 1) | gpio_board_rev_val[2];

  return iom_board_id;

default_iom_board_id:
  syslog(LOG_WARNING, "%s: cannot get IOM board ID, used default setting: MP", __func__);
  iom_board_id = BOARD_MP;

  return iom_board_id;
}

int
pal_get_pcie_port_config (uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  uint8_t pcie_port_config[SIZE_PCIE_PORT_CONFIG] = {0};
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};
  int msb, lsb;
  int ret;
  int i;
  int j = 0;

  sprintf(key, "server_pcie_port_config");

  ret = pal_get_key_value(key, str);
  if (ret) {
    *res_len = 0;
    return ret;
  }

  for (i = 0; i < 2 * SIZE_PCIE_PORT_CONFIG; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    pcie_port_config[j] = (msb << 4) | lsb;

    j++;
  }

  memcpy(res_data, pcie_port_config, SIZE_PCIE_PORT_CONFIG);
  *res_len = SIZE_PCIE_PORT_CONFIG;

  return 0;
}

int
pal_set_pcie_port_config (uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  uint8_t pcie_port_config[SIZE_PCIE_PORT_CONFIG] = {0};
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};
  int i;

  *res_len = 0;

  sprintf(key, "server_pcie_port_config");

  memcpy(pcie_port_config, req_data, SIZE_PCIE_PORT_CONFIG);
  for (i = 0; i < SIZE_PCIE_PORT_CONFIG; i++) {
    snprintf(tstr, 3, "%02x", pcie_port_config[i]);
    strncat(str, tstr, 3);
  }

  return pal_set_key_value(key, str);
}

int
pal_get_edb_value(char *key, char *value) {
  int i = 0;
  int ret = 0;

  for (i = 0; i < RETRY_COUNT; i++) {
    ret = 0;
    ret = kv_get(key, value, NULL, 0);
    if (ret != 0) {
      syslog(LOG_ERR, "%s, failed to read edb key (%s), ret: %d, retry: %d", __func__, key, ret, i);
    }
    else {
      break;
    }
    msleep(100);
  }

  return ret;
}

int
pal_set_edb_value(char *key, char *value) {
  int i = 0;
  int ret = 0;

  for (i = 0; i < RETRY_COUNT; i++) {
    ret = 0;
    ret = kv_set(key, value, 0, 0);
    if (ret != 0) {
      syslog(LOG_ERR, "%s, failed to write edb key (%s), ret: %d, retry: %d", __func__, key, ret, i);
    }
    else {
      break;
    }
    msleep(100);
  }

  return ret;
}

int
pal_powering_on_flag(uint8_t slot_id) {
  int pid_file;
  char path[128];

  sprintf(path, SERVER_PWR_ON_LOCK, slot_id);
  pid_file = open(path, O_CREAT | O_RDWR, 0666);
  if (pid_file == -1) {
    return -1;
  }
  close(pid_file);

  return 0;
}

void
pal_rm_powering_on_flag(uint8_t slot_id) {
  char path[128];

  sprintf(path, SERVER_PWR_ON_LOCK, slot_id);
  remove(path);
}

//Overwrite the one in obmc-pal.c without systme call of flashcp check
bool
pal_is_fw_update_ongoing(uint8_t fruid) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  int ret;
  struct timespec ts;

  sprintf(key, "fru%d_fwupd", fruid);
  ret = kv_get(key, value, NULL, 0);
  if (ret < 0) {
     return false;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec)
     return true;

  return false;
}

//check power policy and power state to power on/off server after AC power restore
void
pal_power_policy_control(uint8_t fru, char *last_ps) {
  uint8_t chassis_status[5] = {0};
  uint8_t chassis_status_length;
  uint8_t power_policy = POWER_CFG_UKNOWN;
  char pwr_state[MAX_VALUE_LEN] = {0};


  //get power restore policy
  //defined by IPMI Spec/Section 28.2.
  pal_get_chassis_status(fru, NULL, chassis_status, &chassis_status_length);

  //byte[1], bit[6:5]: power restore policy
  power_policy = (*chassis_status >> 5);

  //Check power policy and last power state
  if(power_policy == POWER_CFG_LPS) {
    if (!last_ps) {
      pal_get_last_pwr_state(fru, pwr_state);
      last_ps = pwr_state;
    }
    if (!(strcmp(last_ps, "on"))) {
      sleep(3);
      pal_set_server_power(fru, SERVER_POWER_ON);
    }
  }
  else if(power_policy == POWER_CFG_ON) {
    sleep(3);
    pal_set_server_power(fru, SERVER_POWER_ON);
  }
}

// OEM Command "CMD_OEM_BYPASS_CMD" 0x34
int pal_bypass_cmd(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len){
    int ret;
    int completion_code = CC_UNSPECIFIED_ERROR;
    uint8_t netfn, cmd, select;
    uint8_t tlen;
    uint8_t tbuf[256] = {0x00};
    uint8_t status;

    if (slot != FRU_SLOT1) {
      return CC_PARAM_OUT_OF_RANGE;
    }

    ret = pal_is_fru_prsnt(slot, &status);
    if (ret < 0) {
      return -1;
    }
    if (status == 0) {
      return CC_UNSPECIFIED_ERROR;
    }

    ret = pal_is_server_12v_on(slot, &status);
    if((ret < 0) || (status == CTRL_DISABLE)) {
      return CC_NOT_SUPP_IN_CURR_STATE;
    }

    select = req_data[0];
    netfn = req_data[1];
    cmd = req_data[2];
    tlen = req_len - 6; // payload_id, netfn, cmd, data[0] (select), data[1] (bypass netfn), data[2] (bypass cmd)

    if (tlen < 0) {
      return completion_code;
    }

    if (0 == select) { //BIC
      // Bypass command to Bridge IC
      if (tlen != 0) {
        ret = bic_ipmb_wrapper(slot, netfn, cmd, &req_data[3], tlen, res_data, res_len);
      } else {
        ret = bic_ipmb_wrapper(slot, netfn, cmd, NULL, 0, res_data, res_len);
      }

      if (0 == ret) {
        completion_code = CC_SUCCESS;
      }

    } else if (1 == select) { //ME
      tlen += 2;
      memcpy(tbuf, &req_data[1], tlen);
      tbuf[0] = tbuf[0] << 2;
      // Bypass command to ME
      ret = bic_me_xmit(slot, tbuf, tlen, res_data, res_len);
      if (0 == ret) {
         completion_code = CC_SUCCESS;
      }
    }

    return completion_code;
}
