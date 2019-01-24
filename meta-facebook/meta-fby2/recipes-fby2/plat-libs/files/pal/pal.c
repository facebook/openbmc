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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stddef.h>
#include "pal.h"
#include <facebook/bic.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-sensor.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <openbmc/ncsi.h>


#define BIT(value, index) ((value >> index) & 1)

#define FBY2_PLATFORM_NAME "FBY2"
#define LAST_KEY "last_key"
#define FBY2_MAX_NUM_SLOTS 4
#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define GPIO_DIR "/sys/class/gpio/gpio%d/direction"

#define PAGE_SIZE  0x1000
#define AST_SCU_BASE 0x1e6e2000
#define PIN_CTRL1_OFFSET 0x80
#define PIN_CTRL2_OFFSET 0x84
#define WDT_OFFSET 0x3C

#define UART1_TXD (1 << 22)
#define UART2_TXD (1 << 30)
#define UART3_TXD (1 << 22)
#define UART4_TXD (1 << 30)

#define AST_VIC_BASE 0x1e6c0000
#define HW_HB_STATUS_OFFSET 0x60
#define HB_LED_OUTPUT_OFFSET 0x64
#define SW_BLINK (1 << 4)

#define AST_GPIO_BASE 0x1e780000
#define GPIO_AB_AA_Z_Y 0x1e0

#define DELAY_GRACEFUL_SHUTDOWN 1
#define DELAY_POWER_OFF 6
#define DELAY_POWER_CYCLE 10
#if 0 // 20 seconds is a temporarily workaround for voltage leakage
#define DELAY_12V_CYCLE 5
#else
#define DELAY_12V_CYCLE 20
#endif

#define LARGEST_DEVICE_NAME 120
#define PWM_DIR "/sys/devices/platform/ast_pwm_tacho.0"
#define PWM_UNIT_MAX 96

#define GUID_SIZE 16
#define OFFSET_DEV_GUID 0x1800
#define FRU_EEPROM "/sys/devices/platform/ast-i2c.8/i2c-8/8-0051/eeprom"

#define MAX_READ_RETRY 10
#define MAX_CHECK_RETRY 2
#define MAX_BIC_CHECK_RETRY 15

#define PLATFORM_FILE "/tmp/system.bin"
#define SLOT_RECORD_FILE "/tmp/slot%d.rc"
#define SV_TYPE_RECORD_FILE "/tmp/server_type%d.rc"

#define HOTSERVICE_SCRPIT "/usr/local/bin/hotservice-reinit.sh"
#define HOTSERVICE_FILE "/tmp/slot%d_reinit"
#define HOTSERVICE_PID  "/tmp/hotservice_reinit.pid"
#define HOTSERVICE_BLOCK "/tmp/slot%d_reinit_block"

#define FRUID_SIZE        256
#define EEPROM_DC       "/sys/class/i2c-adapter/i2c-%d/%d-0051/eeprom"
#define BIN_SLOT        "/tmp/fruid_slot%d.bin"

#define GMAC0_DIR "/sys/devices/platform/ftgmac100.0/net/eth0"

#define SPB_REV_FILE "/tmp/spb_rev"
#define SPB_REV_PVT 3

#define IMC_VER_SIZE 8

#define RAS_CRASHDUMP_FILE "/mnt/data/crashdump_slot"

#define IMC_START_FILE "/tmp/start_check_slot%d"
#define IMC_LOG_FILE "/var/log/imc_log_slot"
#define IMC_LOG_FILE_BAK "/var/log/imc_log_slot%d_bak"
#define IMC_LOG_FILE_SIZE (24*1024)   //24KB

#define REINIT_TYPE_FULL            0
#define REINIT_TYPE_HOST_RESOURCE   1

#define POST_TIMEOUT  300
//declare for clearing TPM presence flag
#define TPM_Timeout 600

#define CHUNK_OF_CRS_HEADER_LEN 2

#ifndef max
#define max(a, b) ((a) > (b)) ? (a) : (b)
#endif


static int nic_powerup_prep(uint8_t slot_id, uint8_t reinit_type);


const static uint8_t gpio_rst_btn[] = { 0, GPIO_RST_SLOT1_SYS_RESET_N, GPIO_RST_SLOT2_SYS_RESET_N, GPIO_RST_SLOT3_SYS_RESET_N, GPIO_RST_SLOT4_SYS_RESET_N };
const static uint8_t gpio_led[] = { 0, GPIO_PWR1_LED, GPIO_PWR2_LED, GPIO_PWR3_LED, GPIO_PWR4_LED };      // TODO: In DVT, Map to ML PWR LED
const static uint8_t gpio_id_led[] = { 0,  GPIO_SYSTEM_ID1_LED_N, GPIO_SYSTEM_ID2_LED_N, GPIO_SYSTEM_ID3_LED_N, GPIO_SYSTEM_ID4_LED_N };  // Identify LED
const static uint8_t gpio_slot_id_led[] = { 0,  GPIO_SLOT1_LED, GPIO_SLOT2_LED, GPIO_SLOT3_LED, GPIO_SLOT4_LED }; // Slot ID LED on each TL card
const static uint8_t gpio_prsnt_prim[] = { 0, GPIO_SLOT1_PRSNT_N, GPIO_SLOT2_PRSNT_N, GPIO_SLOT3_PRSNT_N, GPIO_SLOT4_PRSNT_N };
const static uint8_t gpio_prsnt_ext[] = { 0, GPIO_SLOT1_PRSNT_B_N, GPIO_SLOT2_PRSNT_B_N, GPIO_SLOT3_PRSNT_B_N, GPIO_SLOT4_PRSNT_B_N };
const static uint8_t gpio_bic_ready[] = { 0, GPIO_I2C_SLOT1_ALERT_N, GPIO_I2C_SLOT2_ALERT_N, GPIO_I2C_SLOT3_ALERT_N, GPIO_I2C_SLOT4_ALERT_N };
const static uint8_t gpio_power[] = { 0, GPIO_PWR_SLOT1_BTN_N, GPIO_PWR_SLOT2_BTN_N, GPIO_PWR_SLOT3_BTN_N, GPIO_PWR_SLOT4_BTN_N };
const static uint8_t gpio_power_en[] = { 0, GPIO_SLOT1_POWER_EN, GPIO_SLOT2_POWER_EN, GPIO_SLOT3_POWER_EN, GPIO_SLOT4_POWER_EN };
const static uint8_t gpio_12v[] = { 0, GPIO_P12V_STBY_SLOT1_EN, GPIO_P12V_STBY_SLOT2_EN, GPIO_P12V_STBY_SLOT3_EN, GPIO_P12V_STBY_SLOT4_EN };
const static uint8_t gpio_slot_latch[] = { 0, GPIO_SLOT1_EJECTOR_LATCH_DETECT_N, GPIO_SLOT2_EJECTOR_LATCH_DETECT_N, GPIO_SLOT3_EJECTOR_LATCH_DETECT_N, GPIO_SLOT4_EJECTOR_LATCH_DETECT_N};

const char pal_fru_list[] = "all, slot1, slot2, slot3, slot4, spb, nic";
const char pal_server_list[] = "slot1, slot2, slot3, slot4";

#ifdef CONFIG_FBY2_GPV2
const char pal_dev_list[] = "all, device0, device1, device2, device3, device4, device5, device6, device7, device8, device9, device10, device11";
const char pal_dev_pwr_option_list[] = "status, off, on, cycle";
#endif

size_t pal_pwm_cnt = 2;
size_t pal_tach_cnt = 2;
const char pal_pwm_list[] = "0, 1";
const char pal_tach_list[] = "0, 1";

uint8_t g_dev_guid[GUID_SIZE] = {0};

static bool m_sled_ac_start = false;
static unsigned char m_slot_pwr_ctrl[MAX_NODES+1] = {0};

static void *m_gpio_hand_sw = NULL;
static void *m_hbled_output = NULL;

static int ignore_thresh = 0;
static uint8_t fscd_watchdog_counter = 0;

enum {
  POST_END_COUNTER_IGNORE_LOG  = -2,
  POST_END_COUNTER_SHOW_LOG = -1,
};

static long post_end_counter = POST_END_COUNTER_IGNORE_LOG;

typedef struct {
  uint16_t flag;
  float ucr;
  float unc;
  float unr;
  float lcr;
  float lnc;
  float lnr;
  float pos_hyst;
  float neg_hyst;
  int curr_st;
  char name[32];

} _sensor_thresh_t;

typedef struct {
  uint16_t flag;
  float unr;
  float ucr;
  float lcr;
  uint8_t retry_cnt;
  uint8_t val_valid;
  float last_val;

} sensor_check_t;

static sensor_check_t m_snr_chk[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

typedef struct {
  char name[32];

} sensor_desc_t;

static sensor_desc_t m_snr_desc[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

static uint8_t otp_server_12v_off_flag[MAX_NODES+1] = {0};

static int key_func_por_cfg(int event, void *arg);
static int key_func_ntp(int event, void *arg);
static int key_func_pwr_last_state(int event, void *arg);
static int key_func_iden_slot(int event, void *arg);
static int key_func_iden_sled(int event, void *arg);

enum key_event {
  KEY_BEFORE_SET,
  KEY_AFTER_INI,
};

struct pal_key_cfg {
  char *name;
  char *def_val;
  int (*function)(int, void*);
} key_cfg[] = {
  /* name, default value, function */
  {"pwr_server1_last_state", "on", key_func_pwr_last_state},
  {"pwr_server2_last_state", "on", key_func_pwr_last_state},
  {"pwr_server3_last_state", "on", key_func_pwr_last_state},
  {"pwr_server4_last_state", "on", key_func_pwr_last_state},
  {"sysfw_ver_slot1", "0", NULL},
  {"sysfw_ver_slot2", "0", NULL},
  {"sysfw_ver_slot3", "0", NULL},
  {"sysfw_ver_slot4", "0", NULL},
  {"identify_sled", "off", key_func_iden_sled},
  {"identify_slot1", "off", key_func_iden_slot},
  {"identify_slot2", "off", key_func_iden_slot},
  {"identify_slot3", "off", key_func_iden_slot},
  {"identify_slot4", "off", key_func_iden_slot},
  {"timestamp_sled", "0", NULL},
  {"slot1_por_cfg", "lps", key_func_por_cfg},
  {"slot2_por_cfg", "lps", key_func_por_cfg},
  {"slot3_por_cfg", "lps", key_func_por_cfg},
  {"slot4_por_cfg", "lps", key_func_por_cfg},
  {"slot1_sensor_health", "1", NULL},
  {"slot2_sensor_health", "1", NULL},
  {"slot3_sensor_health", "1", NULL},
  {"slot4_sensor_health", "1", NULL},
  {"spb_sensor_health", "1", NULL},
  {"nic_sensor_health", "1", NULL},
  {"slot1_sel_error", "1", NULL},
  {"slot2_sel_error", "1", NULL},
  {"slot3_sel_error", "1", NULL},
  {"slot4_sel_error", "1", NULL},
  {"slot1_boot_order", "0000000", NULL},
  {"slot2_boot_order", "0000000", NULL},
  {"slot3_boot_order", "0000000", NULL},
  {"slot4_boot_order", "0000000", NULL},
  {"slot1_cpu_ppin", "0", NULL},
  {"slot2_cpu_ppin", "0", NULL},
  {"slot3_cpu_ppin", "0", NULL},
  {"slot4_cpu_ppin", "0", NULL},
  {"fru1_restart_cause", "3", NULL},
  {"fru2_restart_cause", "3", NULL},
  {"fru3_restart_cause", "3", NULL},
  {"fru4_restart_cause", "3", NULL},
  {"slot1_trigger_hpr", "on", NULL},
  {"slot2_trigger_hpr", "on", NULL},
  {"slot3_trigger_hpr", "on", NULL},
  {"slot4_trigger_hpr", "on", NULL},
  {"ntp_server", "", key_func_ntp},
  /* Add more Keys here */
  {LAST_KEY, LAST_KEY, NULL} /* This is the last key of the list */
};

struct power_coeff {
  float val;
  float coeff;
};

static const struct power_coeff curr_cali_table[] = {
  { 5.56,  0.90556 },
  { 10.96, 0.91518 },
  { 16.37, 0.91826 },
  { 21.78, 0.91959 },
  { 27.18, 0.92020 },
  { 32.61, 0.92107 },
  { 38.01, 0.92123 },
  { 43.41, 0.92186 },
  { 48.83, 0.92190 },
  { 54.22, 0.92221 },
  { 59.63, 0.92259 },
  { 0.0,   0.0 }
};

static const struct power_coeff pwr_cali_table[] = {
  { 67.95,  0.90602 },
  { 132.98, 0.91589 },
  { 198.05, 0.91815 },
  { 262.54, 0.91984 },
  { 326.86, 0.92008 },
  { 390.79, 0.92119 },
  { 454.36, 0.92127 },
  { 517.45, 0.92182 },
  { 580.07, 0.92221 },
  { 642.23, 0.92253 },
  { 704.14, 0.92299 },
  { 0.0,    0.0 }
};

static const char *sock_path_asd_bic[MAX_NODES+1] = {
  "",
  SOCK_PATH_ASD_BIC "_1",
  SOCK_PATH_ASD_BIC "_2",
  SOCK_PATH_ASD_BIC "_3",
  SOCK_PATH_ASD_BIC "_4"
};

static const char *sock_path_jtag_msg[MAX_NODES+1] = {
  "",
  SOCK_PATH_JTAG_MSG "_1",
  SOCK_PATH_JTAG_MSG "_2",
  SOCK_PATH_JTAG_MSG "_3",
  SOCK_PATH_JTAG_MSG "_4"
};

static const char *ras_dump_path[MAX_NODES+1] = {
  "",
  RAS_CRASHDUMP_FILE "1",
  RAS_CRASHDUMP_FILE "2",
  RAS_CRASHDUMP_FILE "3",
  RAS_CRASHDUMP_FILE "4"
};

static const char *imc_log_path[MAX_NODES+1] = {
  "",
  IMC_LOG_FILE "1",
  IMC_LOG_FILE "2",
  IMC_LOG_FILE "3",
  IMC_LOG_FILE "4"
};

struct ras_pcie_aer_info {
  char *name;
  int offset;
};

struct ras_pcie_aer_info aer_root_err_sts[] = {
  {"ERR_COR_RECEIVED", 0},
  {"Multiple ERR_COR Received", 1},
  {"ERR_FATAL/NONFATAL Received", 2},
  {"Multiple ERR_FATAL/NONFATAL Received", 3},
  {"First Uncorrectable Fatal", 4},
  {"Non-Fatal Error Messages Received", 5},
  {"Fatal Error Messages Received", 6},
  {"Advanced Error Interrupt Message Number", 27}
};

struct ras_pcie_aer_info aer_uncor_err_sts[] = {
  {"Data Link Protocol Error", 4},
  {"Surprise Down Error", 5},
  {"Poisoned TLP", 12},
  {"Flow Control Protocol Error", 13},
  {"Completion Timeout", 14},
  {"Completer Abort", 15},
  {"Unexpected Completion", 16},
  {"Receiver Overflow", 17},
  {"Malformed TLP", 18},
  {"ECRC Error", 19},
  {"Unsupported Request Error", 20},
  {"ACS Violation", 21},
  {"Uncorrectable Internal Error", 22},
  {"MC Blocked TLP", 23},
  {"AtomicOp Egress Blocked", 24},
  {"TLP Prefix Blocked Error", 25}
};

struct ras_pcie_aer_info aer_cor_err_sts[] = {
  {"Receiver Error", 0},
  {"Bad TLP", 6},
  {"Bad DLLP", 7},
  {"REPLAY_NUM Rollover", 8},
  {"Replay Timer Timeout", 12},
  {"Advisory Non-Fatal Error", 13},
  {"Corrected Internal Error", 14},
  {"Header Log Overflow", 15}
};

static const char * const pcie_port_type_strs[] = {
  "PCIe end point",
  "legacy PCI end point device",
  "unknown",
  "unknown",
  "root port",
  "upstream switch port",
  "downstream switch port",
  "PCIe to PCI/PCI-X bridge",
  "PCI/PCI-X to PCIe bridge",
  "root complex integrated endpoint device",
  "root complex event collector",
};

static const char * const memory_error_type_strs[] = {
  "Unknown",
  "No eeror",
  "Single-bit ECC",
  "Multi-bit ECC",
  "Single-symbol ChipKill ECC",
  "Multi-symbol ChipKill ECC",
  "Master abort",
  "Target abort",
  "Parity Error",
  "Watchdog timeout",
  "invalid address",
  "Mirror Broken",
  "Memory Sparing",
  "Scrub corrected error",
  "Scrub uncorrected error",
  "Physical Memory Map-out event",
};

static const char * const error_severity_strs[] = {
  "Recoverable",
  "Fatal",
  "Corrected",
  "None",
};

const static uint8_t Memory_Error_Section[16] = {0x14, 0x11, 0xbc, 0xa5, 0x64, 0x6f, 0xde, 0x4e, 0xb8, 0x63, 0x3e, 0x83, 0xed, 0x7c, 0x83, 0xb1};
const static uint8_t PCIe_Error_Section[16] = {0x54, 0xe9, 0x95, 0xd9, 0xc1, 0xbb, 0x0f, 0x43, 0xad, 0x91, 0xb4, 0x4d, 0xcb, 0x3c, 0x6f, 0x35};
const static uint8_t OEM_Error_Section[16] = {0x15, 0xbc, 0xd1, 0x62, 0x9d, 0xae, 0x47, 0x44, 0xa9, 0x36, 0x5d, 0x6a, 0xc5, 0x7c, 0xd2, 0xfc};

struct fsc_monitor {
  char *sensor_name;
  uint8_t sensor_num;
  uint8_t offset;
};

static struct fsc_monitor fsc_monitor_tl_m2_list[] = {
  {"nvme1_ctemp", BIC_SENSOR_NVME1_CTEMP, 0},
  {"nvme2_ctemp", BIC_SENSOR_NVME2_CTEMP, 1}
};

static int fsc_monitor_tl_m2_list_size = sizeof(fsc_monitor_tl_m2_list) / sizeof(struct fsc_monitor);

static struct fsc_monitor fsc_monitor_gp_m2_list[] = {
  {"dc_nvme1_ctemp", DC_SENSOR_NVMe1_CTEMP, 2},
  {"dc_nvme2_ctemp", DC_SENSOR_NVMe2_CTEMP, 3},
  {"dc_nvme3_ctemp", DC_SENSOR_NVMe3_CTEMP, 4},
  {"dc_nvme4_ctemp", DC_SENSOR_NVMe4_CTEMP, 5},
  {"dc_nvme5_ctemp", DC_SENSOR_NVMe5_CTEMP, 6},
  {"dc_nvme6_ctemp", DC_SENSOR_NVMe6_CTEMP, 7}
};

static int fsc_monitor_gp_m2_list_size = sizeof(fsc_monitor_gp_m2_list) / sizeof(struct fsc_monitor);

/* curr/power calibration */
static void
power_value_adjust(const struct power_coeff *table, float *value) {
  float x0, x1, y0, y1, x;
  int i;

  x = *value;
  x0 = table[0].val;
  y0 = table[0].coeff;
  if (x0 >= *value) {
    *value = x * y0;
    return;
  }

  for (i = 1; table[i].val > 0.0; i++) {
    if (*value < table[i].val)
      break;

    x0 = table[i].val;
    y0 = table[i].coeff;
  }
  if (table[i].val <= 0.0) {
    *value = x * y0;
    return;
  }

  // if value is bwtween x0 and x1, use linear interpolation method.
  x1 = table[i].val;
  y1 = table[i].coeff;
  *value = (y0 + (((y1 - y0)/(x1 - x0)) * (x - x0))) * x;
  return;
}

typedef struct _inlet_corr_t {
  uint8_t duty;
  int8_t delta_t;
} inlet_corr_t;

static inlet_corr_t g_ict[] = {
  // Inlet Sensor:
  // duty cycle vs delta_t
  { 10, 2 },
  { 16, 1 },
  { 38, 0 },
};

static uint8_t g_ict_count = sizeof(g_ict)/sizeof(inlet_corr_t);

static void apply_inlet_correction(float *value) {
  static int8_t dt = 0;
  int i;
  uint8_t pwm[2] = {0};

  // Get PWM value
  if (pal_get_pwm_value(0, &pwm[0]) || pal_get_pwm_value(1, &pwm[1])) {
    // If error reading PWM value, use the previous deltaT
    *value -= dt;
    return;
  }
  pwm[0] = (pwm[0] + pwm[1]) /2;

  // Scan through the correction table to get correction value for given PWM
  dt=g_ict[0].delta_t;
  for (i=0; i< g_ict_count; i++) {
    if (pwm[0] >= g_ict[i].duty)
      dt = g_ict[i].delta_t;
    else
      break;
  }

  // Apply correction for the sensor
  *(float*)value -= dt;
}

// Helper Functions
static int
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
  while (strcmp(key_cfg[i].name, LAST_KEY)) {

    // If Key is valid, return success
    if (!strcmp(key, key_cfg[i].name))
      return i;

    i++;
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_key_check: invalid key - %s", key);
#endif
  return -1;
}

static int
key_func_pwr_last_state(int event, void *arg) {
  if (event == KEY_BEFORE_SET) {
    if (strcmp((char *)arg, "on") && strcmp((char *)arg, "off"))
      return -1;
  }

  return 0;
}

static int
key_func_iden_sled(int event, void *arg) {
  if (event == KEY_BEFORE_SET) {
    if (strcmp((char *)arg, "on") && strcmp((char *)arg, "off"))
      return -1;
  }

  return 0;
}

static int
key_func_iden_slot(int event, void *arg) {
  if (event == KEY_BEFORE_SET) {
    if (strcmp((char *)arg, "on") && strcmp((char *)arg, "off"))
      return -1;
  }

  return 0;
}

static int
key_func_por_cfg(int event, void *arg) {
  if (event == KEY_BEFORE_SET) {
    if (strcmp((char *)arg, "lps") && strcmp((char *)arg, "on") && strcmp((char *)arg, "off"))
      return -1;
  }

  return 0;
}

static int
key_func_ntp(int event, void *arg) {
  char cmd[MAX_VALUE_LEN] = {0};
  char ntp_server_new[MAX_VALUE_LEN] = {0};
  char ntp_server_old[MAX_VALUE_LEN] = {0};

  if (event == KEY_BEFORE_SET) {
    // Remove old NTP server
    kv_get("ntp_server", ntp_server_old, NULL, KV_FPERSIST);
    if (strlen(ntp_server_old) > 2) {
      snprintf(cmd, MAX_VALUE_LEN, "sed -i '/^restrict %s$/d' /etc/ntp.conf", ntp_server_old);
      system(cmd);
      snprintf(cmd, MAX_VALUE_LEN, "sed -i '/^server %s$/d' /etc/ntp.conf", ntp_server_old);
      system(cmd);
    }
    // Add new NTP server
    snprintf(ntp_server_new, MAX_VALUE_LEN, "%s", (char *)arg);
    if (strlen(ntp_server_new) > 2) {
      snprintf(cmd, MAX_VALUE_LEN, "echo \"restrict %s\" >> /etc/ntp.conf", ntp_server_new);
      system(cmd);
      snprintf(cmd, MAX_VALUE_LEN, "echo \"server %s\" >> /etc/ntp.conf", ntp_server_new);
      system(cmd);
    }
    // Restart NTP server
    snprintf(cmd, MAX_VALUE_LEN, "/etc/init.d/ntpd restart > /dev/null &");
    system(cmd);
  }

  return 0;
}

int
pal_get_key_value(char *key, char *value) {

  // Check is key is defined and valid
  if (pal_key_check(key) < 0)
    return -1;

  return kv_get(key, value, NULL, KV_FPERSIST);
}

int
pal_set_key_value(char *key, char *value) {
  int index, ret;

  // Check is key is defined and valid
  if ((index = pal_key_check(key)) < 0)
    return -1;

  if (key_cfg[index].function) {
    ret = key_cfg[index].function(KEY_BEFORE_SET, value);
    if (ret < 0)
      return ret;
  }

  return kv_set(key, value, 0, KV_FPERSIST);
}

// Common IPMB Wrapper function
static int
pal_get_ipmb_bus_id(uint8_t slot_id) {
  int bus_id;

  switch(slot_id) {
  case FRU_SLOT1:
    bus_id = IPMB_BUS_SLOT1;
    break;
  case FRU_SLOT2:
    bus_id = IPMB_BUS_SLOT2;
    break;
  case FRU_SLOT3:
    bus_id = IPMB_BUS_SLOT3;
    break;
  case FRU_SLOT4:
    bus_id = IPMB_BUS_SLOT4;
    break;
  default:
    bus_id = -1;
    break;
  }

  return bus_id;
}

/*
 * pal_copy_eeprom_to_bin - copy the eeprom to binary file im /tmp directory
 *
 * @eeprom_file   : path for the eeprom of the device
 * @bin_file      : path for the binary file
 *
 * returns 0 on successful copy
 * returns non-zero on file operation errors
 */
int pal_copy_eeprom_to_bin(const char * eeprom_file, const char * bin_file) {

  int eeprom;
  int bin;
  uint64_t tmp[FRUID_SIZE];
  ssize_t bytes_rd, bytes_wr;

  errno = 0;

  if (access(eeprom_file, F_OK) != -1) {

    eeprom = open(eeprom_file, O_RDONLY);
    if (eeprom == -1) {
      syslog(LOG_ERR, "pal_copy_eeprom_to_bin: unable to open the %s file: %s",
          eeprom_file, strerror(errno));
      return errno;
    }

    bin = open(bin_file, O_WRONLY | O_CREAT, 0644);
    if (bin == -1) {
      syslog(LOG_ERR, "pal_copy_eeprom_to_bin: unable to create %s file: %s",
          bin_file, strerror(errno));
      return errno;
    }

    bytes_rd = read(eeprom, tmp, FRUID_SIZE);
    if (bytes_rd != FRUID_SIZE) {
      syslog(LOG_ERR, "pal_copy_eeprom_to_bin: write to %s file failed: %s",
          eeprom_file, strerror(errno));
      return errno;
    }

    bytes_wr = write(bin, tmp, bytes_rd);
    if (bytes_wr != bytes_rd) {
      syslog(LOG_ERR, "pal_copy_eeprom_to_bin: write to %s file failed: %s",
          bin_file, strerror(errno));
      return errno;
    }

    close(bin);
    close(eeprom);
  }

  return 0;
}

// Update the Reset button input to the server at given slot
int
pal_set_rst_btn(uint8_t slot, uint8_t status) {
  char path[64] = {0};
  char *val;

  if (slot < 1 || slot > 4) {
    return -1;
  }

  if (status) {
    val = "1";
  } else {

    val = "0";

    // send notification to NIC about impending reset
    if (nic_powerup_prep(slot, REINIT_TYPE_HOST_RESOURCE) != 0) {
      syslog(LOG_ERR, "%s: NIC notification failed, abort reset\n",
             __FUNCTION__);
      return -1;
    }
  }

  sprintf(path, GPIO_VAL, gpio_rst_btn[slot]);
  if (write_device(path, val)) {
    return -1;
  }

  return 0;
}

int pal_fruid_init(uint8_t slot_id) {

  int ret=0;
  char path[128] = {0};
  char fpath[64] = {0};

  switch(slot_id) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      switch(fby2_get_slot_type(slot_id))
      {
         case SLOT_TYPE_SERVER:
         case SLOT_TYPE_GPV2:
           // Do not access EEPROM
           break;
         case SLOT_TYPE_CF:
         case SLOT_TYPE_GP:
           sprintf(path, EEPROM_DC, pal_get_ipmb_bus_id(slot_id), pal_get_ipmb_bus_id(slot_id));
           sprintf(fpath, BIN_SLOT, slot_id);
           ret = pal_copy_eeprom_to_bin(path, fpath);
           break;
         case SLOT_TYPE_NULL:
           // Do not access EEPROM
           break;
      }
      break;
    default:
      break;
  }

  return ret;
}

int
pal_get_pair_slot_type(uint8_t fru) {
  int type,type2;

  // PAL_TYPE = 0(Server), 1(Crace Flat), 2(Glacier Point), 3(Empty Slot)
  type = fby2_get_slot_type(fru);
  if (type < 0)
    return type;

  switch(fru)
  {
    case FRU_SLOT1:
    case FRU_SLOT3:
      type2 = fby2_get_slot_type(fru+1);
      if (type2 < 0)
        return type2;
      else
        return ((type2<<4) + type);
    case FRU_SLOT2:
    case FRU_SLOT4:
      type2 = fby2_get_slot_type(fru-1);
      if (type2 < 0)
        return type2;
      else
        return ((type<<4) + type2);
  }

  return -1;
}

static int
power_on_server_physically(uint8_t slot_id){
  char vpath[64] = {0};
  uint8_t ret = -1;
  uint8_t retry = MAX_READ_RETRY;
  bic_gpio_t gpio;

  syslog(LOG_WARNING, "%s is on going for slot%d\n",__func__,slot_id);

  sprintf(vpath, GPIO_VAL, gpio_power[slot_id]);
  if (write_device(vpath, "1")) {
    return -1;
  }

  if (write_device(vpath, "0")) {
    return -1;
  }

  sleep(1);

  if (write_device(vpath, "1")) {
    return -1;
  }

  // Wait for server power good ready
  sleep(2);

  while (retry) {
    ret = bic_get_gpio(slot_id, &gpio);
    if (!ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "%s: Get response successfully for slot%d\n",__func__,slot_id);
#endif
      break;
    }
    msleep(50);
    retry--;
  }

  if (ret) {
#ifdef DEBUG
     syslog(LOG_WARNING, "%s: Bridge IC is no response for slot%d\n",__func__,slot_id);
#endif
     return -1;
  }

  // Check power status
  if (!gpio.pwrgood_cpu) {
    syslog(LOG_WARNING, "%s: Power on is failed for slot%d\n",__func__,slot_id);
    return -1;
  }

  return 0;
}


#define MAX_POWER_PREP_RETRY_CNT    3
static int
write_gmac0_value(const char *device_name, const int value) {
  char full_name[LARGEST_DEVICE_NAME];
  char output_value[LARGEST_DEVICE_NAME];
  int err;
  int retry_cnt=0;

  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", GMAC0_DIR, device_name);
  snprintf(output_value, LARGEST_DEVICE_NAME, "%d", value);

  do {
    err = write_device(full_name, output_value);
    if (err == ENOENT) {
      syslog(LOG_INFO, "Retry powerup_prep for host%d [%d], err=%d", value, (retry_cnt+1), err);
      break;
    } else if (err != 0) {
      syslog(LOG_INFO, "Retry powerup_prep for host%d [%d], err=%d", value, (retry_cnt+1), err);
      sleep(1);
    }
  } while ((err != 0) && (retry_cnt++ < MAX_POWER_PREP_RETRY_CNT));

  return err;
}

// Write to /sys/devices/platform/ftgmac100.0/net/eth0/powerup_prep_host_id
// This is a combo ID consists of the following fields:
// bit 11~8: reinit_type
// bit 7~0:  host_id
static int
nic_powerup_prep(uint8_t slot_id, uint8_t reinit_type) {
  int err;
  uint32_t combo_id = ((uint32_t)reinit_type<<8) | (uint32_t)slot_id;

  err = write_gmac0_value("powerup_prep_host_id", combo_id);

  return err;
}


// Power On the server in a given slot
static int
server_power_on(uint8_t slot_id) {
  int loop = 0;
  int max_retry = 5;
  int val = 0;

  if (slot_id < 1 || slot_id > 4) {
    return -1;
  }

  // Power on server
  for (loop = 0; loop < max_retry; loop++){
    val = power_on_server_physically(slot_id);
    if (val == 0) {
      break;
    }
    syslog(LOG_WARNING, "%s(): Power on server failed for %d time(s).\n", __func__, loop);
    sleep(1);

    // Max retry case
    if (loop == (max_retry-1))
      return -1;
  }

  return 0;
}

// Power Off the server in given slot
static int
server_power_off(uint8_t slot_id, bool gs_flag) {
  char vpath[64] = {0};

  if (slot_id < 1 || slot_id > 4) {
    return -1;
  }

  if (!gs_flag) {
    // only needed in ungraceful-shutdown
    if (nic_powerup_prep(slot_id, REINIT_TYPE_FULL) != 0) {
      return -1;
    }
  }

  sprintf(vpath, GPIO_VAL, gpio_power[slot_id]);

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

  return 0;
}

