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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include "pal.h"

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

#define DELAY_GRACEFUL_SHUTDOWN 1
#define DELAY_POWER_OFF 6
#define DELAY_POWER_CYCLE 10
#define DELAY_12V_CYCLE 5

#define CRASHDUMP_BIN       "/usr/local/bin/dump.sh"
#define CRASHDUMP_FILE      "/mnt/data/crashdump_"

#define LARGEST_DEVICE_NAME 120
#define PWM_DIR "/sys/devices/platform/ast_pwm_tacho.0"
#define PWM_UNIT_MAX 96

#define GUID_SIZE 16
#define OFFSET_DEV_GUID 0x1800
#define FRU_EEPROM "/sys/devices/platform/ast-i2c.8/i2c-8/8-0051/eeprom"

#define MAX_READ_RETRY 10
#define MAX_CHECK_RETRY 2

#define PLATFORM_FILE "/tmp/system.bin"
#define CRASHDUMP_KEY      "slot%d_crashdump"
#define SLOT_FILE "/tmp/slot.bin"

const static uint8_t gpio_rst_btn[] = { 0, GPIO_RST_SLOT1_SYS_RESET_N, GPIO_RST_SLOT2_SYS_RESET_N, GPIO_RST_SLOT3_SYS_RESET_N, GPIO_RST_SLOT4_SYS_RESET_N };
const static uint8_t gpio_led[] = { 0, GPIO_PWR1_LED, GPIO_PWR2_LED, GPIO_PWR3_LED, GPIO_PWR4_LED };      // TODO: In DVT, Map to ML PWR LED
const static uint8_t gpio_id_led[] = { 0,  GPIO_SYSTEM_ID1_LED_N, GPIO_SYSTEM_ID2_LED_N, GPIO_SYSTEM_ID3_LED_N, GPIO_SYSTEM_ID4_LED_N };  // Identify LED
const static uint8_t gpio_prsnt_prim[] = { 0, GPIO_SLOT1_PRSNT_N, GPIO_SLOT2_PRSNT_N, GPIO_SLOT3_PRSNT_N, GPIO_SLOT4_PRSNT_N };
const static uint8_t gpio_prsnt_ext[] = { 0, GPIO_SLOT1_PRSNT_B_N, GPIO_SLOT2_PRSNT_B_N, GPIO_SLOT3_PRSNT_B_N, GPIO_SLOT4_PRSNT_B_N };
const static uint8_t gpio_bic_ready[] = { 0, GPIO_I2C_SLOT1_ALERT_N, GPIO_I2C_SLOT2_ALERT_N, GPIO_I2C_SLOT3_ALERT_N, GPIO_I2C_SLOT4_ALERT_N };
const static uint8_t gpio_power[] = { 0, GPIO_PWR_SLOT1_BTN_N, GPIO_PWR_SLOT2_BTN_N, GPIO_PWR_SLOT3_BTN_N, GPIO_PWR_SLOT4_BTN_N };
const static uint8_t gpio_12v[] = { 0, GPIO_P12V_STBY_SLOT1_EN, GPIO_P12V_STBY_SLOT2_EN, GPIO_P12V_STBY_SLOT3_EN, GPIO_P12V_STBY_SLOT4_EN };

const char pal_fru_list[] = "all, slot1, slot2, slot3, slot4, spb, nic";
const char pal_server_list[] = "slot1, slot2, slot3, slot4";

size_t pal_pwm_cnt = 2;
size_t pal_tach_cnt = 2;
const char pal_pwm_list[] = "0, 1";
const char pal_tach_list[] = "0, 1";

uint8_t g_dev_guid[GUID_SIZE] = {0}; 

typedef struct {
  uint16_t flag;
  float ucr;
  float unc;
  float unr;
  float lcr;
  float lnc;
  float lnr;

} _sensor_thresh_t;

typedef struct {
  uint16_t flag;
  float ucr;
  float lcr;
  uint8_t retry_cnt;
  uint8_t val_valid;
  float last_val;

} sensor_check_t;

