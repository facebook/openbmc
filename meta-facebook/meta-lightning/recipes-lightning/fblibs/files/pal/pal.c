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
#include "pal.h"

#define BIT(value, index) ((value >> index) & 1)

#define YOSEMITE_PLATFORM_NAME "Yosemite"
#define LAST_KEY "last_key"
#define YOSEMITE_MAX_NUM_SLOTS 4
#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define GPIO_DIR "/sys/class/gpio/gpio%d/direction"

#define GPIO_HAND_SW_ID1 138
#define GPIO_HAND_SW_ID2 139
#define GPIO_HAND_SW_ID4 140
#define GPIO_HAND_SW_ID8 141

#define GPIO_RST_BTN 144
#define GPIO_PWR_BTN 24

#define GPIO_USB_SW0 36
#define GPIO_USB_SW1 37

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

#define UART1_TXD (1 << 22)
#define UART2_TXD (1 << 30)
#define UART3_TXD (1 << 22)
#define UART4_TXD (1 << 30)

#define BIT(v, i) ((v >> i) & 1)
#define DELAY_GRACEFUL_SHUTDOWN 1
#define DELAY_POWER_OFF 5

static uint8_t gpio_rst_btn[5] = { 0, 57, 56, 59, 58 };
static uint8_t gpio_led[5] = { 0, 97, 96, 99, 98 };
static uint8_t gpio_prsnt[5] = { 0, 61, 60, 63, 62 };
static uint8_t gpio_power[5] = { 0, 27, 25, 31, 29 };
const char pal_fru_list[] = "all, slot1, slot2, slot3, slot4, spb, nic";
const char pal_server_list[] = "slot1, slot2, slot3, slot4";

char * key_list[] = {
"pwr_server1_por_cfg",
"pwr_server2_por_cfg",
"pwr_server3_por_cfg",
"pwr_server4_por_cfg",
"pwr_server1_last_state",
"pwr_server2_last_state",
"pwr_server3_last_state",
"pwr_server4_last_state",
/* Add more Keys here */
LAST_KEY /* This is the last key of the list */
};

// Helper Functions
static int
read_device(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;

    syslog(LOG_INFO, "failed to open device %s", device);
    return err;
  }

  rc = fscanf(fp, "%d", value);
  fclose(fp);
  if (rc != 1) {
    syslog(LOG_INFO, "failed to read device %s", device);
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

    syslog(LOG_INFO, "failed to open device for write %s", device);
    return err;
  }

  rc = fputs(value, fp);
  fclose(fp);

  if (rc < 0) {
    syslog(LOG_INFO, "failed to write device %s", device);
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
    syslog(LOG_ALERT, "control_sol_txd: open fails\n");
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

  syslog(LOG_ALERT, "pal_post_display: status is %d\n", status);

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
    syslog(LOG_ALERT, "write_device failed for %s\n", path);
    return -1;
  } else {
    return 0;
  }
}

// Platform Abstraction Layer (PAL) Functions
int
pal_get_platform_name(char *name) {
  strcpy(name, YOSEMITE_PLATFORM_NAME);

  return 0;
}

int
pal_get_num_slots(uint8_t *num) {
  *num = YOSEMITE_MAX_NUM_SLOTS;

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

  // TODO: Logic is reversed until DVT board with h/w fix
  if (val == 0x0) {
    *status = 0;
  } else {
    *status = 1;
  }

  return 0;
}