static uint8_t
_get_spb_rev(void) {
  int rev;

  if (read_device(SPB_REV_FILE, &rev)) {
    printf("Get spb revision failed\n");
    return -1;
  }

  return rev;
}

static bool
pal_is_device_pair(uint8_t slot_id) {
  int pair_set_type;

  pair_set_type = pal_get_pair_slot_type(slot_id);
  switch (pair_set_type) {
    case TYPE_CF_A_SV:
    case TYPE_GP_A_SV:
    case TYPE_GPV2_A_SV:
      return true;
  }

  return false;
}

int
pal_baseboard_clock_control(uint8_t slot_id, char *ctrl) {
  char v1path[64] = {0};
  char v2path[64] = {0};
  char v3path[64] = {0};
  uint8_t rev;

  rev = _get_spb_rev();
  switch(slot_id) {
    case FRU_SLOT1:
    case FRU_SLOT2:
      if (rev < SPB_REV_PVT) {
        sprintf(v1path, GPIO_VAL, GPIO_PE_BUFF_OE_0_R_N);
        sprintf(v2path, GPIO_VAL, GPIO_PE_BUFF_OE_1_R_N);
      }
      sprintf(v3path, GPIO_VAL, GPIO_CLK_BUFF1_PWR_EN_N);
      break;
    case FRU_SLOT3:
    case FRU_SLOT4:
      if (rev < SPB_REV_PVT) {
        sprintf(v1path, GPIO_VAL, GPIO_PE_BUFF_OE_2_R_N);
        sprintf(v2path, GPIO_VAL, GPIO_PE_BUFF_OE_3_R_N);
      }
      sprintf(v3path, GPIO_VAL, GPIO_CLK_BUFF2_PWR_EN_N);
      break;
    default:
      return -1;
  }

  if (pal_is_device_pair(slot_id)) {
    if (write_device(v3path, ctrl))
      return -1;
  }

  if (rev < SPB_REV_PVT) {
    if (write_device(v1path, ctrl) || write_device(v2path, ctrl))
      return -1;
  }

  return 0;
}

int
pal_is_server_12v_on(uint8_t slot_id, uint8_t *status) {

  int val;
  char path[64] = {0};

  if (slot_id < 1 || slot_id > 4) {
    return -1;
  }

  sprintf(path, GPIO_VAL, gpio_12v[slot_id]);

  if (read_device(path, &val)) {
    return -1;
  }

  if (val == 0x1) {
    *status = 1;
  } else {
    *status = 0;
  }

  return 0;
}

int
pal_slot_pair_12V_off(uint8_t slot_id) {
  int pair_slot_id;
  int pair_set_type=-1;
  uint8_t status=0;
  int ret=-1;
  char vpath[80]={0};

  if (0 == slot_id%2)
    pair_slot_id = slot_id - 1;
  else
    pair_slot_id = slot_id + 1;

  // If pair slot is not present, donothing
  ret = pal_is_fru_prsnt(pair_slot_id, &status);
  if (ret < 0) {
     printf("%s pal_is_fru_prsnt failed for fru: %d\n", __func__, slot_id);
     return -1;
  }

  if (!status)
     return 0;

  pair_set_type = pal_get_pair_slot_type(slot_id);

  /* Check whether the system is 12V off or on */
  ret = pal_is_server_12v_on(pair_slot_id, &status);
  if (ret < 0) {
    syslog(LOG_ERR, "pal_slot_pair_12V_off: pal_is_server_12v_on failed");
    return -1;
  }

  switch(pair_set_type) {
    case TYPE_SV_A_SV:
    case TYPE_SV_A_CF:
    case TYPE_SV_A_GP:
      // donothing
      break;
    case TYPE_CF_A_CF:
    case TYPE_CF_A_GP:
    case TYPE_CF_A_GPV2:
    case TYPE_GP_A_CF:
    case TYPE_GP_A_GP:
    case TYPE_GP_A_GPV2:
    case TYPE_GPV2_A_CF:
    case TYPE_GPV2_A_GP:
    case TYPE_GPV2_A_GPV2:
      // Need to 12V-off pair slot
      // Pair Slot should be 12V-off when pair slots are device
      if (status) {
        sprintf(vpath, GPIO_VAL, gpio_12v[pair_slot_id]);
        if (write_device(vpath, "0")) {
          return -1;
        }
      }
      break;
    case TYPE_CF_A_SV:
    case TYPE_GP_A_SV:
    case TYPE_GPV2_A_SV:
      // Need to 12V-off pair slot
      if (status) {
        sprintf(vpath, GPIO_VAL, gpio_12v[pair_slot_id]);
        if (write_device(vpath, "0")) {
          return -1;
        }
      }
      break;
  }

  return 0;
}

static bool
pal_is_valid_pair(uint8_t slot_id, int *pair_set_type) {
  *pair_set_type = pal_get_pair_slot_type(slot_id);
  switch (*pair_set_type) {
    case TYPE_SV_A_SV:
    case TYPE_CF_A_SV:
    case TYPE_GP_A_SV:
    case TYPE_GPV2_A_SV:
    case TYPE_NULL_A_SV:
    case TYPE_SV_A_NULL:
      return true;
  }

  return false;
}

static int
_set_slot_12v_en_time(uint8_t slot_id) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += 6;

  sprintf(key, "slot%u_12v_en", slot_id);
  sprintf(value, "%ld", ts.tv_sec);
  if (kv_set(key, value, 0, 0) < 0) {
    return -1;
  }

  return 0;
}

static bool
_check_slot_12v_en_time(uint8_t slot_id) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  sprintf(key, "slot%u_12v_en", slot_id);
  if (kv_get(key, value, NULL, 0)) {
     return true;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (ts.tv_sec >= strtoul(value, NULL, 10))
     return true;

  return false;
}

static int
pal_slot_pair_12V_on(uint8_t slot_id) {
  uint8_t pair_slot_id, dc_slot_id;
  int pair_set_type=-1;
  uint8_t status=0;
  char vpath[80]={0};
  int ret=-1;
  uint8_t pwr_slot = slot_id;
  int retry;
  uint8_t self_test_result[2]={0};
#if defined(CONFIG_FBY2_EP)
  uint8_t server_type;
#endif

  pair_set_type = pal_get_pair_slot_type(slot_id);
  switch(pair_set_type) {
     case TYPE_SV_A_SV:
      //do nothing
       break;
     case TYPE_SV_A_CF:
     case TYPE_SV_A_GP:
     case TYPE_SV_A_GPV2:
       if(slot_id == 2 || slot_id == 4)
       {
         /* Check whether the system is 12V off or on */
         ret = pal_is_server_12v_on(slot_id, &status);
         if (ret < 0) {
           syslog(LOG_ERR, "pal_slot_pair_12V_on: pal_is_server_12v_on failed");
           return -1;
         }

         // Need to 12V-off self slot
         // Self slot should be 12V-off due to device card is on slot2 or slot4
         if (status) {
           sprintf(vpath, GPIO_VAL, gpio_12v[slot_id]);
           if (write_device(vpath, "0")) {
             return -1;
           }
         }
       }
       break;
     case TYPE_GP_A_NULL:
     case TYPE_CF_A_NULL:
     case TYPE_GPV2_A_NULL:
     case TYPE_NULL_A_GP:
     case TYPE_NULL_A_CF:
     case TYPE_NULL_A_GPV2:
       /* Check whether the system is 12V off or on */
       ret = pal_is_server_12v_on(slot_id, &status);
       if (ret < 0) {
         syslog(LOG_ERR, "pal_slot_pair_12V_on: pal_is_server_12v_on failed");
         return -1;
       }

       // Need to 12V-off self slot
       // Self slot should be 12V-off when pair slot is empty
       if (status) {
         sprintf(vpath, GPIO_VAL, gpio_12v[slot_id]);
         if (write_device(vpath, "0")) {
           return -1;
         }
       }
       break;
     case TYPE_CF_A_CF:
     case TYPE_CF_A_GP:
     case TYPE_CF_A_GPV2:
     case TYPE_GP_A_CF:
     case TYPE_GP_A_GP:
     case TYPE_GP_A_GPV2:
     case TYPE_GPV2_A_CF:
     case TYPE_GPV2_A_GP:
     case TYPE_GPV2_A_GPV2:
       /* Check whether the system is 12V off or on */
       ret = pal_is_server_12v_on(slot_id, &status);
       if (ret < 0) {
         syslog(LOG_ERR, "pal_slot_pair_12V_on: pal_is_server_12v_on failed");
         return -1;
       }

       // Need to 12V-off self slot
       // Self slot should be 12V-off when couple of slots are all device card
       if (status) {
         sprintf(vpath, GPIO_VAL, gpio_12v[slot_id]);
         if (write_device(vpath, "0")) {
           return -1;
         }
       }
       break;
     case TYPE_CF_A_SV:
     case TYPE_GP_A_SV:
     case TYPE_GPV2_A_SV:
       if (0 == slot_id%2) {
         dc_slot_id = slot_id - 1;
         pair_slot_id = dc_slot_id;
       } else {
         dc_slot_id = slot_id;
         pwr_slot = slot_id + 1;
         pair_slot_id = pwr_slot;
       }

       /* Check whether the system is 12V off or on */
       ret = pal_is_server_12v_on(dc_slot_id, &status);
       if (ret < 0) {
         syslog(LOG_ERR, "pal_slot_pair_12V_on: pal_is_server_12v_on failed");
         return -1;
       }

       // Need to 12V-on pair slot
       if (!status) {
         _set_slot_12v_en_time(dc_slot_id);
         sprintf(vpath, GPIO_VAL, gpio_12v[dc_slot_id]);
         if (write_device(vpath, "1")) {
           return -1;
         }
         pal_set_hsvc_ongoing(dc_slot_id, 0, 1);

         msleep(300);
       }
       pal_baseboard_clock_control(dc_slot_id, "0");

       // Check whether the system is 12V off or on
       if (pal_is_server_12v_on(pwr_slot, &status) < 0) {
         syslog(LOG_ERR, "%s: pal_is_server_12v_on failed", __func__);
         return -1;
       }
       if (!status) {
         _set_slot_12v_en_time(pwr_slot);
         sprintf(vpath, GPIO_VAL, gpio_12v[pwr_slot]);
         if (write_device(vpath, "1")) {
           return -1;
         }
         pal_set_hsvc_ongoing(pwr_slot, 0, 1);
       }

       sprintf(vpath, HOTSERVICE_BLOCK, pair_slot_id);
       if (access(vpath, F_OK) == 0) {
         unlink(vpath);
         sprintf(vpath, "%s slot%u start", HOTSERVICE_SCRPIT, pair_slot_id);
         system(vpath);
       }

       // Check BIC Self Test Result
       retry = 4;
       while (retry > 0) {
         ret = bic_get_self_test_result(pwr_slot, self_test_result);
         if (ret == 0) {
           syslog(LOG_INFO, "bic_get_self_test_result: %X %X\n", self_test_result[0], self_test_result[1]);
           break;
         }
         retry--;
         sleep(5);
       }

#if defined(CONFIG_FBY2_EP)
       retry = 2;
       while (retry > 0) {
         ret = fby2_get_server_type(pwr_slot, &server_type);
         if (!ret) {
           if (server_type == SERVER_TYPE_EP) {
             ret = bic_set_pcie_config(pwr_slot, (pair_set_type == TYPE_CF_A_SV) ? 0x2 : 0x1);
             if (ret == 0)
               break;
           } else {
             break;
           }
         }
         retry--;
         msleep(100);
       }
#endif

       sleep(2);  // wait BIC ready to reply Get GPIO Status command
       if (pal_get_server_power(pwr_slot, &status) < 0) {
         syslog(LOG_ERR, "%s: pal_get_server_power failed", __func__);
         return -1;
       }

       retry = 4;
       if (status) {
         while (retry > 0) {
           ret = pal_set_server_power(pwr_slot, SERVER_POWER_CYCLE);
           if (0 == ret)
             break;
           retry--;
           sleep(1);
         }

         if (0 == retry) {
           syslog(LOG_ERR, "%s: Failed to do power cycle", __func__);
         }
       }
       break;
  }

  return 0;
}

bool
pal_is_hsvc_ongoing(uint8_t slot_id) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};

  sprintf(key, "fru%u_hsvc", slot_id);
  if (kv_get(key, value, NULL, 0)) {
     return false;
  }

  if (atoi(value))
     return true;

  return false;
}

int
pal_set_hsvc_ongoing(uint8_t slot_id, uint8_t status, uint8_t ident) {
  char key[MAX_KEY_LEN];

  sprintf(key, "fru%u_hsvc", slot_id);
  if (kv_set(key, (status) ? "1" : "0", 0, 0)) {
     return -1;
  }

  if (ident) {
    sprintf(key, "identify_slot%u", slot_id);
    if (pal_set_key_value(key, (status) ? "on" : "off")) {
      syslog(LOG_ERR, "pal_set_key_value: set %s failed", key);
      return -1;
    }
  }

  return 0;
}

static int
pal_hot_service_action(uint8_t slot_id) {
  uint8_t pair_slot_id;
  uint8_t block_12v;
  char cmd[128];
  char hspath[80], vpath[80];
  int ret;

  if (0 == slot_id%2)
    pair_slot_id = slot_id - 1;
  else
    pair_slot_id = slot_id + 1;

  block_12v = (pal_is_device_pair(slot_id) && pal_is_hsvc_ongoing(pair_slot_id));

  // Re-init system configuration
  sprintf(hspath, HOTSERVICE_FILE, slot_id);
  if (access(hspath, F_OK) == 0) {
    _set_slot_12v_en_time(slot_id);
    sprintf(vpath, GPIO_VAL, gpio_12v[slot_id]);
    if (write_device(vpath, "1")) {
      syslog(LOG_ERR, "%s: write_device failed", __func__);
    }

    sleep(2);

    sprintf(cmd, "/usr/local/bin/check_slot_type.sh slot%u", slot_id);
    system(cmd);

    unlink(hspath);
    block_12v = (pal_is_device_pair(slot_id) && pal_is_hsvc_ongoing(pair_slot_id));
    if (!block_12v) {
      sprintf(cmd, "%s slot%u start", HOTSERVICE_SCRPIT, slot_id);
    } else {
      sprintf(cmd, "touch "HOTSERVICE_BLOCK, slot_id);
    }
    system(cmd);

    if (0 != pal_fruid_init(slot_id)) {
      syslog(LOG_ERR, "%s: pal_fruid_init failed", __func__);
    }

    pal_system_config_check(slot_id);
    pal_set_hsvc_ongoing(slot_id, 0, 1);
  }

  if (block_12v) {
    syslog(LOG_CRIT, "12V-on blocked due to pair slot is in hot-service, FRU: %d", slot_id);
    sprintf(vpath, GPIO_VAL, gpio_12v[slot_id]);
    if (write_device(vpath, "0")) {
      syslog(LOG_ERR, "%s: write_device failed", __func__);
    }
    return -1;
  }

  // Check if pair slot is swap
  sprintf(hspath, HOTSERVICE_FILE, pair_slot_id);
  if (access(hspath, F_OK) != 0) {
    ret = pal_slot_pair_12V_on(slot_id);
    if (0 != ret)
      printf("%s pal_slot_pair_12V_on failed for fru: %d\n", __func__, slot_id);
  } else {
    if (pal_is_device_pair(slot_id)) {
      printf("fail to 12V-on fru%d, due to fru%d is doing hot service! \n", slot_id, pair_slot_id);
    }
  }

  return 0;
}

// Turn off 12V for the server in given slot
static int
server_12v_off(uint8_t slot_id) {
  char vpath[64] = {0};
  int ret=0;
  int pair_set_type;
  uint8_t pair_slot_id;
  uint8_t runoff_id = slot_id;

  if (slot_id < 1 || slot_id > 4) {
    return -1;
  }

  if (nic_powerup_prep(slot_id, REINIT_TYPE_FULL) != 0) {
    return -1;
  }

  if (0 == slot_id%2)
    pair_slot_id = slot_id - 1;
  else
    pair_slot_id = slot_id + 1;

  pair_set_type = pal_get_pair_slot_type(slot_id);

  if (0 != slot_id%2) {
     switch(pair_set_type) {
        case TYPE_SV_A_SV:
        case TYPE_SV_A_CF:
        case TYPE_SV_A_GP:
        case TYPE_CF_A_CF:
        case TYPE_CF_A_GP:
        case TYPE_GP_A_CF:
        case TYPE_GP_A_GP:
           // do nothing
           break;
        case TYPE_CF_A_SV:
        case TYPE_GP_A_SV:
        case TYPE_GPV2_A_SV:
           // Need to 12V-off pair slot first to avoid accessing device card error
           runoff_id = pair_slot_id;
           break;
     }
  }

  sprintf(vpath, GPIO_VAL, gpio_12v[runoff_id]);
  if (write_device(vpath, "0")) {
    return -1;
  }

  pal_baseboard_clock_control(runoff_id, "1");

  ret=pal_slot_pair_12V_off(runoff_id);
  if (0 != ret)
    printf("%s pal_slot_pair_12V_off failed for fru: %d\n", __func__, slot_id);

  return ret;
}

int
pal_system_config_check(uint8_t slot_id) {
  char vpath[80] = {0};
  int slot_type = -1;
  int last_slot_type = -1;
  char slot_str[80] = {0};
  char last_slot_str[80] = {0};

  // 0(Server), 1(Crane Flat), 2(Glacier Point), 3(Empty Slot)
  slot_type = fby2_get_slot_type(slot_id);
  switch (slot_type) {
     case SLOT_TYPE_SERVER: {
#if defined(CONFIG_FBY2_RC) || defined(CONFIG_FBY2_EP)
       uint8_t server_type = 0xFF;
       int ret=-1;
       ret = fby2_get_server_type(slot_id, &server_type);
       if (ret) {
         syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
         return ret;
       }
       switch (server_type) {
         case SERVER_TYPE_RC:
           sprintf(slot_str,"RC");
           break;
         case SERVER_TYPE_EP:
           sprintf(slot_str,"EP");
           break;
         case SERVER_TYPE_TL:
           sprintf(slot_str,"Twin Lakes");
           break;
         default:
           sprintf(slot_str,"Undefined server type");
           break;
       }
#else
       sprintf(slot_str,"1S Server");
#endif
       }
       break;
     case SLOT_TYPE_CF:
       sprintf(slot_str,"Crane Flat");
       break;
     case SLOT_TYPE_GP:
       sprintf(slot_str,"Glacier Point");
       break;
     case SLOT_TYPE_GPV2:
       sprintf(slot_str,"Glacier Point V2");
       break;
     case SLOT_TYPE_NULL:
       sprintf(slot_str,"Empty Slot");
       break;
     default:
       sprintf(slot_str,"Device is not in AVL");
       break;
  }

  // Get last slot type
  sprintf(vpath,SLOT_RECORD_FILE,slot_id);
  if (read_device(vpath, &last_slot_type)) {
    printf("Get last slot type failed\n");
    return -1;
  }
  unlink(vpath);

  // 0(Server), 1(Crane Flat), 2(Glacier Point), 3(Empty Slot)
  switch (last_slot_type) {
     case SLOT_TYPE_SERVER: {
#if defined(CONFIG_FBY2_RC) || defined(CONFIG_FBY2_EP)
      int last_server_type = -1;
       sprintf(vpath, SV_TYPE_RECORD_FILE, slot_id);
       if (read_device(vpath, &last_server_type)) {
         syslog(LOG_ERR, "%s, Get last server type failed\n", __func__);
         return -1;
       }
       unlink(vpath);

       switch (last_server_type) {
         case SERVER_TYPE_RC:
           sprintf(last_slot_str,"RC");
           break;
         case SERVER_TYPE_EP:
           sprintf(last_slot_str,"EP");
           break;
         case SERVER_TYPE_TL:
           sprintf(last_slot_str,"Twin Lakes");
           break;
         default:
           sprintf(last_slot_str,"Undefined server type");
           break;
       }
#else
       sprintf(last_slot_str,"1S Server");
#endif
       }
       break;
     case SLOT_TYPE_CF:
       sprintf(last_slot_str,"Crane Flat");
       break;
     case SLOT_TYPE_GP:
       sprintf(last_slot_str,"Glacier Point");
       break;
     case SLOT_TYPE_GPV2:
       sprintf(last_slot_str,"Glacier Point V2");
       break;
     case SLOT_TYPE_NULL:
       sprintf(last_slot_str,"Empty Slot");
       break;
     default:
       sprintf(last_slot_str,"Device is not in AVL");
       break;
  }

  if (slot_type != last_slot_type) {
    syslog(LOG_CRIT, "Unexpected swap on SLOT%u from %s to %s, FRU: %u", slot_id, last_slot_str, slot_str, slot_id);
  }

  return 0;
}

// Control 12V to the server in a given slot
static int
server_12v_on(uint8_t slot_id) {
  char vpath[64] = {0};
  int ret=-1;
  uint8_t slot_prsnt, slot_latch;
  int rc, pid_file;
  int retry = MAX_BIC_CHECK_RETRY;
  bic_gpio_t gpio;
  uint8_t pair_slot_id;
  int pair_set_type=-1;
  uint8_t is_sled_out = 1;
#if defined(CONFIG_FBY2_EP)
  uint8_t server_type, config;
#endif

  if (slot_id < 1 || slot_id > 4) {
    return -1;
  }

  // Check if another hotservice-reinit.sh instance of slotX is running
  while(1) {
    pid_file = open(HOTSERVICE_PID, O_CREAT | O_RDWR, 0666);
    rc = flock(pid_file, LOCK_EX);
    if (rc) {
      printf("slot%d is waitting\n", slot_id);
      sleep(1);
    } else {
      break;
    }
  }

  ret = pal_is_fru_prsnt(slot_id, &slot_prsnt);
  if (ret < 0) {
    printf("%s pal_is_fru_prsnt failed for fru: %d\n", __func__, slot_id);
    flock(pid_file, LOCK_UN);
    close(pid_file);
    return -1;
  }

  // Reject 12V-on action when SLOT is not present or SLOT ejector latch pin is high
  if (1 != slot_prsnt) {
    flock(pid_file, LOCK_UN);
    close(pid_file);
    return -1;
  }

  ret = pal_hot_service_action(slot_id);
  flock(pid_file, LOCK_UN);
  close(pid_file);
  if (ret < 0) {
    return -1;
  }

  if (!pal_is_valid_pair(slot_id, &pair_set_type)) {
    return -1;
  }

  if (!pal_is_device_pair(slot_id)) {  // Write 12V on
    _set_slot_12v_en_time(slot_id);
    sprintf(vpath, GPIO_VAL, gpio_12v[slot_id]);
    if (write_device(vpath, "1")) {
      return -1;
    }
    pal_set_hsvc_ongoing(slot_id, 0, 1);
  }

  sleep(2); // Wait for latch pin stable

  // FAN Latch Detect
  if (pal_get_fan_latch(&is_sled_out) != 0) {
    syslog(LOG_WARNING, "Get SLED status in/out is failed.");
    is_sled_out = 1; // default sled out
  }

  ret = pal_is_slot_latch_closed(slot_id, &slot_latch);
  if (ret < 0) {
    syslog(LOG_WARNING,"%s: pal_is_slot_latch_closed failed for slot: %d", __func__, slot_id);
    return -1;
  }

  if (1 != slot_latch) {
    if (is_sled_out) { // Only activate 12V off action when sled is out
      server_12v_off(slot_id);
      return -1;
    }
  }

  if (pal_is_device_pair(slot_id)) {
    if (0 == slot_id%2)
      pair_slot_id = slot_id - 1;
    else
      pair_slot_id = slot_id + 1;

    ret = pal_is_slot_latch_closed(pair_slot_id, &slot_latch);
    if (ret < 0) {
      syslog(LOG_WARNING,"%s: pal_is_slot_latch_closed failed for slot: %d", __func__, pair_slot_id);
      return -1;
    }

    if (1 != slot_latch) {
      if (is_sled_out) { // Only activate 12V off action when sled is out
        server_12v_off(pair_slot_id);
        return -1;
      }
    }
  }

  if (!pal_is_slot_server(slot_id))
    return 0;

  // Wait for BIC ipmb interface is ready
  while (retry) {
    ret = bic_get_gpio(slot_id, &gpio);
    if (!ret)
      break;
    sleep(1);
    retry--;
  }

#if defined(CONFIG_FBY2_EP)
  retry = 2;
  while (retry > 0) {
    ret = fby2_get_server_type(slot_id, &server_type);
    if (!ret) {
      if (server_type == SERVER_TYPE_EP) {
        config = (pair_set_type == TYPE_CF_A_SV) ? 0x2 : (pair_set_type == TYPE_GP_A_SV) ? 0x1 : 0x0;
        ret = bic_set_pcie_config(slot_id, config);
        if (ret == 0)
          break;
      } else {
        break;
      }
    }
    retry--;
    msleep(100);
  }
#endif

  if (ret) {
    syslog(LOG_INFO, "%s: bic_get_gpio returned error during 12V off to on for fru %d",__func__ ,slot_id);
  }

  return 0;
}

// Debug Card's UART and BMC/SoL port share UART port and need to enable only
// one TXD i.e. either BMC's TXD or Debug Port's TXD.
static int
control_sol_txd(uint8_t slot, uint8_t dis_tx) {
  uint32_t scu_fd;
  uint32_t ctrl;
  void *scu_reg;
  void *scu_pin_ctrl1;
  void *scu_pin_ctrl2;

  scu_fd = open("/dev/mem", O_RDWR | O_SYNC );
  if (scu_fd < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "control_sol_txd: open fails\n");
#endif
    return -1;
  }

  scu_reg = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, scu_fd,
             AST_SCU_BASE);
  scu_pin_ctrl1 = (char*)scu_reg + PIN_CTRL1_OFFSET;
  scu_pin_ctrl2 = (char*)scu_reg + PIN_CTRL2_OFFSET;

  switch(slot) {
  case 1:
    // Disable UART1's TXD and enable others
    ctrl = *(volatile uint32_t*) scu_pin_ctrl2;
    ctrl = (dis_tx) ? ctrl & ~UART1_TXD : ctrl | UART1_TXD;
    ctrl |= UART2_TXD;
    *(volatile uint32_t*) scu_pin_ctrl2 = ctrl;

    ctrl = *(volatile uint32_t*) scu_pin_ctrl1;
    ctrl |= UART3_TXD | UART4_TXD;
    *(volatile uint32_t*) scu_pin_ctrl1 = ctrl;
    break;
  case 2:
    // Disable UART2's TXD and enable others
    ctrl = *(volatile uint32_t*) scu_pin_ctrl2;
    ctrl |= UART1_TXD;
    ctrl = (dis_tx) ? ctrl & ~UART2_TXD : ctrl | UART2_TXD;
    *(volatile uint32_t*) scu_pin_ctrl2 = ctrl;

    ctrl = *(volatile uint32_t*) scu_pin_ctrl1;
    ctrl |= UART3_TXD | UART4_TXD;
    *(volatile uint32_t*) scu_pin_ctrl1 = ctrl;
    break;
  case 3:
    // Disable UART3's TXD and enable others
    ctrl = *(volatile uint32_t*) scu_pin_ctrl2;
    ctrl |= UART1_TXD | UART2_TXD;
    *(volatile uint32_t*) scu_pin_ctrl2 = ctrl;

    ctrl = *(volatile uint32_t*) scu_pin_ctrl1;
    ctrl = (dis_tx) ? ctrl & ~UART3_TXD : ctrl | UART3_TXD;
    ctrl |= UART4_TXD;
    *(volatile uint32_t*) scu_pin_ctrl1 = ctrl;
    break;
  case 4:
    // Disable UART4's TXD and enable others
    ctrl = *(volatile uint32_t*) scu_pin_ctrl2;
    ctrl |= UART1_TXD | UART2_TXD;
    *(volatile uint32_t*) scu_pin_ctrl2 = ctrl;

    ctrl = *(volatile uint32_t*) scu_pin_ctrl1;
    ctrl |= UART3_TXD;
    ctrl = (dis_tx) ? ctrl & ~UART4_TXD : ctrl | UART4_TXD;
    *(volatile uint32_t*) scu_pin_ctrl1 = ctrl;
    break;
  default:
    // Any other slots we need to enable all TXDs
    ctrl = *(volatile uint32_t*) scu_pin_ctrl2;
    ctrl |= UART1_TXD | UART2_TXD;
    *(volatile uint32_t*) scu_pin_ctrl2 = ctrl;

    ctrl = *(volatile uint32_t*) scu_pin_ctrl1;
    ctrl |= UART3_TXD | UART4_TXD;
    *(volatile uint32_t*) scu_pin_ctrl1 = ctrl;
    break;
  }

  munmap(scu_reg, PAGE_SIZE);
  close(scu_fd);

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

// Platform Abstraction Layer (PAL) Functions
int
pal_get_platform_name(char *name) {
  strcpy(name, FBY2_PLATFORM_NAME);

  return 0;
}

int
pal_get_num_slots(uint8_t *num) {
  *num = FBY2_MAX_NUM_SLOTS;

  return 0;
}

int
pal_get_num_devs(uint8_t slot, uint8_t *num) {

  switch (fby2_get_slot_type(slot)) {
#ifdef CONFIG_FBY2_GPV2
    case SLOT_TYPE_GPV2:
      *num = MAX_NUM_DEVS;
      break;
#endif
    case SLOT_TYPE_SERVER:
    case SLOT_TYPE_CF:
    case SLOT_TYPE_GP:
    case SLOT_TYPE_NULL:
    default:
      *num = 0;
      break;
  }

  return 0;
}

int
pal_is_slot_latch_closed(uint8_t slot_id, uint8_t *status) {
  int val_latch;
  char path[64] = {0};

  switch (slot_id) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(path, GPIO_VAL, gpio_slot_latch[slot_id]);
      if (read_device(path, &val_latch)) {
        return -1;
      }

      if (val_latch == 0x0) {
        *status = 1;
      } else {
        *status = 0;
      }
      break;
    default:
      return -1;
  }

  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  int val, val_prim, val_ext;
  char path[64] = {0};

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(path, GPIO_VAL, gpio_prsnt_prim[fru]);
      if (read_device(path, &val_prim)) {
        return -1;
      }
      sprintf(path, GPIO_VAL, gpio_prsnt_ext[fru]);

      if (read_device(path, &val_ext)) {
        return -1;
      }

      val = (val_prim || val_ext);

      if (val == 0x0) {
        *status = 1;
      } else {
        *status = 0;
      }
      break;
    case FRU_SPB:
    case FRU_NIC:
      *status = 1;
      break;
    default:
      return -1;
  }

  return 0;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  int val=0;
  char path[64] = {0};
  int ret=-1;

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      switch(fby2_get_slot_type(fru))
      {
        case SLOT_TYPE_SERVER:
        case SLOT_TYPE_GPV2:
          sprintf(path, GPIO_VAL, gpio_bic_ready[fru]);

          if (read_device(path, &val)) {
            return -1;
          }

          if (val == 0x0) {
            *status = 1;
          } else {
            *status = 0;
          }
          break;
       case SLOT_TYPE_CF:
       case SLOT_TYPE_GP:
           ret = pal_is_fru_prsnt(fru,status);
           if(ret < 0)
              return -1;

           /* Check whether the system is 12V off or on */
           ret = pal_is_server_12v_on(fru, (uint8_t *)&val);
           if (ret < 0) {
             syslog(LOG_ERR, "pal_is_fru_ready: pal_is_server_12v_on failed");
             return -1;
           }

           if (1 != val) {
             *status = 0;
           }
           break;
      }
      break;
   case FRU_SPB:
   case FRU_NIC:
     *status = 1;
     break;
   default:
      return -1;
  }

  return 0;
}

int
pal_is_slot_server(uint8_t fru)
{
  if (fby2_get_slot_type(fru) == SLOT_TYPE_SERVER) {
    return 1;
  }
  return 0;
}

