/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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
//#define DEBUG
//#define VERBOSE

#include "gpio.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/errno.h>

#include <openbmc/log.h>

void gpio_init_default(gpio_st *g) {
  g->gs_gpio = -1;
  g->gs_fd = -1;
}

int gpio_open(gpio_st *g, int gpio)
{
  char buf[128];
  int rc;

  snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%u/value", gpio);
  rc = open(buf, O_RDWR);
  if (rc == -1) {
    rc = errno;
    LOG_ERR(rc, "Failed to open %s", buf);
    return -rc;
  }
  g->gs_fd = rc;
  g->gs_gpio = gpio;
  return 0;
}

void gpio_close(gpio_st *g)
{
  if (g && g->gs_fd != -1) {
    close(g->gs_fd);
  }
  gpio_init_default(g);
}

gpio_value_en gpio_read(gpio_st *g)
{
  char buf[32] = {0};
  gpio_value_en v;
  lseek(g->gs_fd, 0, SEEK_SET);
  read(g->gs_fd, buf, sizeof(buf));
  v = atoi(buf) ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;
  LOG_VER("read gpio=%d value=%d %d", g->gs_gpio, atoi(buf), v);
  return v;
}

void gpio_write(gpio_st *g, gpio_value_en v)
{
  lseek(g->gs_fd, 0, SEEK_SET);
  write(g->gs_fd, (v == GPIO_VALUE_HIGH) ? "1" : "0", 1);
  LOG_VER("write gpio=%d value=%d", g->gs_gpio, v);
}

int gpio_change_direction(gpio_st *g, gpio_direction_en dir)
{
  char buf[128];
  char *val;
  int fd = -1;
  int rc = 0;

  snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%u/direction", g->gs_gpio);
  fd = open(buf, O_WRONLY);
  if (fd == -1) {
    rc = errno;
    LOG_ERR(rc, "Failed to open %s", buf);
    return -rc;
  }

  val = (dir == GPIO_DIRECTION_IN) ? "in" : "out";
  write(fd, val, strlen(val));

  LOG_VER("change gpio=%d direction=%s", g->gs_gpio, val);

 out:
  if (fd != -1) {
    close(fd);
  }
  return -rc;
}
