/*
 * sensord
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <openbmc/gpio.h>

#define POLL_TIMEOUT -1 /* Forever */
#define MAX_NUM_SLOTS 4

#define GPIO_BIC_READY_SLOT1_N 107
#define GPIO_BIC_READY_SLOT2_N 106
#define GPIO_BIC_READY_SLOT3_N 109
#define GPIO_BIC_READY_SLOT4_N 108

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"

static int last_bic_ready[MAX_NODES + 1];
static struct timespec last_bic_down_ts[MAX_NODES + 1];
static struct timespec last_bic_up_ts[MAX_NODES + 1];

typedef struct {
  uint8_t def_val;
  char name[64];
  uint8_t num;
  char log[256];
} def_chk_info;

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

// Generic Event Handler for GPIO changes
static void gpio_event_handle(gpio_poll_st *gp)
{
  int value;
  uint8_t slot_id, status;
  char vpath[80] = {0};
  struct timespec ts;

  if ((gp->gs.gs_gpio == gpio_num("GPION2")) || (gp->gs.gs_gpio == gpio_num("GPION3")) ||
      (gp->gs.gs_gpio == gpio_num("GPION4")) || (gp->gs.gs_gpio == gpio_num("GPION5"))) {
    slot_id = (gp->gs.gs_gpio % 2) ? (gp->gs.gs_gpio - 106) : (gp->gs.gs_gpio - 104);

    if (pal_is_fru_prsnt(slot_id, &status) || !status)
      return;
    if (pal_is_server_12v_on(slot_id, &status) || !status)
      return;
    if (last_bic_ready[slot_id] == gp->value)
      return;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    last_bic_ready[slot_id] = gp->value;
    if (gp->value == 1) {
      if (last_bic_down_ts[slot_id].tv_sec && ((ts.tv_sec - last_bic_down_ts[slot_id].tv_sec) < 2)) {
        return;
      }
      last_bic_down_ts[slot_id].tv_sec = ts.tv_sec;

      syslog(LOG_CRIT, "FRU: %u, BIC is down", slot_id);
    } else {
      if (last_bic_up_ts[slot_id].tv_sec && ((ts.tv_sec - last_bic_up_ts[slot_id].tv_sec) < 2)) {
        return;
      }
      last_bic_up_ts[slot_id].tv_sec = ts.tv_sec;

      syslog(LOG_CRIT, "FRU: %u, BIC is up", slot_id);
    }
  }
}

static gpio_poll_st g_gpios[] = {
  // {{gpio, fd}, edge, gpioValue, call-back function, GPIO description}
  {{0, 0}, GPIO_EDGE_BOTH, 0, gpio_event_handle, "GPION2", "FRU: 2, BIC is down"},
  {{0, 0}, GPIO_EDGE_BOTH, 0, gpio_event_handle, "GPION3", "FRU: 1, BIC is down"},
  {{0, 0}, GPIO_EDGE_BOTH, 0, gpio_event_handle, "GPION4", "FRU: 4, BIC is down"},
  {{0, 0}, GPIO_EDGE_BOTH, 0, gpio_event_handle, "GPION5", "FRU: 3, BIC is down"},
};

static int g_count = sizeof(g_gpios) / sizeof(gpio_poll_st);

static def_chk_info def_bic_ready[] = {
  // { default value, gpio name, gpio num, log }
  { 0, "", 0, "" },
  { 0, "GPIO_BIC_READY_SLOT1_N", GPIO_BIC_READY_SLOT1_N, "FRU: 1, BIC is down" },
  { 0, "GPIO_BIC_READY_SLOT2_N", GPIO_BIC_READY_SLOT2_N, "FRU: 2, BIC is down" },
  { 0, "GPIO_BIC_READY_SLOT3_N", GPIO_BIC_READY_SLOT3_N, "FRU: 3, BIC is down" },
  { 0, "GPIO_BIC_READY_SLOT4_N", GPIO_BIC_READY_SLOT4_N, "FRU: 4, BIC is down" },
};

static void default_gpio_check(void) {
  int value;
  uint8_t fru, status;
  char vpath[80] = {0};

  for (fru = 1; fru <= MAX_NUM_SLOTS; fru++) {
    if (pal_is_fru_prsnt(fru, &status) || !status)
      continue;

    if (pal_is_server_12v_on(fru, &status) || !status)
      continue;

    sprintf(vpath, GPIO_VAL, def_bic_ready[fru].num);
    if (read_device(vpath, &value) == 0) {
      if (value != def_bic_ready[fru].def_val) {
        last_bic_ready[fru] = value;
        syslog(LOG_CRIT, def_bic_ready[fru].log);
      }
    }
  }
}

int
main(int argc, void **argv) {
  int i;
  int pid_file, rc;

  for (i = 1; i < MAX_NODES + 1; i++) {
    last_bic_ready[i] = 0;
    last_bic_down_ts[i].tv_sec = 0;
    last_bic_up_ts[i].tv_sec = 0;
  }

  default_gpio_check();

  pid_file = open("/var/run/gpiointrd.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if (rc) {
    if (EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another gpiointrd instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("gpiointrd", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "gpiointrd: daemon started");

    gpio_poll_open(g_gpios, g_count);
    gpio_poll(g_gpios, g_count, POLL_TIMEOUT);
    gpio_poll_close(g_gpios, g_count);
  }

  return 0;
}