int
pal_is_slot_support_update(uint8_t fru)
{
  switch(fby2_get_slot_type(fru))
  {
    case SLOT_TYPE_SERVER:
    case SLOT_TYPE_GPV2:
      return 1;
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
  int ret, val;
  char value[MAX_VALUE_LEN];
  bic_gpio_t gpio;
  uint8_t retry = MAX_READ_RETRY;

  /* Check whether the system is 12V off or on */
  ret = pal_is_server_12v_on(slot_id, status);
  if (ret < 0) {
    syslog(LOG_ERR, "pal_get_server_power: pal_is_server_12v_on failed");
    return -1;
  }

  /* If 12V-off, return */
  if (!(*status)) {
    *status = SERVER_12V_OFF;
    return 0;
  }

  if (!pal_is_slot_server(slot_id)) {
    sprintf(value, GPIO_VAL, gpio_power_en[slot_id]);
    if (!read_device(value, &val)) {
      *status = (val == 0x1) ? SERVER_POWER_ON : SERVER_POWER_OFF;
    } else {
      *status = SERVER_POWER_OFF;
    }
    return 0;
  }

  /* If 12V-on, check if the CPU is turned on or not */
  while (retry) {
    ret = bic_get_gpio(slot_id, &gpio);
    if (!ret)
      break;
    msleep(50);
    retry--;
  }
  if (ret) {
    // Check for if the BIC is irresponsive due to 12V_OFF or 12V_CYCLE
    syslog(LOG_INFO, "pal_get_server_power: bic_get_gpio returned error hence"
        " reading the kv_store for last power state  for fru %d", slot_id);
    memset(value, 0, MAX_VALUE_LEN);
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

  return 0;
}

int
pal_get_device_power(uint8_t slot_id, uint8_t dev_id, uint8_t *status, uint8_t *type) {
  int ret;
  uint8_t retry = MAX_READ_RETRY;

  if (fby2_get_slot_type(slot_id) == SLOT_TYPE_GPV2) {
    /* Check whether the system is 12V off or on */
    ret = pal_is_server_12v_on(slot_id, status);
    if (ret < 0) {
      syslog(LOG_ERR, "pal_get_device_power: pal_is_server_12v_on failed");
      return -1;
    }

    /* If 12V-off, return */
    if (!(*status)) {
      *status = DEVICE_POWER_OFF;
      syslog(LOG_WARNING, "pal_get_device_power: pal_is_server_12v_on 12V-off");
      return 0;
    }

    while (retry) {
      ret = bic_get_dev_power_status(slot_id,dev_id, status,type);
      if (!ret)
        break;
      msleep(50);
      retry--;
    }
    return 0;
  }

  return -1;
}

//check power policy and power state to power on/off server after AC power restore
void
pal_power_policy_control(uint8_t slot_id, char *last_ps) {
  uint8_t chassis_status[5] = {0};
  uint8_t chassis_status_length;
  uint8_t power_policy = POWER_CFG_UKNOWN;
  char pwr_state[MAX_VALUE_LEN] = {0};

  //get power restore policy
  //defined by IPMI Spec/Section 28.2.
  pal_get_chassis_status(slot_id, NULL, chassis_status, &chassis_status_length);

  //byte[1], bit[6:5]: power restore policy
  power_policy = (*chassis_status >> 5);

  //Check power policy and last power state
  if (power_policy == POWER_CFG_LPS) {
    if (!last_ps) {
      pal_get_last_pwr_state(slot_id, pwr_state);
      last_ps = pwr_state;
    }
    if (!(strcmp(last_ps, "on"))) {
      sleep(3);
      pal_set_server_power(slot_id, SERVER_POWER_ON);
    }
  }
  else if(power_policy == POWER_CFG_ON) {
    sleep(3);
    pal_set_server_power(slot_id, SERVER_POWER_ON);
  }
}

static int
server_12v_cycle_physically(uint8_t slot_id){
  uint8_t pair_slot_id;
  int pair_set_type=-1;
  char pwr_state[MAX_VALUE_LEN] = {0};

  if (slot_id == 1 || slot_id == 3) {
    pair_set_type = pal_get_pair_slot_type(slot_id);
    switch(pair_set_type) {
      case TYPE_CF_A_SV:
      case TYPE_GP_A_SV:
      case TYPE_GPV2_A_SV:
        pair_slot_id = slot_id + 1;
        pal_get_last_pwr_state(pair_slot_id, pwr_state);
        if (server_12v_off(pair_slot_id))          //Need to 12V off server first when configuration type is pair config
          return -1;
        sleep(DELAY_12V_CYCLE);
        if (server_12v_on(slot_id))
          return -1;
        pal_power_policy_control(pair_slot_id, pwr_state);
        return 0;
      default:
        break;
    }
  }
  if (server_12v_off(slot_id))
    return -1;

  sleep(DELAY_12V_CYCLE);

  return (server_12v_on(slot_id));
}

int
pal_set_device_power(uint8_t slot_id, uint8_t dev_id, uint8_t cmd) {
  int ret;
  uint8_t status, type;

  if (slot_id != 1 && slot_id != 3) {
    return -1;
  }

  ret = pal_is_fru_ready(slot_id, &status); //Break out if fru is not ready
  if ((ret < 0) || (status == 0)) {
    return -2;
  }

  if (pal_get_device_power(slot_id, dev_id, &status, &type) < 0) {
    return -1;
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if (status == SERVER_POWER_ON)
        return 1;
      else {
        ret = bic_set_dev_power_status(slot_id, dev_id, 1);
        if (ret == 0) {
          pal_set_m2_prsnt(slot_id, dev_id, 1);
        }
        return ret;
      }
      break;

    case SERVER_POWER_OFF:
      if (status == SERVER_POWER_OFF)
        return 1;
      else {
        ret = bic_set_dev_power_status(slot_id, dev_id, 2);
        if (ret == 0) {
          pal_set_m2_prsnt(slot_id, dev_id, 0);
        }
        return ret;
      }
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {

        ret = bic_set_dev_power_status(slot_id, dev_id, 2);
        if (ret == 0) {
          pal_set_m2_prsnt(slot_id, dev_id, 0);
        } else {
          return ret;
        }

        sleep(DELAY_POWER_CYCLE);

        ret = bic_set_dev_power_status(slot_id, dev_id, 1);
        if (ret == 0) {
          pal_set_m2_prsnt(slot_id, dev_id, 1);
        }
        return ret;

      } else if (status == SERVER_POWER_OFF) {

        ret = bic_set_dev_power_status(slot_id, dev_id, 1);
        if (ret == 0) {
          pal_set_m2_prsnt(slot_id, dev_id, 1);
        }
        return ret;
      }
      break;

    default:
      return -1;
  }

  return 0;
}
// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t slot_id, uint8_t cmd) {
  int ret;
  uint8_t status;
  bool gs_flag = false;
  uint8_t pair_slot_id;
  int pair_set_type=-1;

  if (slot_id < 1 || slot_id > 4) {
    return -1;
  }

  if ((cmd != SERVER_12V_OFF) && (cmd != SERVER_12V_ON) && (cmd != SERVER_12V_CYCLE)) {
    ret = pal_is_fru_ready(slot_id, &status); //Break out if fru is not ready
    if ((ret < 0) || (status == 0)) {
      return -2;
    }

    if (pal_get_server_power(slot_id, &status) < 0) {
      return -1;
    }
   }

  switch(cmd) {     //avoid power control on GP and CF
    case SERVER_POWER_OFF:
    case SERVER_POWER_CYCLE:
    case SERVER_POWER_RESET:
    case SERVER_GRACEFUL_SHUTDOWN:
    case SERVER_POWER_ON:
      if(pal_is_slot_server(slot_id) == 0) {
        printf("Should not execute power on/off/graceful_shutdown/cycle/reset on device card\n");
        return -2;
      }
      break;
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
        return server_power_off(slot_id, gs_flag);
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        if (server_power_off(slot_id, gs_flag))
          return -1;

        sleep(DELAY_POWER_CYCLE);

        return server_power_on(slot_id);

      } else if (status == SERVER_POWER_OFF) {

        return (server_power_on(slot_id));
      }
      break;

    case SERVER_POWER_RESET:
      if (status == SERVER_POWER_ON) {

        ret = pal_set_rst_btn(slot_id, 0);
        if (ret < 0)
          return ret;

        msleep(100); //some server miss to detect a quick pulse, so delay 100ms between low high

        ret = pal_set_rst_btn(slot_id, 1);
        if (ret < 0)
          return ret;
      } else if (status == SERVER_POWER_OFF) {
        printf("Ignore to execute power reset action when the power status of server is off\n");
        return -2;
      }
      break;

    case SERVER_GRACEFUL_SHUTDOWN:
      if (status == SERVER_POWER_OFF) {
        return 1;
      } else {
        gs_flag = true;
        return server_power_off(slot_id, gs_flag);
      }
      break;

    case SERVER_12V_ON:
      /* Check whether the system is 12V off or on */
      ret = pal_is_server_12v_on(slot_id, &status);
      if (ret < 0) {
        syslog(LOG_ERR, "pal_set_server_power: pal_is_server_12v_on failed");
        return -1;
      }
      if(status)  //Have already 12V-ON
        return 1;

      if (slot_id == 1 || slot_id == 3) {     //Handle power policy for pair configuration
        pair_set_type = pal_get_pair_slot_type(slot_id);
        switch(pair_set_type) {
          case TYPE_CF_A_SV:
          case TYPE_GP_A_SV:
          case TYPE_GPV2_A_SV:
            pair_slot_id = slot_id + 1;
            ret = server_12v_on(slot_id);
            if (ret != 0)
              return ret;
            pal_power_policy_control(pair_slot_id, NULL);
            return ret;
          default:
            break;
        }
      }
      return server_12v_on(slot_id);

    case SERVER_12V_OFF:
      /* Check whether the system is 12V off or on */
      ret = pal_is_server_12v_on(slot_id, &status);
      if (ret < 0) {
        syslog(LOG_ERR, "pal_set_server_power: pal_is_server_12v_off failed");
        return -1;
      }
      if (!status)  //Have already 12V-OFF
        return 1;

      if (slot_id == 1 || slot_id == 3) {      //Need to 12V off server first when configuration type is pair config
        pair_set_type = pal_get_pair_slot_type(slot_id);
        switch(pair_set_type) {
          case TYPE_CF_A_SV:
          case TYPE_GP_A_SV:
          case TYPE_GPV2_A_SV:
            pair_slot_id = slot_id + 1;
            return server_12v_off(pair_slot_id);
          default:
            break;
        }
      }
      return server_12v_off(slot_id);

    case SERVER_12V_CYCLE:
      ret = server_12v_cycle_physically(slot_id);
      return ret;

    case SERVER_GLOBAL_RESET:
      return server_power_off(slot_id, false);

    default:
      return -1;
  }

  return 0;
}

int
pal_get_fan_latch(uint8_t *status) {
  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_FAN_LATCH_DETECT);
  if (read_device(path, &val)) {
    return -1;
  }

  if (1 == val)
    *status = 1;
  else
    *status = 0;

  return 0;
}

int
pal_sled_cycle(void) {
  // Remove the adm1275 module as the HSC device is busy
  system("rmmod adm1275");

  // Send command to HSC power cycle
  system("i2cset -y 10 0x40 0xd9 c");

  return 0;
}

// Read the Front Panel Hand Switch and return the position
int
pal_get_hand_sw_physically(uint8_t *pos) {
  int gpio_fd;
  void *gpio_reg;
  uint8_t loc;

  if (!m_gpio_hand_sw) {
    gpio_fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (gpio_fd < 0) {
      return -1;
    }

    gpio_reg = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, gpio_fd, AST_GPIO_BASE);
    m_gpio_hand_sw = (char*)gpio_reg + GPIO_AB_AA_Z_Y;
    close(gpio_fd);
  }

  loc = (((*(volatile uint32_t*) m_gpio_hand_sw) >> 20) & 0xF) % 5;  // GPIOAA[7:4]

  switch(loc) {
  case 0:
    *pos = HAND_SW_SERVER1;
    break;
  case 1:
    *pos = HAND_SW_SERVER2;
    break;
  case 2:
    *pos = HAND_SW_SERVER3;
    break;
  case 3:
    *pos = HAND_SW_SERVER4;
    break;
  default:
    *pos = HAND_SW_BMC;
    break;
  }

  return 0;
}

int
pal_get_hand_sw(uint8_t *pos) {
  char value[MAX_VALUE_LEN] = {0};
  uint8_t loc;
  int ret;

  ret = kv_get("spb_hand_sw", value, NULL, 0);
  if (!ret) {
    loc = atoi(value);
    if ((loc > HAND_SW_BMC) || (loc < HAND_SW_SERVER1)) {
      return pal_get_hand_sw_physically(pos);
    }

    *pos = loc;
    return 0;
  }

  return pal_get_hand_sw_physically(pos);
}

// Return the Front panel Power Button
int
pal_get_pwr_btn(uint8_t *status) {
  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_PWR_BTN);
  if (read_device(path, &val)) {
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
  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_RST_BTN);
  if (read_device(path, &val)) {
    return -1;
  }

  if (val) {
    *status = 0x0;
  } else {
    *status = 0x1;
  }

  return 0;
}

// Update the SLED LED for sled fully seated
int
pal_set_sled_led(uint8_t status) {
  char path[64] = {0};

  memset(path, 0, sizeof(path));
  sprintf(path, GPIO_VAL, GPIO_SLED_SEATED_N);
  if (status) {
    if (write_device(path, "1")) {
      return -1;
    }
  }
  else {
    if (write_device(path, "0")) {
      return -1;
    }
  }

  return 0;
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
  int vic_fd;
  void *vic_reg;
  void *vic_hb_mode;
  uint32_t ctrl;

  if (!m_hbled_output) {
    vic_fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (vic_fd < 0) {
      return -1;
    }

    vic_reg = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, vic_fd, AST_VIC_BASE);
    vic_hb_mode = (char*)vic_reg + HW_HB_STATUS_OFFSET;
    m_hbled_output = (char*)vic_reg + HB_LED_OUTPUT_OFFSET;
    close(vic_fd);

    ctrl = *(volatile uint32_t*) vic_hb_mode;
    if (!(ctrl & SW_BLINK)) {
      ctrl |= SW_BLINK;
      *(volatile uint32_t*) vic_hb_mode = ctrl;
    }
  }

  *(volatile uint32_t*) m_hbled_output = status;

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

int
pal_set_slot_id_led(uint8_t slot, uint8_t status) {
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

  sprintf(path, GPIO_VAL, gpio_slot_id_led[slot]);
  if (write_device(path, val)) {
    return -1;
  }

  return 0;
}

static int
set_usb_mux(uint8_t state) {
  int val;
  char *new_state;
  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_USB_MUX_EN_N);

  if (read_device(path, &val)) {
    return -1;
  }

  // This GPIO Pin is active low
  if ((!val) == state)
    return 0;

  if (state)
    new_state = "0";
  else
    new_state = "1";

  if (write_device(path, new_state) < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "write_device failed for %s\n", path);
#endif
    return -1;
  }

  return 0;
}

int
pal_enable_usb_mux(uint8_t state) {
  return set_usb_mux(state);
}

// Update the VGA Mux to the server at given slot
int
pal_switch_vga_mux(uint8_t slot) {
  char *gpio_sw0, *gpio_sw1;
  char path[64] = {0};

  // Based on the VGA mux table in Schematics
  switch(slot) {
  case HAND_SW_SERVER1:
    gpio_sw0 = "0";
    gpio_sw1 = "0";
    break;
  case HAND_SW_SERVER2:
    gpio_sw0 = "1";
    gpio_sw1 = "0";
    break;
  case HAND_SW_SERVER3:
    gpio_sw0 = "0";
    gpio_sw1 = "1";
    break;
  case HAND_SW_SERVER4:
    gpio_sw0 = "1";
    gpio_sw1 = "1";
    break;
  default:   // default case, assumes server 1
    gpio_sw0 = "0";
    gpio_sw1 = "0";
    break;
  }

  sprintf(path, GPIO_VAL, GPIO_VGA_SW0);
  if (write_device(path, gpio_sw0) < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "write_device failed for %s\n", path);
#endif
    return -1;
  }

  sprintf(path, GPIO_VAL, GPIO_VGA_SW1);
  if (write_device(path, gpio_sw1) < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "write_device failed for %s\n", path);
#endif
    return -1;
  }

  return 0;
}

// Update the USB Mux to the server at given slot
int
pal_switch_usb_mux(uint8_t slot) {
  char *gpio_sw0, *gpio_sw1;
  char path[64] = {0};
  uint8_t status;

  // Based on the USB mux table in Schematics
  switch(slot) {
  case HAND_SW_SERVER1:
    gpio_sw0 = "0";
    gpio_sw1 = "0";
    break;
  case HAND_SW_SERVER2:
    gpio_sw0 = "1";
    gpio_sw1 = "0";
    break;
  case HAND_SW_SERVER3:
    gpio_sw0 = "0";
    gpio_sw1 = "1";
    break;
  case HAND_SW_SERVER4:
    gpio_sw0 = "1";
    gpio_sw1 = "1";
    break;
  case HAND_SW_BMC:
    // Disable the USB MUX
    if (set_usb_mux(USB_MUX_OFF) < 0)
      return -1;
    else
      return 0;
  default:
    return 0;
  }

  if (!pal_is_slot_server(slot) || (!pal_get_server_power(slot, &status) && (status != SERVER_POWER_ON))) {
    if (set_usb_mux(USB_MUX_OFF) < 0) {
      return -1;
    }
    return 0;
  }

  sprintf(path, GPIO_VAL, GPIO_USB_SW0);
  if (write_device(path, gpio_sw0) < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "write_device failed for %s\n", path);
#endif
    return -1;
  }

  sprintf(path, GPIO_VAL, GPIO_USB_SW1);
  if (write_device(path, gpio_sw1) < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "write_device failed for %s\n", path);
#endif
    return -1;
  }

  // Enable the USB MUX
  if (set_usb_mux(USB_MUX_ON) < 0)
    return -1;

  return 0;
}

// Switch the UART mux to the given slot
int
pal_switch_uart_mux(uint8_t slot) {
  char * gpio_uart_sel0;
  char * gpio_uart_sel1;
  char * gpio_uart_sel2;
  char * gpio_uart_rx;
  char path[64] = {0};
  uint8_t prsnt;
  int ret;

  ret = pal_is_debug_card_prsnt(&prsnt);
  if (ret) {
    goto uart_exit;
  }

  // Refer the UART select table in schematic
  switch(slot) {
  case HAND_SW_SERVER1:
    gpio_uart_sel2 = "0";
    gpio_uart_sel1 = "0";
    gpio_uart_sel0 = "0";
    gpio_uart_rx = (prsnt) ? "0" : "1";
    break;
  case HAND_SW_SERVER2:
    gpio_uart_sel2 = "0";
    gpio_uart_sel1 = "0";
    gpio_uart_sel0 = "1";
    gpio_uart_rx = (prsnt) ? "0" : "1";
    break;
  case HAND_SW_SERVER3:
    gpio_uart_sel2 = "0";
    gpio_uart_sel1 = "1";
    gpio_uart_sel0 = "0";
    gpio_uart_rx = (prsnt) ? "0" : "1";
    break;
  case HAND_SW_SERVER4:
    gpio_uart_sel2 = "0";
    gpio_uart_sel1 = "1";
    gpio_uart_sel0 = "1";
    gpio_uart_rx = (prsnt) ? "0" : "1";
    break;
  default:
    // for all other cases, assume BMC
    gpio_uart_sel2 = "1";
    gpio_uart_sel1 = "0";
    gpio_uart_sel0 = "0";
    gpio_uart_rx = "1";
    break;
  }

  //  Diable TXD path from BMC to avoid conflict with SoL
  ret = control_sol_txd(slot, prsnt);
  if (ret) {
    goto uart_exit;
  }

  // Enable Debug card path
  sprintf(path, GPIO_VAL, GPIO_UART_SEL2);
  ret = write_device(path, gpio_uart_sel2);
  if (ret) {
    goto uart_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_UART_SEL1);
  ret = write_device(path, gpio_uart_sel1);
  if (ret) {
    goto uart_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_UART_SEL0);
  ret = write_device(path, gpio_uart_sel0);
  if (ret) {
    goto uart_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_UART_RX);
  ret = write_device(path, gpio_uart_rx);
  if (ret) {
    goto uart_exit;
  }

uart_exit:
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_switch_uart_mux: write_device failed: %s\n", path);
#endif
    return ret;
  } else {
    return 0;
  }
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

  if (0 == t->bits.post) {
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
  uint8_t prsnt, pos;
  int ret;

  // Check for debug card presence
  ret = pal_is_debug_card_prsnt(&prsnt);
  if (ret) {
    return ret;
  }

  // No debug card  present, return
  if (!prsnt) {
    return 0;
  }

  // Get the hand switch position
  ret = pal_get_hand_sw(&pos);
  if (ret) {
    return ret;
  }

  // If the give server is not selected, return
  if (pos != slot) {
    return 0;
  }

  // Display the post code in the debug card
  ret = pal_post_display(status);
  if (ret) {
    return ret;
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

  return fby2_common_fru_id(str, fru);
}

int
pal_get_dev_id(char *str, uint8_t *dev) {

  return fby2_common_dev_id(str, dev);
}

int
pal_get_fru_name(uint8_t fru, char *name) {

  return fby2_common_fru_name(fru, name);
}

int
pal_get_dev_name(uint8_t fru, uint8_t dev, char *name)
{
  int ret = fby2_get_fruid_name(fru, name);
  if (ret < 0)
    return ret;

  sprintf(name, "%s Device %u", name, dev-1);
  return 0;
}

int
pal_get_fru_sdr_path(uint8_t fru, char *path) {
  return fby2_sensor_sdr_path(fru, path);
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      switch(fby2_get_slot_type(fru))
      {
        case SLOT_TYPE_SERVER: {
#if defined(CONFIG_FBY2_RC) || defined(CONFIG_FBY2_EP)
            uint8_t server_type = 0xFF;
            int ret = -1;
            ret = fby2_get_server_type(fru, &server_type);
            if (ret) {
              syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
            }
            switch (server_type) {
#if defined(CONFIG_FBY2_RC)
              case SERVER_TYPE_RC:
                *sensor_list = (uint8_t *) bic_rc_sensor_list;
                *cnt = bic_rc_sensor_cnt;
                break;
#endif
#if defined(CONFIG_FBY2_EP)
              case SERVER_TYPE_EP:
                *sensor_list = (uint8_t *) bic_ep_sensor_list;
                *cnt = bic_ep_sensor_cnt;
                break;
#endif
              case SERVER_TYPE_TL:
                *sensor_list = (uint8_t *) bic_sensor_list;
                *cnt = bic_sensor_cnt;
                break;
              default:
                syslog(LOG_ERR, "%s, Undefined server type, using Twin Lake sensor list as default\n", __func__);
                *sensor_list = (uint8_t *) bic_sensor_list;
                *cnt = bic_sensor_cnt;
              break;
            }
#else
            *sensor_list = (uint8_t *) bic_sensor_list;
            *cnt = bic_sensor_cnt;
#endif
            }
            break;
        case SLOT_TYPE_CF:
            *sensor_list = (uint8_t *) dc_cf_sensor_list;
            *cnt = dc_cf_sensor_cnt;
            break;
        case SLOT_TYPE_GP:
            *sensor_list = (uint8_t *) dc_sensor_list;
            *cnt = dc_sensor_cnt;
            break;
#ifdef CONFIG_FBY2_GPV2
        case SLOT_TYPE_GPV2:
            *sensor_list = (uint8_t *) gpv2_sensor_list;
            *cnt = gpv2_sensor_cnt;
            break;
#endif
        default:
            return -1;
            break;
      }
      break;
    case FRU_SPB:
      *sensor_list = (uint8_t *) spb_sensor_list;
      *cnt = spb_sensor_cnt;
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
pal_read_nic_fruid(const char *path, int size) {
  uint8_t wbuf[8], rbuf[32];
  uint8_t offs_len, addr;
  char *bus;
  int offset, count;
  int fd = -1, dev = -1, ret = -1;

  fd = open(path, O_WRONLY | O_CREAT, 0644);
  if (fd < 0) {
    goto error_exit;
  }

  if (pal_is_ocp30_nic()) {
    bus = "/dev/i2c-11";
    addr = 0xA0;
    offs_len = 2;
  } else {
    bus = "/dev/i2c-12";
    addr = 0xA2;
    offs_len = (fby2_get_nic_mfgid() == MFG_BROADCOM) ? 1 : 2;
  }

  dev = open(bus, O_RDWR);
  if (dev < 0) {
    goto error_exit;
  }

  // Read chunks of FRUID binary data in a loop
  for (offset = 0; offset < size; offset += 8) {
    if (offs_len == 2) {
      wbuf[0] = offset >> 8;
      wbuf[1] = offset;
    } else {
      wbuf[0] = offset;
    }
    ret = i2c_rdwr_msg_transfer(dev, addr, wbuf, offs_len, rbuf, 8);
    if (ret) {
      break;
    }

    count = write(fd, rbuf, 8);
    if (count != 8) {
      ret = -1;
      break;
    }
  }

error_exit:
  if (fd > 0 ) {
    close(fd);
  }

  if (dev > 0 ) {
    close(dev);
  }

  return ret;
}

static int
_write_nic_fruid(const char *path) {
  uint8_t wbuf[32];
  uint8_t offs_len, addr;
  char *bus;
  int offset, count;
  int fd = -1, dev = -1, ret = -1;

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    goto error_exit;
  }

  if (pal_is_ocp30_nic()) {
    bus = "/dev/i2c-11";
    addr = 0xA0;
    offs_len = 2;
  } else {
    bus = "/dev/i2c-12";
    addr = 0xA2;
    offs_len = (fby2_get_nic_mfgid() == MFG_BROADCOM) ? 1 : 2;
  }

  dev = open(bus, O_RDWR);
  if (dev < 0) {
    goto error_exit;
  }

  // Write chunks of FRUID binary data in a loop
  offset = 0;
  while (1) {
    count = read(fd, &wbuf[offs_len], 8);
    if (count <= 0) {
      break;
    }

    if (offs_len == 2) {
      wbuf[0] = offset >> 8;
      wbuf[1] = offset;
    } else {
      wbuf[0] = offset;
    }
    ret = i2c_rdwr_msg_transfer(dev, addr, wbuf, count+offs_len, NULL, 0);
    if (ret) {
      break;
    }

    offset += count;
  }

error_exit:
  if (fd > 0 ) {
    close(fd);
  }

  if (dev > 0 ) {
    close(dev);
  }

  return ret;
}

int
pal_fruid_write(uint8_t fru, char *path) {
  if (fru == FRU_NIC) {
    return _write_nic_fruid(path);
  }
  return bic_write_fruid(fru, 0, path);
}

int
pal_dev_fruid_write(uint8_t fru, uint8_t dev_id, char *path) {
  return bic_write_fruid(fru, dev_id, path);
}

int
pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  uint8_t status;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      pal_is_fru_prsnt(fru, &status);
      break;
    case FRU_SPB:
    case FRU_NIC:
      status = 1;
      break;
  }

  if (status)
    return fby2_sensor_sdr_init(fru, sinfo);
  else
    return -1;
}

static sensor_check_t *
get_sensor_check(uint8_t fru, uint8_t snr_num) {

  if (fru < 1 || fru > MAX_NUM_FRUS) {
    syslog(LOG_WARNING, "get_sensor_check: Wrong FRU ID %d\n", fru);
    return NULL;
  }

  return &m_snr_chk[fru-1][snr_num];
}

static sensor_desc_t *
get_sensor_desc(uint8_t fru, uint8_t snr_num) {

  if (fru < 1 || fru > MAX_NUM_FRUS) {
    syslog(LOG_WARNING, "get_sensor_desc: Wrong FRU ID %d\n", fru);
    return NULL;
  }

  return &m_snr_desc[fru-1][snr_num];
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {

  uint8_t status;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int ret;
  uint8_t val;
  uint8_t retry = MAX_READ_RETRY;
  sensor_check_t *snr_chk;
  static long last_counter = 0;
  long current_counter = 0;
  static uint8_t last_post = 0;
  uint8_t current_post = 0;
  static uint8_t is_last_post_time_out = 0;
  uint8_t is_post_time_out = 0;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(key, "slot%d_sensor%d", fru, sensor_num);
      if(pal_is_fru_prsnt(fru, &status) < 0)
         return -1;
      if (!status) {
         return -1;
      }
      break;
    case FRU_SPB:
      sprintf(key, "spb_sensor%d", sensor_num);
      break;
    case FRU_NIC:
      sprintf(key, "nic_sensor%d", sensor_num);
      break;
    default:
      return -1;
  }
  snr_chk = get_sensor_check(fru, sensor_num);

  while (retry) {
    ret = fby2_sensor_read(fru, sensor_num, value);
    if ((ret >= 0) || (ret == EER_READ_NA))
      break;
    msleep(50);
    retry--;
  }

  if(ret < 0) {
    if ((ret == EER_READ_NA) && snr_chk->val_valid) {
      snr_chk->val_valid = 0;
      return -1;
    }
    snr_chk->val_valid = 0;

    if (ret == EER_READ_NA)
      return ERR_SENSOR_NA;

    if(fru == FRU_SPB || fru == FRU_NIC)
      return -1;
    if(pal_get_server_power(fru, &status) < 0)
      return -1;
    // This check helps interpret the IPMI packet loss scenario
    if(status == SERVER_POWER_ON)
      return -1;

    return ERR_SENSOR_NA;
  }
  else {
    // On successful sensor read
    if (fru == FRU_SPB) {
      if (sensor_num == SP_SENSOR_HSC_OUT_CURR || sensor_num == SP_SENSOR_HSC_PEAK_IOUT) {
        power_value_adjust(curr_cali_table, (float *)value);
      }
      if (sensor_num == SP_SENSOR_HSC_IN_POWER || sensor_num == SP_SENSOR_HSC_PEAK_PIN) {
        power_value_adjust(pwr_cali_table, (float *)value);
      }
      if (sensor_num == SP_SENSOR_INLET_TEMP) {
        apply_inlet_correction((float *)value);
      }
      if ((sensor_num == SP_SENSOR_P12V_SLOT1) || (sensor_num == SP_SENSOR_P12V_SLOT2) ||
          (sensor_num == SP_SENSOR_P12V_SLOT3) || (sensor_num == SP_SENSOR_P12V_SLOT4)) {
        /* Check whether the system is 12V off or on */
        ret = pal_is_server_12v_on(sensor_num - SP_SENSOR_P12V_SLOT1 + 1, &val);
        if (ret < 0) {
          syslog(LOG_ERR, "%s: pal_is_server_12v_on failed",__func__);
        }
        if (!val) {
          sprintf(str, "%.2f",*((float*)value));
          kv_set(key, str, 0, 0);
          return -1;
        }
      }
      if ((sensor_num == SP_SENSOR_FAN0_TACH) || (sensor_num == SP_SENSOR_FAN1_TACH)) {
        uint8_t is_sled_out = 1;

        if (pal_get_fan_latch(&is_sled_out) != 0) {
          syslog(LOG_WARNING, "Fans' UNC masks removed: SLED status (in/out) is unreadable");
          is_sled_out = 1; // default sled out
        }

        is_post_time_out = pal_is_post_time_out();
        if (is_post_time_out || is_sled_out) {
          if (is_post_time_out && !is_last_post_time_out) {
            syslog(LOG_WARNING, "Fans' UNC masks removed: slot%d host POST hangs up",is_post_time_out);
          }
          ignore_thresh = 0;
        } else {
          /* Check whether POST is ongoning or not */
          ignore_thresh = 0;

          current_post = pal_is_post_ongoing();
          if (last_post != current_post) {
            if (current_post == 0) {
              // POST END
              syslog(LOG_WARNING, "all fru POST END");
              post_end_counter = pal_get_fscd_counter();
              if (post_end_counter == 0) {
                // fscd not start yet or fscd hangs up at the first run
                syslog(LOG_WARNING, "fscd not start yet or fscd hangs up at the first run");
                current_counter = 0;
                post_end_counter = POST_END_COUNTER_SHOW_LOG;
                // fscd watchdog counter update
                fscd_watchdog_counter = 1;
              } else {
                fscd_watchdog_counter = 0;
              }
            } else {
              ignore_thresh = 1;
            }
            // update POST status;
            last_post = current_post;
          }

          if (current_post == 1) {
            // POST is ongoing
            ignore_thresh = 1;
          } else {
            // POST is not ongoing
            if (post_end_counter > 0) {
              // check counter change
              current_counter = pal_get_fscd_counter();
              if (last_counter != current_counter) {
                // fscd counter update
                fscd_watchdog_counter = 0;
                last_counter = current_counter;
                if (current_counter-post_end_counter >= 4) { //after 4 fscd cycle
                  syslog(LOG_WARNING, "Fans' UNC masks removed: after 4 fscd cycle");
                  post_end_counter = 0;
                  ignore_thresh = 0;
                } else { //before 4 fscd cycle
                  ignore_thresh = 1;
                }
              } else {
                // fscd watchdog counter update
                pal_check_fscd_watchdog();
              }
            } else if (post_end_counter < 0) {
              // fscd not start yet or fscd hangs up at the first run
              pal_check_fscd_watchdog();
            }
          }
        }
        is_last_post_time_out = is_post_time_out;
        pal_set_ignore_thresh(ignore_thresh);
      }
    }
    if ((GETBIT(snr_chk->flag, UCR_THRESH) && (*((float*)value) >= snr_chk->ucr)) ||
        (GETBIT(snr_chk->flag, LCR_THRESH) && (*((float*)value) <= snr_chk->lcr))) {
      if (snr_chk->retry_cnt < MAX_CHECK_RETRY) {
        snr_chk->retry_cnt++;
        if (!snr_chk->val_valid)
          return -1;

        *((float*)value) = snr_chk->last_val;
      }
    }
    else {
      snr_chk->last_val = *((float*)value);
      snr_chk->val_valid = 1;
      snr_chk->retry_cnt = 0;
    }
  }

  return ret;
}

