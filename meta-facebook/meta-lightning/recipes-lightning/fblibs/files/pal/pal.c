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

#define LIGHTNING_PLATFORM_NAME "Lightning"
#define LAST_KEY "last_key"
#define LIGHTNING_MAX_NUM_SLOTS 0
#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define GPIO_DIR "/sys/class/gpio/gpio%d/direction"

#define GPIO_HAND_SW_ID1 138
#define GPIO_HAND_SW_ID2 139
#define GPIO_HAND_SW_ID4 140
#define GPIO_HAND_SW_ID8 141

#define GPIO_RST_BTN 144
#define GPIO_PWR_BTN 24

#define GPIO_HB_LED 135

#define GPIO_USB_SW0 36
#define GPIO_USB_SW1 37
#define GPIO_USB_MUX_EN_N 147

#define GPIO_UART_SEL0 32
#define GPIO_UART_SEL1 33
#define GPIO_UART_SEL2 34
#define GPIO_UART_RX 35

#define GPIO_POSTCODE_0 48
#define GPIO_POSTCODE_1 49
#define GPIO_POSTCODE_2 50
#define GPIO_POSTCODE_3 51
#define GPIO_POSTCODE_4 124
#define GPIO_POSTCODE_5 125
#define GPIO_POSTCODE_6 126
#define GPIO_POSTCODE_7 127

#define GPIO_DBG_CARD_PRSNT 137

#define PAGE_SIZE  0x1000
#define AST_SCU_BASE 0x1e6e2000
#define PIN_CTRL1_OFFSET 0x80
#define PIN_CTRL2_OFFSET 0x84
#define AST_WDT_BASE 0x1e785000
#define WDT_OFFSET 0x10

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
const static uint8_t gpio_rst_btn[] = { 0, 57, 56, 59, 58 };
const static uint8_t gpio_led[] = { 0, 97, 96, 99, 98 };
const static uint8_t gpio_id_led[] = { 0, 41, 40, 43, 42 };
const static uint8_t gpio_prsnt[] = { 0, 61, 60, 63, 62 };
const static uint8_t gpio_power[] = { 0, 27, 25, 31, 29 };
const static uint8_t gpio_12v[] = { 0, 117, 116, 119, 118 };
const char pal_fru_list[] = "all, peb, pdpb, fcb";
const char pal_server_list[] = "slot1, slot2, slot3, slot4";

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
"timestamp_sled",
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
  "0", /* timestamp_sled */
  /* Add more def values for the correspoding keys*/
  LAST_KEY /* Same as last entry of the key_list */
};

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
    // Disable UART2's TXD and enable others
    ctrl = *(volatile uint32_t*) scu_pin_ctrl2;
    ctrl |= UART1_TXD;
    ctrl &= (~UART2_TXD); //Disable
    *(volatile uint32_t*) scu_pin_ctrl2 = ctrl;

    ctrl = *(volatile uint32_t*) scu_pin_ctrl1;
    ctrl |= UART3_TXD | UART4_TXD;
    *(volatile uint32_t*) scu_pin_ctrl1 = ctrl;
    break;
  case 2:
    // Disable UART1's TXD and enable others
    ctrl = *(volatile uint32_t*) scu_pin_ctrl2;
    ctrl &= (~UART1_TXD); // Disable
    ctrl |= UART2_TXD;
    *(volatile uint32_t*) scu_pin_ctrl2 = ctrl;

    ctrl = *(volatile uint32_t*) scu_pin_ctrl1;
    ctrl |= UART3_TXD | UART4_TXD;
    *(volatile uint32_t*) scu_pin_ctrl1 = ctrl;
    break;
  case 3:
    // Disable UART4's TXD and enable others
    ctrl = *(volatile uint32_t*) scu_pin_ctrl2;
    ctrl |= UART1_TXD | UART2_TXD;
    *(volatile uint32_t*) scu_pin_ctrl2 = ctrl;

    ctrl = *(volatile uint32_t*) scu_pin_ctrl1;
    ctrl |= UART3_TXD;
    ctrl &= (~UART4_TXD); // Disable
    *(volatile uint32_t*) scu_pin_ctrl1 = ctrl;
    break;
  case 4:
    // Disable UART3's TXD and enable others
    ctrl = *(volatile uint32_t*) scu_pin_ctrl2;
    ctrl |= UART1_TXD | UART2_TXD;
    *(volatile uint32_t*) scu_pin_ctrl2 = ctrl;

    ctrl = *(volatile uint32_t*) scu_pin_ctrl1;
    ctrl &= (~UART3_TXD); // Disable
    ctrl |= UART4_TXD;
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

