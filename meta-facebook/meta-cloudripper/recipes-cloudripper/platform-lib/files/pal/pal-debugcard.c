/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

 /*
  * This file contains functions and logics that depends on Cloudripper specific
  * hardware and kernel drivers. Here, some of the empty "default" functions
  * are overridden with simple functions that returns non-zero error code.
  * This is for preventing any potential escape of failures through the
  * default functions that will return 0 no matter what.
  */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <openbmc/log.h>
#include <openbmc/libgpio.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/sensor-correction.h>
#include <openbmc/misc-utils.h>
#include <facebook/bic.h>
#include <facebook/wedge_eeprom.h>
#include "pal-debugcard.h"
#include "pal.h"

int pal_set_post_gpio_out(void) {
  char path[LARGEST_DEVICE_NAME + 1];
  int ret;
  char *val = "out";

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_0, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_1, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_2, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_3, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_4, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_5, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_6, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_7, "direction");
  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  post_exit:
  if (ret) {
    OBMC_WARN("device_write_buff failed for %s\n", path);
    return -1;
  } else {
    return 0;
  }
}

// Display the given POST code using GPIO port
int pal_post_display(uint8_t status) {
  char path[LARGEST_DEVICE_NAME + 1];
  int ret;
  char *val;

  OBMC_WARN("pal_post_display: status is %d\n", status);

  ret = pal_set_post_gpio_out();
  if (ret) {
    return -1;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_0, "value");
  if (BIT(status, 0)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_1, "value");
  if (BIT(status, 1)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_2, "value");
  if (BIT(status, 2)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_3, "value");
  if (BIT(status, 3)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_4, "value");
  if (BIT(status, 4)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_5, "value");
  if (BIT(status, 5)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_6, "value");
  if (BIT(status, 6)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_POSTCODE_7, "value");
  if (BIT(status, 7)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(path, val);
  if (ret) {
    goto post_exit;
  }

post_exit:
  if (ret) {
    OBMC_WARN("device_write_buff failed for %s\n", path);
    return -1;
  } else {
    return 0;
  }
}

// Handle the received post code, for now display it on debug card
int pal_post_handle(uint8_t slot, uint8_t status) {
  uint8_t prsnt;
  int ret;

  // Check for debug card presence
  ret = pal_is_debug_card_prsnt(&prsnt);
  if (ret) {
    return ret;
  }

  // No debug card present, return
  if (!prsnt) {
    return 0;
  }

  // Display the post code in the debug card
  ret = pal_post_display(status);
  if (ret) {
    return ret;
  }

  return 0;
}

/* Add button function for Debug Card */
/* Read the Front Panel Hand Switch and return the position */
int pal_get_hand_sw_physically(uint8_t *pos) {
  char path[LARGEST_DEVICE_NAME + 1];
  int loc;

  snprintf(path, LARGEST_DEVICE_NAME, BMC_UART_SEL);
  if (device_read(path, &loc)) {
    return -1;
  }
  *pos = loc;

  return 0;
}

int pal_get_hand_sw(uint8_t *pos) {
  char value[MAX_VALUE_LEN];
  uint8_t loc;
  int ret;

  ret = kv_get("scm_hand_sw", value, NULL, 0);
  if (!ret) {
    loc = atoi(value);
    if (loc > HAND_SW_BMC) {
      return pal_get_hand_sw_physically(pos);
    }
    *pos = loc;

    return 0;
  }

  return pal_get_hand_sw_physically(pos);
}

/* Return the Front Panel Power Button */
int pal_get_dbg_pwr_btn(uint8_t *status) {
  char cmd[MAX_KEY_LEN + 32] = {0};
  char value[MAX_VALUE_LEN];
  char *p;
  FILE *fp;
  int val = 0;

  sprintf(cmd, "/usr/sbin/i2cget -f -y 4 0x27 1");
  fp = popen(cmd, "r");
  if (!fp) {
    return -1;
  }

  if (fgets(value, MAX_VALUE_LEN, fp) == NULL) {
    pclose(fp);
    return -1;
  }

  for (p = value; *p != '\0'; p++) {
    if (*p == '\n' || *p == '\r') {
      *p = '\0';
      break;
    }
  }

  sscanf(value, "%x", &val);
  if ((!(val & 0x2)) && (val & 0x1)) {
    *status = 1;      /* PWR BTN status pressed */
    OBMC_WARN("%s PWR pressed  0x%x\n", __FUNCTION__, val);
  } else {
    *status = 0;      /* PWR BTN status cleared */
  }
  pclose(fp);
  return 0;
}

/* Return the Debug Card Reset Button status */
int pal_get_dbg_rst_btn(uint8_t *status) {
  char cmd[MAX_KEY_LEN + 32] = {0};
  char value[MAX_VALUE_LEN];
  char *p;
  FILE *fp;
  int val = 0;

  sprintf(cmd, "/usr/sbin/i2cget -f -y 4 0x27 1");
  fp = popen(cmd, "r");
  if (!fp) {
    return -1;
  }

  if (fgets(value, MAX_VALUE_LEN, fp) == NULL) {
    pclose(fp);
    return -1;
  }

  for (p = value; *p != '\0'; p++) {
    if (*p == '\n' || *p == '\r') {
      *p = '\0';
      break;
    }
  }

  sscanf(value, "%x", &val);
  if ((!(val & 0x1)) && (val & 0x2)) {
    *status = 1;      /* RST BTN status pressed */
  } else {
    *status = 0;      /* RST BTN status cleared */
  }
  pclose(fp);
  return 0;
}

/* Update the Reset button input to the server at given slot */
int pal_set_rst_btn(uint8_t slot, uint8_t status) {
  char *val;
  char path[64];
  int ret;
  if (slot != FRU_SCM) {
    return -1;
  }

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  sprintf(path, SCM_SYSFS, SCM_COM_RST_BTN);

  ret = device_write_buff(path, val);
  if (ret) {
    return -1;
  }
  return 0;
}

/* Return the Debug Card UART Sel Button status */
int pal_get_dbg_uart_btn(uint8_t *status) {
  char cmd[MAX_KEY_LEN + 32] = {0};
  char value[MAX_VALUE_LEN];
  char *p;
  FILE *fp;
  int val = 0;

  sprintf(cmd, "/usr/sbin/i2cget -f -y 4 0x27 3");
  fp = popen(cmd, "r");
  if (!fp) {
    return -1;
  }

  if (fgets(value, MAX_VALUE_LEN, fp) == NULL) {
    pclose(fp);
    return -1;
  }
  pclose(fp);
  for (p = value; *p != '\0'; p++) {
    if (*p == '\n' || *p == '\r') {
      *p = '\0';
      break;
    }
  }
  sscanf(value, "%x", &val);
  if (!(val & 0x80)) {
    *status = 1;      /* UART BTN status pressed */
  } else {
    *status = 0;      /* UART BTN status cleared */
  }
  return 0;
}

/* Clear Debug Card UART Sel Button status */
int pal_clr_dbg_uart_btn() {
  int ret;
  ret = run_command("/usr/sbin/i2cset -f -y 4 0x27 3 0xff");
  if (ret)
    return -1;
  return 0;
}

/* Switch the UART mux to userver or BMC */
int pal_switch_uart_mux() {
  char path[LARGEST_DEVICE_NAME + 1];
  char *val;
  int loc;

  /* Refer the UART select status */
  snprintf(path, LARGEST_DEVICE_NAME, BMC_UART_SEL);
  if (device_read(path, &loc)) {
    return -1;
  }

  if (loc == 3) {
    val = "2";
  } else {
    val = "3";
  }

  if (device_write_buff(path, val)) {
    return -1;
  }

  return 0;
}