void
pal_check_fscd_watchdog() {
  fscd_watchdog_counter++;
  if (fscd_watchdog_counter > 50) {
    //fscd is killed or fscd hangs up
    syslog(LOG_WARNING, "Fans' UNC masks removed: after POST, fscd works abnormally");
    post_end_counter = 0;
    ignore_thresh = 0;
  } else {
    ignore_thresh = 1;
  }

  long counter = pal_get_fscd_counter();
  if ((post_end_counter < 0) && counter) {
    //fscd start working after post end
    if (post_end_counter == POST_END_COUNTER_SHOW_LOG) {
      syslog(LOG_WARNING, "fscd start working after POST end");
    }
    post_end_counter = counter;
  }
}

int
pal_ignore_thresh(uint8_t fru, uint8_t snr_num, uint8_t thresh) {
  if (fru== FRU_SPB) {
    if ((snr_num == SP_SENSOR_FAN0_TACH) || (snr_num == SP_SENSOR_FAN1_TACH)) {
      if (thresh == UNC_THRESH) {
        if (ignore_thresh) {
          return 1;
        }
      }
    }
  }
  return 0;
}

int
pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag) {


  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      if (fby2_get_slot_type(fru) == SLOT_TYPE_SERVER) {
#ifdef CONFIG_FBY2_RC
        int ret;
        uint8_t server_type = 0xFF;
        ret = fby2_get_server_type(fru, &server_type);
        if (ret) {
          syslog(LOG_INFO, "%s, Get server type failed, using Twinlake");
        }
        switch (server_type) {
          case SERVER_TYPE_RC:
            break;
          case SERVER_TYPE_TL:
            if (snr_num == BIC_SENSOR_SOC_THERM_MARGIN)
              *flag = GETMASK(SENSOR_VALID) | GETMASK(UCR_THRESH);
            else if (snr_num == BIC_SENSOR_SOC_PACKAGE_PWR)
              *flag = GETMASK(SENSOR_VALID);
            else if (snr_num == BIC_SENSOR_SOC_TJMAX)
              *flag = GETMASK(SENSOR_VALID);
            break;
          default:
            break;
        }
#else
        if (snr_num == BIC_SENSOR_SOC_THERM_MARGIN)
          *flag = GETMASK(SENSOR_VALID) | GETMASK(UCR_THRESH);
        else if (snr_num == BIC_SENSOR_SOC_PACKAGE_PWR)
          *flag = GETMASK(SENSOR_VALID);
        else if (snr_num == BIC_SENSOR_SOC_TJMAX)
          *flag = GETMASK(SENSOR_VALID);
#endif
      }
      break;
    case FRU_SPB:
    case FRU_NIC:
      break;
  }

  return 0;
}

int
pal_alter_sensor_thresh_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag) {

  uint8_t val;
  int ret;
  uint8_t slot_id;

  if ( fru == FRU_SPB ) {
    /*
     * TODO: This is a HACK (t11229576)
     */
    if(( snr_num == SP_SENSOR_P12V_SLOT1 ) || ( snr_num == SP_SENSOR_P12V_SLOT2 )
      || ( snr_num == SP_SENSOR_P12V_SLOT3 ) || ( snr_num == SP_SENSOR_P12V_SLOT4 )){
      /* Check whether the system is 12V off or on */
      slot_id = snr_num - SP_SENSOR_P12V_SLOT1 + 1;
      ret = pal_is_server_12v_on(slot_id, &val);
      if (ret < 0) {
        syslog(LOG_ERR, "%s: pal_is_server_12v_on failed",__func__);
      }
      if (!val || !_check_slot_12v_en_time(slot_id)) {
        *flag = GETMASK(SENSOR_VALID);
      }
    } else if (( snr_num == SP_SENSOR_FAN0_TACH ) || ( snr_num == SP_SENSOR_FAN1_TACH )) {
      // Check POST status
      int ret = pal_get_ignore_thresh(&ignore_thresh);
      if ((ret == 0) && ignore_thresh) {
        *flag = CLEARBIT(*flag,UNC_THRESH);
      }
    }
  }

  return 0;
}


int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  return fby2_sensor_threshold(fru, sensor_num, thresh, value);
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  return fby2_sensor_name(fru, sensor_num, name);
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  return fby2_sensor_units(fru, sensor_num, units);
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  return fby2_get_fruid_path(fru, DEV_NONE, path);
}

int
pal_get_dev_fruid_path(uint8_t fru, uint8_t dev_id, char *path) {
  return fby2_get_fruid_path(fru, dev_id, path);
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  return fby2_get_fruid_eeprom_path(fru, path);
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  return fby2_get_fruid_name(fru, name);
}

int
pal_set_def_key_value() {

  int ret;
  int i;
  int fru;
  char key[MAX_KEY_LEN] = {0};

  i = 0;
  for (i = 0; strcmp(key_cfg[i].name, LAST_KEY) != 0; i++) {
    if ((ret = kv_set(key_cfg[i].name, key_cfg[i].def_val, 0, KV_FPERSIST | KV_FCREATE)) < 0) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_set_def_key_value: kv_set failed. %d", ret);
#endif
    }
    if (key_cfg[i].function) {
      key_cfg[i].function(KEY_AFTER_INI, key_cfg[i].name);
    }
  }

  /* Actions to be taken on Power On Reset */
  if (pal_is_bmc_por()) {

    for (fru = 1; fru <= MAX_NUM_FRUS; fru++) {

      /* Clear all the SEL errors */
      memset(key, 0, MAX_KEY_LEN);

      switch(fru) {
        case FRU_SLOT1:
        case FRU_SLOT2:
        case FRU_SLOT3:
        case FRU_SLOT4:
          sprintf(key, "slot%d_sel_error", fru);
        break;

        case FRU_SPB:
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
        case FRU_SLOT2:
        case FRU_SLOT3:
        case FRU_SLOT4:
          sprintf(key, "slot%d_sensor_health", fru);
        break;

        case FRU_SPB:
          continue;

        case FRU_NIC:
          continue;

        default:
          return -1;
      }

      /* Write the value "1" which means FRU_STATUS_GOOD */
      ret = pal_set_key_value(key, "1");
    }

    system("i2cset -y -f 10 0x40 0x03 0");
  }

  return 0;
}

int
pal_get_fru_devtty(uint8_t fru, char *devtty) {

  switch(fru) {
    case FRU_SLOT1:
      sprintf(devtty, "/dev/ttyS1");
      break;
    case FRU_SLOT2:
      sprintf(devtty, "/dev/ttyS2");
      break;
    case FRU_SLOT3:
      sprintf(devtty, "/dev/ttyS3");
      break;
    case FRU_SLOT4:
      sprintf(devtty, "/dev/ttyS4");
      break;
    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_get_fru_devtty: Wrong fru id %u", fru);
#endif
      return -1;
  }
    return 0;
}

void
pal_dump_key_value(void) {
  int i = 0;
  int ret;

  char value[MAX_VALUE_LEN] = {0x0};

  while (strcmp(key_cfg[i].name, LAST_KEY)) {
    printf("%s:", key_cfg[i].name);
    if ((ret = kv_get(key_cfg[i].name, value, NULL, KV_FPERSIST)) < 0) {
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

  sprintf(key, "pwr_server%d_last_state", (int) fru);

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
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:

      sprintf(key, "pwr_server%d_last_state", (int) fru);

      ret = pal_get_key_value(key, state);
      if (ret < 0) {
#ifdef DEBUG
        syslog(LOG_WARNING, "pal_get_last_pwr_state: pal_get_key_value failed for "
            "fru %u", fru);
#endif
      }
      return ret;
    case FRU_SPB:
    case FRU_NIC:
      sprintf(state, "on");
      return 0;
  }
  return -1;
}

// Write GUID into EEPROM
static int
pal_set_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_wr;

  errno = 0;

  // Check for file presence
  if (access(FRU_EEPROM, F_OK) == -1) {
      syslog(LOG_ERR, "pal_set_guid: unable to access the %s file: %s",
          FRU_EEPROM, strerror(errno));
      return errno;
  }

  // Open file
  fd = open(FRU_EEPROM, O_WRONLY);
  if (fd == -1) {
    syslog(LOG_ERR, "pal_set_guid: unable to open the %s file: %s",
        FRU_EEPROM, strerror(errno));
    return errno;
  }

  // Seek the offset
  lseek(fd, offset, SEEK_SET);

  // Write GUID data
  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    syslog(LOG_ERR, "pal_set_guid: write to %s file failed: %s",
        FRU_EEPROM, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

// Read GUID from EEPROM
static int
pal_get_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_rd;

  errno = 0;

  // Check if file is present
  if (access(FRU_EEPROM, F_OK) == -1) {
      syslog(LOG_ERR, "pal_get_guid: unable to access the %s file: %s",
          FRU_EEPROM, strerror(errno));
      return errno;
  }

  // Open the file
  fd = open(FRU_EEPROM, O_RDONLY);
  if (fd == -1) {
    syslog(LOG_ERR, "pal_get_guid: unable to open the %s file: %s",
        FRU_EEPROM, strerror(errno));
    return errno;
  }

  // seek to the offset
  lseek(fd, offset, SEEK_SET);

  // Read bytes from location
  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    syslog(LOG_ERR, "pal_get_guid: read to %s file failed: %s",
        FRU_EEPROM, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

// GUID based on RFC4122 format @ https://tools.ietf.org/html/rfc4122
static void
pal_populate_guid(uint8_t *guid, char *str) {
  unsigned int secs;
  unsigned int usecs;
  struct timeval tv;
  uint8_t count;
  uint8_t lsb, msb;
  int i, r;

  // Populate time
  gettimeofday(&tv, NULL);

  secs = tv.tv_sec;
  usecs = tv.tv_usec;
  guid[0] = usecs & 0xFF;
  guid[1] = (usecs >> 8) & 0xFF;
  guid[2] = (usecs >> 16) & 0xFF;
  guid[3] = (usecs >> 24) & 0xFF;
  guid[4] = secs & 0xFF;
  guid[5] = (secs >> 8) & 0xFF;
  guid[6] = (secs >> 16) & 0xFF;
  guid[7] = (secs >> 24) & 0x0F;

  // Populate version
  guid[7] |= 0x10;

  // Populate clock seq with randmom number
  //getrandom(&guid[8], 2, 0);
  srand(time(NULL));
  //memcpy(&guid[8], rand(), 2);
  r = rand();
  guid[8] = r & 0xFF;
  guid[9] = (r>>8) & 0xFF;

  // Use string to populate 6 bytes unique
  // e.g. LSP62100035 => 'S' 'P' 0x62 0x10 0x00 0x35
  count = 0;
  for (i = strlen(str)-1; i >= 0; i--) {
    if (count == 6) {
      break;
    }

    // If alphabet use the character as is
    if (isalpha(str[i])) {
      guid[15-count] = str[i];
      count++;
      continue;
    }

    // If it is 0-9, use two numbers as BCD
    lsb = str[i] - '0';
    if (i > 0) {
      i--;
      if (isalpha(str[i])) {
        i++;
        msb = 0;
      } else {
        msb = str[i] - '0';
      }
    } else {
      msb = 0;
    }
    guid[15-count] = (msb << 4) | lsb;
    count++;
  }

  // zero the remaining bytes, if any
  if (count != 6) {
    memset(&guid[10], 0, 6-count);
  }

}

int
pal_set_sys_guid(uint8_t slot, char *str) {
  uint8_t guid[GUID_SIZE] = {0x00};

  pal_populate_guid(guid, str);

  return bic_set_sys_guid(slot, guid);
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

  if (!pal_is_slot_server(slot)) {
    return -1;
  }

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

int
pal_get_80port_record(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {

  int ret;
  uint8_t status;

  if (slot < FRU_SLOT1 || slot > FRU_SLOT4) {
    return PAL_ENOTSUP;
  }

  ret = pal_is_fru_prsnt(slot, &status);
  if (ret < 0) {
     return -1;
  }
  if (status == 0) {
    return PAL_ENOTREADY;
  }

  ret = pal_is_server_12v_on(slot, &status);
  if(ret < 0 || 0 == status) {
    return PAL_ENOTREADY;
  }

  if(!pal_is_slot_server(slot)) {
    return PAL_ENOTSUP;
  }

  // Send command to get 80 port record from Bridge IC
  ret = bic_request_post_buffer_data(slot, res_data, res_len);

  return ret;
}

int
pal_is_bmc_por(void) {
  FILE *fp;
  int por = 0;

  fp = fopen("/tmp/ast_por", "r");
  if (fp != NULL) {
    fscanf(fp, "%d", &por);
    fclose(fp);
  }

  return (por)?1:0;
}

int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      if (pal_is_slot_server(fru)) {
#if defined(CONFIG_FBY2_RC)
        int ret = -1;
        uint8_t server_type = 0xFF;
        ret = fby2_get_server_type(fru, &server_type);
        if (ret) {
          syslog(LOG_ERR, "%s, Get server type failed", __func__);
        }
        switch (server_type) {
          case SERVER_TYPE_RC:
            *sensor_list = (uint8_t *) bic_rc_discrete_list;
            *cnt = bic_rc_discrete_cnt;
            break;
          case SERVER_TYPE_TL:
            *sensor_list = (uint8_t *) bic_discrete_list;
            *cnt = bic_discrete_cnt;
            break;
          default:
            syslog(LOG_ERR, "%s, Undefined server type, using Twin Lake discrete sensor list as default", __func__);
            *sensor_list = (uint8_t *) bic_discrete_list;
            *cnt = bic_discrete_cnt;
            break;
        }
#else
        *sensor_list = (uint8_t *) bic_discrete_list;
        *cnt = bic_discrete_cnt;
#endif
      } else {
        *sensor_list = NULL;
        *cnt = 0;
      }
      break;
    case FRU_SPB:
    case FRU_NIC:
      *sensor_list = NULL;
      *cnt = 0;
      break;
    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_get_fru_discrete_list: Wrong fru id %u", fru);
#endif
      return -1;
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

#if defined(CONFIG_FBY2_RC)
void
pal_sensor_discrete_check_rc(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val) {

  char name[32], crisel[128];
  bool valid = false;
  uint8_t diff = o_val ^ n_val;

  if (GETBIT(diff, 1)) {
    switch(snr_num) {
      case BIC_RC_SENSOR_VR_HOT:
        sprintf(name, "510_VR_Hot");
        valid = true;
        break;
    }
    if (valid) {
      _print_sensor_discrete_log(fru, snr_num, snr_name, GETBIT(n_val, 1), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 2)) {
    switch(snr_num) {
      case BIC_RC_SENSOR_SYSTEM_STATUS:
        sprintf(name, "CPU0_Thermal_Trip");
        valid = true;

        sprintf(crisel, "%s - %s,FRU:%u", name, GETBIT(n_val, 2)?"ASSERT":"DEASSERT", fru);
        pal_add_cri_sel(crisel);
        break;
      case BIC_RC_SENSOR_VR_HOT:
        sprintf(name, "423_VR_Hot");
        valid = true;
        break;
    }
    if (valid) {
      _print_sensor_discrete_log(fru, snr_num, snr_name, GETBIT(n_val, 2), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 4)) {
    switch(snr_num) {
      case BIC_RC_SENSOR_SYSTEM_STATUS:
        sprintf(name, "CPU0_FIVR_Fault");
        valid = true;
        break;
      case BIC_RC_SENSOR_PROC_FAIL:
        sprintf(name, "FRB3");
        valid = true;
        break;
    }
    if (valid) {
      _print_sensor_discrete_log(fru, snr_num, snr_name, GETBIT(n_val, 4), name);
      valid = false;
    }
  }
}
#endif

void
pal_sensor_discrete_check_tl(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val) {

  char name[32], crisel[128];
  bool valid = false;
  uint8_t diff = o_val ^ n_val;

  if (GETBIT(diff, 0)) {
    switch(snr_num) {
      case BIC_SENSOR_SYSTEM_STATUS:
        sprintf(name, "SOC_Thermal_Trip");
        valid = true;

        sprintf(crisel, "%s - %s,FRU:%u", name, GETBIT(n_val, 0)?"ASSERT":"DEASSERT", fru);
        pal_add_cri_sel(crisel);
        break;
      case BIC_SENSOR_VR_HOT:
        sprintf(name, "SOC_VR_Hot");
        valid = true;
        break;
    }
    if (valid) {
      _print_sensor_discrete_log(fru, snr_num, snr_name, GETBIT(n_val, 0), name);
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
        sprintf(name, "SOC_DIMM_AB_VR_Hot");
        valid = true;
        break;
      case BIC_SENSOR_CPU_DIMM_HOT:
        sprintf(name, "SOC_MEMHOT");
        valid = true;
        break;
    }
    if (valid) {
      _print_sensor_discrete_log(fru, snr_num, snr_name, GETBIT(n_val, 1), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 2)) {
    switch(snr_num) {
      case BIC_SENSOR_SYSTEM_STATUS:
        sprintf(name, "SYS_Throttle");
        valid = true;
        break;
      case BIC_SENSOR_VR_HOT:
        sprintf(name, "SOC_DIMM_DE_VR_Hot");
        valid = true;
        break;
    }
    if (valid) {
      _print_sensor_discrete_log(fru, snr_num, snr_name, GETBIT(n_val, 2), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 3)) {
    if (snr_num == BIC_SENSOR_SYSTEM_STATUS) {
      _print_sensor_discrete_log(fru, snr_num, snr_name, GETBIT(n_val, 3), "PCH_Thermal_Trip");
    }
  }

  if (GETBIT(diff, 4)) {
    if (snr_num == BIC_SENSOR_PROC_FAIL) {
      _print_sensor_discrete_log(fru, snr_num, snr_name, GETBIT(n_val, 4), "FRB3");
    }
  }
}

int
pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val) {
#if defined(CONFIG_FBY2_RC)
  int ret = -1;
  uint8_t server_type = 0xFF;
  ret = fby2_get_server_type(fru, &server_type);
  if (ret) {
    syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
  }
  switch (server_type) {
    case SERVER_TYPE_RC:
      pal_sensor_discrete_check_rc(fru, snr_num, snr_name, o_val, n_val);
      break;
    case SERVER_TYPE_TL:
      pal_sensor_discrete_check_tl(fru, snr_num, snr_name, o_val, n_val);
      break;
    default:
      syslog(LOG_ERR, "%s, Undefined server type", __func__);
      return -1;
  }
#else
  pal_sensor_discrete_check_tl(fru, snr_num, snr_name, o_val, n_val);
#endif

  return 0;
}

#if defined(CONFIG_FBY2_RC)
int
fby2_rc_event_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  switch(sensor_num) {
    case BIC_RC_SENSOR_RAS_CRIT:
      sprintf(name, "RAS_CRITICAL");
      break;
    case BIC_RC_SENSOR_RAS_INFO:
      sprintf(name, "RAS_INFO");
      break;
    case BIC_RC_SENSOR_RAS_FATAL:
      sprintf(name, "RAS_FATAL");
      break;
    case BIC_RC_SENSOR_THROTTLE_STATUS:
      sprintf(name, "THROTTLE_STATUS");
      break;
    default:
      return -1;
  }
  return 0;
}
#endif

int
pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name) {
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];

  // If SNR_TYPE is OS_BOOT, sensor name is OS
  switch (snr_type) {
    case OS_BOOT:
      // OS_BOOT used by OS
      sprintf(name, "OS");
      return 0;
    default:
      if (fby2_sensor_name(fru, snr_num, name) != 0) {
        break;
      }
      return 0;
  }

#if defined(CONFIG_FBY2_RC)
  {
    uint8_t server_type = 0xFF;
    int ret = -1;
    ret = fby2_get_server_type(fru, &server_type);
    if (ret) {
      syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
    }
    switch(server_type){
      case SERVER_TYPE_RC:
        if(fby2_rc_event_sensor_name(fru, snr_num, name) == 0) {
          return 0;
        }
        break;
      case SERVER_TYPE_TL:
        break;
    }
  }
#endif

  // Otherwise, translate it based on snr_num
  return pal_get_x86_event_sensor_name(fru, snr_num, name);
}

static int
pal_store_crashdump(uint8_t fru, bool ierr) {
  uint8_t status;
  char cmd[256];

  if (!pal_get_server_power(fru, &status) && !status) {
    syslog(LOG_WARNING, "pal_store_crashdump: fru %u is OFF", fru);
    return -1;
  }

  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "sys_runtime=$(awk '{print $1}' /proc/uptime) ; sys_runtime=$(printf \"%%0.f\" $sys_runtime) ; echo $((sys_runtime+1200)) > /tmp/cache_store/fru%d_crashdump", fru);
  system(cmd);

  return fby2_common_crashdump(fru, ierr, false);
}

#if defined(CONFIG_FBY2_RC) || defined(CONFIG_FBY2_EP)
static int
pal_store_cpld_dump(uint8_t fru) {
  return fby2_common_cpld_dump(fru);
}

static int
pal_store_sboot_cpld_dump(uint8_t fru) {
  return fby2_common_sboot_cpld_dump(fru);
}
#endif

int
pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  static int assert_cnt[FBY2_MAX_NUM_SLOTS] = {0};

  /* For every SEL event received from the BIC, set the critical LED on */
  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
    {
#if defined(CONFIG_FBY2_RC) || defined(CONFIG_FBY2_EP)
      int ret;
      uint8_t server_type = 0xFF;
      ret = fby2_get_server_type(fru, &server_type);
      if (ret) {
        syslog(LOG_ERR, "%s, Get server type failed for slot%u", __func__, fru);
      }

      switch (server_type) {
        case SERVER_TYPE_RC:
          switch(snr_num) {
            case BIC_RC_SENSOR_POWER_ERR:
              if (event_data[3] == 0x02) {   // 01h:PWR FAIL, 02h:SLOW BOOT
                pal_store_sboot_cpld_dump(fru);
              } else {
                pal_store_cpld_dump(fru);
              }
              break;

            case 0x00:  // don't care sensor number 00h
              return 0;
          }
          break;
#if defined(CONFIG_FBY2_EP)
        case SERVER_TYPE_EP:
          switch(snr_num) {
            case BIC_EP_SENSOR_POWER_ERR:
              pal_store_cpld_dump(fru);
              break;

            case 0x00:  // don't care sensor number 00h
              return 0;
          }
          break;
#endif
        case SERVER_TYPE_TL:
          switch(snr_num) {
            case CATERR_B:
              pal_store_crashdump(fru, (event_data[3] == 0x00));  // 00h:IERR, 0Bh:MCERR
              break;

            case 0x00:  // don't care sensor number 00h
              return 0;
          }
          break;
        default:
          syslog(LOG_ERR, "%s, Undefined server type for slot%u", __func__, fru);
          return -1;
      }
#else
      switch(snr_num) {
        case CATERR_B:
          pal_store_crashdump(fru, (event_data[3] == 0x00));  // 00h:IERR, 0Bh:MCERR
          break;

        case 0x00:  // don't care sensor number 00h
          return 0;
      }
#endif
      sprintf(key, "slot%d_sel_error", fru);

      fru -= 1;
      if ((event_data[2] & 0x80) == 0) {  // 0: Assertion,  1: Deassertion
         assert_cnt[fru]++;
      } else {
        if (--assert_cnt[fru] < 0)
           assert_cnt[fru] = 0;
      }
      sprintf(cvalue, "%s", (assert_cnt[fru] > 0) ? "0" : "1");
      break;
    }
    case FRU_SPB:
      return 0;

    case FRU_NIC:
      return 0;

    default:
      return -1;
  }

  /* Write the value "0" which means FRU_STATUS_BAD */
  return pal_set_key_value(key, cvalue);
}

#if defined(CONFIG_FBY2_RC)
int
pal_parse_sel_rc(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  uint8_t *ed = &event_data[3];
  uint8_t sen_type = event_data[0];
  char temp_log[512] = {0};
  bool parsed = false;
  char crisel[128];

  switch(snr_num) {
    case PROCHOT_EXT:
      strcpy(error_log, "");
      parsed = true;

      sprintf(crisel, "PROCHOT ASSERT,FRU:%u", fru);
      pal_add_cri_sel(crisel);
      break;
    case BIC_RC_SENSOR_POWER_ERR:
      strcpy(error_log, "");
      if (ed[0] == 0x1) {
        strcat(error_log, "SYS_PWROK failure");
        /* Also try logging to Critial log file, if available */
        sprintf(crisel, "SYS_PWROK failure,FRU:%u", fru);
        pal_add_cri_sel(crisel);
      } else if (ed[0] == 0x2) {
        strcat(error_log, "Slow Boot");
        /* Also try logging to Critial log file, if available */
        sprintf(crisel, "Slow Boot,FRU:%u", fru);
        pal_add_cri_sel(crisel);
      }
      else
        strcat(error_log, "Unknown");

      parsed = true;
      break;
    case BIC_RC_SENSOR_RAS_CRIT:
    case BIC_RC_SENSOR_RAS_INFO:
    case BIC_RC_SENSOR_RAS_FATAL:
      strcpy(error_log, "");
      switch (ed[0] & 0x0F) {
        case 0x01:
          strcat(error_log, "State Asserted");
          break;
        default:
          strcat(error_log, "Unknown");
          break;
      }
      parsed = true;
      break;
    case BIC_RC_SENSOR_PROC_FAIL:
      strcpy(error_log, "");
      if (ed[0] == 0x05) {
        strcat(error_log, "Configuration Error - Secure Boot Failure");
      } else {
        strcat(error_log, "Unknown");
      }
      parsed = true;
      break;
    case BIC_RC_SENSOR_THROTTLE_STATUS:
      strcpy(error_log, "");
      switch (ed[0]) {
        case 0x00:
          strcat(error_log, "INA230_Throttle");
          break;
        case 0x01:
          strcat(error_log, "DDR2_Event_Throttle");
          break;
        case 0x02:
          strcat(error_log, "DDR3_Event_Throttle");
          break;
        case 0x03:
          strcat(error_log, "DDR4_Event_Throttle");
          break;
        case 0x04:
          strcat(error_log, "DDR5_Event_Throttle");
          break;
        case 0x05:
          strcat(error_log, "APC_CBF_Hot_Throttle");
          break;
        case 0x06:
          strcat(error_log, "VR510_Hot_Throttle");
          break;
        case 0x07:
          strcat(error_log, "VR423_Hot_Throttle");
          break;
        case 0x08:
          strcat(error_log, "Fast_Throttle");
          break;
        case 0x09:
          strcat(error_log, "QDF_Throttle");
          break;
        case 0x0A:
          strcat(error_log, "QDF_Light_Throttle");
          break;
        default:
          strcat(error_log, "Unknown");
          break;
      }
      parsed = true;
      break;
  }

  if (parsed == true) {
    if ((event_data[2] & 0x80) == 0) {
      strcat(error_log, " Assertion");
    } else {
      strcat(error_log, " Deassertion");
    }
    return 0;
  }

  pal_parse_sel_helper(fru, sel, error_log);

  return 0;
}
#endif

#if defined(CONFIG_FBY2_EP)
int
pal_parse_sel_ep(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  uint8_t *ed = &event_data[3];
  uint8_t sen_type = event_data[0];
  char temp_log[512] = {0};
  bool parsed = false;

  switch(snr_num) {
    case MEMORY_ECC_ERR:
    case MEMORY_ERR_LOG_DIS:
      strcpy(error_log, "");
      if (snr_num == MEMORY_ECC_ERR) {
        // SEL from MEMORY_ECC_ERR Sensor
        if ((ed[0] & 0x0F) == 0x0) {
          if (sen_type == 0x0C) {
            strcat(error_log, "Correctable");
          } else if (sen_type == 0x10)
            strcat(error_log, "Correctable ECC error Logging Disabled");
        } else if ((ed[0] & 0x0F) == 0x1) {
          strcat(error_log, "Uncorrectable");
        } else if ((ed[0] & 0x0F) == 0x2) {
          strcat(error_log, "Parity Error");
        } else if ((ed[0] & 0x0F) == 0x5)
          strcat(error_log, "Correctable ECC error Logging Limit Reached");
        else
          strcat(error_log, "Unknown");
      } else {
        // SEL from MEMORY_ERR_LOG_DIS Sensor
        if ((ed[0] & 0x0F) == 0x0)
          strcat(error_log, "Correctable Memory Error Logging Disabled");
        else
          strcat(error_log, "Unknown");
      }

      if (((ed[1] & 0xC) >> 2) == 0x0) {  /* All Info Valid */
        /* If critical SEL logging is available, do it */
        if (sen_type == 0x0C) {
          if ((ed[0] & 0x0F) == 0x0) {
            sprintf(temp_log, "DIMM%02X ECC err,FRU:%u", ed[2], fru);
            pal_add_cri_sel(temp_log);
          } else if ((ed[0] & 0x0F) == 0x1) {
            sprintf(temp_log, "DIMM%02X UECC err,FRU:%u", ed[2], fru);
            pal_add_cri_sel(temp_log);
          } else if ((ed[0] & 0x0F) == 0x2) {
            sprintf(temp_log, "DIMM%02X Parity err,FRU:%u", ed[2], fru);
            pal_add_cri_sel(temp_log);
          }
        }
        /* Then continue parse the error into a string. */
        /* All Info Valid                               */
        sprintf(temp_log, " (DIMM %02X) Logical Rank %d", ed[2], ed[1] & 0x03);
      } else if (((ed[1] & 0xC) >> 2) == 0x1) {
        /* DIMM info not valid */
        sprintf(temp_log, " (CPU# %d, CHN# %d)",
            (ed[2] & 0xE0) >> 5, (ed[2] & 0x1C) >> 2);
      } else if (((ed[1] & 0xC) >> 2) == 0x2) {
        /* CHN info not valid */
        sprintf(temp_log, " (CPU# %d, DIMM# %d)",
            (ed[2] & 0xE0) >> 5, ed[2] & 0x3);
      } else if (((ed[1] & 0xC) >> 2) == 0x3) {
        /* CPU info not valid */
        sprintf(temp_log, " (CHN# %d, DIMM# %d)",
            (ed[2] & 0x1C) >> 2, ed[2] & 0x3);
      }
      strcat(error_log, temp_log);
      parsed = true;
      break;
    case BIC_EP_SENSOR_SYSTEM_STATUS:
      strcpy(error_log, "");
      switch (ed[0] & 0x0F) {
        case 0x05:
          strcat(error_log, "HSC_Throttle");
          break;
        case 0x06:
          strcat(error_log, "MB_Throttle");
          break;
        default:
          strcat(error_log, "Unknown");
          break;
      }
      parsed = true;
      break;
    case BIC_EP_SENSOR_VR_HOT:
      strcpy(error_log, "");
      switch (ed[0] & 0x0F) {
        case 0x00:
          strcat(error_log, "SRAM_CORE_VR_HOT");
          break;
        case 0x01:
          strcat(error_log, "MEM_SOC_VR_HOT");
          break;
        default:
          strcat(error_log, "Unknown");
          break;
      }
      parsed = true;
      break;
    case BIC_EP_SENSOR_CPU_DIMM_HOT:
      strcpy(error_log, "");
      if ((ed[0] << 16 | ed[1] << 8 | ed[2]) == 0x00FFFF)
        strcat(error_log, "PROCHOT");
      else
        strcat(error_log, "Unknown");
      sprintf(temp_log, "CPU_DIMM_HOT %s", error_log);
      pal_add_cri_sel(temp_log);
      parsed = true;
      break;
    case NBU_ERROR:
      strcpy(error_log, "");
      if (ed[0] == 0xAB) {
        strcat(error_log, "Uncorrectable");
      } else if (ed[0] == 0xAC) {
        strcat(error_log, "Correctable");
      } else {
        strcat(error_log, "Unknown");
      }
      sprintf(temp_log, " (POST code %02X) ", ed[1]);
      strcat(error_log, temp_log);
      switch (ed[2]) {
        case 0x00:
          strcat(error_log, "NBU Tag Correctable ECC Error");
          break;
        case 0x01:
          strcat(error_log, "NBU Tag Uncorrectable ECC Error");
          break;
        case 0x02:
          strcat(error_log, "NBU BAR Address Error");
          break;
        case 0x03:
          strcat(error_log, "NBU Snoop Filter Correctable ECC Error");
          break;
        case 0x04:
          strcat(error_log, "NBU Snoop Filter Uncorrectable ECC Error");
          break;
        case 0x05:
          strcat(error_log, "NBU Timeout Error");
          break;
        default:
          strcat(error_log, "Unknown");
          break;
      }
      parsed = true;
      break;
  }

  if (parsed == true) {
    if ((event_data[2] & 0x80) == 0) {
      strcat(error_log, " Assertion");
    } else {
      strcat(error_log, " Deassertion");
    }
    return 0;
  }

  pal_parse_sel_helper(fru, sel, error_log);

  return 0;

}
#endif

int
pal_parse_sel_tl(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  uint8_t *ed = &event_data[3];
  uint8_t sen_type = event_data[0];
  char temp_log[512] = {0};
  bool parsed = false;

  switch(snr_num) {
    case MEMORY_ECC_ERR:
    case MEMORY_ERR_LOG_DIS:
      strcpy(error_log, "");
      if (snr_num == MEMORY_ECC_ERR) {
        // SEL from MEMORY_ECC_ERR Sensor
        if ((ed[0] & 0x0F) == 0x0) {
          if (sen_type == 0x0C) {
            strcat(error_log, "Correctable");
          } else if (sen_type == 0x10)
            strcat(error_log, "Correctable ECC error Logging Disabled");
        } else if ((ed[0] & 0x0F) == 0x1) {
          strcat(error_log, "Uncorrectable");
        } else if ((ed[0] & 0x0F) == 0x2) {
          strcat(error_log, "Parity Error");
        } else if ((ed[0] & 0x0F) == 0x5)
          strcat(error_log, "Correctable ECC error Logging Limit Reached");
        else
          strcat(error_log, "Unknown");
      } else {
        // SEL from MEMORY_ERR_LOG_DIS Sensor
        if ((ed[0] & 0x0F) == 0x0)
          strcat(error_log, "Correctable Memory Error Logging Disabled");
        else
          strcat(error_log, "Unknown");
      }

      if (((ed[1] & 0xC) >> 2) == 0x0) {  /* All Info Valid */
        /* If critical SEL logging is available, do it */
        if (sen_type == 0x0C) {
          if ((ed[0] & 0x0F) == 0x0) {
            sprintf(temp_log, "DIMM%02X ECC err,FRU:%u", ed[2], fru);
            pal_add_cri_sel(temp_log);
          } else if ((ed[0] & 0x0F) == 0x1) {
            sprintf(temp_log, "DIMM%02X UECC err,FRU:%u", ed[2], fru);
            pal_add_cri_sel(temp_log);
          } else if ((ed[0] & 0x0F) == 0x2) {
            sprintf(temp_log, "DIMM%02X Parity err,FRU:%u", ed[2], fru);
            pal_add_cri_sel(temp_log);
          }
        }
        /* Then continue parse the error into a string. */
        /* All Info Valid                               */
        sprintf(temp_log, " (DIMM %02X) Logical Rank %d", ed[2], ed[1] & 0x03);
      } else if (((ed[1] & 0xC) >> 2) == 0x1) {
        /* DIMM info not valid */
        sprintf(temp_log, " (CPU# %d, CHN# %d)",
            (ed[2] & 0xE0) >> 5, (ed[2] & 0x1C) >> 2);
      } else if (((ed[1] & 0xC) >> 2) == 0x2) {
        /* CHN info not valid */
        sprintf(temp_log, " (CPU# %d, DIMM# %d)",
            (ed[2] & 0xE0) >> 5, ed[2] & 0x3);
      } else if (((ed[1] & 0xC) >> 2) == 0x3) {
        /* CPU info not valid */
        sprintf(temp_log, " (CHN# %d, DIMM# %d)",
            (ed[2] & 0x1C) >> 2, ed[2] & 0x3);
      }
      strcat(error_log, temp_log);
      parsed = true;
      break;
    case PROCHOT_EXT:
      strcpy(error_log, "");  //Just show event raw data for now
      parsed = true;
      break;
    case BIC_SENSOR_SYSTEM_STATUS:
      strcpy(error_log, "");
      switch (ed[0] & 0x0F) {
        case 0x00:
          strcat(error_log, "SOC_Thermal_Trip");
          break;
        case 0x02:
          strcat(error_log, "SYS_Throttle");
          break;
        case 0x03:
          strcat(error_log, "PCH_Thermal_Trip");
          break;
        case 0x04:
          strcat(error_log, "FM_Throttle");
          break;
        case 0x05:
          strcat(error_log, "HSC_Throttle");
          break;
        case 0x06:
          strcat(error_log, "MB_Throttle");
          break;
        default:
          strcat(error_log, "Unknown");
          break;
      }
      parsed = true;
      break;
    case BIC_SENSOR_VR_HOT:
      strcpy(error_log, "");
      switch (ed[0] & 0x0F) {
        case 0x00:
          strcat(error_log, "SOC_VR_Hot");
          break;
        case 0x01:
          strcat(error_log, "SOC_DIMM_AB_VR_Hot");
          break;
        case 0x02:
          strcat(error_log, "SOC_DIMM_DE_VR_Hot");
          break;
        default:
          strcat(error_log, "Unknown");
          break;
      }
      parsed = true;
      break;
  }

  if (parsed == true) {
    if ((event_data[2] & 0x80) == 0) {
      strcat(error_log, " Assertion");
    } else {
      strcat(error_log, " Deassertion");
    }
    return 0;
  }

  pal_parse_sel_helper(fru, sel, error_log);

  return 0;

}

int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
#if defined(CONFIG_FBY2_RC) || defined(CONFIG_FBY2_EP)
  int ret = -1;
  uint8_t server_type = 0xFF;

  ret = fby2_get_server_type(fru, &server_type);
  if (ret) {
    syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
  }

  if (server_type > SERVER_TYPE_EP) {
    ret = fby2_get_server_type_directly(fru, &server_type);
    if (ret) {
      syslog(LOG_ERR, "%s, Get server type directly failed", __func__);
    }
  }

  switch (server_type) {
#if defined(CONFIG_FBY2_RC)
    case SERVER_TYPE_RC:
      pal_parse_sel_rc(fru, sel, error_log);
      break;
#endif
#if defined(CONFIG_FBY2_EP)
    case SERVER_TYPE_EP:
      pal_parse_sel_ep(fru, sel, error_log);
      break;
#endif
    case SERVER_TYPE_TL:
      pal_parse_sel_tl(fru, sel, error_log);
      break;
    default:
      syslog(LOG_ERR, "%s, Undefined server type", __func__);
      pal_parse_sel_helper(fru, sel, error_log);
      return -1;
  }
#else
  pal_parse_sel_tl(fru, sel, error_log);
#endif

  return 0;
}

int
pal_parse_oem_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  char crisel[128];
  uint8_t mfg_id[] = {0x4c, 0x1c, 0x00};

  error_log[0] = '\0';

  // Record Type: 0xC0 (OEM)
  if ((sel[2] == 0xC0) && !memcmp(&sel[7], mfg_id, sizeof(mfg_id))) {
    snprintf(crisel, sizeof(crisel), "Slot %u PCIe err,FRU:%u", sel[14], fru);
    pal_add_cri_sel(crisel);
  }

  return 0;
}

