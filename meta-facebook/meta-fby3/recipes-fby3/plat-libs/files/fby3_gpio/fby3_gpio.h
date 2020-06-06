/* Copyright 2015-present Facebook. All Rights Reserved.
 *
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

#ifndef __FBY3_GPIO_H__
#define __FBY3_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <facebook/bic.h>

#define MAX_READ_RETRY 3
#define CPLD_INTENT_CTRL_ADDR 0x70
#define SLOT_BUS_BASE 3

typedef struct {
  uint8_t err_id;
  char *err_des;
} err_t;

enum {
  PLATFORM_STATE_OFFSET = 0x03,
  LAST_RCVY_OFFSET      = 0x05,
  LAST_PANIC_OFFSET     = 0x07,
  MAJOR_ERR_OFFSET      = 0x08,
  MINOR_ERR_OFFSET      = 0x09,
};

uint8_t fby3_get_gpio_list_size(void);
int fby3_get_gpio_name(uint8_t fru, uint8_t gpio, char *name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBY3_GPIO_H__ */
