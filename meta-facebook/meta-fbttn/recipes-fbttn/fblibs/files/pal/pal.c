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

#define FBTTN_PLATFORM_NAME "FBTTN"
#define LAST_KEY "last_key"
#define FBTTN_MAX_NUM_SLOTS 1
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
 * IOM_FULL_PGOOD       GPIOAB2  218
 * BMC_LOC_HEARTBEAT    GPIOO1   113
 * BMC_UART_SEL         GPIOS1   145 // output; 0:cpu 1:bmc
 * DEBUG_HDR_UART_SEL   GPIOS2   146 // input
 * DB_PRSNT_BMC_N       GPIOQ6   134
 * SYS_PWR_LED          GPIOA3   3
 * ENCL_FAULT_LED       GPIOO3   115
 * IOM_TYPE0            GPIOJ4   76
 * IOM_TYPE1            GPIOJ5   77
 * IOM_TYPE2            GPIOJ6   78
 * IOM_TYPE3            GPIOJ7   79
 * */

//Update at 9/12/2016 for Triton
#define GPIO_PWR_BTN 26
#define GPIO_PWR_BTN_N 27
#define GPIO_RST_BTN 28
#define GPIO_SYS_RST_BTN 29
#define GPIO_COMP_PWR_EN 119
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

#define BMC_EXT1_LED_Y 37
#define BMC_EXT2_LED_Y 39

#define GPIO_UART_SEL 145
#define GPIO_DEBUG_HDR_UART_SEL 146

#define GPIO_POSTCODE_0 56
#define GPIO_POSTCODE_1 57
#define GPIO_POSTCODE_2 58
#define GPIO_POSTCODE_3 59
#define GPIO_POSTCODE_4 60
#define GPIO_POSTCODE_5 61
#define GPIO_POSTCODE_6 62
#define GPIO_POSTCODE_7 63

#define GPIO_DBG_CARD_PRSNT 134
//??
#define GPIO_BMC_READY_N    28

#define PAGE_SIZE  0x1000
#define AST_SCU_BASE 0x1e6e2000
#define PIN_CTRL1_OFFSET 0x80
#define PIN_CTRL2_OFFSET 0x84
#define WDT_OFFSET 0x3C

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
#define PWM_UNIT_MAX 96

#define TACH_RPM "/sys/devices/platform/ast_pwm_tacho.0/tacho%d_rpm"
#define TACH_BMC_RMT_HB 0
#define TACH_SCC_LOC_HB 4
#define TACH_SCC_RMT_HB 5

#define PLATFORM_FILE "/tmp/system.bin"
// SHIFT to 16
#define UART1_TXD 0

unsigned char g_err_code[ERROR_CODE_NUM];

 /*For Triton Power Sequence
  * After BMC ready
  *
  * 1.       Output : SCC_STBY_PWR_EN
  * //2. Check Input  : SCC_STBY_PWR_GOOD //OVER GPIO_EXP
  * 3.       Output : SCC_LOC_FULL_PWR_EN
  * //4. Check Input  : SCC_LOC_FULL_PWR_GOOD //OVER GPIO_EXP
  *  5.      Output : IOM_FULL_PWR_EN
  *  6.  Check Input  :IOM_FULL_PGOOD
*/
const static uint8_t gpio_rst_btn[] = { 0, GPIO_SYS_RST_BTN };
const static uint8_t gpio_led[] = { 0, GPIO_PWR_LED };      // TODO: In DVT, Map to ML PWR LED
const static uint8_t gpio_id_led[] = { 0,  GPIO_PWR_LED };  // Identify LED
//const static uint8_t gpio_prsnt[] = { 0, 61 };
//const static uint8_t gpio_bic_ready[] = { 0, 107 };
const static uint8_t gpio_power[] = { 0, GPIO_PWR_BTN_N };
const static uint8_t gpio_12v[] = { 0, GPIO_COMP_PWR_EN };
const char pal_fru_list[] = "all, slot1, iom, scc, dpb, nic";
const char pal_server_list[] = "slot1";