int
pal_set_sensor_health(uint8_t fru, uint8_t value) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(key, "slot%d_sensor_health", fru);
      break;
    case FRU_SPB:
      sprintf(key, "spb_sensor_health");
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
pal_set_fru_post(uint8_t fru, uint8_t value) {
  if (value == 0) {
    syslog(LOG_WARNING, "FRU: %d, POST END", fru);
    return pal_set_post_start_timestamp(fru,POST_RESET);
  } else {
    syslog(LOG_WARNING, "FRU: %d, POST Start", fru);
    return pal_set_post_start_timestamp(fru,POST_SET);
  }
}

int
pal_get_fru_post(uint8_t fru, uint8_t *value) {

  long post_start_timestamp,post_end_timestamp;
  int ret;

  ret = pal_get_post_start_timestamp(fru, &post_start_timestamp);
  if (ret != 0) {
    //default value
    post_start_timestamp = -1;
  }

  if (post_start_timestamp == -1) {
    *value = 0;
    return 0;
  }

  ret = pal_get_post_end_timestamp(fru, &post_end_timestamp);
  if (ret != 0) {
    //default value
    post_end_timestamp = -1;
  }

  if (post_start_timestamp > post_end_timestamp) {
    //post is onging
    *value = 1;
  } else {
    *value = 0;
  }
  return 0;
}

int
pal_set_post_start_timestamp(uint8_t fru, uint8_t method) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  struct timespec ts;
  long value = -1;

  if (method == POST_SET) {
    clock_gettime(CLOCK_MONOTONIC,&ts);
    value = ts.tv_sec;
  } else if (method == POST_RESET) {
    value = -1;
  } else {
    return -1;
  }

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(key, "slot%d_post_start_timestamp", fru);
      break;

    default:
      return -1;
  }

  sprintf(cvalue,"%ld",value);

  return kv_set(key,cvalue,0,0);
}

int
pal_get_post_start_timestamp(uint8_t fru, long *value) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  int ret;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(key, "slot%d_post_start_timestamp", fru);
      break;

    default:
      return -1;
  }

  ret = kv_get(key, cvalue, NULL, 0);
  if (ret) {
    return ret;
  }
  *value = strtol(cvalue,NULL,10);
  return 0;
}

int
pal_set_post_end_timestamp(uint8_t fru) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  long value = ts.tv_sec;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(key, "slot%d_post_end_timestamp", fru);
      break;

    default:
      return -1;
  }

  sprintf(cvalue,"%ld",value);

  return kv_set(key,cvalue,0,0);
}

int
pal_get_post_end_timestamp(uint8_t fru, long *value) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  int ret;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(key, "slot%d_post_end_timestamp", fru);
      break;

    default:
      return -1;
  }

  ret = kv_get(key, cvalue,NULL,0);
  if (ret) {
    return ret;
  }
  *value = strtol(cvalue,NULL,10);
  return 0;
}

uint8_t
pal_is_post_time_out() {
  long post_start_timestamp,post_end_timestamp,current_timestamp;
  struct timespec ts;
  int ret;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  current_timestamp = ts.tv_sec;

  for (int fru=1;fru <= 4;fru++) {
    ret = pal_get_post_start_timestamp(fru, &post_start_timestamp);
    if (ret != 0) {
      //default value
      post_start_timestamp = -1;
    }
    ret = pal_get_post_end_timestamp(fru, &post_end_timestamp);
    if (ret != 0) {
      //default value
      post_end_timestamp = -1;
    }
    if (post_start_timestamp != -1) {
      if (post_start_timestamp > post_end_timestamp) {
        if (current_timestamp-post_start_timestamp > POST_TIMEOUT) {
          return fru;
        }
      }
    }
  }
  return 0;
}

uint8_t
pal_is_post_ongoing() {
  uint8_t is_post_ongoing = 0;
  uint8_t value = 0;
  for (int fru=1;fru <= 4;fru++) {
    //get bios post status from kv_store
    pal_get_fru_post(fru, &value);
    is_post_ongoing |= value;
  }
  return is_post_ongoing;
}

int
pal_set_ignore_thresh(int value) {
  char* key = "ignore_thresh";
  char cvalue[MAX_VALUE_LEN] = {0};

  sprintf(cvalue, (value > 0) ? "1" : "0");

  return kv_set(key,cvalue, 0, 0);
}

int
pal_get_ignore_thresh(int *value) {
  char* key = "ignore_thresh";
  char cvalue[MAX_VALUE_LEN] = {0};
  int ret;

  ret = kv_get(key, cvalue, NULL, 0);
  if (ret) {
    return ret;
  }
  *value = atoi(cvalue);
  return 0;
}

long
pal_get_fscd_counter() {
  char* key = "fscd_counter";
  char cvalue[MAX_VALUE_LEN] = {0};
  int ret;

  ret = kv_get(key, cvalue,NULL,0);
  if (ret) {
    // fscd_counter not set yet
    // start at 0
    return 0;
  }

  return strtol(cvalue, NULL, 10);
}

int
pal_set_fscd_counter(long value) {
  char* key = "fscd_counter";
  char cvalue[MAX_VALUE_LEN] = {0};

  sprintf(cvalue,"%ld",value);

  return kv_set(key,cvalue,0,0);
}

