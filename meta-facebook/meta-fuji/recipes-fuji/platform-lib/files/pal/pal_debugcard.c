/*
 *
 * Copyright 2021-present Facebook. All Rights Reserved.
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

#include <openbmc/log.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/misc-utils.h>
#include "pal.h"

// Display the given POST code
static int display_postcode(uint8_t status) {
  char buf[5];
  char path[LARGEST_DEVICE_NAME + 1];

  snprintf(path, sizeof(path), DEBUGCARD_POSTCODE);
  snprintf(buf, sizeof(buf), "0x%2x", status);
  if (device_write_buff(path, buf)) {
    return -1;
  }

  return 0;
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
  ret = display_postcode(status);
  if (ret) {
    return ret;
  }

  return 0;
}

/* Read the Front Panel Hand Switch and return the position */
static int get_hand_sw_physically(uint8_t *pos) {
  char path[LARGEST_DEVICE_NAME + 1];
  int loc;

  snprintf(path, sizeof(path), BMC_UART_SEL);
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
      return get_hand_sw_physically(pos);
    }
    *pos = loc;

    return 0;
  }

  return get_hand_sw_physically(pos);
}

/* Return the Front Panel Power Button */
int pal_get_dbg_pwr_btn(uint8_t *status) {
  int val;
  char path[LARGEST_DEVICE_NAME + 1];

  snprintf(path, sizeof(path), DEBUGCARD_BUTTON_STATUS);
  if (device_read(path, &val)) {
    return -1;
  }

  if (val == 0xfd) {
    *status = 1;      /* PWR BTN status pressed */
  } else {
    *status = 0;      /* PWR BTN status cleared */
  }

  return 0;
}

/* Clear Debug Card Button status */
int pal_clr_dbg_btn() {
  char path[LARGEST_DEVICE_NAME + 1];

  snprintf(path, sizeof(path), DEBUGCARD_BUTTON_STATUS);
  if (device_write_buff(path, "0xff")) {
    return -1;
  }

  return 0;
}

/* Return the Debug Card Reset Button status */
int pal_get_dbg_rst_btn(uint8_t *status) {
  int val;
  char path[LARGEST_DEVICE_NAME + 1];

  snprintf(path, sizeof(path), DEBUGCARD_BUTTON_STATUS);
  if (device_read(path, &val)) {
    return -1;
  }

  if (val == 0xfe) {
    *status = 1;      /* RST BTN status pressed */
  } else {
    *status = 0;      /* RST BTN status cleared */
  }

  return 0;
}

/* Update the Reset button input to the server at given slot */
int pal_set_rst_btn(uint8_t slot, uint8_t status) {
  char *val;
  int ret;
  if (slot != FRU_SCM) {
    return -1;
  }

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  ret = device_write_buff(SCM_COM_RST_BTN, val);
  if (ret) {
    return -1;
  }
  return 0;
}

/* Return the Debug Card UART Sel Button status */
int pal_get_dbg_uart_btn(uint8_t *status) {
  int val;
  char path[LARGEST_DEVICE_NAME + 1];

  snprintf(path, sizeof(path), DEBUGCARD_BUTTON_STATUS);
  if (device_read(path, &val)) {
    return -1;
  }

  if (val == 0x7f) {
    *status = 1;      /* UART BTN status pressed */
  } else {
    *status = 0;      /* UART BTN status cleared */
  }
  return 0;
}

/* Switch the UART mux to userver or BMC */
int pal_switch_uart_mux() {
  char path[LARGEST_DEVICE_NAME + 1];
  char *val;
  int loc;

  /* Refer the UART select status */
  snprintf(path, sizeof(path), BMC_UART_SEL);
  if (device_read(path, &loc)) {
    return -1;
  }

  if (loc == 0) {
    val = "1";
  } else {
    val = "0";
  }

  if (device_write_buff(path, val)) {
    return -1;
  }

  return 0;
}

/* Return the Debug Card present status */
int pal_is_debug_card_prsnt(uint8_t *status) {
  int val;
  char path[LARGEST_DEVICE_NAME + 1];
  *status = 0;

  snprintf(path, sizeof(path), SMBCPLD_PATH_FMT, DEBUGCARD_PRSNT_STATUS);
  if (device_read(path, &val)) {
    return -1;
  }

  if (val == 0x1) {
    *status = 1; /* present */
  } else {
    *status = 0; /* not present */
  }

  return 0;
}
