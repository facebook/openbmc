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
#include <poll.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/errno.h>

#include <openbmc/log.h>

#define MAX_PINS 64
/* Famous last words, 32k is more than
 * sufficient for anyone. */
#define STACK_SIZE (1024 * 32)

static void strip(char *str) {
  while(*str != '\0') {
    if (!isalnum(*str)) {
      *str = '\0';
      break;
    }
    str++;
  }
}

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

void gpio_close_legacy(gpio_st *g)
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

  if (fd != -1) {
    close(fd);
  }
  return -rc;
}

int gpio_current_direction(gpio_st *g, gpio_direction_en *dir)
{
  char buf[128] = {0};
  int fd = -1;
  int rc = 0;

  snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%u/direction", g->gs_gpio);
  fd = open(buf, O_RDONLY);
  if (fd == -1) {
    rc = errno;
    LOG_ERR(rc, "Failed to open %s", buf);
    return -rc;
  }

  read(fd, buf, sizeof(buf));
  strip(buf);
  if (!strcmp(buf, "in")) {
    *dir = GPIO_DIRECTION_IN;
  } else if (!strcmp(buf, "out")) {
    *dir = GPIO_DIRECTION_OUT;
  } else {
    rc = -1;
  }
  close(fd);
  return rc;
}

int gpio_change_edge(gpio_st *g, gpio_edge_en edge)
{
  char buf[128] = {0};
  char str[16] = {0};
  int fd = -1;
  int rc = 0;

  snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/edge", g->gs_gpio);
  fd = open(buf, O_WRONLY);
  if (fd == -1) {
    rc = errno;
    LOG_ERR(rc, "Failed to open %s", buf);
    return -rc;
  }

  switch(edge) {
  case GPIO_EDGE_NONE:
    snprintf(str, sizeof(str), "none");
    break;
  case GPIO_EDGE_RISING:
    snprintf(str, sizeof(str), "rising");
    break;
  case GPIO_EDGE_FALLING:
    snprintf(str, sizeof(str), "falling");
    break;
  case GPIO_EDGE_BOTH:
    snprintf(str, sizeof(str), "both");
    break;
  default:
    LOG_ERR(rc, "gpio_change_edge: Wrong Option %d", edge);
    goto edge_exit;
  }

  write(fd, str, strlen(str) + 1);

edge_exit:
  close(fd);
  return rc;
}

int gpio_current_edge(gpio_st *g, gpio_edge_en *edge)
{
  char buf[128] = {0};
  int fd = -1;
  int rc = 0;

  snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/edge", g->gs_gpio);
  fd = open(buf, O_RDONLY);
  if (fd == -1) {
    rc = errno;
    LOG_ERR(rc, "Failed to open %s", buf);
    return -rc;
  }

  read(fd, buf, sizeof(buf));
  strip(buf);

  if (!strcmp(buf, "none")) {
    *edge = GPIO_EDGE_NONE;
  } else if (!strcmp(buf, "rising")) {
    *edge = GPIO_EDGE_RISING;
  } else if (!strcmp(buf, "falling")) {
    *edge = GPIO_EDGE_FALLING;
  } else if (!strcmp(buf, "both")) {
    *edge = GPIO_EDGE_BOTH;
  } else {
    rc = -1;
  }
  close(fd);
  return rc;
}

int gpio_export(int gpio)
{
  char buf[128] = {0};
  int fd = -1;
  int rc = 0;
  int len;

  snprintf(buf, sizeof(buf), "/sys/class/gpio/export");
  fd = open(buf, O_WRONLY);
  if (fd == -1) {
    rc = errno;
    LOG_ERR(rc, "Failed to open %s", buf);
    return -rc;
  }

  len = snprintf(buf, sizeof(buf), "%d", gpio);
  write(fd, buf, len);
  close(fd);

  return rc;
}

int gpio_unexport_legacy(int gpio)
{
  char buf[128] = {0};
  int fd = -1;
  int rc = 0;
  int len;

  snprintf(buf, sizeof(buf), "/sys/class/gpio/unexport");
  fd = open(buf, O_WRONLY);
  if (fd == -1) {
    rc = errno;
    LOG_ERR(rc, "Failed to open %s", buf);
    return -rc;
  }

  len =  snprintf(buf, sizeof(buf), "%d", gpio);
  write(fd, buf, len);
  close(fd);

  return rc;
}