size_t pal_pwm_cnt = 2;
size_t pal_tach_cnt = 2;
const char pal_pwm_list[] = "0, 1";
const char pal_tach_list[] = "0, 1";

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
"slot1_sel_error",
"scc_sensor_timestamp",
"dpb_sensor_timestamp",
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
  "1", /* slot_sel_error */
  "0", /* scc_sensor_timestamp */
  "0", /* dpb_sensor_timestamp */
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
server_power_off(uint8_t slot_id, bool gs_flag, bool cycle_flag) {
  char vpath[64] = {0};
  uint8_t status;
  int retry = 0;

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

#ifdef CONFIG_FBTTN
  // When ML-CPU is made sure shutdown that is not power-cycle, we should power-off M.2/IOC by BMC.
  if (cycle_flag == false) {
    do {
      if (pal_get_server_power(slot_id, &status) < 0) {
        return -1;
      }
      sleep(DELAY_FULL_POWER_DOWN);
      if (retry > RETRY_COUNT) {
        return -1;
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
  }
#endif

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

  if (slot_id < 1 || slot_id > 1) {
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
control_sol_txd(uint8_t fru) {
  #if 0
  // BMC IO1 <-> UART1 don't need to rout
  #endif
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
  *num = FBTTN_MAX_NUM_SLOTS;

  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  int val;
  char path[64] = {0};

  switch (fru) {
    case FRU_SLOT1:
    //only one slot; TBD,if there is a Mono Lake prsnt
    *status = 1;
      break;
    case FRU_IOM:
    case FRU_DPB:
    case FRU_SCC:
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

  switch (fru) {
    case FRU_SLOT1:
    //TODO: is_SCC_ready needs to be implemented
   case FRU_IOM:
   case FRU_DPB:
   case FRU_SCC:
   case FRU_NIC:
     *status = 1;
     break;
   default:
      return -1;
  }

  return 0;
}

int
pal_is_server_12v_on(uint8_t slot_id, uint8_t *status) {

  int val;
  char path[64] = {0};

  if (slot_id < 1 || slot_id > 1) {
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

  return 0;
}

// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t slot_id, uint8_t cmd) {
  uint8_t status;
  bool gs_flag = false;
  bool cycle_flag = false;

  if (slot_id < 1 || slot_id > 1) {
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

        return (server_power_on(slot_id));
      }
      break;

    case SERVER_GRACEFUL_SHUTDOWN:
      if (status == SERVER_POWER_OFF)
        return 1;
      else
        gs_flag = true;
        return server_power_off(slot_id, gs_flag, cycle_flag);
      break;

    case SERVER_12V_ON:
      if (status == SERVER_12V_ON)
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

      return (server_12v_on(slot_id));
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
  system("i2cset -y 6 0x20 0xd9 c");

  return 0;
}

// Read the Front Panel Hand Switch and return the position
int
pal_get_hand_sw(uint8_t *pos) {
  static int prev_state = -1;
  static uint8_t prev_uart = HAND_SW_BMC;
  static int count = 0;
  int curr_state = -1;
  uint8_t curr_uart = -1;
  char path[64] = {0};

  // GPIO_DEBUG_HDR_UART_SEL: GPIOS2 (146)
  sprintf(path, GPIO_VAL, GPIO_DEBUG_HDR_UART_SEL);
  if (read_device(path, &curr_state)) {
    return -1;
  }

  // When a status changed (a pulse), switch the UART selection
  if (curr_state != prev_state) {
    count++;
    if (count == 2) {
      curr_uart = ~(prev_uart & 0x1);   // 0: HAND_SW_BMC; 1: HAND_SW_SERVER1
      count = 0;
    }
  } else {
    curr_uart = prev_uart;
  }

  *pos = curr_uart;
  prev_state = curr_state;
  prev_uart = curr_uart;

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

  sprintf(path, GPIO_VAL, gpio_rst_btn[slot]);
  if (write_device(path, val)) {
    return -1;
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

// Switch the UART mux to the given slot
int
pal_switch_uart_mux(uint8_t fru) {
  char * gpio_uart_sel;
  char path[64] = {0};
  int ret;

	if(fru != 0)//BMC
	  gpio_uart_sel = "1";
	else
	  gpio_uart_sel = "0";

  sprintf(path, GPIO_VAL, GPIO_UART_SEL);
  ret = write_device(path, gpio_uart_sel);
  return ret;
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

  switch(fru) {
    case FRU_SLOT1:
      *sensor_list = (uint8_t *) bic_sensor_list;
      *cnt = bic_sensor_cnt;
      break;
    case FRU_IOM:
      sku = pal_get_iom_type();
      if (sku == 1) { // SKU: Type 5
        *sensor_list = (uint8_t *) iom_sensor_list_type5;
        *cnt = iom_sensor_cnt_type5;
      } else {        // SKU: Type 7
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

  switch(fru) {
    case FRU_SLOT1:
      pal_is_fru_prsnt(fru, &status);
      break;
    case FRU_IOM:
    case FRU_DPB:
    case FRU_SCC:
    case FRU_NIC:
      status = 1;
      break;
  }

  if (status)
    return fbttn_sensor_sdr_init(fru, sinfo);
  else
    return -1;
}

int
pal_sensor_read(uint8_t fru, uint8_t sensor_num, void *value) {

  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int ret;

  switch(fru) {
    case FRU_SLOT1:
      sprintf(key, "slot%d_sensor%d", fru, sensor_num);
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

  switch(fru) {
    case FRU_SLOT1:
      sprintf(key, "slot%d_sensor%d", fru, sensor_num);
      if(pal_is_fru_prsnt(fru, &status) < 0)
         return -1;
      if (!status) {
         return -1;
      }
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

  ret = fbttn_sensor_read(fru, sensor_num, value);
  if(ret < 0) {
    if(fru == FRU_IOM || fru == FRU_DPB || fru == FRU_SCC || fru == FRU_NIC)
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
syslog(LOG_CRIT, "pal_get_fru_devtty: Wrong fru id %u", fru);
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
      return -1;
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

  return fbttn_common_crashdump(fru);
}

int
pal_sel_handler(uint8_t fru, uint8_t snr_num) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  /* For every SEL event received from the BIC, set the critical LED on */
  switch(fru) {
    case FRU_SLOT1:
      switch(snr_num) {
        case CATERR:
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
pal_get_event_sensor_name(uint8_t fru, uint8_t snr_num, char *name) {

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
pal_parse_sel(uint8_t fru, uint8_t snr_num, uint8_t *event_data,
    char *error_log) {

  char *ed = &event_data[3];
  char temp_log[128] = {0};
  uint8_t temp;
  uint8_t sen_type = event_data[0];

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

  ret = pal_sensor_read(FRU_DPB, DPB_SENSOR_FAN0_FRONT + fan , &value);

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
  sprintf(tstr, "%d", ts.tv_sec);

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
pal_get_boot_order(uint8_t fru, uint8_t *boot) {

  return 0;
}

int
pal_set_boot_order(uint8_t fru, uint8_t *boot) {

  return 0;
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {

  return 0;
}

void
pal_log_clear(char *fru) {
  if (!strcmp(fru, "slot1")) {
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
int pal_get_locl(void){
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
int pal_fault_led(uint8_t state, uint8_t mode) {
  // mode: 0 - 1 auto (BMC control); 1 - manual (user control); 2 - disable manual
  static int run_mode = 0;  // run_mode: 0 - auto; 1 - manual
  char path[64] = {0};

  // ENCL_FAULT_LED: GPIOO3 (115)
  sprintf(path, GPIO_VAL, GPIO_ENCL_FAULT_LED);

  if (run_mode == mode) {       // keep mode, set LED value
    if (state == 0) {           // LED off
      if (write_device(path, "1")) {
        return -1;
      }
    } else {                    // LED on
      if (write_device(path, "0")) {
        return -1;
      }
    }
  } else if (run_mode < mode){  // enable/disable manual control
    if (mode != 2) {
      run_mode = mode;
    } else {
      run_mode = 0;
    }
    if (write_device(path, "1")) {
      return -1;
    }
  }
  return run_mode;
}

//For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7e
int pal_get_plat_sku_id(void){
  int sku = 0;
  int location = 0;
  uint8_t platform_info;

  sku = pal_get_iom_type();
  location = pal_get_locl();

  if(sku == 1) {//type 5
    if(location == 1) {
      platform_info = 2; //Triton Type 5A
    }
    else if(location == 2) {
      platform_info = 3; //Triton Type 5B
    }
  }
  else if (sku == 2) {//type 7
    platform_info = 4; //Triton 7SS
  }
  else
    return -1;

  return platform_info;
}

//Use part of the function for OEM Command "CMD_OEM_GET_POSS_PCIE_CONFIG" 0xF4
int pal_get_poss_pcie_config(uint8_t *pcie_config){

  int sku = 0;

  sku = pal_get_iom_type();

  if(sku == 1)
    *pcie_config = 0x6;
  else if (sku == 2)
    *pcie_config = 0x8;
  else
    return -1;

  return 0;
}

int pal_minisas_led(uint8_t port, uint8_t state) {
   char path[64] = {0};

  // ENCL_FAULT_LED: GPIOO3 (115)
  if(port)
    sprintf(path, GPIO_VAL, BMC_EXT2_LED_Y);
  else
    sprintf(path, GPIO_VAL, BMC_EXT1_LED_Y);
   if (state == 1) {           // LED on
      if (write_device(path, "1")) {
        return -1;
      }
    } else {                    // LED off
      if (write_device(path, "0")) {
        return -1;
      }
    }
    return 0;
}

int
pal_get_pwm_value(uint8_t fan_num, uint8_t *value) {

  // TODO: Add support to display PWM in fan-util output (similar to lightning pal)
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
  }

  ret = pal_get_key_value(key, cvalue);
    if (ret < 0) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_expander_sensor_check: pal_get_key_value failed for "
          "fru %u", fru);
#endif
      return ret;
    }

  timestamp = atoi(cvalue);

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%d", ts.tv_sec);
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
  if ( (current_time - timestamp) > (5 - tolerance) ) {
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
        //DO the Second transaction only when sensor count is not 1
        if (sensor_cnt != 1) {
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
      sprintf(tstr, "%d", ts.tv_sec);
      pal_set_key_value(key, tstr);
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
    tbuf[0] = 1;
    tbuf[1] = sensor_num;
    tlen = 2;
  }

  //TODO paste sensor_num to tbuf, to get spcific sensor data from exp
  ret = expander_ipmb_wrapper(NETFN_OEM_REQ, CMD_EXP_GET_SENSOR_READING, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    #ifdef DEBUG
       syslog(LOG_WARNING, "pal_exp_dpb_read_sensor_wrapper: expander_ipmb_wrapper failed.");
    #endif
    return ret;
  }

  if(rbuf[0] != sensor_cnt) //if the response counter is not equal to request counter
    return -1;

  for(i = 0; i < sensor_cnt; i++) {
    // search the corresponding sensor table to fill up the raw data and statsu
    // rbuf[5*i+1] sensor number
    // rbuf[5*i+2] sensor raw data1
    // rbuf[5*i+3] sensor raw data2
    // rbuf[5*i+4] sensor status
    // rbuf[5*i+5] reserved
    ret = fbttn_sensor_units(fru, rbuf[5*i+1], units);
    if (ret)
      return ret;

    if( strcmp(units,"C") == 0 )
      value = rbuf[5*i+2];
    else
    {
      value =  (((rbuf[5*i+2] << 8) + rbuf[5*i+3]));
      value = value/100;
    }
    //cache sensor reading
    sprintf(key, "dpb_sensor%d", rbuf[5*i+1]);
    sprintf(str, "%.2f",(float)value);

    if(rbuf[5*i+4] != 0){
	  sprintf(str, "NA");
	}

    if(edb_cache_set(key, str) < 0) {
    }
  #ifdef DEBUG
       syslog(LOG_WARNING, "pal_exp_dpb_read_sensor_wrapper: cache_set key = %s, str = %s failed.", key, str);
  #endif
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

  //TODO paste sensor_num to tbuf, to get spcific sensor data from exp
  ret = expander_ipmb_wrapper(NETFN_OEM_REQ, CMD_EXP_GET_SENSOR_READING, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    #ifdef DEBUG
       syslog(LOG_WARNING, "pal_exp_scc_read_sensor_wrapper: expander_ipmb_wrapper failed.");
    #endif
    return ret;
  }

  if(rbuf[0] != sensor_cnt) //if the response counter is not equal to request counter
    return -1;

  for(i = 0; i < sensor_cnt; i++) {
    // search the corresponding sensor table to fill up the raw data and statsu
    // rbuf[5*i+1] sensor number
    // rbuf[5*i+2] sensor raw data1
    // rbuf[5*i+3] sensor raw data2
    // rbuf[5*i+4] sensor status
    // rbuf[5*i+5] reserved
    ret = fbttn_sensor_units(fru, rbuf[5*i+1], units);
    if (ret)
        return ret;

    if( strcmp(units,"C") == 0 )
      value = rbuf[5*i+2];
    else
    {
		 value = (((rbuf[5*i+2] << 8) + rbuf[5*i+3]));
         value = value/100;
    }
    //cache sensor reading
    sprintf(key, "scc_sensor%d", rbuf[5*i+1]);
    sprintf(str, "%.2f",(float)value);

    if(rbuf[5*i+4] != 0){
	  sprintf(str, "NA");
	}

    if(edb_cache_set(key, str) < 0) {
    }
  #ifdef DEBUG
       syslog(LOG_WARNING, "pal_exp_scc_read_sensor_wrapper: cache_set key = %s, str = %s failed.", key, str);
  #endif
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
  int stat = 0;
  int bit_stat = 0;

  if(num <= 100) {
    return;
  }
  stat = num / 8;
  bit_stat = num % 8;
  if(bit_stat!=0) {
    g_err_code[stat] |= 1 << bit_stat;
  }
  else {
    g_err_code[stat] |= 1;
  }
}

void pal_err_code_disable(unsigned char num) {
  int stat = 0;
  int bit_stat = 0;
  if(num <= 100) {
    return;
  }
  stat = num / 8;
  bit_stat = num % 8;
  g_err_code[stat] &= (unsigned char)~(1 << bit_stat);
}

/*
 * Calculate the sum of error code
 * If Err happen, the sum does not equal to 0
 *
 */
unsigned char pal_sum_error_code(void) {
  unsigned char ret = 0;
  int i = 0;

  for(i = 1; i < ERROR_CODE_NUM; i++) {
    if(g_err_code[i] != 0) {
      ret = 1;
      return ret;
    }
  }
  return ret;
}