// Platform Abstraction Layer (PAL) Functions
int
pal_get_platform_name(char *name) {
  strcpy(name, LIGHTNING_PLATFORM_NAME);

  return 0;
}

int
pal_get_num_slots(uint8_t *num) {
  *num = LIGHTNING_MAX_NUM_SLOTS;

  return 0;
}

int
pal_is_server_prsnt(uint8_t slot_id, uint8_t *status) {
  int val;
  char path[64] = {0};

  if (slot_id < 1 || slot_id > 4) {
    return -1;
  }

  sprintf(path, GPIO_VAL, gpio_prsnt[slot_id]);

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

  return 0;
}

// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t slot_id, uint8_t cmd) {

  return 0;
}

int
pal_sled_cycle(void) {

  return 0;
}

// Read the Front Panel Hand Switch and return the position
int
pal_get_hand_sw(uint8_t *pos) {

  return 0;
}

// Return the Front panel Power Button
int
pal_get_pwr_btn(uint8_t *status) {

  return 0;
}

// Return the front panel's Reset Button status
int
pal_get_rst_btn(uint8_t *status) {

  return 0;
}

// Update the Reset button input to the server at given slot
int
pal_set_rst_btn(uint8_t slot, uint8_t status) {

  return 0;
}

// Update the LED for the given slot with the status
int
pal_set_led(uint8_t slot, uint8_t status) {

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

  return 0;
}

// Update the USB Mux to the server at given slot
int
pal_switch_usb_mux(uint8_t slot) {

  return 0;
}

// Get the last post code of the given slot
int
pal_post_get_last(uint8_t slot, uint8_t *status) {

  return 0;
}

// Handle the received post code, for now display it on debug card
int
pal_post_handle(uint8_t slot, uint8_t status) {

  return 0;
}