static sensor_check_t m_snr_chk[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

char * key_list[] = {
"pwr_server1_last_state",
"pwr_server2_last_state",
"pwr_server3_last_state",
"pwr_server4_last_state",
"sysfw_ver_slot1",
"sysfw_ver_slot2",
"sysfw_ver_slot3",
"sysfw_ver_slot4",
"identify_sled",
"identify_slot1",
"identify_slot2",
"identify_slot3",
"identify_slot4",
"timestamp_sled",
"slot1_por_cfg",
"slot2_por_cfg",
"slot3_por_cfg",
"slot4_por_cfg",
"slot1_sensor_health",
"slot2_sensor_health",
"slot3_sensor_health",
"slot4_sensor_health",
"spb_sensor_health",
"nic_sensor_health",
"slot1_sel_error",
"slot2_sel_error",
"slot3_sel_error",
"slot4_sel_error",
"slot1_boot_order",   
"slot2_boot_order",   
"slot3_boot_order",
"slot4_boot_order",
/* Add more Keys here */
LAST_KEY /* This is the last key of the list */
};

char * def_val_list[] = {
  "on", /* pwr_server1_last_state */
  "on", /* pwr_server2_last_state */
  "on", /* pwr_server3_last_state */
  "on", /* pwr_server4_last_state */
  "0", /* sysfw_ver_slot1 */
  "0", /* sysfw_ver_slot2 */
  "0", /* sysfw_ver_slot3 */
  "0", /* sysfw_ver_slot4 */
  "off", /* identify_sled */
  "off", /* identify_slot1 */
  "off", /* identify_slot2 */
  "off", /* identify_slot3 */
  "off", /* identify_slot4 */
  "0", /* timestamp_sled */
  "lps", /* slot1_por_cfg */
  "lps", /* slot2_por_cfg */
  "lps", /* slot3_por_cfg */
  "lps", /* slot4_por_cfg */
  "1", /* slot1_sensor_health */
  "1", /* slot2_sensor_health */
  "1", /* slot3_sensor_health */
  "1", /* slot4_sensor_health */
  "1", /* spb_sensor_health */
  "1", /* nic_sensor_health */
  "1", /* slot1_sel_error */
  "1", /* slot2_sel_error */
  "1", /* slot3_sel_error */
  "1", /* slot4_sel_error */
  "0000000", /* slot1_boot_order */  
  "0000000", /* slot2_boot_order */
  "0000000", /* slot3_boot_order */
  "0000000", /* slot4_boot_order */
  /* Add more def values for the correspoding keys*/
  LAST_KEY /* Same as last entry of the key_list */
};

struct power_coeff {
  float ein;
  float coeff;
};
/* Quanta BMC correction table */
struct power_coeff power_table[] = {
  {51.0,  0.98},
  {115.0, 0.9775},
  {178.0, 0.9755},
  {228.0, 0.979},
  {290.0, 0.98},
  {353.0, 0.977},
  {427.0, 0.977},
  {476.0, 0.9765},
  {526.0, 0.9745},
  {598.0, 0.9745},
  {0.0,   0.0}
};

/* Adjust power value */
static void
power_value_adjust(float *value)
{
    float x0, x1, y0, y1, x;
    int i;
    x = *value;
    x0 = power_table[0].ein;
    y0 = power_table[0].coeff;
    if (x0 > *value) {
      *value = x * y0;
      return;
    }
    for (i = 0; power_table[i].ein > 0.0; i++) {
       if (*value < power_table[i].ein)
         break;
      x0 = power_table[i].ein;
      y0 = power_table[i].coeff;
    }
    if (power_table[i].ein <= 0.0) {
      *value = x * y0;
      return;
    }
   //if value is bwtween x0 and x1, use linear interpolation method.
   x1 = power_table[i].ein;
   y1 = power_table[i].coeff;
   *value = (y0 + (((y1 - y0)/(x1 - x0)) * (x - x0))) * x;
   return;
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

  int ret;
  int i;

  i = 0;
  while(strcmp(key_list[i], LAST_KEY)) {

    // If Key is valid, return success
    if (!strcmp(key, key_list[i]))
      return 0;

    i++;
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_key_check: invalid key - %s", key);
#endif
  return -1;
}

int
pal_get_key_value(char *key, char *value) {

  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  return kv_get(key, value);
}
int
pal_set_key_value(char *key, char *value) {

  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  return kv_set(key, value);
}

// Power On the server in a given slot
static int
server_power_on(uint8_t slot_id) {
  char vpath[64] = {0};

  if (slot_id < 1 || slot_id > 4) {
    return -1;
  }
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

  return 0;
}

// Power Off the server in given slot
static int
server_power_off(uint8_t slot_id, bool gs_flag) {
  char vpath[64] = {0};

  if (slot_id < 1 || slot_id > 4) {
    return -1;
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

// Control 12V to the server in a given slot
static int
server_12v_on(uint8_t slot_id) {
  char vpath[64] = {0};

  if (slot_id < 1 || slot_id > 4) {
    return -1;
  }

  sprintf(vpath, GPIO_VAL, gpio_12v[slot_id]);

  if (write_device(vpath, "1")) {
    return -1;
  }

  return 0;
}

// Turn off 12V for the server in given slot
static int
server_12v_off(uint8_t slot_id) {
  char vpath[64] = {0};

  if (slot_id < 1 || slot_id > 4) {
    return -1;
  }

  sprintf(vpath, GPIO_VAL, gpio_12v[slot_id]);

  if (write_device(vpath, "0")) {
    return -1;
  }

  return 0;
}

// Debug Card's UART and BMC/SoL port share UART port and need to enable only
// one TXD i.e. either BMC's TXD or Debug Port's TXD.
static int
control_sol_txd(uint8_t slot) {
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
    ctrl &= (~UART1_TXD); //Disable
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
    ctrl &= (~UART2_TXD); // Disable
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
    ctrl &= (~UART3_TXD); // Disable
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
    ctrl &= (~UART4_TXD); // Disable
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
  int val;
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
  switch(fby2_get_slot_type(fru))
  {
    case SLOT_TYPE_SERVER:
      break;
    case SLOT_TYPE_CF:
    case SLOT_TYPE_GP:
      return 0;
      break;
  }

  return 1;
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

// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t slot_id, uint8_t cmd) {
  int ret;
  uint8_t status;
  bool gs_flag = false;

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

    case SERVER_GRACEFUL_SHUTDOWN:
      if (status == SERVER_POWER_OFF)
        return 1;
      else
        gs_flag = true;
        return server_power_off(slot_id, gs_flag);
      break;

    case SERVER_12V_ON:
      return server_12v_on(slot_id);

    case SERVER_12V_OFF:
      return server_12v_off(slot_id);

    case SERVER_12V_CYCLE:
      if (server_12v_off(slot_id)) {
        return -1;
      }

      sleep(DELAY_12V_CYCLE);

      return (server_12v_on(slot_id));

    case SERVER_GLOBAL_RESET:
      return server_power_off(slot_id, false);

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
  system("i2cset -y 10 0x40 0xd9 c");

  return 0;
}

// Read the Front Panel Hand Switch and return the position
int
pal_get_hand_sw(uint8_t *pos) {
  char path[64] = {0};
  int id1, id2, id4, id8;
  uint8_t loc;
  // Read 4 GPIOs to read the current position
  // id1: GPIOAA4(GPIO_HAND_SW_ID1)
  // id2: GPIOAA5(GPIO_HAND_SW_ID2)
  // id4: GPIOAA6(GPIO_HAND_SW_ID3)
  // id8: GPIOAA7(GPIO_HAND_SW_ID4)

  // Read ID1
  sprintf(path, GPIO_VAL, GPIO_HAND_SW_ID1);
  if (read_device(path, &id1)) {
    return -1;
  }

  // Read ID2
  sprintf(path, GPIO_VAL, GPIO_HAND_SW_ID2);
  if (read_device(path, &id2)) {
    return -1;
  }

  // Read ID4
  sprintf(path, GPIO_VAL, GPIO_HAND_SW_ID4);
  if (read_device(path, &id4)) {
    return -1;
  }

  // Read ID8
  sprintf(path, GPIO_VAL, GPIO_HAND_SW_ID8);
  if (read_device(path, &id8)) {
    return -1;
  }

  loc = ((id8 << 3) | (id4 << 2) | (id2 << 1) | (id1));

  switch(loc) {
  case 0:
  case 5:
    *pos = HAND_SW_SERVER1;
    break;
  case 1:
  case 6:
    *pos = HAND_SW_SERVER2;
    break;
  case 2:
  case 7:
    *pos = HAND_SW_SERVER3;
    break;
  case 3:
  case 8:
    *pos = HAND_SW_SERVER4;
    break;
  default:
    *pos = HAND_SW_BMC;
    break;
  }

  return 0;
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
  }

  // Disable GPIOD pass-through function
  system("devmem 0x1e6e207c w 0x600000");

  sprintf(path, GPIO_VAL, gpio_rst_btn[slot]);
  if (write_device(path, val)) {
    // Enable GPIOD pass-through function
    system("devmem 0x1e6e2070 w 0xF12AE206");
    return -1;
  }

  // Enable GPIOD pass-through function
  system("devmem 0x1e6e2070 w 0xF12AE206");
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
  if (!val == state)
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
    gpio_sw0 = "0";
    gpio_sw1 = "1";
    break;
  case HAND_SW_SERVER3:
    gpio_sw0 = "1";
    gpio_sw1 = "0";
    break;
  case HAND_SW_SERVER4:
    gpio_sw0 = "1";
    gpio_sw1 = "1";
    break;
  default:
    return 0;
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

  // Based on the USB mux table in Schematics
  switch(slot) {
  case HAND_SW_SERVER1:
    gpio_sw0 = "0";
    gpio_sw1 = "0";
    break;
  case HAND_SW_SERVER2:
    gpio_sw0 = "0";
    gpio_sw1 = "1";
    break;
  case HAND_SW_SERVER3:
    gpio_sw0 = "1";
    gpio_sw1 = "0";
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

  // Enable the USB MUX
  if (set_usb_mux(USB_MUX_ON) < 0)
    return -1;

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
  int ret;

  // Refer the UART select table in schematic
  switch(slot) {
  case HAND_SW_SERVER1:
    gpio_uart_sel2 = "0";
    gpio_uart_sel1 = "0";
    gpio_uart_sel0 = "0";
    gpio_uart_rx = "0";
    break;
  case HAND_SW_SERVER2:
    gpio_uart_sel2 = "0";
    gpio_uart_sel1 = "0";
    gpio_uart_sel0 = "1";
    gpio_uart_rx = "0";
    break;
  case HAND_SW_SERVER3:
    gpio_uart_sel2 = "0";
    gpio_uart_sel1 = "1";
    gpio_uart_sel0 = "0";
    gpio_uart_rx = "0";
    break;
  case HAND_SW_SERVER4:
    gpio_uart_sel2 = "0";
    gpio_uart_sel1 = "1";
    gpio_uart_sel0 = "1";
    gpio_uart_rx = "0";
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
  ret = control_sol_txd(slot);
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
  int i;
  bic_config_t config = {0};
  bic_config_u *t = (bic_config_u *) &config;

  ret = bic_get_config(slot, &config);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "post_enable: bic_get_config failed for fru: %d\n", slot);
#endif
    return ret;
  }

  t->bits.post = 1;

  ret = bic_set_config(slot, &config);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "post_enable: bic_set_config failed\n");
#endif
    return ret;
  }

  return 0;
}

// Disable POST buffer for the server in given slot
int
pal_post_disable(uint8_t slot) {
  int ret;
  int i;
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
  int i;

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
pal_get_fru_name(uint8_t fru, char *name) {

  return fby2_common_fru_name(fru, name);
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
        case SLOT_TYPE_SERVER:
            *sensor_list = (uint8_t *) bic_sensor_list;
            *cnt = bic_sensor_cnt;
            break;
        case SLOT_TYPE_CF:
            //To do
            break;
        case SLOT_TYPE_GP:
            *sensor_list = (uint8_t *) dc_sensor_list;
            *cnt = dc_sensor_cnt;
            break;
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
pal_fruid_write(uint8_t fru, char *path) {
  return bic_write_fruid(fru, 0, path);
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

int
pal_sensor_read(uint8_t fru, uint8_t sensor_num, void *value) {

  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int ret;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(key, "slot%d_sensor%d", fru, sensor_num);
      break;
    case FRU_SPB:
      sprintf(key, "spb_sensor%d", sensor_num);
      break;
    case FRU_NIC:
      sprintf(key, "nic_sensor%d", sensor_num);
      break;
  }

  ret = edb_cache_get(key, str);
  if(ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_sensor_read: cache_get %s failed.", key);
#endif
    return ret;
  }
  if(strcmp(str, "NA") == 0)
    return -1;
  *((float*)value) = atof(str);
  return ret;
}
int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {

  uint8_t status;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int ret;
  uint8_t retry = MAX_READ_RETRY;
  sensor_check_t *snr_chk;

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
    if(ret >= 0)
      break;
    msleep(50);
    retry--;
  }
  if(ret < 0) {
    snr_chk->val_valid = 0;

    if(fru == FRU_SPB || fru == FRU_NIC)
      return -1;
    if(pal_get_server_power(fru, &status) < 0)
      return -1;
    // This check helps interpret the IPMI packet loss scenario
    if(status == SERVER_POWER_ON)
      return -1;
    strcpy(str, "NA");
  }
  else {
    // On successful sensor read
    if(fru == FRU_SPB && sensor_num == SP_SENSOR_HSC_IN_POWER) {
      power_value_adjust(value);
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

    sprintf(str, "%.2f",*((float*)value));
  }

  if(edb_cache_set(key, str) < 0) {
#ifdef DEBUG
     syslog(LOG_WARNING, "pal_sensor_read_raw: cache_set key = %s, str = %s failed.", key, str);
#endif
    return -1;
  }
  else {
    return ret;
  }
}

int
pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag) {

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      if (snr_num == BIC_SENSOR_SOC_THERM_MARGIN)
        *flag = GETMASK(SENSOR_VALID) | GETMASK(UCR_THRESH);
      else if (snr_num == BIC_SENSOR_SOC_PACKAGE_PWR)
        *flag = GETMASK(SENSOR_VALID);
      else if (snr_num == BIC_SENSOR_SOC_TJMAX)
        *flag = GETMASK(SENSOR_VALID);
      break;
    case FRU_SPB:
      /*
       * TODO: This is a HACK (t11229576)
       */
      switch(snr_num) {
        case SP_SENSOR_P12V_SLOT1:
        case SP_SENSOR_P12V_SLOT2:
        case SP_SENSOR_P12V_SLOT3:
        case SP_SENSOR_P12V_SLOT4:
          *flag = GETMASK(SENSOR_VALID);
          break;
      }
    case FRU_NIC:
      break;
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
  return fby2_get_fruid_path(fru, path);
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
  char kpath[MAX_KEY_PATH_LEN] = {0};

  i = 0;
  while(strcmp(key_list[i], LAST_KEY)) {

    memset(key, 0, MAX_KEY_LEN);
    memset(kpath, 0, MAX_KEY_PATH_LEN);

    sprintf(kpath, KV_STORE, key_list[i]);

    if (access(kpath, F_OK) == -1) {

      if ((ret = kv_set(key_list[i], def_val_list[i])) < 0) {
#ifdef DEBUG
          syslog(LOG_WARNING, "pal_set_def_key_value: kv_set failed. %d", ret);
#endif
      }
    }

    i++;
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
  int i;
  int ret;

  char value[MAX_VALUE_LEN] = {0x0};

  while (strcmp(key_list[i], LAST_KEY)) {
    printf("%s:", key_list[i]);
    if (ret = kv_get(key_list[i], value) < 0) {
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
}

// Write GUID into EEPROM
static int
pal_set_guid(uint16_t offset, char *guid) {
  int fd = 0;
  uint64_t tmp[GUID_SIZE];
  ssize_t bytes_wr;
  int i = 0;

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
  uint64_t tmp[GUID_SIZE];
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
pal_populate_guid(uint8_t *guid, uint8_t *str) {
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
  int ret;
  int i=0;
  uint8_t guid[GUID_SIZE] = {0x00};

  pal_populate_guid(&guid, str);

  return bic_set_sys_guid(slot, &guid);
}

int
pal_get_sys_guid(uint8_t slot, char *guid) {
  int ret;

  return bic_get_sys_guid(slot, guid);
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

int
pal_get_80port_record(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
    
    int ret;
    int completion_code=CC_UNSPECIFIED_ERROR;
    // Send command to get 80 port record from Bridge IC
    ret = bic_request_post_buffer_data(slot, res_data, res_len);

    if(0 == ret)
       completion_code = CC_SUCCESS;

    return completion_code;
}

int
pal_is_bmc_por(void) {
  uint32_t scu_fd;
  uint32_t wdt;
  void *scu_reg;
  void *scu_wdt;

  scu_fd = open("/dev/mem", O_RDWR | O_SYNC );
  if (scu_fd < 0) {
    return 0;
  }

  scu_reg = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, scu_fd,
             AST_SCU_BASE);
  scu_wdt = (char*)scu_reg + WDT_OFFSET;

  wdt = *(volatile uint32_t*) scu_wdt;

  munmap(scu_reg, PAGE_SIZE);
  close(scu_fd);

  if (wdt & 0x6) {
    return 0;
  } else {
    return 1;
  }
}

int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      *sensor_list = (uint8_t *) bic_discrete_list;
      *cnt = bic_discrete_cnt;
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
}

static int
pal_store_crashdump(uint8_t fru) {

  return fby2_common_crashdump(fru);
}

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
      switch(snr_num) {
        case CATERR:
          sprintf(key, CRASHDUMP_KEY, fru);
          edb_cache_set(key, "1");
          pal_store_crashdump(fru);
          edb_cache_set(key, "0");
          break;

        case 0x00:  // don't care sensor number 00h
          return 0;
      }
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

  switch(snr_num) {
    case SYSTEM_EVENT:
      sprintf(name, "SYSTEM_EVENT");
      break;
    case THERM_THRESH_EVT:
      sprintf(name, "THERM_THRESH_EVT");
      break;
    case CRITICAL_IRQ:
      sprintf(name, "CRITICAL_IRQ");
      break;
    case POST_ERROR:
      sprintf(name, "POST_ERROR");
      break;
    case MACHINE_CHK_ERR:
      sprintf(name, "MACHINE_CHK_ERR");
      break;
    case PCIE_ERR:
      sprintf(name, "PCIE_ERR");
      break;
    case IIO_ERR:
      sprintf(name, "IIO_ERR");
      break;
    case MEMORY_ECC_ERR:
      sprintf(name, "MEMORY_ECC_ERR");
      break;
    case PWR_ERR:
      sprintf(name, "PWR_ERR");
      break;
    case CATERR:
      sprintf(name, "CATERR");
      break;
    case CPU_DIMM_HOT:
      sprintf(name, "CPU_DIMM_HOT");
      break;
    case CPU0_THERM_STATUS:
      sprintf(name, "CPU0_THERM_STATUS");
      break;
    case SPS_FW_HEALTH:
      sprintf(name, "SPS_FW_HEALTH");
      break;
    case NM_EXCEPTION:
      sprintf(name, "NM_EXCEPTION");
      break;
    case PWR_THRESH_EVT:
      sprintf(name, "PWR_THRESH_EVT");
      break;
    default:
      sprintf(name, "Unknown");
      break;
  }

  return 0;
}

int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log) {
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];
  char *event_data = &sel[10];
  char *ed = &event_data[3];
  char temp_log[128] = {0};
  uint8_t temp;
  uint8_t sen_type = event_data[0];

  switch (snr_type) {
    case OS_BOOT:
      // OS_BOOT used by OS
      sprintf(error_log, "");
      switch (ed[0] & 0xF) {
        case 0x07:
          strcat(error_log, "Base OS/Hypervisor Installation started");
          break;
        case 0x08:
          strcat(error_log, "Base OS/Hypervisor Installation completed");
          break;
        case 0x09:
          strcat(error_log, "Base OS/Hypervisor Installation aborted");
          break;
        case 0x0A:
          strcat(error_log, "Base OS/Hypervisor Installation failed");
          break;
        default:
          strcat(error_log, "Unknown");
          break;
      }
      return 0;
  }

  switch(snr_num) {
    case SYSTEM_EVENT:
      sprintf(error_log, "");
      if (ed[0] == 0xE5) {
        strcat(error_log, "Cause of Time change - ");

        if (ed[2] == 0x00)
          strcat(error_log, "NTP");
        else if (ed[2] == 0x01)
          strcat(error_log, "Host RTL");
        else if (ed[2] == 0x02)
          strcat(error_log, "Set SEL time cmd ");
        else if (ed[2] == 0x03)
          strcat(error_log, "Set SEL time UTC offset cmd");
        else
          strcat(error_log, "Unknown");

        if (ed[1] == 0x00)
          strcat(error_log, " - First Time");
        else if(ed[1] == 0x80)
          strcat(error_log, " - Second Time");

      }
      break;

    case THERM_THRESH_EVT:
      sprintf(error_log, "");
      if (ed[0] == 0x1)
        strcat(error_log, "Limit Exceeded");
      else
        strcat(error_log, "Unknown");
      break;

    case CRITICAL_IRQ:
      sprintf(error_log, "");
      if (ed[0] == 0x0)
        strcat(error_log, "NMI / Diagnostic Interrupt");
      else if (ed[0] == 0x03)
        strcat(error_log, "Software NMI");
      else
        strcat(error_log, "Unknown");
      break;

    case POST_ERROR:
      sprintf(error_log, "");
      if ((ed[0] & 0x0F) == 0x0)
        strcat(error_log, "System Firmware Error");
      else
        strcat(error_log, "Unknown");
      if (((ed[0] >> 6) & 0x03) == 0x3) {
        // TODO: Need to implement IPMI spec based Post Code
        strcat(error_log, ", IPMI Post Code");
       } else if (((ed[0] >> 6) & 0x03) == 0x2) {
         sprintf(temp_log, ", OEM Post Code 0x%X 0x%X", ed[2], ed[1]);
         strcat(error_log, temp_log);
       }
      break;

    case MACHINE_CHK_ERR:
      sprintf(error_log, "");
      if ((ed[0] & 0x0F) == 0x0B) {
        strcat(error_log, "Uncorrectable");
      } else if ((ed[0] & 0x0F) == 0x0C) {
        strcat(error_log, "Correctable");
      } else {
        strcat(error_log, "Unknown");
      }

      sprintf(temp_log, ", Machine Check bank Number %d ", ed[1]);
      strcat(error_log, temp_log);
      sprintf(temp_log, ", CPU %d, Core %d ", ed[2] >> 5, ed[2] & 0x1F);
      strcat(error_log, temp_log);

      break;

    case PCIE_ERR:
      sprintf(error_log, "");
      if ((ed[0] & 0xF) == 0x4)
        strcat(error_log, "PCI PERR");
      else if ((ed[0] & 0xF) == 0x5)
        strcat(error_log, "PCI SERR");
      else if ((ed[0] & 0xF) == 0x7)
        strcat(error_log, "Correctable");
      else if ((ed[0] & 0xF) == 0x8)
        strcat(error_log, "Uncorrectable");
      else if ((ed[0] & 0xF) == 0xA)
        strcat(error_log, "Bus Fatal");
      else
        strcat(error_log, "Unknown");
      break;

    case IIO_ERR:
      sprintf(error_log, "");
      if ((ed[0] & 0xF) == 0) {

        sprintf(temp_log, "CPU %d, Error ID 0x%X", (ed[2] & 0xE0) >> 5,
            ed[1]);
        strcat(error_log, temp_log);

        temp = ed[2] & 0x7;
        if (temp == 0x0)
          strcat(error_log, " - IRP0");
        else if (temp == 0x1)
          strcat(error_log, " - IRP1");
        else if (temp == 0x2)
          strcat(error_log, " - IIO-Core");
        else if (temp == 0x3)
          strcat(error_log, " - VT-d");
        else if (temp == 0x4)
          strcat(error_log, " - Intel Quick Data");
        else if (temp == 0x5)
          strcat(error_log, " - Misc");
        else
          strcat(error_log, " - Reserved");
      } else
        strcat(error_log, "Unknown");
      break;

    case MEMORY_ECC_ERR:
      sprintf(error_log, "");
      if ((ed[0] & 0x0F) == 0x0) {
        if (sen_type == 0x0C)
          strcat(error_log, "Correctable");
        else if (sen_type == 0x10)
          strcat(error_log, "Correctable ECC error Logging Disabled");
      } else if ((ed[0] & 0x0F) == 0x1)
        strcat(error_log, "Uncorrectable");
      else if ((ed[0] & 0x0F) == 0x5)
        strcat(error_log, "Correctable ECC error Logging Limit Reached");
      else
        strcat(error_log, "Unknown");

      if (((ed[1] & 0xC) >> 2) == 0x0) {
        /* All Info Valid */
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

    case PWR_ERR:
      sprintf(error_log, "");
      if (ed[0] == 0x2)
        strcat(error_log, "PCH_PWROK failure");
      else
        strcat(error_log, "Unknown");
      break;

    case CATERR:
      sprintf(error_log, "");
      if (ed[0] == 0x0)
        strcat(error_log, "IERR");
      else if (ed[0] == 0xB)
        strcat(error_log, "MCERR");
      else
        strcat(error_log, "Unknown");
      break;

    case CPU_DIMM_HOT:
      sprintf(error_log, "");
      if ((ed[0] << 16 | ed[1] << 8 | ed[2]) == 0x01FFFF)
        strcat(error_log, "SOC MEMHOT");
      else
        strcat(error_log, "Unknown");
      break;

    case SPS_FW_HEALTH:
      sprintf(error_log, "");
      if (event_data[0] == 0xDC && ed[1] == 0x06) {
        strcat(error_log, "FW UPDATE");
        return 1;
      } else
         strcat(error_log, "Unknown");
      break;

    default:
      sprintf(error_log, "Unknown");
      break;
  }

  return 0;
}

// Helper function for msleep
void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
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
read_fan_value(const int fan, const char *device, int *value) {
  char device_name[LARGEST_DEVICE_NAME];
  char output_value[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR,device_name);
  return read_device(full_name, value);
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
    write_fan_value(fan, "pwm%d_en", 0);
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

   // Redirect FAN to sensor cache
   ret = pal_sensor_read(FRU_SPB, SP_SENSOR_FAN0_TACH + fan, &value);

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
  sprintf(tstr, "%d", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

int
pal_handle_dcmi(uint8_t fru, uint8_t *request, uint8_t req_len, uint8_t *response, uint8_t *rlen) {
  return bic_me_xmit(fru, request, req_len, response, rlen);
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

      sprintf(key, "slot%d_boot_order",slot);

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
      sprintf(key, "slot%d_boot_order",slot);

      for (i = 0; i < SIZE_BOOT_ORDER; i++) {
	    snprintf(tstr, 3, "%02x", boot[i]);
   	    strncat(str, tstr, 3);
      }
	 
      return pal_set_key_value(key, str);
}

int
pal_set_dev_guid(uint8_t slot, char *guid) {
      pal_populate_guid(g_dev_guid, guid);

      return pal_set_guid(OFFSET_DEV_GUID + (slot - 1)*GUID_SIZE, g_dev_guid);
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {
      pal_get_guid(OFFSET_DEV_GUID + (fru - 1)*GUID_SIZE, g_dev_guid);
      memcpy(guid, g_dev_guid, GUID_SIZE);

      return 0;
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

//Use part of the function for OEM Command "CMD_OEM_GET_POSS_PCIE_CONFIG" 0xF4
int pal_get_poss_pcie_config(uint8_t *pcie_config){
   return 0;
}

int
pal_get_platform_id(uint8_t *id) {
   return 0;
}

void
pal_sensor_assert_handle(uint8_t snr_num, float val, uint8_t thresh) {
  return;
}

void
pal_sensor_deassert_handle(uint8_t snr_num, float val, uint8_t thresh) {
  return;
}
void
pal_set_post_end(void) {
  return;
}

void pal_post_end_chk(uint8_t *post_end_chk) {
  return;
}

int
pal_get_fw_info(unsigned char target, unsigned char* res, unsigned char* res_len)
{
    return -1;
}

int
pal_is_crashdump_ongoing(uint8_t slot)
{
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int ret;
  sprintf(key, CRASHDUMP_KEY, slot);
  ret = edb_cache_get(key, value);
  if (ret < 0) {
#ifdef DEBUG
     syslog(LOG_INFO, "pal_get_crashdumpe: failed");
#endif
     return 0;
  }
  if (atoi(value) > 0)
     return 1;
  return 0;
}

int
pal_is_fw_update_ongoing(uint8_t fru) {

  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  int ret;
  struct timespec ts;

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(key, "slot%d_fwupd", fru);
      break;
    case FRU_SPB:
    case FRU_NIC:
    default:
      return 0;
  }

  ret = edb_cache_get(key, value);
  if (ret < 0) {
     return 0;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec)
     return 1;

  return 0;
}

int
pal_init_sensor_check(uint8_t fru, uint8_t snr_num, void *snr) {

  sensor_check_t *snr_chk;
  _sensor_thresh_t *psnr = (_sensor_thresh_t *)snr;

  snr_chk = get_sensor_check(fru, snr_num);
  snr_chk->flag = psnr->flag;
  snr_chk->ucr = psnr->ucr;
  snr_chk->lcr = psnr->lcr;
  snr_chk->retry_cnt = 0;
  snr_chk->val_valid = 0;
  snr_chk->last_val = 0;

  return 0;
}

void pal_add_cri_sel(char *str)
{

}

int
pal_bmc_err_enable() {
  // dummy function
  return 0;
}

int
pal_bmc_err_disable() {
  // dummy function
  return 0;
}

int
pal_set_bios_current_boot_list(uint8_t slot, uint8_t *boot_list, uint8_t list_length, uint8_t *cc) {
  return 0;
}

int
pal_get_bios_current_boot_list(uint8_t slot, uint8_t *boot_list, uint8_t *list_length) {
  return 0;
}

int
pal_set_bios_fixed_boot_device(uint8_t slot, uint8_t *fixed_boot_device) {
  return 0;
}

int
pal_get_bios_fixed_boot_device(uint8_t slot, uint8_t *fixed_boot_device) {
  return 0;
}

int
pal_set_bios_restores_default_setting(uint8_t slot, uint8_t *default_setting) {
  return 0;
}

int
pal_get_bios_restores_default_setting(uint8_t slot, uint8_t *default_setting) {
  return 0;
}

// TODO: Extend pal_get_status to support multiple servers
// For now just, return the value of 'slot1_por_cfg' for all servers
uint8_t
pal_get_status(void) {
  char str_server_por_cfg[64];
  char *buff[MAX_VALUE_LEN];
  int policy = 3;
  uint8_t status, data, ret;

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

unsigned char option_offset[] = {0,1,2,3,4,6,11,20,37,164};
unsigned char option_size[]   = {1,1,1,1,2,5,9,17,127};

void
pal_set_boot_option(unsigned char para,unsigned char* pbuff)
{
  return;
}

int
pal_get_boot_option(unsigned char para,unsigned char* pbuff)
{
  unsigned char size = option_size[para];
  memset(pbuff, 0, size);
  return size;
}
