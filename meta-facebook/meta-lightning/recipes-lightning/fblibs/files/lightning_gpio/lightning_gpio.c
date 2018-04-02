/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#include <unistd.h>
#include <syslog.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "lightning_gpio.h"

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define GPIO_DIR "/sys/class/gpio/gpio%d/direction"

#define GPIO_BMC_UART_SWITCH 123
#define GPIO_RESET_SSD_SWITCH 134
#define GPIO_PEER_BMC_HB 117

size_t pal_tach_count = 12;

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
gpio_peer_tray_detection(uint8_t *value) {

  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_PEER_BMC_HB);
  if ((*value = gpio_get(GPIO_PEER_BMC_HB)) == GPIO_VALUE_INVALID)
    return -1;

  return 0;
}


// Reset SSD Switch
int
gpio_reset_ssd_switch() {

  char path[64] = {0};
  sprintf(path, GPIO_VAL, GPIO_RESET_SSD_SWITCH);

  if (gpio_set(GPIO_RESET_SSD_SWITCH, GPIO_VALUE_LOW) == -1)
    return -1;

  msleep(100);

  if (gpio_set(GPIO_RESET_SSD_SWITCH, GPIO_VALUE_HIGH) == -1)
    return -1;

  msleep(100);

  return 0;
}