int
pal_get_fru_health(uint8_t fru, uint8_t *value) {

  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(key, "slot%d_sensor_health", fru);
      break;
    case FRU_SPB:
      sprintf(key, "spb_sensor_health");
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
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(key, "slot%d_sel_error", fru);
      break;
    case FRU_SPB:
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

void
pal_inform_bic_mode(uint8_t fru, uint8_t mode) {
  switch(mode) {
  case BIC_MODE_NORMAL:
    // Bridge IC entered normal mode
    // Inform BIOS that BMC is ready
    bic_set_gpio(fru, BMC_READY_N, 0);
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

  switch(num) {

    case FAN_0:
      sprintf(name, "Fan 0");
      break;

    case FAN_1:
      sprintf(name, "Fan 1");
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

  // Convert the percentage to our 1/96th unit.
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

   // Read sensor cache
   ret = sensor_cache_read(FRU_SPB, SP_SENSOR_FAN0_TACH + fan, &value);
   if (ret != 0) // Read sensor value
      ret = sensor_raw_read(FRU_SPB, SP_SENSOR_FAN0_TACH + fan, &value);

   if (0 == ret)
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
  int ret;
  uint8_t rbuf[256] = {0x00}, len = 0;

  ret = bic_me_xmit(fru, request, req_len, rbuf, &len);
  if (ret || (len < 1)) {
    return -1;
  }

  if (rbuf[0] != 0x00) {
    return -1;
  }

  *rlen = len - 1;
  memcpy(response, &rbuf[1], *rlen);

  return 0;
}

void
pal_log_clear(char *fru) {
  char key[MAX_KEY_LEN] = {0};
  int i;
  if (!strcmp(fru, "slot1")) {
    pal_set_key_value("slot1_sensor_health", "1");
    pal_set_key_value("slot1_sel_error", "1");
  } else if (!strcmp(fru, "slot2")) {
    pal_set_key_value("slot2_sensor_health", "1");
    pal_set_key_value("slot2_sel_error", "1");
  } else if (!strcmp(fru, "slot3")) {
    pal_set_key_value("slot3_sensor_health", "1");
    pal_set_key_value("slot3_sel_error", "1");
  } else if (!strcmp(fru, "slot4")) {
    pal_set_key_value("slot4_sensor_health", "1");
    pal_set_key_value("slot4_sel_error", "1");
  } else if (!strcmp(fru, "spb")) {
    pal_set_key_value("spb_sensor_health", "1");
  } else if (!strcmp(fru, "nic")) {
    pal_set_key_value("nic_sensor_health", "1");
  } else if (!strcmp(fru, "all")) {
    for (i = 1; i <= 4; i++) {
      sprintf(key, "slot%d_sensor_health", i);
      pal_set_key_value(key, "1");
      sprintf(key, "slot%d_sel_error", i);
      pal_set_key_value(key, "1");
    }
    pal_set_key_value("spb_sensor_health", "1");
    pal_set_key_value("nic_sensor_health", "1");
  }
}
int
pal_get_pwm_value(uint8_t fan_num, uint8_t *value) {
  char path[LARGEST_DEVICE_NAME] = {0};
  char device_name[LARGEST_DEVICE_NAME] = {0};
  int val = 0;
  int pwm_enable = 0;

  if(fan_num < 0 || fan_num >= pal_pwm_cnt) {
    syslog(LOG_INFO, "pal_get_pwm_value: fan number is invalid - %d", fan_num);
    return -1;
  }

// Need check pwmX_en to determine the PWM is 0 or 100.
 snprintf(device_name, LARGEST_DEVICE_NAME, "pwm%d_en", fan_num);
 snprintf(path, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR, device_name);
 if (read_device(path, &pwm_enable)) {
    syslog(LOG_INFO, "pal_get_pwm_value: read %s failed", path);
    return -1;
  }

  if(pwm_enable) {
    snprintf(device_name, LARGEST_DEVICE_NAME, "pwm%d_falling", fan_num);
    snprintf(path, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR, device_name);
    if (read_device_hex(path, &val)) {
      syslog(LOG_INFO, "pal_get_pwm_value: read %s failed", path);
      return -1;
    }

    if(val == 0)
      *value = 100;
    else
      *value = (100 * val + (PWM_UNIT_MAX-1)) / PWM_UNIT_MAX;
    } else {
    *value = 0;
    }

    return 0;
}

static uint8_t fan_dead_actived_flag = 0; // bit0 : fan 0, bit1 : fan 1
static uint8_t both_fan_dead_control = 0;
int
pal_fan_dead_handle(int fan_num) {
  int ret;
  uint8_t slot, status = 0xFF, slot_type = 0xFF;

  if (1 == both_fan_dead_control)
    return 0;

  fan_dead_actived_flag = SETBIT(fan_dead_actived_flag, fan_num);
  if (GETBIT(fan_dead_actived_flag, 0) && GETBIT(fan_dead_actived_flag, 1))
    both_fan_dead_control = 1;

  if (1 == both_fan_dead_control) {
    for (slot = 1; slot <= 4; slot++) {
      slot_type = fby2_get_slot_type(slot);
      if (SLOT_TYPE_SERVER == slot_type) {
        pal_get_server_power(slot, &status);
        if (SERVER_12V_OFF != status) {
          // power off server 12V HSC
          ret = server_12v_off(slot);
          if (ret) {
            syslog(LOG_ERR, "server_12v_off() failed, slot%d", slot);
          }
        }
      }
    }

    syslog(LOG_CRIT, "FRU: %u, 12V-off all slots due to both fan fail", FRU_SPB);
  }

  return 0;
}

int
pal_fan_recovered_handle(int fan_num) {

  fan_dead_actived_flag = CLEARBIT(fan_dead_actived_flag, fan_num);
  both_fan_dead_control = 0;

  return 0;
}

int
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
	int BOARD_ID, BOARD_REV_ID0, BOARD_REV_ID1, BOARD_REV_ID2, SLOT_TYPE;
	char path[64] = {0};
	unsigned char *data = res_data;
	int completion_code = CC_UNSPECIFIED_ERROR;

	sprintf(path, GPIO_VAL, GPIO_BOARD_ID);
	if (read_device(path, &BOARD_ID)) {
		*res_len = 0;
		return completion_code;
	}

	sprintf(path, GPIO_VAL, GPIO_BOARD_REV_ID0);
	if (read_device(path, &BOARD_REV_ID0)) {
		*res_len = 0;
		return completion_code;
	}

	sprintf(path, GPIO_VAL, GPIO_BOARD_REV_ID1);
	if (read_device(path, &BOARD_REV_ID1)) {
		*res_len = 0;
		return completion_code;
	}

	sprintf(path, GPIO_VAL, GPIO_BOARD_REV_ID2);
	if (read_device(path, &BOARD_REV_ID2)) {
		*res_len = 0;
		return completion_code;
	}


	switch(fby2_get_slot_type(slot))
	{
		case SLOT_TYPE_SERVER:
			SLOT_TYPE = 0x00;
			break;
		case SLOT_TYPE_CF:
			SLOT_TYPE = 0x02;
			break;
		case SLOT_TYPE_GP:
			SLOT_TYPE = 0x01;
			break;
		case SLOT_TYPE_GPV2:
			SLOT_TYPE = 0x04;
			break;
		default :
			*res_len = 0;
			return completion_code;
	}

	*data++ = BOARD_ID;
	*data++ = (BOARD_REV_ID2 << 2) | (BOARD_REV_ID1 << 1) | BOARD_REV_ID0;
	*data++ = slot;
	*data++ = SLOT_TYPE;
	*res_len = data - res_data;
	completion_code = CC_SUCCESS;

	return completion_code;
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

  sprintf(key, "slot%d_boot_order", slot);
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
  int i, j, network_dev = 0;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};
  enum {
    BOOT_DEVICE_IPV4 = 0x1,
    BOOT_DEVICE_IPV6 = 0x9,
  };

  *res_len = 0;

  for (i = 0; i < SIZE_BOOT_ORDER; i++) {
    if ((i > 0) && (boot[i] != 0xFF)) {  // byte[0] is boot mode, byte[1:5] are boot order
      for (j = i+1; j < SIZE_BOOT_ORDER; j++) {
        if (boot[i] == boot[j])
          return CC_INVALID_PARAM;
      }

      // If bit[2:0] is 001b (Network), bit[3] is IPv4/IPv6 order
      // bit[3]=0b: IPv4 first
      // bit[3]=1b: IPv6 first
      if ((boot[i] == BOOT_DEVICE_IPV4) || (boot[i] == BOOT_DEVICE_IPV6))
        network_dev++;
    }

    snprintf(tstr, 3, "%02x", boot[i]);
    strncat(str, tstr, 3);
  }

  // not allow having more than 1 network boot device in the boot order
  if (network_dev > 1)
    return CC_INVALID_PARAM;

  sprintf(key, "slot%d_boot_order", slot);
  return pal_set_key_value(key, str);
}

int
pal_set_dev_guid(uint8_t slot, char *guid) {
      pal_populate_guid(g_dev_guid, guid);

      return pal_set_guid(OFFSET_DEV_GUID, (char *)g_dev_guid);
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {
      pal_get_guid(OFFSET_DEV_GUID, (char *)g_dev_guid);
      memcpy(guid, g_dev_guid, GUID_SIZE);

      return 0;
}

void
pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {

  char key[MAX_KEY_LEN] = {0};
  char buff[MAX_VALUE_LEN] = {0};
  int policy = 3;
  uint8_t status, ret;
  uint8_t pos = HAND_SW_SERVER1;
  unsigned char *data = res_data;

  if (slot == FRU_SPB) {
    slot = (pal_get_hand_sw(&pos) == 0) ? pos : HAND_SW_BMC;
  }

  if (slot != HAND_SW_BMC) {
    // Platform Power Policy
    sprintf(key, "slot%d_por_cfg", slot);
    if (pal_get_key_value(key, buff) == 0) {
      if (!memcmp(buff, "off", strlen("off")))
        policy = 0;
      else if (!memcmp(buff, "lps", strlen("lps")))
        policy = 1;
      else if (!memcmp(buff, "on", strlen("on")))
        policy = 2;
      else
        policy = 3;
    }

    // Current Power State
    ret = pal_get_server_power(slot, &status);
    if (ret >= 0) {
      *data++ = status | (policy << 5);
    } else {
      // load default
      syslog(LOG_WARNING, "ipmid: pal_get_server_power failed for slot1\n");
      *data++ = 0x00 | (policy << 5);
    }
  } else {
    *data++ = 0x00 | (policy << 5);
  }

  *data++ = 0x00;   // Last Power Event
  *data++ = 0x40;   // Misc. Chassis Status
  *data++ = 0x00;   // Front Panel Button Disable
  *res_len = data - res_data;
}

uint8_t
pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data) {

	uint8_t completion_code;
	char key[MAX_KEY_LEN] = {0};
	sprintf(key, "slot%d_por_cfg", slot);
	completion_code = CC_SUCCESS;   // Fill response with default values
	unsigned char policy = *pwr_policy & 0x07;  // Power restore policy

	switch (policy)
	{
	  case 0:
	    if (pal_set_key_value(key, "off") != 0)
	      completion_code = CC_UNSPECIFIED_ERROR;
	    break;
	  case 1:
	    if (pal_set_key_value(key, "lps") != 0)
	      completion_code = CC_UNSPECIFIED_ERROR;
	    break;
	  case 2:
	    if (pal_set_key_value(key, "on") != 0)
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
pal_set_ppin_info(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
	char key[MAX_KEY_LEN] = {0};
	char str[MAX_VALUE_LEN] = {0};
	char tstr[10] = {0};
	int i;
	int completion_code = CC_UNSPECIFIED_ERROR;
	*res_len = 0;
#if defined(CONFIG_FBY2_EP)
	int ret;
	uint8_t server_type = 0xFF;
	ret = fby2_get_server_type(slot, &server_type);
	if (ret) {
		syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
	}

	if (server_type == SERVER_TYPE_EP) {
		return CC_INVALID_CMD;
	}
#endif
	sprintf(key, "slot%d_cpu_ppin", slot);

	for (i = 0; i < SIZE_CPU_PPIN; i++) {
		sprintf(tstr, "%02x", req_data[i]);
		strcat(str, tstr);
	}

	if (pal_set_key_value(key, str) != 0)
		return completion_code;

	completion_code = CC_SUCCESS;

	return completion_code;
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

//For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7e
int pal_get_plat_sku_id(void){
  return 0x01; // Yosemite V2
}

//Do slot power control
static void * slot_pwr_ctrl(void *ptr) {
  uint8_t slot = (int)ptr & 0xFF;
  uint8_t cmd = ((int)ptr >> 8) & 0xFF;
  char pwr_state[MAX_VALUE_LEN] = {0};

  pthread_detach(pthread_self());
  msleep(500);

  if (cmd == SERVER_12V_CYCLE) {
    pal_get_last_pwr_state(slot, pwr_state);
  }

  if (!pal_set_server_power(slot, cmd)) {
    switch (cmd) {
      case SERVER_POWER_OFF:
        syslog(LOG_CRIT, "SERVER_POWER_OFF successful for FRU: %d", slot);
        break;
      case SERVER_POWER_ON:
        syslog(LOG_CRIT, "SERVER_POWER_ON successful for FRU: %d", slot);
        break;
      case SERVER_POWER_CYCLE:
        syslog(LOG_CRIT, "SERVER_POWER_CYCLE successful for FRU: %d", slot);
        break;
      case SERVER_POWER_RESET:
        syslog(LOG_CRIT, "SERVER_POWER_RESET successful for FRU: %d", slot);
        break;
      case SERVER_GRACEFUL_SHUTDOWN:
        syslog(LOG_CRIT, "SERVER_GRACEFUL_SHUTDOWN successful for FRU: %d", slot);
        break;
      case SERVER_12V_CYCLE:
        syslog(LOG_CRIT, "SERVER_12V_CYCLE successful for FRU: %d", slot);
        pal_power_policy_control(slot, pwr_state);
        break;
    }
  }

  m_slot_pwr_ctrl[slot] = false;
  pthread_exit(0);
}

int pal_chassis_control(uint8_t slot, uint8_t *req_data, uint8_t req_len)
{
  uint8_t comp_code = CC_SUCCESS, cmd = 0;
  int ret, cmd_slot;
  pthread_t tid;

  if ((slot < 1) || (slot > 4)) {
    return CC_UNSPECIFIED_ERROR;
  }

  if (req_len != 1) {
    return CC_INVALID_LENGTH;
  }

  if (m_slot_pwr_ctrl[slot] != false) {
    return CC_NOT_SUPP_IN_CURR_STATE;
  }

  switch (req_data[0]) {
    case 0x00:  // power off
      cmd = SERVER_POWER_OFF;
      break;
    case 0x01:  // power on
      cmd = SERVER_POWER_ON;
      break;
    case 0x02:  // power cycle
      cmd = SERVER_POWER_CYCLE;
      break;
    case 0x03:  // power reset
      cmd = SERVER_POWER_RESET;
      break;
    case 0x05:  // graceful-shutdown
      cmd = SERVER_GRACEFUL_SHUTDOWN;
      break;
    default:
      comp_code = CC_INVALID_DATA_FIELD;
      break;
  }

  if (comp_code == CC_SUCCESS) {
    m_slot_pwr_ctrl[slot] = true;
    cmd_slot = (cmd << 8) | slot;
    ret = pthread_create(&tid, NULL, slot_pwr_ctrl, (void *)cmd_slot);
    if (ret < 0) {
      syslog(LOG_WARNING, "[%s] Create slot_pwr_ctrl thread failed!", __func__);
      m_slot_pwr_ctrl[slot] = false;
      return CC_NODE_BUSY;
    }
  }

  return comp_code;
}

static int
pal_slot_ac_cycle(uint8_t slot) {
  int ret, cmd_slot;
  pthread_t tid;

  if (m_slot_pwr_ctrl[slot] != false) {
    return CC_NOT_SUPP_IN_CURR_STATE;
  }

  m_slot_pwr_ctrl[slot] = true;
  cmd_slot = (SERVER_12V_CYCLE << 8) | slot;
  ret = pthread_create(&tid, NULL, slot_pwr_ctrl, (void *)cmd_slot);
  if (ret < 0) {
    syslog(LOG_WARNING, "[%s] Create slot_ac_cycle thread failed!", __func__);
    m_slot_pwr_ctrl[slot] = false;
    return CC_NODE_BUSY;
  }

  return CC_SUCCESS;
}

//Do sled ac cycle
static void * sled_ac_cycle_handler(void *arg) {
  pthread_detach(pthread_self());
  msleep(500);

  syslog(LOG_CRIT, "SLED_CYCLE starting...");
  pal_update_ts_sled();
  sync();
  sleep(1);
  pal_sled_cycle();

  m_sled_ac_start = false;
  pthread_exit(0);
}

static int
sled_ac_cycle(void) {
  int ret;
  pthread_t tid;

  if (m_sled_ac_start != false) {
    return CC_NOT_SUPP_IN_CURR_STATE;
  }

  m_sled_ac_start = true;
  ret = pthread_create(&tid, NULL, sled_ac_cycle_handler, NULL);
  if (ret < 0) {
    syslog(LOG_WARNING, "[%s] Create sled_ac_cycle_handler thread failed!", __func__);
    m_sled_ac_start = false;
    return CC_NODE_BUSY;
  }

  return CC_SUCCESS;
}

int pal_sled_ac_cycle(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  uint8_t comp_code = CC_UNSPECIFIED_ERROR;
  uint8_t *data = req_data;

  if ((slot < 1) || (slot > 4)) {
    return comp_code;
  }

  if ((*data != 0x55) || (*(data+1) != 0x66)) {
    return comp_code;
  }

  switch (*(data+2)) {
    case 0x0f:  //do slot ac cycle
      comp_code = pal_slot_ac_cycle(slot);
      break;
    case 0xac:  //do sled ac cycle
      comp_code = sled_ac_cycle();
      break;
    default:
      comp_code = CC_INVALID_DATA_FIELD;
      break;
  }

  return comp_code;
}

//Use part of the function for OEM Command "CMD_OEM_GET_POSS_PCIE_CONFIG" 0xF4
int pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {

  uint8_t completion_code = CC_SUCCESS;
  uint8_t pcie_conf = 0x00;
  uint8_t *data = res_data;
  int pair_set_type;

  pair_set_type = pal_get_pair_slot_type(slot);
  switch (pair_set_type) {
    case TYPE_CF_A_SV:
      pcie_conf = 0x0f;
      break;
    case TYPE_GP_A_SV:
      pcie_conf = 0x01;
      break;
    case TYPE_GPV2_A_SV:
      pcie_conf = 0x010;
      break;
    default:
      pcie_conf = 0x00;
      break;
  }

  *data++ = pcie_conf;
  *res_len = data - res_data;
  return completion_code;
}

int pal_set_imc_version(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {

  uint8_t completion_code = CC_SUCCESS;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};
  int i;
  *res_len = 0;
  sprintf(key, "slot%d_imc_ver", (int)slot);
  for (i = 0; i < IMC_VER_SIZE; i++) {
    sprintf(tstr, "%x", req_data[i]);
    strcat(str, tstr);
  }

  if(kv_set(key, str,0,KV_FPERSIST))
    completion_code = CC_INVALID_PARAM;

  return completion_code;
}

int pal_set_slot_led(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {

   uint8_t completion_code = CC_UNSPECIFIED_ERROR;
   uint8_t *data = req_data;
   int slot_id = slot;
   char tstr[64] = {0};
   int ret=-1;

   *res_len = 0;
   sprintf(tstr, "identify_slot%d", slot_id);

   /* There are 2 option bytes for Chassis Identify Command
    * Byte 1 : Identify Interval in seconds. (Not support, OpenBMC only support turn off action)
    *          00h = Turn off Identify
    * Byte 2 : Force Identify On
    *          BIT0 : 1b = Turn on Identify indefinitely. This overrides the values in byte 1.
    *                 0b = Identify state driven according to byte 1.
    */
   if (req_len <= 5) {
     if (5 == req_len) {
       if (GETBIT(*(data+1), 0)) {
         ret = pal_set_key_value(tstr, "on");
         if (ret < 0) {
           syslog(LOG_ERR, "pal_set_key_value: set %s on failed",tstr);
           return completion_code;
         }
       } else if (0 == *data) {
         ret = pal_set_key_value(tstr, "off");
         if (ret < 0) {
           syslog(LOG_ERR, "pal_set_key_value: set %s off failed",tstr);
           return completion_code;
         }
       } else {
         completion_code = CC_INVALID_PARAM;
         return completion_code;
       }
     } else if (4 == req_len) {
       if (0 == *data) {
         ret = pal_set_key_value(tstr, "off");
         if (ret < 0) {
           syslog(LOG_ERR, "pal_set_key_value: set %s off failed",tstr);
           return completion_code;
         }
       } else {
         completion_code = CC_INVALID_PARAM;
         return completion_code;
       }
     } else {
       completion_code = CC_UNSPECIFIED_ERROR;
       return completion_code;
     }
   } else {
     completion_code = CC_PARAM_OUT_OF_RANGE;
     return completion_code;
   }

   completion_code = CC_SUCCESS;
   return completion_code;
}

int
pal_get_platform_id(uint8_t *id) {
   return 0;
}

int
pal_nic_otp_enable (float val) {
  uint8_t slot, status = 0xFF, slot_type=0xFF;
  int retry = MAX_NIC_TEMP_RETRY;  // no retry here, since check_thresh_assert has retried
  int ret;
  float thresh_val = nic_sensor_threshold[MEZZ_SENSOR_TEMP][UNR_THRESH];
  sensor_check_t *snr_chk;

  if ((snr_chk = get_sensor_check(FRU_NIC, MEZZ_SENSOR_TEMP)) != NULL) {
    thresh_val = snr_chk->unr;
  }

  while (retry < MAX_NIC_TEMP_RETRY) {
    ret = pal_sensor_read_raw(FRU_NIC, MEZZ_SENSOR_TEMP, &val);
    if (!ret && (val < thresh_val)) {
      break;
    }
    retry++;
    msleep(200);
  }
  if (retry < MAX_NIC_TEMP_RETRY)
    return 0;

  for (slot = 1; slot <= 4; slot++) {
    slot_type = fby2_get_slot_type(slot);
    if (SLOT_TYPE_SERVER == slot_type) {
      pal_get_server_power(slot, &status);
      if (SERVER_12V_OFF != status) {
        // power off server 12V HSC
        syslog(LOG_CRIT, "FRU: %u, Power Off Server 12V due to NIC temp UNR reached. (val = %.2f)", slot, val);
        ret = server_12v_off(slot);
        if (ret) {
          syslog(LOG_ERR, "server_12v_off() failed, slot%d", slot);
        } else {
          otp_server_12v_off_flag[slot] = 1;
        }
      }
    }
  }

  return 0;
}

int
pal_nic_otp_disable (float val) {
  int ret;
  uint8_t slot, status = 0xFF, slot_type = 0xFF;
  char pwr_state[MAX_VALUE_LEN] = {0};

  for (slot = 1; slot <= 4; slot++) {
    // Check if it is a server
    slot_type = fby2_get_slot_type(slot);
    if (SLOT_TYPE_SERVER == slot_type) {
      pal_get_server_power(slot, &status);
      if ((SERVER_12V_ON != status) && (1 == otp_server_12v_off_flag[slot])) {
        // power on server 12V HSC
        syslog(LOG_CRIT, "FRU: %u, Power On Server 12V due to NIC temp UNC deassert. (val = %.2f)", slot, val);
        memset(pwr_state, 0, MAX_VALUE_LEN);
        pal_get_last_pwr_state(slot, pwr_state);
        ret = server_12v_on(slot);
        if (ret) {
          syslog(LOG_ERR, "server_12v_on() failed, slot%d", slot);
        } else {
          // Set power policy based on last power state
          pal_power_policy_control(slot, pwr_state);
          otp_server_12v_off_flag[slot] = 0;
        }
      }
    }
  }
  return 0;
}

void
pal_sensor_assert_handle_tl(uint8_t fru, uint8_t snr_num, float val, char* thresh_name) {
  char crisel[128];
  sensor_desc_t *snr_desc;

  switch (snr_num) {
    case BIC_SENSOR_SOC_TEMP:
      sprintf(crisel, "SOC Temp %s %.0fC - ASSERT,FRU:%u", thresh_name, val, fru);
      break;
    case BIC_SENSOR_P3V3_MB:
    case BIC_SENSOR_P12V_MB:
    case BIC_SENSOR_P1V05_PCH:
    case BIC_SENSOR_P3V3_STBY_MB:
    case BIC_SENSOR_PV_BAT:
    case BIC_SENSOR_PVDDR_AB:
    case BIC_SENSOR_PVDDR_DE:
    case BIC_SENSOR_PVNN_PCH:
    case BIC_SENSOR_VCCIN_VR_VOL:
    case BIC_SENSOR_VCCIO_VR_VOL:
    case BIC_SENSOR_1V05_PCH_VR_VOL:
    case BIC_SENSOR_VDDR_AB_VR_VOL:
    case BIC_SENSOR_VDDR_DE_VR_VOL:
    case BIC_SENSOR_VCCSA_VR_VOL:
    case BIC_SENSOR_INA230_VOL:
      snr_desc = get_sensor_desc(fru, snr_num);
      sprintf(crisel, "%s %s %.2fV - ASSERT,FRU:%u", snr_desc->name, thresh_name, val, fru);
      break;
    default:
      return;
  }

  pal_add_cri_sel(crisel);
  return;
}

void
pal_sensor_assert_handle_rc(uint8_t fru, uint8_t snr_num, float val, char* thresh_name) {
  char crisel[128];
  sensor_desc_t *snr_desc;

  switch (snr_num) {
    case BIC_RC_SENSOR_SOC_TEMP_DIODE:
      sprintf(crisel, "SOC Temp DIODE %s %.0fC - ASSERT,FRU:%u", thresh_name, val, fru);
      break;
    case BIC_RC_SENSOR_SOC_TEMP_IMC:
      sprintf(crisel, "SOC Temp IMC %s %.0fC - ASSERT,FRU:%u", thresh_name, val, fru);
      break;
    case BIC_RC_SENSOR_P12V_MB:
    case BIC_RC_SENSOR_P3V3_STBY_MB:
    case BIC_RC_SENSOR_P3V2_MB:
    case BIC_RC_SENSOR_PV_BAT:
    case BIC_RC_SENSOR_PVDDQ_423:
    case BIC_RC_SENSOR_PVDDQ_510:
    case BIC_RC_SENSOR_PVDDQ_423_VR_VOL:
    case BIC_RC_SENSOR_PVDDQ_510_VR_VOL:
    case BIC_RC_SENSOR_CVR_APC_VOL:
    case BIC_RC_SENSOR_CVR_CBF_VOL:
    case BIC_RC_SENSOR_INA230_VOL:
      snr_desc = get_sensor_desc(fru, snr_num);
      sprintf(crisel, "%s %s %.2fV - ASSERT,FRU:%u", snr_desc->name, thresh_name, val, fru);
      break;
    default:
      return;
  }

  pal_add_cri_sel(crisel);
  return;
}

void
pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  char crisel[128];
  char thresh_name[8];
  sensor_desc_t *snr_desc;
  int slot_type = SLOT_TYPE_NULL;

  switch (thresh) {
    case UNR_THRESH:
      sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
      sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
      sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
      sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
      sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
      sprintf(thresh_name, "LNCR");
      break;
    default:
      syslog(LOG_WARNING, "pal_sensor_assert_handle: wrong thresh enum value");
      return;
  }

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      slot_type = fby2_get_slot_type(fru);
      if (slot_type == SLOT_TYPE_SERVER) {
#if defined(CONFIG_FBY2_RC)
        int ret;
        uint8_t server_type = 0xFF;
        ret = fby2_get_server_type(fru, &server_type);
        if (ret) {
          return;
        }
        switch (server_type) {
          case SERVER_TYPE_RC:
            pal_sensor_assert_handle_rc(fru, snr_num, val, thresh_name);
            break;
          case SERVER_TYPE_TL:
            pal_sensor_assert_handle_tl(fru, snr_num, val, thresh_name);
            break;
        }
        return;
#else
        switch (snr_num) {
          case BIC_SENSOR_P3V3_MB:
          case BIC_SENSOR_P12V_MB:
          case BIC_SENSOR_P1V05_PCH:
          case BIC_SENSOR_P3V3_STBY_MB:
          case BIC_SENSOR_PV_BAT:
          case BIC_SENSOR_PVDDR_AB:
          case BIC_SENSOR_PVDDR_DE:
          case BIC_SENSOR_PVNN_PCH:
          case BIC_SENSOR_VCCIN_VR_VOL:
          case BIC_SENSOR_VCCIO_VR_VOL:
          case BIC_SENSOR_1V05_PCH_VR_VOL:
          case BIC_SENSOR_VDDR_AB_VR_VOL:
          case BIC_SENSOR_VDDR_DE_VR_VOL:
          case BIC_SENSOR_VCCSA_VR_VOL:
          case BIC_SENSOR_INA230_VOL:
            snr_desc = get_sensor_desc(fru, snr_num);
            sprintf(crisel, "%s %s %.2fV - ASSERT,FRU:%u", snr_desc->name, thresh_name, val, fru);
            break;
          default:
            return;
        }
#endif
      } else if (slot_type == SLOT_TYPE_GPV2) {
#if defined(CONFIG_FBY2_GPV2)
        switch (snr_num) {
          case GPV2_SENSOR_P12V_BIC_SCALED:
          case GPV2_SENSOR_P3V3_STBY_BIC_SCALED:
          case GPV2_SENSOR_P0V92_BIC_SCALED:
          case GPV2_SENSOR_P1V8_BIC_SCALED:
          case GPV2_SENSOR_INA230_VOLT:
          // VR
          case GPV2_SENSOR_3V3_VR_Vol:
          case GPV2_SENSOR_0V92_VR_Vol:
          //M.2 A
          case GPV2_SENSOR_M2A_INA231_VOL:
          case GPV2_SENSOR_M2A_FPGA_VOL:
          case GPV2_SENSOR_M2A_DDR_VOL:
          //M.2 B
          case GPV2_SENSOR_M2B_INA231_VOL:
          case GPV2_SENSOR_M2B_FPGA_VOL:
          case GPV2_SENSOR_M2B_DDR_VOL:
          //M.2 C
          case GPV2_SENSOR_M2C_INA231_VOL:
          case GPV2_SENSOR_M2C_FPGA_VOL:
          case GPV2_SENSOR_M2C_DDR_VOL:
          //M.2 D
          case GPV2_SENSOR_M2D_INA231_VOL:
          case GPV2_SENSOR_M2D_FPGA_VOL:
          case GPV2_SENSOR_M2D_DDR_VOL:
          //M.2 E
          case GPV2_SENSOR_M2E_INA231_VOL:
          case GPV2_SENSOR_M2E_FPGA_VOL:
          case GPV2_SENSOR_M2E_DDR_VOL:
          //M.2 F
          case GPV2_SENSOR_M2F_INA231_VOL:
          case GPV2_SENSOR_M2F_FPGA_VOL:
          case GPV2_SENSOR_M2F_DDR_VOL:
          //M.2 G
          case GPV2_SENSOR_M2G_INA231_VOL:
          case GPV2_SENSOR_M2G_FPGA_VOL:
          case GPV2_SENSOR_M2G_DDR_VOL:
          //M.2 H
          case GPV2_SENSOR_M2H_INA231_VOL:
          case GPV2_SENSOR_M2H_FPGA_VOL:
          case GPV2_SENSOR_M2H_DDR_VOL:
          //M.2 I
          case GPV2_SENSOR_M2I_INA231_VOL:
          case GPV2_SENSOR_M2I_FPGA_VOL:
          case GPV2_SENSOR_M2I_DDR_VOL:
          //M.2 J
          case GPV2_SENSOR_M2J_INA231_VOL:
          case GPV2_SENSOR_M2J_FPGA_VOL:
          case GPV2_SENSOR_M2J_DDR_VOL:
          //M.2 K
          case GPV2_SENSOR_M2K_INA231_VOL:
          case GPV2_SENSOR_M2K_FPGA_VOL:
          case GPV2_SENSOR_M2K_DDR_VOL:
          //M.2 L
          case GPV2_SENSOR_M2L_INA231_VOL:
          case GPV2_SENSOR_M2L_FPGA_VOL:
          case GPV2_SENSOR_M2L_DDR_VOL:
            snr_desc = get_sensor_desc(fru, snr_num);
            sprintf(crisel, "%s %s %.2fV - ASSERT,FRU:%u", snr_desc->name, thresh_name, val, fru);
            break;
          default:
            return;
        }
#else
        return;
#endif
      } else {
        return;
      }
      break;
    case FRU_SPB:
      switch (snr_num) {
        case SP_SENSOR_FAN0_TACH:
          sprintf(crisel, "Fan0 %s %.0fRPM - ASSERT", thresh_name, val);
          break;
        case SP_SENSOR_FAN1_TACH:
          sprintf(crisel, "Fan1 %s %.0fRPM - ASSERT", thresh_name, val);
          break;
        case SP_SENSOR_P1V15_BMC_STBY:
          sprintf(crisel, "SP_P1V15_STBY %s %.2fV - ASSERT", thresh_name, val);
          break;
        case SP_SENSOR_P1V2_BMC_STBY:
          sprintf(crisel, "SP_P1V2_STBY %s %.2fV - ASSERT", thresh_name, val);
          break;
        case SP_SENSOR_P2V5_BMC_STBY:
          sprintf(crisel, "SP_P2V5_STBY %s %.2fV - ASSERT", thresh_name, val);
          break;
        case SP_SENSOR_P5V:
        case SP_SENSOR_P12V:
        case SP_SENSOR_P3V3_STBY:
        case SP_SENSOR_P12V_SLOT1:
        case SP_SENSOR_P12V_SLOT2:
        case SP_SENSOR_P12V_SLOT3:
        case SP_SENSOR_P12V_SLOT4:
        case SP_SENSOR_P3V3:
        case SP_P1V8_STBY:
        case SP_SENSOR_HSC_IN_VOLT:
          snr_desc = get_sensor_desc(FRU_SPB, snr_num);
          sprintf(crisel, "%s %s %.2fV - ASSERT", snr_desc->name, thresh_name, val);
          break;
        default:
          return;
      }
      break;
    case FRU_NIC:
      switch (snr_num) {
        case MEZZ_SENSOR_TEMP:
          if (thresh >= UNR_THRESH) {
            pal_nic_otp_enable(val);
          }
          return;
        default:
          return;
      }
      break;
    default:
      return;
  }

  pal_add_cri_sel(crisel);
  return;
}

void
pal_sensor_deassert_handle_tl(uint8_t fru, uint8_t snr_num, float val, char* thresh_name) {
  char crisel[128];
  sensor_desc_t *snr_desc;

  switch (snr_num) {
    case BIC_SENSOR_SOC_TEMP:
      sprintf(crisel, "SOC Temp %s %.0fC - DEASSERT,FRU:%u", thresh_name, val, fru);
      break;
    case BIC_SENSOR_P3V3_MB:
    case BIC_SENSOR_P12V_MB:
    case BIC_SENSOR_P1V05_PCH:
    case BIC_SENSOR_P3V3_STBY_MB:
    case BIC_SENSOR_PV_BAT:
    case BIC_SENSOR_PVDDR_AB:
    case BIC_SENSOR_PVDDR_DE:
    case BIC_SENSOR_PVNN_PCH:
    case BIC_SENSOR_VCCIN_VR_VOL:
    case BIC_SENSOR_VCCIO_VR_VOL:
    case BIC_SENSOR_1V05_PCH_VR_VOL:
    case BIC_SENSOR_VDDR_AB_VR_VOL:
    case BIC_SENSOR_VDDR_DE_VR_VOL:
    case BIC_SENSOR_VCCSA_VR_VOL:
    case BIC_SENSOR_INA230_VOL:
      snr_desc = get_sensor_desc(fru, snr_num);
      sprintf(crisel, "%s %s %.2fV - DEASSERT,FRU:%u", snr_desc->name, thresh_name, val, fru);
      break;
    default:
      return;
  }

  pal_add_cri_sel(crisel);
  return;
}

void
pal_sensor_deassert_handle_rc(uint8_t fru, uint8_t snr_num, float val, char* thresh_name) {
  char crisel[128];
  sensor_desc_t *snr_desc;

  switch (snr_num) {
    case BIC_RC_SENSOR_SOC_TEMP_DIODE:
      sprintf(crisel, "SOC Temp DIODE %s %.0fC - DEASSERT,FRU:%u", thresh_name, val, fru);
      break;
    case BIC_RC_SENSOR_SOC_TEMP_IMC:
      sprintf(crisel, "SOC Temp IMC %s %.0fC - DEASSERT,FRU:%u", thresh_name, val, fru);
      break;
    case BIC_RC_SENSOR_P12V_MB:
    case BIC_RC_SENSOR_P3V3_STBY_MB:
    case BIC_RC_SENSOR_P3V2_MB:
    case BIC_RC_SENSOR_PV_BAT:
    case BIC_RC_SENSOR_PVDDQ_423:
    case BIC_RC_SENSOR_PVDDQ_510:
    case BIC_RC_SENSOR_PVDDQ_423_VR_VOL:
    case BIC_RC_SENSOR_PVDDQ_510_VR_VOL:
    case BIC_RC_SENSOR_CVR_APC_VOL:
    case BIC_RC_SENSOR_CVR_CBF_VOL:
    case BIC_RC_SENSOR_INA230_VOL:
      snr_desc = get_sensor_desc(fru, snr_num);
      sprintf(crisel, "%s %s %.2fV - DEASSERT,FRU:%u", snr_desc->name, thresh_name, val, fru);
      break;
    default:
      return;
  }

  pal_add_cri_sel(crisel);
  return;
}

void
pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  char crisel[128];
  char thresh_name[8];
  sensor_desc_t *snr_desc;
  int slot_type = SLOT_TYPE_NULL;

  switch (thresh) {
    case UNR_THRESH:
      sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
      sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
      sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
      sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
      sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
      sprintf(thresh_name, "LNCR");
      break;
    default:
      syslog(LOG_WARNING, "pal_sensor_deassert_handle: wrong thresh enum value");
      return;
  }

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      slot_type = fby2_get_slot_type(fru);
      if (slot_type == SLOT_TYPE_SERVER) {
#if defined(CONFIG_FBY2_RC)
        int ret;
        uint8_t server_type = 0xFF;
        ret = fby2_get_server_type(fru, &server_type);
        if (ret) {
          return;
        }
        switch (server_type) {
          case SERVER_TYPE_RC:
            pal_sensor_deassert_handle_rc(fru, snr_num, val, thresh_name);
            break;
          case SERVER_TYPE_TL:
            pal_sensor_deassert_handle_tl(fru, snr_num, val, thresh_name);
            break;
        }
        return;
#else
        switch (snr_num) {
          case BIC_SENSOR_P3V3_MB:
          case BIC_SENSOR_P12V_MB:
          case BIC_SENSOR_P1V05_PCH:
          case BIC_SENSOR_P3V3_STBY_MB:
          case BIC_SENSOR_PV_BAT:
          case BIC_SENSOR_PVDDR_AB:
          case BIC_SENSOR_PVDDR_DE:
          case BIC_SENSOR_PVNN_PCH:
          case BIC_SENSOR_VCCIN_VR_VOL:
          case BIC_SENSOR_VCCIO_VR_VOL:
          case BIC_SENSOR_1V05_PCH_VR_VOL:
          case BIC_SENSOR_VDDR_AB_VR_VOL:
          case BIC_SENSOR_VDDR_DE_VR_VOL:
          case BIC_SENSOR_VCCSA_VR_VOL:
          case BIC_SENSOR_INA230_VOL:
            snr_desc = get_sensor_desc(fru, snr_num);
            sprintf(crisel, "%s %s %.2fV - DEASSERT,FRU:%u", snr_desc->name, thresh_name, val, fru);
            break;
          default:
            return;
        }
#endif
      } else if (slot_type == SLOT_TYPE_GPV2) {
#if defined(CONFIG_FBY2_GPV2)
        switch (snr_num) {
          case GPV2_SENSOR_P12V_BIC_SCALED:
          case GPV2_SENSOR_P3V3_STBY_BIC_SCALED:
          case GPV2_SENSOR_P0V92_BIC_SCALED:
          case GPV2_SENSOR_P1V8_BIC_SCALED:
          case GPV2_SENSOR_INA230_VOLT:
          // VR
          case GPV2_SENSOR_3V3_VR_Vol:
          case GPV2_SENSOR_0V92_VR_Vol:
          //M.2 A
          case GPV2_SENSOR_M2A_INA231_VOL:
          case GPV2_SENSOR_M2A_FPGA_VOL:
          case GPV2_SENSOR_M2A_DDR_VOL:
          //M.2 B
          case GPV2_SENSOR_M2B_INA231_VOL:
          case GPV2_SENSOR_M2B_FPGA_VOL:
          case GPV2_SENSOR_M2B_DDR_VOL:
          //M.2 C
          case GPV2_SENSOR_M2C_INA231_VOL:
          case GPV2_SENSOR_M2C_FPGA_VOL:
          case GPV2_SENSOR_M2C_DDR_VOL:
          //M.2 D
          case GPV2_SENSOR_M2D_INA231_VOL:
          case GPV2_SENSOR_M2D_FPGA_VOL:
          case GPV2_SENSOR_M2D_DDR_VOL:
          //M.2 E
          case GPV2_SENSOR_M2E_INA231_VOL:
          case GPV2_SENSOR_M2E_FPGA_VOL:
          case GPV2_SENSOR_M2E_DDR_VOL:
          //M.2 F
          case GPV2_SENSOR_M2F_INA231_VOL:
          case GPV2_SENSOR_M2F_FPGA_VOL:
          case GPV2_SENSOR_M2F_DDR_VOL:
          //M.2 G
          case GPV2_SENSOR_M2G_INA231_VOL:
          case GPV2_SENSOR_M2G_FPGA_VOL:
          case GPV2_SENSOR_M2G_DDR_VOL:
          //M.2 H
          case GPV2_SENSOR_M2H_INA231_VOL:
          case GPV2_SENSOR_M2H_FPGA_VOL:
          case GPV2_SENSOR_M2H_DDR_VOL:
          //M.2 I
          case GPV2_SENSOR_M2I_INA231_VOL:
          case GPV2_SENSOR_M2I_FPGA_VOL:
          case GPV2_SENSOR_M2I_DDR_VOL:
          //M.2 J
          case GPV2_SENSOR_M2J_INA231_VOL:
          case GPV2_SENSOR_M2J_FPGA_VOL:
          case GPV2_SENSOR_M2J_DDR_VOL:
          //M.2 K
          case GPV2_SENSOR_M2K_INA231_VOL:
          case GPV2_SENSOR_M2K_FPGA_VOL:
          case GPV2_SENSOR_M2K_DDR_VOL:
          //M.2 L
          case GPV2_SENSOR_M2L_INA231_VOL:
          case GPV2_SENSOR_M2L_FPGA_VOL:
          case GPV2_SENSOR_M2L_DDR_VOL:
            snr_desc = get_sensor_desc(fru, snr_num);
            sprintf(crisel, "%s %s %.2fV - DEASSERT,FRU:%u", snr_desc->name, thresh_name, val, fru);
            break;
          default:
            return;
        }
#else
        return;
#endif
      } else {
        return;
      }
      break;
    case FRU_SPB:
      switch (snr_num) {
        case SP_SENSOR_FAN0_TACH:
          sprintf(crisel, "Fan0 %s %.0fRPM - DEASSERT", thresh_name, val);
          break;
        case SP_SENSOR_FAN1_TACH:
          sprintf(crisel, "Fan1 %s %.0fRPM - DEASSERT", thresh_name, val);
          break;
        case SP_SENSOR_P1V15_BMC_STBY:
          sprintf(crisel, "SP_P1V15_STBY %s %.2fV - DEASSERT", thresh_name, val);
          break;
        case SP_SENSOR_P1V2_BMC_STBY:
          sprintf(crisel, "SP_P1V2_STBY %s %.2fV - DEASSERT", thresh_name, val);
          break;
        case SP_SENSOR_P2V5_BMC_STBY:
          sprintf(crisel, "SP_P2V5_STBY %s %.2fV - DEASSERT", thresh_name, val);
          break;
        case SP_SENSOR_P5V:
        case SP_SENSOR_P12V:
        case SP_SENSOR_P3V3_STBY:
        case SP_SENSOR_P12V_SLOT1:
        case SP_SENSOR_P12V_SLOT2:
        case SP_SENSOR_P12V_SLOT3:
        case SP_SENSOR_P12V_SLOT4:
        case SP_SENSOR_P3V3:
        case SP_P1V8_STBY:
        case SP_SENSOR_HSC_IN_VOLT:
          snr_desc = get_sensor_desc(FRU_SPB, snr_num);
          sprintf(crisel, "%s %s %.2fV - DEASSERT", snr_desc->name, thresh_name, val);
          break;
        default:
          return;
      }
      break;
    case FRU_NIC:
      switch (snr_num) {
        case MEZZ_SENSOR_TEMP:
          if (thresh == UNC_THRESH) {
            pal_nic_otp_disable(val);
          }
          return;
        default:
          return;
      }
      break;
    default:
      return;
  }

  pal_add_cri_sel(crisel);
  return;
}

void pal_post_end_chk(uint8_t *post_end_chk) {
  return;
}

int
pal_get_fw_info(uint8_t fru, unsigned char target, unsigned char* res, unsigned char* res_len)
{
  uint8_t max_index = FW_P1V05_VR;
#if defined(CONFIG_FBY2_RC) || defined(CONFIG_FBY2_EP)
  int ret;
  uint8_t server_type = 0xFF;

  ret = fby2_get_server_type(fru, &server_type);
  if (ret) {
    syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
    return -1;
  }

  switch (server_type) {
    case SERVER_TYPE_RC:
      max_index = FW_DDR423_VR;
      break;
    case SERVER_TYPE_EP:
      max_index = FW_DDR_BH_VR;
      break;
  }
#endif

  if (target > max_index)
    return -1;

  if (target >= FW_CPLD) {
    if (bic_get_fw_ver(fru, target, res))
      return -1;

    switch (target) {
      case FW_CPLD:
        *res_len = 4;
        break;
      case FW_BIC:
      case FW_BIC_BOOTLOADER:
        *res_len = 2;
        break;
      default:
#if defined(CONFIG_FBY2_RC) || defined(CONFIG_FBY2_EP)
        switch (server_type) {
          case SERVER_TYPE_RC:
            switch (target) {
              case FW_IMC:
                *res_len = 8;
                break;
              default:
                *res_len = 5;
                break;
            }
            break;
          case SERVER_TYPE_EP:
            switch (target) {
              case FW_M3:
              case FW_DDR_AG_VR:
              case FW_DDR_BH_VR:
                *res_len = 2;
                break;
              default:
                *res_len = 4;
                break;
            }
            break;
          default:
            switch (target) {
              case FW_ME:
                *res_len = 5;
                break;
              default:
                *res_len = 4;
                break;
            }
            break;
        }
#else
        switch (target) {
          case FW_ME:
            *res_len = 5;
            break;
          default:
            *res_len = 4;
            break;
        }
#endif
        break;
    }
  } else {
    if (pal_get_sysfw_ver(fru, res)) {
      return -1;
    }
    *res_len = SIZE_SYSFW_VER;
  }

  return 0;
}

int
pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr) {

  _sensor_thresh_t *psnr = (_sensor_thresh_t *)snr;
  sensor_check_t *snr_chk;
  sensor_desc_t *snr_desc;

  snr_chk = get_sensor_check(fru, snr_num);
  snr_chk->flag = psnr->flag;
  snr_chk->unr = psnr->unr;
  snr_chk->ucr = psnr->ucr;
  snr_chk->lcr = psnr->lcr;
  snr_chk->retry_cnt = 0;
  snr_chk->val_valid = 0;
  snr_chk->last_val = 0;

  snr_desc = get_sensor_desc(fru, snr_num);
  strncpy(snr_desc->name, psnr->name, sizeof(snr_desc->name));
  snr_desc->name[sizeof(snr_desc->name)-1] = 0;

  return 0;
}

// TODO: Extend pal_get_status to support multiple servers
// For now just, return the value of 'slot1_por_cfg' for all servers
uint8_t
pal_get_status(void) {
  char str_server_por_cfg[64];
  char buff[MAX_VALUE_LEN] = {0};
  int policy = 3;
  uint8_t data;

  // Platform Power Policy
  memset(str_server_por_cfg, 0 , sizeof(char) * 64);
  sprintf(str_server_por_cfg, "%s", "slot1_por_cfg");

  if (pal_get_key_value(str_server_por_cfg, buff) == 0)
  {
    if (!memcmp(buff, "off", strlen("off")))
      policy = 0;
    else if (!memcmp(buff, "lps", strlen("lps")))
      policy = 1;
    else if (!memcmp(buff, "on", strlen("on")))
      policy = 2;
    else
      policy = 3;
  }

  data = 0x01 | (policy << 5);

  return data;
}

int
pal_send_nl_msg(NCSI_NL_MSG_T *nl_msg, NCSI_NL_RSP_T *rcv_buf)
{
  int sock_fd, ret;
  struct sockaddr_nl src_addr, dest_addr;
  struct nlmsghdr *nlh = NULL;
  struct iovec iov;
  struct msghdr msg;
  int req_msg_size = offsetof(NCSI_NL_MSG_T, msg_payload) + nl_msg->payload_length;
  int msg_size = max(sizeof(NCSI_NL_RSP_T), req_msg_size);

  /* open NETLINK socket to send message to kernel */
  sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
  if(sock_fd < 0)  {
    syslog(LOG_ERR, "%s Error: failed to allocate socket", __func__);
    return -1;
  }

  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = getpid(); /* self pid */

  bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0; /* For Linux Kernel */
  dest_addr.nl_groups = 0; /* unicast */

  nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(msg_size));
  if (!nlh) {
    syslog(LOG_ERR, "%s Error, failed to allocate message buffer", __func__);
    goto close_and_exit;
  }
  memset(nlh, 0, NLMSG_SPACE(msg_size));
  nlh->nlmsg_len = NLMSG_SPACE(req_msg_size);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;

  /* the actual NC-SI command from user */
  memcpy(NLMSG_DATA(nlh), nl_msg, req_msg_size);

  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;

  memset(&msg, 0, sizeof(msg));
  msg.msg_name = (void *)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  ret = sendmsg(sock_fd,&msg,0);
  if (ret == -1) {
    syslog(LOG_ERR, "%s Error: errno=%d",__func__ ,errno);
  }

  /* Read message from kernel */
  iov.iov_len = NLMSG_SPACE(msg_size);
  recvmsg(sock_fd, &msg, 0);
  memcpy(rcv_buf, (NCSI_NL_RSP_T *)NLMSG_DATA(nlh), sizeof(NCSI_NL_RSP_T));

  free(nlh);
close_and_exit:
  close(sock_fd);

  return 0;
}