static int
read_kv(char *key, char *value) {

  FILE *fp;
  int rc;

  fp = fopen(key, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "read_kv: failed to open %s", key);
#endif
    return err;
  }

  rc = (int) fread(value, 1, MAX_VALUE_LEN, fp);
  fclose(fp);
  if (rc <= 0) {
#ifdef DEBUG
    syslog(LOG_INFO, "read_kv: failed to read %s", key);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
write_kv(char *key, char *value) {

  FILE *fp;
  int rc;

  fp = fopen(key, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_WARNING, "write_kv: failed to open %s", key);
#endif
    return err;
  }

  rc = fwrite(value, 1, strlen(value), fp);
  fclose(fp);

  if (rc < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "write_kv: failed to write to %s", key);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

int
pal_get_fru_id(char *str, uint8_t *fru) {

  return lightning_common_fru_id(str, fru);
}

int
pal_get_fru_name(uint8_t fru, char *name) {

  return lightning_common_fru_name(fru, name);
}

int
pal_get_fru_sdr_path(uint8_t fru, char *path) {
  return lightning_sensor_sdr_path(fru, path);
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {

  switch(fru) {
    case FRU_PEB:
      *sensor_list = (uint8_t *) peb_sensor_list;
      *cnt = peb_sensor_cnt;
      break;
    case FRU_PDPB:
      *sensor_list = (uint8_t *) pdpb_sensor_list;
      *cnt = pdpb_sensor_cnt;
      break;
    case FRU_FCB:
      *sensor_list = (uint8_t *) fcb_sensor_list;
      *cnt = fcb_sensor_cnt;
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
pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  return lightning_sensor_sdr_init(fru, sinfo);
}

int
pal_sensor_read(uint8_t fru, uint8_t sensor_num, void *value) {
  return lightning_sensor_read(fru, sensor_num, value);
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  return lightning_sensor_threshold(fru, sensor_num, thresh, value);
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  return lightning_sensor_name(fru, sensor_num, name);
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  return lightning_sensor_units(fru, sensor_num, units);
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  return lightning_get_fruid_path(fru, path);
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  return lightning_get_fruid_eeprom_path(fru, path);
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  return lightning_get_fruid_name(fru, name);
}

int
pal_fruid_write(uint8_t slot, char *path) {

  return 0;
}

static int
get_key_value(char* key, char *value) {

  char kpath[64] = {0};

  sprintf(kpath, KV_STORE, key);

  if (access(KV_STORE_PATH, F_OK) == -1) {
    mkdir(KV_STORE_PATH, 0777);
  }

  return read_kv(kpath, value);
}

static int
set_key_value(char *key, char *value) {

  char kpath[64] = {0};

  sprintf(kpath, KV_STORE, key);

  if (access(KV_STORE_PATH, F_OK) == -1) {
    mkdir(KV_STORE_PATH, 0777);
  }

  return write_kv(kpath, value);
}

int
pal_get_key_value(char *key, char *value) {

  int ret;
  int i;

  i = 0;
  while(strcmp(key_list[i], LAST_KEY)) {

    if (!strcmp(key, key_list[i])) {
      // Key is valid
      if ((ret = get_key_value(key, value)) < 0 ) {
#ifdef DEBUG
        syslog(LOG_WARNING, "pal_get_key_value: get_key_value failed. %d", ret);
#endif
        return ret;
      }
      return ret;
    }
    i++;
  }

  return -1;
}

int
pal_set_def_key_value() {

  int ret;
  int i;
  char kpath[64] = {0};

  i = 0;
  while(strcmp(key_list[i], LAST_KEY)) {

  sprintf(kpath, KV_STORE, key_list[i]);

  if (access(kpath, F_OK) == -1) {
      if ((ret = set_key_value(key_list[i], def_val_list[i])) < 0) {
#ifdef DEBUG
        syslog(LOG_WARNING, "pal_set_def_key_value: set_key_value failed. %d", ret);
#endif
      }
    }
    i++;
  }

  return 0;
}

int
pal_set_key_value(char *key, char *value) {

  int ret;
  int i;

  i = 0;
  while(strcmp(key_list[i], LAST_KEY)) {

    if (!strcmp(key, key_list[i])) {
      // Key is valid
      if ((ret = set_key_value(key, value)) < 0) {
#ifdef DEBUG
        syslog(LOG_WARNING, "pal_set_key_value: set_key_value failed. %d", ret);
#endif
        return ret;
      }
      return ret;
    }
    i++;
  }

  return -1;
}

int
pal_get_fru_devtty(uint8_t fru, char *devtty) {

  return 0;
}

void
pal_dump_key_value(void) {
  int i;
  int ret;

  char value[MAX_VALUE_LEN] = {0x0};

  while (strcmp(key_list[i], LAST_KEY)) {
    printf("%s:", key_list[i]);
    if (ret = get_key_value(key_list[i], value) < 0) {
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

  return 0;
}

int
pal_get_last_pwr_state(uint8_t fru, char *state) {

  return 0;
}

int
pal_get_sys_guid(uint8_t slot, char *guid) {
  int ret;

  return 0;
}

int
pal_set_sysfw_ver(uint8_t slot, uint8_t *ver) {

  return 0;
}

int
pal_get_sysfw_ver(uint8_t slot, uint8_t *ver) {

  return 0;
}

int
pal_is_bmc_por(void) {

  return 0;
}

int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {

  return 0;
}

int
pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val) {

  return 0;
}

static int
pal_store_crashdump(uint8_t fru) {

  return 0;
}

int
pal_sel_handler(uint8_t fru, uint8_t snr_num) {

  return 0;
}

int
pal_get_event_sensor_name(uint8_t fru, uint8_t snr_num, char *name) {

  return 0;
}

int
pal_parse_sel(uint8_t fru, uint8_t snr_num, uint8_t *event_data, char *error_log) {

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

  return 0;
}

int
pal_get_sensor_health(uint8_t fru, uint8_t *value) {

  return 0;
}

int
pal_get_fru_list(char *list) {

  strcpy(list, pal_fru_list);
  return 0;
}

int
pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint8_t *flag) {

  return 0;
}