int
pal_get_server_power(uint8_t slot_id, uint8_t *status) {
  int ret;
  bic_gpio_t gpio;

  ret = bic_get_gpio(slot_id, &gpio);
  if (ret) {
    return ret;
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

  if (slot_id < 1 || slot_id > 4) {
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
        return server_power_off(slot_id, gs_flag);
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON)
        return (server_power_off(slot_id, gs_flag) || server_power_on(slot_id));
      else if (status == SERVER_POWER_OFF)
        return (server_power_on(slot_id));
      break;

    case SERVER_GRACEFUL_SHUTDOWN:
      if (status == SERVER_POWER_OFF)
        return 1;
      else
        gs_flag = true;
        return server_power_off(slot_id, gs_flag);
      break;
    default:
      return -1;
  }

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
pal_get_hand_sw(uint8_t *pos) {
  char path[64] = {0};
  int id1, id2, id4, id8;
  uint8_t loc;
  // Read 4 GPIOs to read the current position
  // id1: GPIOR2(138)
  // id2: GPIOR3(139)
  // id4: GPIOR4(140)
  // id8: GPIOR5(141)

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
  case 1:
  case 6:
    *pos = HAND_SW_SERVER1;
    break;
  case 2:
  case 7:
    *pos = HAND_SW_SERVER2;
    break;
  case 3:
  case 8:
    *pos = HAND_SW_SERVER3;
    break;
  case 4:
  case 9:
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
    val = "0";
  } else {
    val = "1";
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

// Update the USB Mux to the server at given slot
int
pal_switch_usb_mux(uint8_t slot) {
  char *gpio_sw0, *gpio_sw1;
  char path[64] = {0};

  // Based on the USB mux table in Schematics
  switch(slot) {
  case 1:
    gpio_sw0 = "1";
    gpio_sw1 = "0";
    break;
  case 2:
    gpio_sw0 = "0";
    gpio_sw1 = "0";
    break;
  case 3:
    gpio_sw0 = "1";
    gpio_sw1 = "1";
    break;
  case 4:
    gpio_sw0 = "0";
    gpio_sw1 = "1";
    break;
  default:
    // Default is for BMC itself
    return 0;
  }

  sprintf(path, GPIO_VAL, GPIO_USB_SW0);
  if (write_device(path, gpio_sw0) < 0) {
    syslog(LOG_ALERT, "write_device failed for %s\n", path);
    return -1;
  }

  sprintf(path, GPIO_VAL, GPIO_USB_SW1);
  if (write_device(path, gpio_sw1) < 0) {
    syslog(LOG_ALERT, "write_device failed for %s\n", path);
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
  case 1:
    gpio_uart_sel2 = "0";
    gpio_uart_sel1 = "0";
    gpio_uart_sel0 = "1";
    gpio_uart_rx = "0";
    break;
  case 2:
    gpio_uart_sel2 = "0";
    gpio_uart_sel1 = "0";
    gpio_uart_sel0 = "0";
    gpio_uart_rx = "0";
    break;
  case 3:
    gpio_uart_sel2 = "0";
    gpio_uart_sel1 = "1";
    gpio_uart_sel0 = "1";
    gpio_uart_rx = "0";
    break;
  case 4:
    gpio_uart_sel2 = "0";
    gpio_uart_sel1 = "1";
    gpio_uart_sel0 = "0";
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
    syslog(LOG_ALERT, "pal_switch_uart_mux: write_device failed: %s\n", path);
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
    syslog(LOG_ALERT, "post_enable: bic_get_config failed\n");
    return ret;
  }

  t->bits.post = 1;

  ret = bic_set_config(slot, &config);
  if (ret) {
    syslog(LOG_ALERT, "post_enable: bic_set_config failed\n");
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


static int
read_kv(char *key, char *value) {

  FILE *fp;
  int rc;

  fp = fopen(key, "r");
  if (!fp) {
    int err = errno;
    syslog(LOG_ALERT, "read_kv: failed to open %s", key);
    return err;
  }

  rc = (int) fread(value, 1, MAX_VALUE_LEN, fp);
  fclose(fp);
  if (rc <= 0) {
    syslog(LOG_INFO, "read_kv: failed to read %s", key);
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
    syslog(LOG_ALERT, "write_kv: failed to open %s", key);
    return err;
  }

  rc = fwrite(value, 1, strlen(value), fp);
  fclose(fp);

  if (rc < 0) {
    syslog(LOG_ALERT, "write_kv: failed to write to %s", key);
    return ENOENT;
  } else {
    return 0;
  }
}

int
pal_get_fru_id(char *str, uint8_t *fru) {

  return yosemite_common_fru_id(str, fru);
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      *sensor_list = (uint8_t *) bic_sensor_list;
      *cnt = bic_sensor_cnt;
      break;
    case FRU_SPB:
      *sensor_list = (uint8_t *) spb_sensor_list;
      *cnt = spb_sensor_cnt;
      break;
    case FRU_NIC:
      *sensor_list = NULL; // (uint8_t *) nic_sensor_list;
      *cnt = 0; //nic_sensor_cnt;
      break;
    default:
      syslog(LOG_ALERT, "pal_get_fru_sensor_list: Wrong fru id %u", fru);
      return -1;
  }
    return 0;
}

int
pal_sensor_read(uint8_t fru, uint8_t sensor_num, void *value) {
  return yosemite_sensor_read(fru, sensor_num, value);
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  return yosemite_sensor_name(fru, sensor_num, name);
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  return yosemite_sensor_units(fru, sensor_num, units);
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  return yosemite_get_fruid_path(fru, path);
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  return yosemite_get_fruid_name(fru, name);
}


static int
get_key_value(char* key, char *value) {

  int ret;
  char kpath[64] = {0};

  sprintf(kpath, KV_STORE, key);

  if (access(KV_STORE_PATH, F_OK) == -1) {
    mkdir(KV_STORE_PATH, 0777);
  }

  if (ret = read_kv(kpath, value)) {
    return ret;
  }

}

static int
set_key_value(char *key, char *value) {

  int ret;
  char kpath[64] = {0};

  sprintf(kpath, KV_STORE, key);

  if (access(KV_STORE_PATH, F_OK) == -1) {
    mkdir(KV_STORE_PATH, 0777);
  }

  if (ret = write_kv(kpath, value)) {
    return ret;
  }
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
        syslog(LOG_ALERT, "pal_get_key_value: get_key_value failed. %d", ret);
        return ret;
      }
      return ret;
    }
    i++;
  }

  return -1;
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
        syslog(LOG_ALERT, "pal_set_key_value: set_key_value failed. %d", ret);
        printf("pal_set_key_value: ret = %d\n", ret);
        return ret;
      }
      return ret;
    }
    i++;
  }

  return -1;
}