// OEM Command "CMD_OEM_BYPASS_CMD" 0x34
int pal_bypass_cmd(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len){
  int ret;
  int completion_code=CC_UNSPECIFIED_ERROR;
  uint8_t netfn, cmd, select;
  uint8_t tlen, rlen;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t status;
  NCSI_NL_MSG_T *msg = NULL;
  NCSI_NL_RSP_T *rcv_buf = NULL;
  uint8_t channel = 0;
  uint8_t netdev = 0;
  uint8_t netenable = 0;
  char sendcmd[128] = {0};
  int i;

  *res_len = 0;

  if (slot < FRU_SLOT1 || slot > FRU_SLOT4) {
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
  if(ret < 0 || 0 == status) {
    return CC_NOT_SUPP_IN_CURR_STATE;
  }

  if(!pal_is_slot_server(slot)) {
    return CC_UNSPECIFIED_ERROR;
  }

  select = req_data[0];

  switch (select) {
    case BYPASS_BIC:
      tlen = req_len - 6; // payload_id, netfn, cmd, data[0] (select), data[1] (bypass netfn), data[2] (bypass cmd)
      if (tlen < 0) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }

      netfn = req_data[1];
      cmd = req_data[2];

      // Bypass command to Bridge IC
      if (tlen != 0) {
        ret = bic_ipmb_wrapper(slot, netfn, cmd, &req_data[3], tlen, res_data, res_len);
      } else {
        ret = bic_ipmb_wrapper(slot, netfn, cmd, NULL, 0, res_data, res_len);
      }

      if (0 == ret) {
        completion_code = CC_SUCCESS;
      }
      break;
    case BYPASS_ME:
      tlen = req_len - 6; // payload_id, netfn, cmd, data[0] (select), data[1] (bypass netfn), data[2] (bypass cmd)
      if (tlen < 0) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }

      netfn = req_data[1];
      cmd = req_data[2];

      tlen += 2;
      memcpy(tbuf, &req_data[1], tlen);
      tbuf[0] = tbuf[0] << 2;

      // Bypass command to ME
      ret = bic_me_xmit(slot, tbuf, tlen, rbuf, &rlen);
      if (0 == ret) {
        completion_code = rbuf[0];
        memcpy(&res_data[0], &rbuf[1], (rlen - 1));
        *res_len = rlen - 1;
      }
      break;
    case BYPASS_IMC:
      tlen = req_len - 6; // payload_id, netfn, cmd, data[0] (select), data[1] (bypass netfn), data[2] (bypass cmd)
      if (tlen < 0) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }

      netfn = req_data[1];
      cmd = req_data[2];

      tlen += 2;
      memcpy(tbuf, &req_data[1], tlen);
      tbuf[0] = tbuf[0] << 2;

      // Bypass command to IMC
      ret = bic_imc_xmit(slot, tbuf, tlen, rbuf, &rlen);
      if (0 == ret) {
        completion_code = rbuf[0];
        memcpy(&res_data[0], &rbuf[1], (rlen - 1));
        *res_len = rlen - 1;
      }
      break;
    case BYPASS_NCSI:
      tlen = req_len - 7; // payload_id, netfn, cmd, data[0] (select), netdev, channel, cmd
      if (tlen < 0) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }

      netdev = req_data[1];
      channel = req_data[2];
      cmd = req_data[3];

      msg = calloc(1, sizeof(NCSI_NL_MSG_T));
      rcv_buf = calloc(1, sizeof(NCSI_NL_RSP_T));

      if (!msg || !rcv_buf) {
        syslog(LOG_ERR, "%s Error: failed buffer allocation", __func__);
        break;
      }
      memset(msg, 0, sizeof(NCSI_NL_MSG_T));
      memset(msg, 0, sizeof(NCSI_NL_RSP_T));

      sprintf(msg->dev_name, "eth%d", netdev);
      msg->channel_id = channel;
      msg->cmd = cmd;
      msg->payload_length = tlen;

      for (i=0; i<msg->payload_length; i++) {
        msg->msg_payload[i] = req_data[4+i];
      }

      pal_send_nl_msg(msg, rcv_buf);

      memcpy(&res_data[0], &rcv_buf->msg_payload[0], rcv_buf->hdr.payload_length);
      *res_len = rcv_buf->hdr.payload_length;

      free(msg);
      free(rcv_buf);
      completion_code = CC_SUCCESS;
      break;
    case BYPASS_NETWORK:
      tlen = req_len - 6; // payload_id, netfn, cmd, data[0] (select), netdev, netenable
      if (tlen != 0) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }

      netdev = req_data[1];
      netenable = req_data[2];

      if (netenable) {
        if (netenable > 1) {
          completion_code = CC_INVALID_PARAM;
          break;
        }

        sprintf(sendcmd, "ifup eth%d", netdev);
      } else {
        sprintf(sendcmd, "ifdown eth%d", netdev);
      }
      system(sendcmd);
      completion_code = CC_SUCCESS;
      break;
    default:
      return completion_code;
  }

  return completion_code;
}

unsigned char option_offset[] = {0,1,2,3,4,6,11,20,37,164};
unsigned char option_size[]   = {1,1,1,1,2,5,9,17,127};

int
pal_get_boot_option(unsigned char para,unsigned char* pbuff)
{
  unsigned char size = option_size[para];
  memset(pbuff, 0, size);
  return size;
}


int
pal_handle_oem_1s_intr(uint8_t slot, uint8_t *data)
{
  int sock;
  int err;
  struct sockaddr_un server;

  if ((data[0] == PLTRST_N) && (data[1] == 0x01)) {
    bool ierr = false;
    int ret = fby2_common_get_ierr(slot, &ierr);
    if ((ret == 0) && ierr) {
      fby2_common_crashdump(slot, false, true);
    }
    fby2_common_set_ierr(slot,false);
  }

  if (access(sock_path_asd_bic[slot], F_OK) == -1) {
    // SOCK_PATH_ASD_BIC doesn't exist, means ASD daemon for this
    // slot is not running, exit
    return 0;
  }

  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    err = errno;
    syslog(LOG_ERR, "%s failed open socket (errno=%d)", __FUNCTION__, err);
    return -1;
  }

  server.sun_family = AF_UNIX;
  strcpy(server.sun_path, sock_path_asd_bic[slot]);

  if (connect(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un)) < 0) {
    err = errno;
    close(sock);
    syslog(LOG_ERR, "%s failed connecting stream socket (errno=%d), %s",
           __FUNCTION__, err, server.sun_path);
    return -1;
  }
  if (write(sock, data, 2) < 0) {
    err = errno;
    syslog(LOG_ERR, "%s error writing on stream sockets (errno=%d)",
           __FUNCTION__, err);
  }
  close(sock);

  return 0;
}

int
pal_handle_oem_1s_asd_msg_in(uint8_t slot, uint8_t *data, uint8_t data_len)
{
  int sock;
  int err;
  struct sockaddr_un server;

  if (access(sock_path_jtag_msg[slot], F_OK) == -1) {
    // SOCK_PATH_JTAG_MSG doesn't exist, means ASD daemon for this
    // slot is not running, exit
    return 0;
  }

  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    err = errno;
    syslog(LOG_ERR, "%s failed open socket (errno=%d)", __FUNCTION__, err);
    return -1;
  }

  server.sun_family = AF_UNIX;
  strcpy(server.sun_path, sock_path_jtag_msg[slot]);

  if (connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_un)) < 0) {
    err = errno;
    close(sock);
    syslog(LOG_ERR, "%s failed connecting stream socket (errno=%d), %s",
           __FUNCTION__, err, server.sun_path);
    return -1;
  }

  if (write(sock, data, data_len) < 0) {
    err = errno;
    syslog(LOG_ERR, "%s error writing on stream sockets (errno=%d)", __FUNCTION__, err);
  }

  close(sock);
  return 0;
}

int
pal_handle_oem_1s_ras_dump_in(uint8_t slot, uint8_t *data, uint8_t data_len)
{
  static uint32_t last_dump_ts[MAX_NODES + 1] = {0};
  struct timespec ts;
  FILE *fp;

  switch (data[0]) {
    //need remove header
    case 0x02:  // only one package
    case 0x03:  // the first package
      clock_gettime(CLOCK_MONOTONIC, &ts);
      fp = fopen(ras_dump_path[slot], (ts.tv_sec > last_dump_ts[slot])?"w":"a+");
      last_dump_ts[slot] = ts.tv_sec + 60;
      if (fp == NULL) {
        syslog(LOG_ERR, "%s: fopen", __FUNCTION__);
        return -1;
      }

      int index = 1 + CHUNK_OF_CRS_HEADER_LEN;
      if (fwrite(&data[index], 1, (data_len - CHUNK_OF_CRS_HEADER_LEN), fp) <= 0) {
        syslog(LOG_ERR, "%s: fwrite", __FUNCTION__);
        fclose(fp);
        return -1;
      }

      fclose(fp);
      break;
    case 0x04:  // the middle package
    case 0x05:  // the last package
      clock_gettime(CLOCK_MONOTONIC, &ts);
      fp = fopen(ras_dump_path[slot], (ts.tv_sec > last_dump_ts[slot])?"w":"a+");
      last_dump_ts[slot] = ts.tv_sec + 60;
      if (fp == NULL) {
        syslog(LOG_ERR, "%s: fopen", __FUNCTION__);
        return -1;
      }

      if (fwrite(&data[1], 1, data_len, fp) <= 0) {
        syslog(LOG_ERR, "%s: fwrite", __FUNCTION__);
        fclose(fp);
        return -1;
      }

      fclose(fp);
      break;
  }

  return 0;
}

int
pal_is_cplddump_ongoing(uint8_t fru) {
  char fname[128];
  char value[MAX_VALUE_LEN] = {0};
  char svalue[MAX_VALUE_LEN] = {0};
  struct timespec ts;
  int ret;
  int cnt = 0;

  sprintf(fname, "/var/run/cplddump%d.pid", fru);
  if ( access(fname, F_OK) == 0 )
  {
    cnt++;
  }

  strcpy(fname, "");

  sprintf(fname, "/var/run/sbootcplddump%d.pid", fru);
  if ( access(fname, F_OK) == 0 )
  {
    cnt++;
  }

  if (cnt == 0) {   //if both pid file not exist, return false
    return 0;
  }

  strcpy(fname, "");

  //check the cplddump file in /tmp/cache_store/fru$1_cplddump
  sprintf(fname, "fru%d_cplddump", fru);
  ret = kv_get(fname, value, NULL, 0);
  if (ret >= 0)
  {
     clock_gettime(CLOCK_MONOTONIC, &ts);
     if (strtoul(value, NULL, 10) > ts.tv_sec)
     {
       return 1;
     }
  }

  strcpy(fname, "");

  //check the cplddump file for slow boot in /tmp/cache_store/fru$1_sboot_cplddump
  sprintf(fname, "fru%d_sboot_cplddump", fru);
  ret = kv_get(fname, svalue, NULL, 0);
  if (ret >= 0)
  {
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (strtoul(svalue, NULL, 10) > ts.tv_sec)
    {
      return 1;
    }
  }

 //over the threshold time, return false
  return 0;                     /* false */
}

bool
pal_is_cplddump_ongoing_system(void) {
  uint8_t i;

  for (i = FRU_SLOT1; i <= FRU_SLOT4; i++) {
    if (pal_is_cplddump_ongoing(i) == 1)
      return true;
  }

  return false;
}

bool
pal_is_fw_update_ongoing_system(void) {
  uint8_t i;

  for (i = FRU_SLOT1; i <= FRU_BMC; i++) {
    if (pal_is_fw_update_ongoing(i) == true)
      return true;
  }

  return false;
}

int
pal_set_fw_update_ongoing(uint8_t fruid, uint16_t tmout) {
  char key[64] = {0};
  char value[64] = {0};
  struct timespec ts;

  if (fruid == FRU_BMC) {    //BMC update action belongs to SPB range
    fruid = FRU_SPB;
  }

  sprintf(key, "fru%d_fwupd", fruid);

  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += tmout;
  sprintf(value, "%ld", ts.tv_sec);

  if (kv_set(key, value, 0, 0) < 0) {
     return -1;
  }

  return 0;
}

int
pal_ipmb_processing(int bus, void *buf, uint16_t size) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];
  struct timespec ts;
  static time_t last_time = 0;

  if ((bus == 13) && (((uint8_t *)buf)[0] == 0x20)) {  // OCP LCD debug card
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ts.tv_sec >= (last_time + 5)) {
      last_time = ts.tv_sec;
      ts.tv_sec += 30;

      sprintf(key, "ocpdbg_lcd");
      sprintf(value, "%ld", ts.tv_sec);
      if (kv_set(key, value, 0, 0) < 0) {
        return -1;
      }
    }
  }

  return 0;
}

bool
pal_is_mcu_working(void) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  sprintf(key, "ocpdbg_lcd");
  if (kv_get(key, value, NULL, 0)) {
     return false;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec)
     return true;

  return false;
}

void
pal_get_me_name(uint8_t fru, char *target_name) {
#if defined(CONFIG_FBY2_RC) || defined(CONFIG_FBY2_EP)
  int ret;
  uint8_t server_type = 0xFF;

  ret = fby2_get_server_type(fru, &server_type);
  if (ret) {
    syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
    return;
  }

  switch (server_type) {
    case SERVER_TYPE_RC:
      strcpy(target_name, "IMC");
      break;
    case SERVER_TYPE_EP:
      strcpy(target_name, "M3");
      break;
    case SERVER_TYPE_TL:
    default:
      strcpy(target_name, "ME");
      break;
  }
#else
  strcpy(target_name, "ME");
#endif
  return;
}