gpio_value_en gpio_get(int gpio)
{
  gpio_st g;
  gpio_value_en ret;

  if (gpio_open(&g, gpio))
    return GPIO_VALUE_INVALID;
  ret = gpio_read(&g);
  gpio_close(&g);
  return ret;
}

int gpio_set(int gpio, gpio_value_en val)
{
  gpio_st g;
  gpio_value_en read_val;

  if (gpio_open(&g, gpio))
    return GPIO_VALUE_INVALID;
  gpio_write(&g, val);
  read_val = gpio_read(&g);
  gpio_close(&g);
  return val == read_val ? 0 : -1;
}

int gpio_poll_open_legacy(gpio_poll_st *gpios, int count)
{
  int i = 0;
  int rc = 0;

  for ( i = 0; i < count; i++) {
    /* If name is provided, use it to calculated the GPIO num,
     * else assume that the number is provided */
    if (gpios[i].name[0] != '\0') {
      gpios[i].gs.gs_gpio = gpio_num(gpios[i].name);
      if (gpios[i].gs.gs_gpio < 0) {
        return -1;
      }
    }
    gpio_export(gpios[i].gs.gs_gpio);
    if (gpio_open(&gpios[i].gs, gpios[i].gs.gs_gpio)) {
        rc = errno;
        LOG_ERR(rc, "gpio_open for %d failed!", gpios[i].gs.gs_gpio);
        continue;
     }
     gpio_change_direction(&gpios[i].gs,  GPIO_DIRECTION_IN);
     gpio_change_edge(&gpios[i].gs, gpios[i].edge);
  }
  return 0;
}

static void *gpio_poll_pin(void *arg)
{
  gpio_poll_st *gpios = (gpio_poll_st *)arg;
  struct pollfd fdset;
  int rc;

  while (1) {
    memset((void *)&fdset, 0, sizeof(fdset));
    fdset.fd = gpios->gs.gs_fd;
    fdset.events = POLLPRI;
    gpios->value = gpio_read(&gpios->gs);

    rc = poll(&fdset, 1, -1); /* -1 : Forever */
    if (rc < 0) {
      if (errno == EINTR) {
        continue;
      } else {
        rc = -errno;
        LOG_ERR(rc, "gpio_poll: %s poll() fails\n", gpios[0].desc);
        pthread_exit(&rc);
      }
    }

    if (rc == 0) {
      LOG_ERR(rc, "gpio_poll: %s poll() timeout\n", gpios[0].desc);
      pthread_exit(&rc);
    }

    if (fdset.revents & POLLPRI) {
      gpios->value = gpio_read(&gpios->gs);
      gpios->fp(gpios);
    }
  }

  pthread_exit(&rc);
}

int gpio_poll_legacy(gpio_poll_st *gpios, int count, int timeout)
{
  pthread_t thread_ids[count];
  int ret;
  int i;
  pthread_attr_t attr;

  if (count > MAX_PINS) {
    return -EINVAL;
  }

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, STACK_SIZE);

  for ( i = 0; i < count; i++) {
    // TODO pass timeout value into thread
    if ((ret = pthread_create(&thread_ids[i], &attr, gpio_poll_pin, &gpios[i])) < 0) {
      LOG_ERR(ret, "pthread_create failed for %s\n", gpios[i].desc);
      return ret;
    }
  }

  for ( i = 0; i < count; i++) {
    if ((ret = pthread_join(thread_ids[i], NULL) < 0)) {
      LOG_ERR(ret, "pthread_join failed for %s\n", gpios[i].desc);
    }
  }

  return 0;
}

int gpio_poll_close_legacy(gpio_poll_st *gpios, int count)
{
  int i = 0;

  for ( i = 0; i < count; i++) {
    int gpio = gpios[i].gs.gs_gpio;
    gpio_change_edge(&gpios[i].gs, GPIO_EDGE_NONE);
    gpio_close(&gpios[i].gs);
    gpio_unexport(gpio);
  }
  return 0;
}