void
arm_err_parse(uint8_t section_sub_type, char *error_log, uint8_t *sel) {
  uint16_t validation_bit = (sel[1] << 8) | sel[0];
  uint8_t trans_type = sel[2] & 0x3;
  uint8_t operation = sel[2] >> 2 & 0xf;
  uint8_t level = ((sel[3] << 8) | sel[2]) >> 6 & 0x7;
  uint8_t pro_con_cor = sel[3] >> 1 & 0x1;
  uint8_t corrected = sel[3] >> 2 & 0x1;
  uint8_t pre_pc = sel[3] >> 3 & 0x1;
  uint8_t res_pc = sel[3] >> 4 & 0x1;
  uint8_t par_type = sel[3] >> 5 & 0x3;
  uint8_t timeout = sel[3] >> 7 & 0x1;
  uint8_t addr_space = sel[4] & 0x3;
  uint16_t mem_addr_attr = ((sel[5] << 8) | sel[4]) >> 2 & 0x1ff;
  uint8_t access_mode = sel[5] >> 3 & 0x1;
  char temp_log[256] = {0};
  char *type[] = {"cache", "TLB", "bus"};

  if((validation_bit >> TRANS_TYPE_VALID & 0x1) == 1) {
    switch(trans_type) {
      case 0:
        sprintf(temp_log, " Transaction Type: %u - Instruction (Type of %s error),", trans_type, type[section_sub_type]);
        break;
      case 1:
        sprintf(temp_log, " Transaction Type: %u - Data Access (Type of %s error),", trans_type, type[section_sub_type]);
        break;
      case 2:
        sprintf(temp_log, " Transaction Type: %u - Generic (Type of %s error),", trans_type, type[section_sub_type]);
        break;
      default:
        sprintf(temp_log, " Transaction Type: %u - Unknown (Type of %s error),", trans_type, type[section_sub_type]);
        break;
    }
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  } else {
    sprintf(temp_log, " Transaction Type: Not Valid,");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if((validation_bit >> OPER_VALID & 0x1) == 1) {
    switch(operation) {
      case 0:
        sprintf(temp_log, " Operation: %u - generic error (type of error cannot be determined),", operation);
        break;
      case 1:
        sprintf(temp_log, " Operation: %u - generic read (type of instruction or data request cannot be determined),", operation);
        break;
      case 2:
        sprintf(temp_log, " Operation: %u - generic write (type of instruction or data request cannot be determined),", operation);
        break;
      case 3:
        sprintf(temp_log, " Operation: %u - data read,", operation);
        break;
      case 4:
        sprintf(temp_log, " Operation: %u - data write,", operation);
        break;
      case 5:
        sprintf(temp_log, " Operation: %u - instruction fetch,", operation);
        break;
      case 6:
        sprintf(temp_log, " Operation: %u - prefetch,", operation);
        break;
      case 7:
        switch(section_sub_type) {
          case CACHE_ERROR:
            sprintf(temp_log, " Operation: %u - eviction,", operation);
            break;
          case TLB_ERROR:
            sprintf(temp_log, " Operation: %u - local management operation (the processor described in this record initiated a TLB management operation that resulted in an error),", operation);
            break;
          default:
            sprintf(temp_log, " Operation: %u - Unknown,", operation);
            break;
        }
        break;
      case 8:
        switch(section_sub_type) {
          case CACHE_ERROR:
            sprintf(temp_log, " Operation: %u - snooping (the processor described in this record initiated a cache snoop that resulted in an error),", operation);
            break;
          case TLB_ERROR:
            sprintf(temp_log, " Operation: %u - external management operation (the processor described in this record raised a TLB error caused by another processor or device broadcasting TLB operations),", operation);
            break;
          default:
            sprintf(temp_log, " Operation: %u - Unknown,", operation);
            break;
        }
        break;
      case 9:
        sprintf(temp_log, " Operation: %u - snooped (the processor described in this record raised a cache error caused by another processor or device snooping into its cache),", operation);
        break;
      case 10:
        sprintf(temp_log, " Operation: %u - management,", operation);
        break;
      default:
        sprintf(temp_log, " Operation: %u - Unknown,", operation);
        break;
    }
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  } else {
    sprintf(temp_log, " Operation: Not Valid,");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if((validation_bit >> LEVEL_VALID & 0x1) == 1) {
    switch(section_sub_type) {
      case CACHE_ERROR:
        sprintf(temp_log, " Level: %u (Cache level),", level);
        break;
      case TLB_ERROR:
        sprintf(temp_log, " Level: %u (TLB level),", level);
        break;
      case BUS_ERROR:
        sprintf(temp_log, " Level: %u (Affinity level at which the bus error occurred),", level);
        break;
      default:
        sprintf(temp_log, " Level: %u (Unknown),", level);
        break;
    }
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  } else {
    sprintf(temp_log, " Level: Not Valid,");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if((validation_bit >> PROC_CONTEXT_CORRUPT_VALID & 0x1) == 1) {
    switch(pro_con_cor) {
      case 0:
        sprintf(temp_log, " Processor Context Corrupt: %u - Processor context not corrupted,", pro_con_cor);
        break;
      case 1:
        sprintf(temp_log, " Processor Context Corrupt: %u - Processor context corrupted,", pro_con_cor);
        break;
      default:
        sprintf(temp_log, " Processor Context Corrupt: %u - Unknown,", pro_con_cor);
        break;
    }
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  } else {
    sprintf(temp_log, " Processor Context Corrupt: Not Valid,");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if((validation_bit >> CORR_VALID & 0x1) == 1) {
    switch(corrected) {
      case 0:
        sprintf(temp_log, " Corrected: %u - Uncorrected,", corrected);
        break;
      case 1:
        sprintf(temp_log, " Corrected: %u - Corrected,", corrected);
        break;
      default:
        sprintf(temp_log, " Corrected: %u - Unknown,", corrected);
        break;
    }
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  } else {
    sprintf(temp_log, " Corrected: Not Valid,");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if((validation_bit >> PRECISE_PC_VALID & 0x1) == 1) {
    sprintf(temp_log, " Precise PC: %u (This field indicates that the program counter that is directly associated with the error),", pre_pc);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  } else {
    sprintf(temp_log, " Precise PC: Not Valid,");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if((validation_bit >> RESTART_PC_VALID & 0x1) == 1) {
    sprintf(temp_log, " Restartable PC: %u (This field indicates that program execution can be restarted reliably at the PC associated with the error)", res_pc);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  } else {
    sprintf(temp_log, " Restartable PC: Not Valid");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  switch(section_sub_type) {
    case BUS_ERROR:
      break;
    case CACHE_ERROR:
    case TLB_ERROR:
    default:
      return;
  }

  if((validation_bit >> PARTICIPATION_TYPE_VALID & 0x1) == 1) {
    switch(par_type) {
      case 0:
        sprintf(temp_log, ", Participation Type: %u - Local Processor originated request", par_type);
        break;
      case 1:
        sprintf(temp_log, ", Participation Type: %u - Local processor Responded to request", par_type);
        break;
      case 2:
        sprintf(temp_log, ", Participation Type: %u - Local processor Observed", par_type);
        break;
      case 3:
        sprintf(temp_log, ", Participation Type: %u - Generic", par_type);
        break;
      default:
        sprintf(temp_log, ", Participation Type: %u - Unknown", par_type);
        break;
    }
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  } else {
    sprintf(temp_log, ", Participation Type: Not Valid");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if((validation_bit >> TIMEOUT_VALID & 0x1) == 1 ) {
    sprintf(temp_log, ", Time Out: %u (This field indicates that the request timed out)", timeout);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  } else {
    sprintf(temp_log, ", Time Out: Not Valid");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if((validation_bit >> ADDRESS_SPACE_VALID & 0x1) == 1) {
    switch(addr_space) {
      case 0:
        sprintf(temp_log, ", Address Space: %u - External Memory Access (e.g. DDR))", addr_space);
        break;
      case 1:
        sprintf(temp_log, ", Address Space: %u - Internal Memory Access (e.g. internal chip ROM))", addr_space);
        break;
      case 2:
        sprintf(temp_log, ", Address Space: %u - Device Memory Access)", addr_space);
        break;
      default:
        sprintf(temp_log, ", Address Space: %u - Unknown", addr_space);
        break;
    }
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  } else {
    sprintf(temp_log, ", Address Space: Not Valid");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if((validation_bit >> MEM_ATTR_VALID & 0x1) == 1) {
    sprintf(temp_log, ", Memory Access Attributes: %u (Memory attribute as described in the ARM specification)", mem_addr_attr);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  } else {
    sprintf(temp_log, ", Memory Access Attributes: Not Valid");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if((validation_bit >> ACCESS_MODE_VALID & 0x1) == 1) {
    switch(access_mode) {
      case 0:
        sprintf(temp_log, ", Access Mode: %u - secure", access_mode);
        break;
      case 1:
        sprintf(temp_log, ", Access Mode: %u - normal", access_mode);
        break;
      default:
        sprintf(temp_log, ", Access Mode: %u - Unknown", access_mode);
        break;
    }
    strcat(error_log, temp_log);
  } else {
    sprintf(temp_log, ", Access Mode: Not Valid");
    strcat(error_log, temp_log);
  }

  return;
}

int
processor_ras_sel_parse(uint8_t slot, char *error_log, uint8_t *sel, char *crisel) {
  uint8_t section_sub_type = sel[0];
  char temp_log[128] = {0};
  char crisel_local[128] = {0};

  switch(section_sub_type) {
    case CACHE_ERROR:
      sprintf(temp_log, " Section Sub-type: Cache Error,");
      strcat(error_log, temp_log);
      arm_err_parse(section_sub_type, error_log, &sel[1]);
      sprintf(crisel_local, "Processor %s Cache Error,FRU:%u",crisel, slot);
      pal_add_cri_sel(crisel_local);
      return 1;
    case TLB_ERROR:
      sprintf(temp_log, " Section Sub-type: TLB Error,");
      strcat(error_log, temp_log);
      arm_err_parse(section_sub_type, error_log, &sel[1]);
      sprintf(crisel_local, "Processor %s TLB Error,FRU:%u",crisel, slot);
      pal_add_cri_sel(crisel_local);
      return 1;
    case BUS_ERROR:
      sprintf(temp_log, " Section Sub-type: Bus Error,");
      strcat(error_log, temp_log);
      arm_err_parse(section_sub_type, error_log, &sel[1]);
      sprintf(crisel_local, "Processor %s Bus Error,FRU:%u",crisel, slot);
      pal_add_cri_sel(crisel_local);
      return 1;
    case MICRO_ARCH_ERROR:
      sprintf(temp_log, " Section Sub-type: Micro-architectural Error,");
      sprintf(crisel_local, "Processor %s Micro-architectural Error,FRU:%u",crisel, slot);
      break;
    default:
      sprintf(temp_log, " Section Sub-type: Unknown(0x%x),", section_sub_type);
      sprintf(crisel_local, "Processor %d(%s) Unknown Error,FRU:%u", slot, crisel, slot);
      break;
  }
  strcat(error_log, temp_log);
  pal_add_cri_sel(crisel_local);
  return 0;
}

void
memory_ras_sel_parse(uint8_t slot, char *error_log, uint8_t *sel, char *crisel) {
  uint8_t section_sub_type = sel[0];
  char temp_log[256] = {0};
  uint16_t node = 0;
  int ch_num = 0;
  int dimm_num = 0;
  uint16_t bank = 0;
  uint16_t dev = 0;
  uint16_t row = 0;
  uint16_t col = 0;
  uint16_t rank = 0;
  char crisel_local[128] = {0};

  switch(section_sub_type) {
    case 0:
      sprintf(temp_log, " Section Sub-type: Unknown,");
      break;
    case 1:
      sprintf(temp_log, " Section Sub-type: No Error,");
      break;
    case 2:
      sprintf(temp_log, " Section Sub-type: Single-bit ECC,");
      break;
    case 3:
      sprintf(temp_log, " Section Sub-type: Multi-bit ECC,");
      break;
    case 4:
      sprintf(temp_log, " Section Sub-type: Single-symbol Chipkill ECC,");
      break;
    case 5:
      sprintf(temp_log, " Section Sub-type: Multi-symbol ChipKill ECC,");
      break;
    case 6:
      sprintf(temp_log, " Section Sub-type: Master Abort,");
      break;
    case 7:
      sprintf(temp_log, " Section Sub-type: Target Abort,");
      break;
    case 8:
      sprintf(temp_log, " Section Sub-type: Parity Error,");
      break;
    case 9:
      sprintf(temp_log, " Section Sub-type: Watchdog Timeout,");
      break;
    case 10:
      sprintf(temp_log, " Section Sub-type: Invalid Address,");
      break;
    case 11:
      sprintf(temp_log, " Section Sub-type: Mirror Broken,");
      break;
    case 12:
      sprintf(temp_log, " Section Sub-type: Memory Sparing,");
      break;
    case 13:
      sprintf(temp_log, " Section Sub-type: Scrub corrected Error,");
      break;
    case 14:
      sprintf(temp_log, " Section Sub-type: Scrub uncorrected error,");
      break;
    case 15:
      sprintf(temp_log, " Section Sub-type: Physical Memory Map-out event,");
      break;
    default:
      sprintf(temp_log, " Section Sub-type: Undefined(0x%x),", section_sub_type);
      break;
  }
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  node = sel[2] << 8 | sel[1];
  ch_num = sel[4] << 8 | sel[3];
  dimm_num = sel[6] << 8 | sel[5];
  bank = sel[8] << 8 | sel[7];
  dev = sel[10] << 8 | sel[9];
  row = sel[12] << 8 | sel[11];
  col = sel[14] << 8 | sel[13];
  rank = sel[16] << 8 | sel[15];

  if(ch_num == 3 && dimm_num == 0) {
    sprintf(temp_log, " Error Specific: \"Node: %d\" \"Card: %d\" \"Module: %d\" \"Bank: %d\" \"Device: %d\" \"Row: %d\" \"Column: %d\" \"Rank Number: %d\" \"Location: DIMM A0\"", node, ch_num, dimm_num, bank, dev, row, col, rank);
    sprintf(crisel_local, "DIMM A0 %s err,FRU:%u",crisel, slot);
  } else if(ch_num == 2 && dimm_num == 0) {
    sprintf(temp_log, " Error Specific: \"Node: %d\" \"Card: %d\" \"Module: %d\" \"Bank: %d\" \"Device: %d\" \"Row: %d\" \"Column: %d\" \"Rank Number: %d\" \"Location: DIMM B0\"", node, ch_num, dimm_num, bank, dev, row, col, rank);
    sprintf(crisel_local, "DIMM B0 %s err,FRU:%u",crisel, slot);
  } else if(ch_num == 4 && dimm_num == 0) {
    sprintf(temp_log, " Error Specific: \"Node: %d\" \"Card: %d\" \"Module: %d\" \"Bank: %d\" \"Device: %d\" \"Row: %d\" \"Column: %d\" \"Rank Number: %d\" \"Location: DIMM C0\"", node, ch_num, dimm_num, bank, dev, row, col, rank);
    sprintf(crisel_local, "DIMM C0 %s err,FRU:%u",crisel, slot);
  } else if(ch_num == 5 && dimm_num == 0) {
    sprintf(temp_log, " Error Specific: \"Node: %d\" \"Card: %d\" \"Module: %d\" \"Bank: %d\" \"Device: %d\" \"Row: %d\" \"Column: %d\" \"Rank Number: %d\" \"Location: DIMM D0\"", node, ch_num, dimm_num, bank, dev, row, col, rank);
    sprintf(crisel_local, "DIMM D0 %s err,FRU:%u",crisel, slot);
  } else {
    sprintf(temp_log, " Error Specific: \"Node: %d\" \"Card: %d\" \"Module: %d\" \"Bank: %d\" \"Device: %d\" \"Row: %d\" \"Column: %d\" \"Rank Number: %d\" \"Location: Unknown\"", node, ch_num, dimm_num, bank, dev, row, col, rank);
    sprintf(crisel_local, "DIMM Unknown %s err,FRU:%u",crisel, slot);
  }

  strcat(error_log, temp_log);
  pal_add_cri_sel(crisel_local);

  return;
}

void
pcie_aer_sel_parse(char *error_log, uint8_t *sel) {
  uint32_t root_err_sts = 0;
  uint32_t uncor_err_sts = 0;
  uint32_t cor_err_sts = 0;
  char temp_log[512] = {0};
  int index = 0;
  uint8_t err_flag = 0;

  root_err_sts = (sel[0] << 24) | (sel[1] << 16) | (sel[2] << 8) | sel[3];

  uncor_err_sts = (sel[4] << 24) | (sel[5] << 16) | (sel[6] << 8) | sel[7];

  cor_err_sts = (sel[8] << 24) | (sel[9] << 16) | (sel[10] << 8) | sel[11];

  sprintf(temp_log, " Root Error Status:");
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  for(index = 0; index < (sizeof(aer_root_err_sts)/sizeof(struct ras_pcie_aer_info) - 1); index++) {
    if((root_err_sts >> aer_root_err_sts[index].offset & 0x1) == 1) {
      sprintf(temp_log, " \"%s\"", aer_root_err_sts[index].name);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }
  }
  sprintf(temp_log, " \"%s: %d\" /", aer_root_err_sts[index].name, root_err_sts >> aer_root_err_sts[index].offset);
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  sprintf(temp_log, " AER Uncorrectable Error Status:");
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  for(index = 0; index < sizeof(aer_uncor_err_sts)/sizeof(struct ras_pcie_aer_info) ; index++) {
    if((uncor_err_sts >> aer_uncor_err_sts[index].offset & 0x1) == 1) {
      sprintf(temp_log, " \"%s\"", aer_uncor_err_sts[index].name);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
      err_flag = 1;
    }
  }

  if(!err_flag) {
    sprintf(temp_log, " None");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  err_flag = 0;

  sprintf(temp_log, " / AER Correctable Error Status:");
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  for(index = 0; index < sizeof(aer_cor_err_sts)/sizeof(struct ras_pcie_aer_info); index++) {
    if((cor_err_sts >> aer_cor_err_sts[index].offset & 0x1) == 1) {
      sprintf(temp_log, " \"%s\"", aer_cor_err_sts[index].name);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
      err_flag = 1;
    }
  }

  if(!err_flag) {
    sprintf(temp_log, " None /");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
    return;
  }

  sprintf(temp_log, " /");
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  return;
}

void
pcie_ras_sel_parse(uint8_t slot, char *error_log, uint8_t *sel, char *crisel) {
  uint8_t section_sub_type = sel[0];
  char temp_log[128] = {0};
  uint8_t fun_num = sel[8];
  uint8_t dev_num = sel[9];
  uint8_t seg_num[2] = {sel[10],sel[11]};
  uint8_t bus_num = sel[12];
  char crisel_local[128] = {0};

  switch(section_sub_type) {
    case 0x00:
      sprintf(temp_log, " Section Sub-type: PCIE error,");
      break;
    default:
      sprintf(temp_log, " Section Sub-type: Unknown(0x%x),", section_sub_type);
      break;
  }
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  sprintf(temp_log, " Error Specific: Segment Number (%02x%02x) / Bus Number (%02x) / Device Number (%02x) / Function Number (%x) /", seg_num[1], seg_num[0], bus_num, dev_num, fun_num);
  strcat(error_log, temp_log);

  pcie_aer_sel_parse(error_log, &sel[17]);

  sprintf(crisel_local, "PCIe err %s (Bus %02X / Dev %02X / Fun %02X),FRU:%u",crisel, bus_num, dev_num, fun_num, slot);
  pal_add_cri_sel(crisel_local);

  return;
}

void
qualcomm_fw_ras_sel_parse(char *error_log, uint8_t *sel) {
  uint8_t section_sub_type = sel[0];
  char temp_log[128] = {0};

  switch(section_sub_type) {
    case 0x00:
      sprintf(temp_log, " Section Sub-type: Invalid,");
      break;
    case 0x01:
      sprintf(temp_log, " Section Sub-type: Imem,");
      break;
    case 0x02:
      sprintf(temp_log, " Section Sub-type: BERT,");
      break;
    case 0x03:
      sprintf(temp_log, " Section Sub-type: IMC Error,");
      break;
    case 0x04:
      sprintf(temp_log, " Section Sub-type: PMIC Over Temp,");
      break;
    case 0x05:
      sprintf(temp_log, " Section Sub-type: XPU Error,");
      break;
    case 0x06:
      sprintf(temp_log, " Section Sub-type: EL3 Firmware Error,");
      break;
    default:
      sprintf(temp_log, " Section Sub-type: Unknown(0x%x),", section_sub_type);
      break;
  }
  strcat(error_log, temp_log);

  return;
}

uint8_t
pal_err_ras_sel_handle(uint8_t slot, uint8_t section_type, char *error_log, uint8_t *sel, char *crisel) {
  char temp_log[256] = {0};
  char tstr[10] = {0};
  char ras_data[256]={0};
  int ret;
  int i;

  switch(section_type) {
    case 0x00:
      ret = processor_ras_sel_parse(slot, error_log, sel, crisel);
      if (ret) {
        return 0;
      }
      break;
    case 0x01:
      memory_ras_sel_parse(slot, error_log, sel, crisel);
      return 0;
    case 0x02:  //Not used
      return 0;
    case 0x03:
      pcie_ras_sel_parse(slot, error_log, sel, crisel);
      return 0;
    case 0x04:
      qualcomm_fw_ras_sel_parse(error_log, sel);
      break;
    default:
      sprintf(temp_log, " Section Sub-type: Unknown(0x%x),", sel[0]);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
      break;
  }

  sprintf(temp_log, " Error Specific Raw Data: ");
  strcat(error_log, temp_log);
  for(i = 1; i < SIZE_RAS_SEL - 7; i++) {
    sprintf(tstr, "%02X:", sel[i]);
    strcat(ras_data, tstr);
  }
  sprintf(tstr, "%02X", sel[SIZE_RAS_SEL - 7]);
  strcat(ras_data, tstr);
  strcat(error_log, ras_data);

  return 0;
}

uint8_t
pal_parse_ras_sel(uint8_t slot, uint8_t *sel, char *error_log) {
  uint8_t error_type = sel[0];
  uint8_t error_severity = sel[1];
  uint8_t section_type = sel[2];
  char temp_log[128] = {0};
  strcpy(error_log, "");
  char crisel[128] = {0};

  switch(error_type) {
    case 0x00:
      sprintf(error_log, " Error Type: SEI,");
      break;
    case 0x01:
      sprintf(error_log, " Error Type: SEA,");
      break;
    case 0x02:
      sprintf(error_log, " Error Type: PEI,");
      break;
    case 0x03:
      sprintf(error_log, " Error Type: BERT/BOOT,");
      break;
    case 0x04:
      sprintf(error_log, " Error Type: PCIe,");
      break;
    default:
      sprintf(error_log, " Error Type: Unknown(0x%x),", error_type);
      break;
  }

  switch(error_severity) {
    case 0x00:
      sprintf(temp_log, " Error Severity: Recoverable(non-fatal uncorrected),");
      sprintf(crisel, "Recoverable(non-fatal uncorrected)");
      break;
    case 0x01:
      sprintf(temp_log, " Error Severity: Fatal,");
      sprintf(crisel, "Fatal");
      break;
    case 0x02:
      sprintf(temp_log, " Error Severity: Corrected,");
      sprintf(crisel, "Corrected");
      break;
    case 0x03:
      sprintf(temp_log, " Error Severity: Informational,");
      sprintf(crisel, "Informational");
      break;
    default:
      sprintf(temp_log, " Error Severity: Unknown(0x%x),", error_severity);
      sprintf(crisel, "Unknown");
      break;
  }
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  switch(section_type) {
    case 0x00:
      sprintf(temp_log, " Section Type: Processor Specific ARM,");
      break;
    case 0x01:
      sprintf(temp_log, " Section Type: Memory,");
      break;
    case 0x02:
      sprintf(temp_log, " Section Type: Firmware");
      break;
    case 0x03:
      sprintf(temp_log, " Section Type: PCIe,");
      break;
    case 0x04:
      sprintf(temp_log, " Section Type: Qualcomm Firmware,");
      break;
    default:
      sprintf(temp_log, " Section Type: Unknown(0x%x),", section_type);
      break;
  }
  strcat(error_log, temp_log);

  pal_err_ras_sel_handle(slot, section_type, error_log, &sel[3], crisel);

  return 0;
}

uint8_t
oem_error_sec_sel_parse(uint8_t slot, uint8_t *req_data, uint8_t req_len)
{
  uint8_t completion_code = CC_UNSPECIFIED_ERROR;
  char error_log[1024] = {0};
  char temp_log[512] = {0};
  uint16_t Error_Type = 0;
  uint8_t Error_location = 0;
  uint32_t Error_Register0 = 0;
  uint32_t Error_Register1 = 0;
  uint16_t Error_Register2 = 0;
  uint8_t Source_node = 0;
  uint8_t Destination_node = 0;
  uint8_t Source_ICI = 0;
  uint8_t Destination_ICI = 0;
  uint32_t CRC_Error_Threshold = 0;
  uint16_t Link_Retrain_threshold = 0;
  uint16_t Link_Retry_Count = 0;
  uint16_t Link_Retrain_Window = 0;
  uint32_t Raw_ICS_Error_Info = 0;
  uint8_t  Error_severity = 0;

  sprintf(temp_log, "SEL Entry: FRU: %d, ", slot);
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  Error_severity = req_data[24];
  sprintf(temp_log, "Error severity:%s, ", error_severity_strs[Error_severity]);
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  Error_Type = req_data[26] << 8 | req_data[25];

  if (Error_Type == 0) {
    sprintf(temp_log, "Section Type:CCPI error, ");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");

    if (req_data[27] & 0x1) {
      sprintf(temp_log, "CCPI Error Type:Fatal Error for Node Tables, ");
    } else if (req_data[27] & 0x2) {
        sprintf(temp_log, "CCPI Error Type:Fatal Error for Req VC,");
    } else if (req_data[27] & 0x4) {
        sprintf(temp_log, "CCPI Error Type:Fatal Error for Snp VC, ");
    } else if (req_data[27] & 0x8) {
        sprintf(temp_log, "CCPI Error Type:Fatal Error for RspSnp VC, ");
    } else if (req_data[27] & 0x10) {
        sprintf(temp_log, "CCPI Error Type:Fatal Error for Rsp VC, ");
    } else if (req_data[27] & 0x20) {
        sprintf(temp_log, "CCPI Error Type:Fatal Error for Cmpl VC, ");
    } else if (req_data[27] & 0x40) {
        sprintf(temp_log, "CCPI Error Type:Fatal Error for Data VC, ");
    } else if (req_data[27] & 0x80) {
        sprintf(temp_log, "CCPI Error Type:Fatal Error for Gic VC, ");
    } else if (req_data[28] & 0x1) {
        sprintf(temp_log, "CCPI Error Type:Fatal Error for NodeCmd Hwf Read/Write, ");
    }
    strcat(error_log, temp_log);
    strcpy(temp_log, "");

    if (req_data[29] & 0x1) {
      Source_node = req_data[31];
      sprintf(temp_log, "Src node:%d, ", Source_node);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    if (req_data[29] & 0x4) {
      Destination_node = req_data[32];
      sprintf(temp_log, "Dst node:%d, ", Destination_node);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    if (req_data[29] & 0x2) {
      Source_ICI = req_data[33];
      sprintf(temp_log, "Src ICI:%d, ", Source_ICI);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    if (req_data[29] & 0x8) {
      Destination_ICI = req_data[34];
      sprintf(temp_log, "Dst ICI:%d, ", Destination_ICI);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    if (req_data[29] & 0x10) {
      CRC_Error_Threshold = req_data[37] << 16 | req_data[36] << 8 | req_data[35];
      sprintf(temp_log, "CRC_Error Threshold:%d, ", CRC_Error_Threshold);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    if (req_data[29] & 0x20) {
      Link_Retrain_threshold = req_data[39] << 8 | req_data[38];
      sprintf(temp_log, "Link Retrain threshold:%d, ", Link_Retrain_threshold);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    if (req_data[29] & 0x40) {
      Link_Retry_Count = req_data[41] << 8 | req_data[40];
      sprintf(temp_log, "Link Retry Count:%d, ", Link_Retry_Count);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    if (req_data[29] & 0x80) {
      Link_Retrain_Window = req_data[43] << 8 | req_data[42];
      sprintf(temp_log, "Link Retrain Window:%d, ", Link_Retrain_Window);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    if (req_data[30] & 0x1) {
      Raw_ICS_Error_Info = req_data[46] << 16 | req_data[45] << 8 | req_data[44];
      sprintf(temp_log, "Raw ICS Error Info:%6x", Raw_ICS_Error_Info);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

  } else if (Error_Type == 1) {
    sprintf(temp_log, "Section Type:NBU error, ");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");

    if (req_data[27] & 0x1) {
      if (req_data[29] & 0x1) {
        sprintf(temp_log, "Error Mask:NBU Tag Correctable ECC Error, ");
      } else if (req_data[29] & 0x2) {
        sprintf(temp_log, "Error Mask:NBU Tag Uncorrectable ECC Error, ");
      } else if (req_data[29] & 0x4) {
        sprintf(temp_log, "Error Mask:NBU BAR Address Error, ");
      } else if (req_data[29] & 0x8) {
        sprintf(temp_log, "Error Mask:NBU Snoop Filter Correctable ECC Error, ");
      } else if (req_data[29] & 0x10) {
        sprintf(temp_log, "Error Mask:NBU Snoop Filter Uncorrectable ECC Error, ");
      } else if (req_data[29] & 0x20) {
        sprintf(temp_log, "Error Mask:NBU Timeout Error, ");
      }
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    if( (req_data[27] & 0x4) || (req_data[27] & 0x8)) {
      Error_location = req_data[32];
      sprintf(temp_log, "Error Location:%d, " ,Error_location);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    if(req_data[27] & 0x10) {
      if (req_data[32] == 0) {
        sprintf(temp_log, "Subtype:NBU BAR Error, ");
      } else if (req_data[32] & 0x1) {
        sprintf(temp_log, "Subtype:NBU Timeout Error, ");
      } else if (req_data[32] & 0x2) {
        sprintf(temp_log, "Subtype:NBU Snoop Filter ECC Error, ");
      } else if ((req_data[32] & 0x1) && (req_data[32] & 0x2)) {
        sprintf(temp_log, "Subtype:NBU Snoop Filter Count Error, ");
      } else if (req_data[32] & 0x4) {
        sprintf(temp_log, "Subtype:NBU Tag Error, ");
      }
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    if(req_data[27] & 0x20) {
      Error_Register0 = req_data[36] << 24 | req_data[35] << 16 | req_data[34] << 8 | req_data[33];
      sprintf(temp_log, "Error Register0:0x%08x, ", Error_Register0);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    if(req_data[27] & 0x40) {
      Error_Register1 = req_data[40] << 24 | req_data[39] << 16 | req_data[38] << 8 | req_data[37];
      sprintf(temp_log, "Error Register1:0x%08x, ", Error_Register1);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    if(req_data[27] & 0x80) {
      Error_Register2 = req_data[42] << 8 | req_data[41];
      sprintf(temp_log, "Error Register2:0x%04x", Error_Register2);
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }
  }
  syslog(LOG_CRIT, "%s", error_log);
  completion_code = CC_SUCCESS;
  return completion_code;
}

uint8_t
pci_express_error_sec_sel_parse(uint8_t slot, uint8_t *req_data, uint8_t req_len)
{
  uint8_t completion_code = CC_UNSPECIFIED_ERROR;
  uint32_t port_type = 0;
  uint8_t minor_version_bcd = 0;
  uint8_t major_version_bcd = 0;
  uint16_t pci_command_register = 0;
  uint16_t pci_status_register = 0;
  uint8_t function_num = 0;
  uint8_t device_num = 0;
  uint16_t seqment_num = 0;
  uint8_t root_port_pri_bus_num = 0;
  uint8_t  Error_severity = 0;
  uint32_t aer_root_err_status = 0;
  uint32_t aer_cor_err_status = 0;
  uint32_t aer_uncor_err_status = 0;
  char error_log[1024] = {0};
  char temp_log[512] = {0};
  bool error_flag = 0;
  int index = 0;

  sprintf(temp_log, "SEL Entry: FRU: %d, ", slot);
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  Error_severity = req_data[24];
  sprintf(temp_log, "Error severity:%s, ", error_severity_strs[Error_severity]);
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  sprintf(temp_log, "Section Type:PCIe error, ");
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  if (req_data[25] & 0x1) {
    //port_type
    port_type = req_data[36] << 24 | req_data[35] << 16 | req_data[34] << 8 | req_data[33];
    if(port_type < (sizeof(pcie_port_type_strs) / sizeof(pcie_port_type_strs[0]))) {
      sprintf(temp_log, "Port Type:%s, ", pcie_port_type_strs[port_type]);
    }
    else {
      sprintf(temp_log, "Port Type:Unknown, ");
    }
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if (req_data[25] & 0x2) {
    //version
    minor_version_bcd = req_data[37];
    major_version_bcd = req_data[38];
    sprintf(temp_log, "Version:%d.%d, ", major_version_bcd, minor_version_bcd);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if (req_data[25] & 0x4) {
    //command & status
    pci_command_register = req_data[42] << 8 | req_data[41];
    pci_status_register =  req_data[44] << 8 | req_data[43];
    sprintf(temp_log, "Command:0x%04x, Status:0x%04x, ", pci_command_register, pci_status_register);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if (req_data[25] & 0x8) {
    //deviceID
    function_num = req_data[56];
    device_num = req_data[57];
    seqment_num = req_data[59] << 8 |  req_data[58];
    root_port_pri_bus_num = req_data[60];
    sprintf(temp_log, "Segment:%04x, Bus:%02x, Device:%02x, Function:%x", seqment_num, root_port_pri_bus_num, device_num, function_num);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if (req_data[25] & 0x80) {
    //AER info
    aer_root_err_status = req_data[188] << 24 | req_data[187] << 16 | req_data[186] << 8 | req_data[185];
    aer_cor_err_status = req_data[156] << 24 | req_data[155] << 16 | req_data[154] << 8 | req_data[153];
    aer_uncor_err_status = req_data[144] << 24 | req_data[143] << 16 | req_data[142] << 8 | req_data[141];

    sprintf(temp_log, ", Root Error Status:");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");

    for(index = 0; index < (sizeof(aer_root_err_sts)/sizeof(struct ras_pcie_aer_info) - 1); index++) {
      if((aer_root_err_status >> aer_root_err_sts[index].offset & 0x1) == 1) {
        sprintf(temp_log, " \"%s\"", aer_root_err_sts[index].name);
        strcat(error_log, temp_log);
        strcpy(temp_log, "");
      }
    }

    sprintf(temp_log, " \"%s:%d\"", aer_root_err_sts[index].name, aer_root_err_status >> aer_root_err_sts[index].offset);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");

    error_flag = 0;
    sprintf(temp_log, ", AER Uncorrectable Error Status:");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");

    for(index = 0; index < sizeof(aer_uncor_err_sts)/sizeof(struct ras_pcie_aer_info) ; index++) {
      if((aer_uncor_err_status >> aer_uncor_err_sts[index].offset & 0x1) == 1) {
        sprintf(temp_log, " \"%s\"", aer_uncor_err_sts[index].name);
        strcat(error_log, temp_log);
        strcpy(temp_log, "");
        error_flag = 1;
      }
    }

    if(error_flag == 0) {
      sprintf(temp_log, "NONE");
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }

    error_flag = 0;
    sprintf(temp_log, ", AER Correctable Error Status:");
    strcat(error_log, temp_log);
    strcpy(temp_log, "");

    for(index = 0; index < sizeof(aer_cor_err_sts)/sizeof(struct ras_pcie_aer_info); index++) {
      if((aer_cor_err_status >> aer_cor_err_sts[index].offset & 0x1) == 1) {
        sprintf(temp_log, " \"%s\"", aer_cor_err_sts[index].name);
        strcat(error_log, temp_log);
        strcpy(temp_log, "");
        error_flag = 1;
      }
    }

    if(error_flag == 0) {
      sprintf(temp_log, "NONE");
      strcat(error_log, temp_log);
      strcpy(temp_log, "");
    }
  }

  syslog(LOG_CRIT, "%s", error_log);
  completion_code = CC_SUCCESS;
  return completion_code;
}

uint8_t
memory_error_sec_sel_parse(uint8_t slot, uint8_t *req_data, uint8_t req_len)
{
  uint8_t completion_code = CC_UNSPECIFIED_ERROR;
  char error_log[512] = {0};
  char temp_log[256] = {0};
  uint16_t node = 0;
  int card = 0;
  int module = 0;
  uint16_t bank = 0;
  uint16_t dev = 0;
  uint32_t row = 0;
  uint16_t col = 0;
  uint16_t bit = 0;
  uint16_t rank = 0;
  uint8_t  ErrorType = 0;
  uint64_t phy_address = 0;
  uint8_t  Error_severity = 0;

  sprintf(temp_log, "SEL Entry: FRU: %d, ", slot);
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  Error_severity = req_data[24];
  sprintf(temp_log, "Error severity:%s, ", error_severity_strs[Error_severity]);
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  sprintf(temp_log, "Section Type:Memroy error, ");
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  if (req_data[25] & 0x2) {
    phy_address = (uint64_t)req_data[48] << 56 |
                  (uint64_t)req_data[47] << 48 |
                  (uint64_t)req_data[46] << 40 |
                  (uint64_t)req_data[45] << 32 |
                  (uint64_t)req_data[44] << 24 |
                  (uint64_t)req_data[43] << 16 |
                  (uint64_t)req_data[42] << 8 |
                  (uint64_t)req_data[41];
    sprintf(temp_log, "Phy_address:0x%llx, ", phy_address);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if(req_data[25] & 0x8) {
    node = req_data[58] << 8 | req_data[57];
    sprintf(temp_log, "Node:%d, ", node);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if(req_data[25] & 0x10) {
    card = req_data[60] << 8 | req_data[59];
    sprintf(temp_log, "Card:%d, ", card);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if(req_data[25] & 0x20) {
    module = req_data[62] << 8 | req_data[61];
    sprintf(temp_log, "Module:%d, ", module);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if(req_data[25] & 0x40) {
    bank = req_data[64] << 8 | req_data[63];
    sprintf(temp_log, "Bank:%d, ", bank);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if (req_data[25] & 0x80) {
    dev = req_data[66] << 8 | req_data[65];
    sprintf(temp_log, "Dev:%d, ", dev);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if (req_data[26] & 0x1) {
    row = req_data[68] << 8 | req_data[67];
    sprintf(temp_log, "Row:%d, ", row);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  } else if (req_data[27] & 0x4) {
    row = ((req_data[98] & 0x3) << 16) | req_data[68] << 8 | req_data[67];
    sprintf(temp_log, "Row:%d, ", row);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if (req_data[26] & 0x2) {
    col = req_data[70] << 8 | req_data[69];
    sprintf(temp_log, "Column:%d, ", col);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if (req_data[26] & 0x4) {
    bit = req_data[72] << 8 | req_data[71];
    sprintf(temp_log, "Bit_Position:%d, ", bit);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if (req_data[26] & 0x40) {
    ErrorType = req_data[97];
    if ( ErrorType < (sizeof(memory_error_type_strs) / sizeof(memory_error_type_strs[0]))) {
      sprintf(temp_log, "ErrorType:%s, ", memory_error_type_strs[ErrorType]);
    } else {
      sprintf(temp_log, "ErrorType:Unknown, ");
    }
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }

  if (req_data[26] & 0x80) {
    rank = req_data[100] << 8 | req_data[99];
    sprintf(temp_log, "Rank_Number:%d, ", rank);
    strcat(error_log, temp_log);
    strcpy(temp_log, "");
  }
  if(card == 0 && module == 0)
    sprintf(temp_log, "Location: DIMM:A");
  else if(card == 2 && module == 0)
    sprintf(temp_log, "Location: DIMM:B");
  else if(card == 3 && module == 0)
    sprintf(temp_log, "Location: DIMM:C");
  else if(card == 1 && module == 0)
    sprintf(temp_log, "Location: DIMM:D");
  strcat(error_log, temp_log);
  strcpy(temp_log, "");

  syslog(LOG_CRIT, "%s", error_log);
  completion_code = CC_SUCCESS;
  return completion_code;
}

uint8_t
pal_add_cper_log(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  uint8_t completion_code = CC_UNSPECIFIED_ERROR;

  if(memcmp(Memory_Error_Section, req_data+8, sizeof(Memory_Error_Section)) == 0) {
    //Memory Error Section
    memory_error_sec_sel_parse(slot, req_data, req_len);
  } else if(memcmp(PCIe_Error_Section, req_data+8, sizeof(PCIe_Error_Section)) == 0) {
    //PCI Express Error Section
    pci_express_error_sec_sel_parse(slot, req_data, req_len);
  } else if(memcmp(OEM_Error_Section, req_data+8, sizeof(OEM_Error_Section)) == 0) {
    //oem specific Error Section
    oem_error_sec_sel_parse(slot, req_data, req_len);
  }

  completion_code = CC_SUCCESS;
  return completion_code;
}

uint8_t
pal_add_imc_log(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {

  uint8_t completion_code = CC_UNSPECIFIED_ERROR;
  int index;
  FILE *pFile = NULL;
  uint8_t status;
  uint8_t data_len;
  struct stat st;
  char path[80] = {0};
  char cmd[128] = {0};


  *res_len = 0;
  status = req_data[0];
  data_len = req_data[1];

  pFile = fopen(imc_log_path[slot], "a+");
  if (pFile == NULL) {
    syslog(LOG_WARNING, "%s: failed to open path: %s", __func__, imc_log_path[slot]);
    return completion_code;
  }

  if (status == IMC_DUMP_START) {
    sprintf(path, IMC_START_FILE, slot);
    sprintf(cmd, "touch %s", path);
    system(cmd);
    syslog(LOG_INFO, "IMC Log Start on slot%u", slot);
  }

  if (status == IMC_DUMP_END) {
    sprintf(path, IMC_START_FILE, slot);
    if(access(path, F_OK) != 0) {
      syslog(LOG_INFO, "IMC Log Start on slot%u", slot);
    } else {
      sprintf(cmd, "rm %s", path);
      system(cmd);
    }
  }

  for (index = 2; index <= data_len + 1; index++) {
    if(req_data[index] == 0x00 || req_data[index] == 0x0D)    //ignore unnecessary ASCII character
      continue;
    fprintf(pFile, "%c", req_data[index]);
  }

  if (status == IMC_DUMP_END) {
    fprintf(pFile, "\n\n");
    syslog(LOG_INFO, "IMC Log Finish on slot%u", slot);

    stat(imc_log_path[slot], &st);
    if(st.st_size > IMC_LOG_FILE_SIZE) {   //Do IMC log backup action
      sprintf(path, IMC_LOG_FILE_BAK, slot);
      sprintf(cmd, "mv %s %s", imc_log_path[slot], path);
      system(cmd);

      memset(cmd, 0, sizeof(cmd));
      sprintf(cmd, "touch %s", imc_log_path[slot]);
      system(cmd);
    }
  }

  fclose(pFile);
  completion_code = CC_SUCCESS;
  return completion_code;
}

int
pal_set_fw_update_state(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  *res_len = 0;
  if (req_len != 2) {
    return CC_INVALID_LENGTH;
  }

  if (pal_set_fw_update_ongoing(slot, (req_data[1]<<8 | req_data[0]))) {
    return CC_UNSPECIFIED_ERROR;
  }

  return CC_SUCCESS;
}

uint8_t
pal_get_server_type(uint8_t fru) {

  uint8_t server_type = 0xFF;
  bic_get_server_type(fru, &server_type);

  return server_type;
}

int
pal_set_tpm_physical_presence(uint8_t slot, uint8_t presence) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};

  snprintf(key,MAX_KEY_LEN, "slot%u_tpm_phy_prsnt", slot);
  snprintf(str,MAX_VALUE_LEN, "%u",presence);
  return kv_set(key, str, 0, 0);
}

int
pal_get_tpm_physical_presence(uint8_t slot) {
  int ret;
  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  sprintf(key, "slot%u_tpm_phy_prsnt", slot);

  ret = kv_get(key, cvalue,NULL,0);
  if (ret) {
    return 0;
  }
  return atoi(cvalue);
}

int
pal_set_tpm_physical_presence_reset(uint8_t slot, uint8_t reset) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};

  snprintf(key,MAX_KEY_LEN, "slot%u_tpm_reset", slot);
  snprintf(str,MAX_VALUE_LEN, "%u",reset);
  return kv_set(key, str, 0, 0);
}

int
pal_get_tpm_physical_presence_reset(uint8_t slot) {
  int ret;
  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  sprintf(key, "slot%u_tpm_reset", slot);

  ret = kv_get(key, cvalue,NULL,0);
  if (ret) {
    return 0;
  }
  return atoi(cvalue);
}

int
pal_set_tpm_timeout(uint8_t slot, int timeout) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};

  snprintf(key,MAX_KEY_LEN, "slot%u_tpm_timeout", slot);
  snprintf(str,MAX_VALUE_LEN, "%d",timeout);
  return kv_set(key, str, 0, 0);
}

int
pal_get_tpm_timeout(uint8_t slot) {
  int ret;
  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  sprintf(key, "slot%u_tpm_timeout", slot);

  ret = kv_get(key, cvalue,NULL,0);
  if (ret) {
    //set default 600
    syslog(LOG_WARNING,"pal_get_tpm_timeout failed");
    return TPM_Timeout;
  }
  return atoi(cvalue);
}

void *
pal_check_start_TPMTimer(void* arg) { //called by daemon threads
  int slot_id = (int) arg;
  while (1) {
    if (pal_get_tpm_physical_presence(slot_id)) { //from 0 to 1
      int timeout = TPM_Timeout;
      int timer = 0;
      int presence = 1;

      timeout = pal_get_tpm_timeout(slot_id);
      syslog(LOG_WARNING, "[pal_check_start_TPMTimer][%lu] slot%d Start Timeout=%d", pthread_self(),slot_id,timeout);

      while (timer < timeout) {
        sleep(1);
        timer++;
        presence = pal_get_tpm_physical_presence(slot_id);
        if (presence == 0) { //if not present, stop timer
          break;
        }
        if (pal_get_tpm_physical_presence_reset(slot_id)) { //if reset, update timer and timeout
          pal_set_tpm_physical_presence_reset(slot_id, 0);
          timer = 0;
          timeout = pal_get_tpm_timeout(slot_id);
          syslog(LOG_WARNING, "[pal_check_start_TPMTimer][%lu] slot%d Reset Timeout: %d\n", pthread_self(),slot_id,timeout);
        }
      }

      //after timout set presence 1 to 0
      if (presence)
        pal_set_tpm_physical_presence(slot_id,0);
      syslog(LOG_WARNING, "[pal_check_start_TPMTimer][%lu] slot%d End Timer: %d\n", pthread_self(),slot_id,timer);
    }
    sleep(1);
  }
  pthread_exit(0);
}

int
pal_create_TPMTimer(int fru) {
  static pthread_t tpm_tid[MAX_NODES]={0};
  pthread_create(&tpm_tid[fru-1], NULL, pal_check_start_TPMTimer, (void*)fru);
  pthread_detach(tpm_tid[fru-1]);
  return 0;
}

int
pal_force_update_bic_fw(uint8_t slot_id, uint8_t comp, char *path) {
  return force_update_bic_fw(slot_id, comp, path);
}

void
pal_specific_plat_fan_check(bool status)
{
  uint8_t is_sled_out = 1;
  if (pal_get_fan_latch(&is_sled_out) != 0)
    syslog(LOG_WARNING, "%s: Get SLED status in/out failed", __func__);

  if(is_sled_out == 0)
    printf("Sled Fan Latch Open: False\n");
  else
    printf("Sled Fan Latch Open: True\n");

  return;
}

int pal_fsc_get_target_snr(const char *sname, struct fsc_monitor *fsc_m2_list, int fsc_m2_list_size)
{
  int index;
  for (index = 0; index < fsc_m2_list_size; index++)
  {
    if(!strcmp(sname, fsc_m2_list[index].sensor_name))
    {
      return index;
    }
  }

  return PAL_ENOTSUP;
}

int
pal_set_m2_prsnt(uint8_t slot_id, uint8_t dev_id, uint8_t present) {
  if(fby2_get_slot_type(slot_id)==SLOT_TYPE_GPV2) {
    char key[MAX_KEY_LEN] = {0};
    char str[MAX_VALUE_LEN] = {0};
    if (dev_id < 1 || dev_id > MAX_NUM_DEVS)
      return -1;
    //slot: 1 dev_id: 1 -> slot1_m2a_pres
    snprintf(key,MAX_KEY_LEN, "slot%u_m2%c_pres", slot_id, 'a'+dev_id-1);
    snprintf(str,MAX_VALUE_LEN, "%d",present);
    syslog(LOG_WARNING, "%s: set %s to %s.", __func__, key, str);
    return kv_set(key, str, 0, 0);
  }
  return -1;
}

bool pal_is_m2_prsnt(char *slot_name, char *sensor_name)
{
  uint8_t slot_id;
  uint8_t runoff_id;
  int ret;
  struct fsc_monitor *fsc_m2_list;
  int fsc_m2_list_size;
  int index;
  int fd = -1;
  char path[64];
  char value[MAX_VALUE_LEN] = {0};
  uint8_t m2_present_status;
  uint8_t read_byte = 1;

  ret = pal_get_fru_id(slot_name, &slot_id);
  if(ret){
    syslog(LOG_WARNING, "%s: pal_get_fru_id failed for %s", __func__, slot_name);
    return false;
  }

  if((slot_id < 1) || (slot_id > 4)) {
    syslog(LOG_WARNING, "%s: invalid slot id %d", __func__, slot_id);
    return false;
  }

  switch(fby2_get_slot_type(slot_id)) {
    case SLOT_TYPE_SERVER:
      fsc_m2_list = fsc_monitor_tl_m2_list;
      fsc_m2_list_size = fsc_monitor_tl_m2_list_size;
      runoff_id = slot_id;
      break;
    case SLOT_TYPE_GP:
      fsc_m2_list = fsc_monitor_gp_m2_list;
      fsc_m2_list_size = fsc_monitor_gp_m2_list_size;
      runoff_id = slot_id + 1;
      break;
    case SLOT_TYPE_GPV2:
      sprintf(path, "slot%u_xxx_pres", slot_id);
      memcpy(path+6, sensor_name, 3);
      if (kv_get(path, value, NULL, 0) == 0) {
        if (atoi(value))
          return true;
      }
      return false;
    default:
      return false;
  }

  index = pal_fsc_get_target_snr(sensor_name, fsc_m2_list, fsc_m2_list_size);

  if (index < 0)
    return true;

  sprintf(path, SYS_CONFIG_PATH "fru%d_m2_%d_info", runoff_id, fsc_m2_list[index].offset);
  fd = open(path, O_RDONLY);

  if (fd < 0)
    return false;

  if(read(fd, &m2_present_status, read_byte) != read_byte)
    return false;

  switch(m2_present_status) {
    case 0x01:     //M.2 is present
      break;
    case 0xFF:     //M.2 is not present
    default:
      close(fd);
      return false;
  }

  close(fd);
  return true;
}

int
pal_get_sensor_util_timeout(uint8_t fru) {
  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      return 10;
    case FRU_SPB:
    case FRU_NIC:
    default:
      return 4;
  }
}

int
pal_is_ocp30_nic(void) {
  int prsnt_a = 0, prsnt_b = 0;
  char path[64];

  sprintf(path, GPIO_VAL, GPIO_MEZZ_PRSNTA2_N);
  if (read_device(path, &prsnt_a)) {
    return 0;
  }

  sprintf(path, GPIO_VAL, GPIO_MEZZ_PRSNTB2_N);
  if (read_device(path, &prsnt_b)) {
    return 0;
  }

  if ((prsnt_a == 0) && (prsnt_b == 1)) {
    return 1;
  }

  return 0;
}
