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
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <pthread.h>
#include <string.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include "fby2_common.h"

#define BIT(value, index) ((value >> index) & 1)

#define CRASHDUMP_BIN       "/usr/local/bin/autodump.sh"
#define CRASHDUMP_FILE      "/mnt/data/crashdump_"
#define CRASHDUMP_PID       "/var/run/autodump%d.pid"

#define CPLDDUMP_BIN       "/usr/local/bin/cpld-dump.sh"
#define CPLDDUMP_PID       "/var/run/cplddump%d.pid"

#define SBOOT_CPLDDUMP_BIN       "/usr/local/bin/sboot-cpld-dump.sh"
#define SBOOT_CPLDDUMP_PID       "/var/run/sbootcplddump%d.pid"

#define FAN_CONFIG_FILE "/tmp/fan_config"

#define PTHREAD_SET_CANCEL_ENABLE() do {                                          \
  if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0) {                 \
    syslog(LOG_CRIT, "%s: pthread_setcancelstate failed: %d\n", __func__, errno); \
  }                                                                               \
  if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) != 0) {            \
    syslog(LOG_CRIT, "%s: pthread_setcanceltype failed: %d\n", __func__, errno);  \
  }                                                                               \
} while(0)

#define log_system(cmd)                                                     \
do {                                                                        \
  int sysret = system(cmd);                                                 \
  if (sysret)                                                               \
    syslog(LOG_WARNING, "%s: system command failed, cmd: \"%s\",  ret: %d", \
            __func__, cmd, sysret);                                         \
} while(0)

struct threadinfo {
  uint8_t is_running;
  uint8_t fru;
  pthread_t pt;
};

static struct threadinfo t_dump[MAX_NUM_FRUS] = {0, };

static struct threadinfo t_cpld_dump[MAX_NUM_FRUS] = {0, };

static struct threadinfo t_sboot_cpld_dump[MAX_NUM_FRUS] = {0, };

int
fby2_common_fru_name(uint8_t fru, char *str) {

  switch(fru) {
    case FRU_SLOT1:
      sprintf(str, "slot1");
      break;

    case FRU_SLOT2:
      sprintf(str, "slot2");
      break;

    case FRU_SLOT3:
      sprintf(str, "slot3");
      break;

    case FRU_SLOT4:
      sprintf(str, "slot4");
      break;

    case FRU_SPB:
      sprintf(str, "spb");
      break;

    case FRU_NIC:
      sprintf(str, "nic");
      break;

    case FRU_BMC:
      sprintf(str, "bmc");
      break;

    default:
#ifdef DEBUG
      syslog(LOG_WARNING, "fby2_common_fru_id: Wrong fru id");
#endif
    return -1;
  }

  return 0;
}

int
fby2_common_fru_id(char *str, uint8_t *fru) {

  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "slot1")) {
    *fru = FRU_SLOT1;
  } else if (!strcmp(str, "slot2")) {
    *fru = FRU_SLOT2;
  } else if (!strcmp(str, "slot3")) {
    *fru = FRU_SLOT3;
  } else if (!strcmp(str, "slot4")) {
    *fru = FRU_SLOT4;
  } else if (!strcmp(str, "spb")) {
    *fru = FRU_SPB;
  } else if (!strcmp(str, "nic")) {
    *fru = FRU_NIC;
  } else if (!strcmp(str, "bmc")) {
    *fru = FRU_BMC;
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "fby2_common_fru_id: Wrong fru id");
#endif
    return -1;
  }

  return 0;
}

int
fby2_common_dev_id(char *str, uint8_t *dev) {

  if (!strcmp(str, "all")) {
    *dev = 0;
  } else if (!strcmp(str, "device0")) {
    *dev = 1;
  } else if (!strcmp(str, "device1")) {
    *dev = 2;
  } else if (!strcmp(str, "device2")) {
    *dev = 3;
  } else if (!strcmp(str, "device3")) {
    *dev = 4;
  } else if (!strcmp(str, "device4")) {
    *dev = 5;
  } else if (!strcmp(str, "device5")) {
    *dev = 6;
  } else if (!strcmp(str, "device6")) {
    *dev = 7;
  } else if (!strcmp(str, "device7")) {
    *dev = 8;
  } else if (!strcmp(str, "device8")) {
    *dev = 9;
  } else if (!strcmp(str, "device9")) {
    *dev = 10;
  } else if (!strcmp(str, "device10")) {
    *dev = 11;
  } else if (!strcmp(str, "device11")) {
    *dev = 12;
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "fby2_common_dev_id: Wrong fru id");
#endif
    return -1;
  }

  return 0;
}

static int
trigger_hpr(uint8_t fru) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};

#ifndef CONFIG_FBY2_ND
  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(key, "slot%u_trigger_hpr", fru);
      if (!kv_get(key, value, NULL, KV_FPERSIST) && !strcmp(value, "off")) {
        return 0;
      }
      return 1;
  }
#endif
  return 0;
}

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

int
fby2_common_get_spb_rev(void) {
  int rev, value;
  int retry = 3;
  char rev_value[MAX_VALUE_LEN] = {0};

  if (read_device(SPB_REV_FILE, &rev)) {
    if(fby2_common_get_gpio_val("BOARD_REV_ID2", &value))
      return -1;
    rev = value * 4;
    if (fby2_common_get_gpio_val("BOARD_REV_ID1", &value))
      return -1;
    rev += value * 2;
    if (fby2_common_get_gpio_val("BOARD_REV_ID0", &value))
      return -1;
    rev += value;
    snprintf(rev_value, sizeof(rev_value),"%d", rev);
    do {
      if (write_device(SPB_REV_FILE, rev_value) == 0)
        break;
      syslog(LOG_WARNING,"fby2_common_get_spb_rev write %s failed",SPB_REV_FILE);
      usleep(10000); // 10 ms
    } while (--retry);
    syslog(LOG_WARNING, "fby2_common_get_spb_rev rev: %d",rev);
    return rev;
  }

  return rev;
}

int
fby2_common_get_gpio_from_cache(char *gpio_shadow, int* gpio_val) {
  char key[MAX_VALUE_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};

  sprintf(key, "gpio_%s", gpio_shadow);

  if (kv_get(key, value, NULL, 0)) {
    if (fby2_common_get_gpio_val(gpio_shadow, gpio_val) != 0) {
      syslog(LOG_WARNING,"%s: get gpio failed, shadow:%s", __func__, gpio_shadow);
      return -1;
    }
    value[0] = (*gpio_val == GPIO_VALUE_HIGH) ? 1:0;
    if (kv_set(key, (char*)&value, 1, KV_FCREATE)) {
      syslog(LOG_WARNING,"%s: kv_set failed, key: %s, val: %s", __func__, key, value);
      return -1;
    }
  } else {
    *gpio_val = value[0];
  }

  if (*gpio_val < 0 || *gpio_val > 1) {
    syslog(LOG_WARNING,"%s: unexpected gpio_val, val: %d", __func__, *gpio_val);
    return -1;
  }

  return 0;
}

int
fby2_common_get_baseboard_id(void) {
  int gpio_val;
  if (fby2_common_get_gpio_from_cache(GPIO_BASEBOARD_ID, &gpio_val)) {
    return -1;
  }
  return gpio_val;
}

int
fby2_common_get_board_id(void) {
  int board_id;
  int retry = 3;
  char value[MAX_VALUE_LEN] = {0};

  if (read_device(SPB_BOARD_ID_FILE, &board_id)) {
    if (fby2_common_get_gpio_val("BOARD_ID", &board_id) != 0)
      return -1;

    snprintf(value, sizeof(value),"%d", board_id);
    do {
      if (write_device(SPB_BOARD_ID_FILE, value) == 0)
        break;
      syslog(LOG_WARNING,"_get_board_id write %s failed",SPB_BOARD_ID_FILE);
      usleep(10000); // 10 ms
    } while (--retry);
    syslog(LOG_WARNING, "_get_board_id board_id: %d",board_id);
    return board_id;
  }

  return board_id;
}

int
_get_spb_type(int *spb_type) {
  int ret = 0;
  int retry = 3;

  do {
    ret = read_device(SPB_TYPE_FILE, spb_type);
    if (ret == 0)
      break;
    syslog(LOG_WARNING, "_get_spb_type failed");
    usleep(10000); // 10 ms
  } while (--retry);

  return ret;
}

int
_set_spb_type(int spb_type) {
  int retry = 3;
  int ret = 0;
  char value[MAX_VALUE_LEN] = {0};

  snprintf(value, sizeof(value),"%d", spb_type);
  do {
    ret = write_device(SPB_TYPE_FILE, value);
    if (ret == 0)
      break;
    syslog(LOG_WARNING,"_set_spb_type failed");
    usleep(10000); // 10 ms
  } while (--retry);

  return ret;
}

/* Baseboard        Board_ID Rev_ID[2] Rev_ID[1] Rev_ID[0] Baesboard_ID
   YV2 ND2 EVT          1       0         0         1           1
   YV2 ND PoC           1       0         0         0           0
   YV2 ND EVT           1       0         0         1           0
   YV2 PoC              0       0         0         0           0
   YV2 EVT              0       0         0         1           0
   YV2 DVT              0       0         1         0           0
   YV2 PVT              0       0         1         1           0
   YV2.50               1       1         X         X           0
*/
int
fby2_common_get_spb_type() {
   int spb_type;
   gpio_value_t baseboard_id;
   gpio_value_t board_id;
   uint8_t rev;

   // Get SPB type from cache
   if ( _get_spb_type(&spb_type) == 0) {
     return spb_type;
   }

   // Baseboard_ID
   baseboard_id = fby2_common_get_baseboard_id();
   if (baseboard_id < 0) {
     syslog(LOG_WARNING, "fby2_common_get_spb_type: fail to get spb baseboard id");
     return -1;
   }
   syslog(LOG_WARNING, "fby2_common_get_spb_type: spb baseboard id %d",baseboard_id);

   // Board_ID
   board_id = fby2_common_get_board_id();
   if (board_id < 0) {
     syslog(LOG_WARNING, "fby2_common_get_spb_type: fail to get spb board id");
     return -1;
   }
   syslog(LOG_WARNING, "fby2_common_get_spb_type: spb board id %d",board_id);

   // Rev_ID
   rev = fby2_common_get_spb_rev();
   if (rev < 0) {
     syslog(LOG_WARNING, "fby2_common_get_spb_type: fail to get spb rev");
     return -1;
   }
   syslog(LOG_WARNING, "fby2_common_get_spb_type: spb rev %d",rev);

   if (GPIO_VALUE_HIGH == board_id && BIT(rev, 2)) {
     spb_type = TYPE_SPB_YV250;
   } else if ((GPIO_VALUE_HIGH == board_id) && (0 == BIT(rev, 2))) {
      if (baseboard_id == GPIO_VALUE_LOW) {
        spb_type = TYPE_SPB_YV2ND;
      } else {
        spb_type = TYPE_SPB_YV2ND2;
      }
   } else {
     spb_type = TYPE_SPB_YV2;
   }

   // Set SPB type to cache
   if (_set_spb_type(spb_type)) {
     syslog(LOG_WARNING, "fby2_common_get_spb_type: fail to set spb type");
   }
   syslog(LOG_WARNING, "fby2_common_get_spb_type: spb type %d",spb_type);

   return spb_type;
}

int
_get_fan_type(int *fan_type) {
  int ret = 0;
  int retry = 3;

  do {
    ret = read_device(FAN_TYPE_FILE, fan_type);
    if (ret == 0)
      break;
    syslog(LOG_WARNING, "_get_fan_type failed");
    usleep(10000); // 10 ms
  } while (--retry);

  return ret;
}

int
_set_fan_type(int fan_type) {
  int retry = 3;
  int ret = 0;
  char value[MAX_VALUE_LEN] = {0};

  snprintf(value, sizeof(value),"%d", fan_type);
  do {
    ret = write_device(FAN_TYPE_FILE, value);
    if (ret == 0)
      break;
    syslog(LOG_WARNING,"_set_fan_type failed");
    usleep(10000); // 10 ms
  } while (--retry);

  return ret;
}

int
fby2_common_get_fan_type() {
   int fan_type;

   if (_get_fan_type(&fan_type) == 0) {
     return fan_type;
   }

   if (fby2_common_get_gpio_val("DUAL_FAN_DETECT", &fan_type) != 0) {
      return -1;
   }

   if (_set_fan_type(fan_type) ) {
     syslog(LOG_WARNING, "fby2_common_get_fan_type: fail to set fan type");
   }
   syslog(LOG_WARNING, "fby2_common_get_fan_type: fan type %d",fan_type);

   return fan_type;
}

static void *
generate_dump(void *arg) {

  uint8_t fru = *(uint8_t *) arg;
  char cmd[128];
  char fname[112];
  char fruname[16];
  bool ierr;

  // Usually the pthread cancel state are enable by default but
  // here we explicitly would like to enable them
  PTHREAD_SET_CANCEL_ENABLE();

  fby2_common_fru_name(fru, fruname);

  memset(fname, 0, sizeof(fname));
  snprintf(fname, 112, "/var/run/autodump%d.pid", fru);
  if (access(fname, F_OK) == 0) {
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd,"rm %s",fname);
    log_system(cmd);
  }

  // Execute automatic crashdump
  memset(cmd, 0, 128);
  sprintf(cmd, "%s %s", CRASHDUMP_BIN, fruname);
  log_system(cmd);

  t_dump[fru-1].is_running = 0;

  if (!fby2_common_get_ierr(fru, &ierr) && ierr && trigger_hpr(fru)) {
    sprintf(cmd, "/usr/local/bin/power-util %s reset", fruname);
    log_system(cmd);
  }
  pthread_exit(NULL);
}

static void *
second_dwr_dump(void *arg) {

  uint8_t fru = *(uint8_t *) arg;
  char cmd[128];
  char fname[112];
  char fruname[16];

  // Usually the pthread cancel state are enable by default but
  // here we explicitly would like to enable them
  PTHREAD_SET_CANCEL_ENABLE();

  fby2_common_fru_name(fru, fruname);

  memset(fname, 0, sizeof(fname));
  snprintf(fname, 112, "/var/run/autodump%d.pid", fru);
  if (access(fname, F_OK) == 0) {
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd,"rm %s",fname);
    log_system(cmd);
  }

  // Execute automatic crashdump
  memset(cmd, 0, 128);
  syslog(LOG_WARNING, "Start Second/DWR Autodump");
  sprintf(cmd, "%s %s --second", CRASHDUMP_BIN, fruname);
  log_system(cmd);

  t_dump[fru-1].is_running = 0;

  pthread_exit(NULL);
}

int
fby2_common_crashdump(uint8_t fru, bool ierr, bool platform_reset) {

  int ret;
  char cmd[100];

  // Check if the crashdump script exist
  if (access(CRASHDUMP_BIN, F_OK) == -1) {
    syslog(LOG_CRIT, "fby2_common_crashdump:Crashdump for FRU: %d failed : "
        "auto crashdump binary is not preset", fru);
    return 0;
  }

  // Check if a crashdump for that fru is already running
  if (t_dump[fru-1].is_running == 2) {
    syslog(LOG_WARNING, "fby2_common_crashdump: second_dwr_dump for FRU %d is running\n", fru);
    return 0;
  }

  if (ierr) {
    fby2_common_set_ierr(fru, true);
  }

  // If yes, kill that thread and start a new one
  if (t_dump[fru-1].is_running) {
    ret = pthread_cancel(t_dump[fru-1].pt);
    if (ret == ESRCH) {
      syslog(LOG_INFO, "fby2_common_crashdump: No Crashdump pthread exists");
    } else {
      pthread_join(t_dump[fru-1].pt, NULL);
      sprintf(cmd, "ps | grep '{dump.sh}' | grep 'slot%d' | awk '{print $1}'| xargs kill", fru);
      log_system(cmd);
      sprintf(cmd, "ps | grep 'me-util' | grep 'slot%d' | awk '{print $1}'| xargs kill", fru);
      log_system(cmd);
      sprintf(cmd, "ps | grep 'bic-util' | grep 'slot%d' | awk '{print $1}'| xargs kill", fru);
      log_system(cmd);
#ifdef DEBUG
      syslog(LOG_INFO, "fby2_common_crashdump: Previous crashdump thread is cancelled");
#endif
      if (platform_reset) {
        sprintf(cmd, "tar zcf %suncompleted_slot%d.tar.gz -C /mnt/data crashdump_slot%d", CRASHDUMP_FILE,fru,fru);
        log_system(cmd);
      }
    }
  }

  // Start a thread to generate the crashdump
  t_dump[fru-1].fru = fru;
  if (platform_reset) {
    if (pthread_create(&(t_dump[fru-1].pt), NULL, second_dwr_dump, (void*) &t_dump[fru-1].fru) < 0) {
      syslog(LOG_WARNING, "fby2_common_crashdump: pthread_create for FRU %d failed\n", fru);
      return -1;
    }
    t_dump[fru-1].is_running = 2;
  } else {
    if (pthread_create(&(t_dump[fru-1].pt), NULL, generate_dump, (void*) &t_dump[fru-1].fru) < 0) {
      syslog(LOG_WARNING, "fby2_common_crashdump: pthread_create for FRU %d failed\n", fru);
      return -1;
    }
    t_dump[fru-1].is_running = 1;
  }

  syslog(LOG_INFO, "fby2_common_crashdump: Crashdump for FRU: %d is being generated.", fru);

  return 0;
}

int
fby2_common_set_ierr(uint8_t fru, bool value) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(key, "slot%d_ierr", fru);
      break;

    default:
      return -1;
  }

  sprintf(cvalue, (value == true) ? "1": "0");

  return kv_set(key, cvalue, 0, 0);
}

int
fby2_common_get_ierr(uint8_t fru, bool *value) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  int ret;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      sprintf(key, "slot%d_ierr", fru);
      break;

    default:
      return -1;
  }

  ret = kv_get(key, cvalue, NULL, 0);
  if (ret) {
    return ret;
  }
  if (atoi(cvalue)){
    *value = true;
  } else {
    *value = false;
  }
  return 0;
}

void *
generate_cpld_dump(void *arg) {

  uint8_t fru = *(uint8_t *) arg;
  char cmd[128];
  char fname[112];
  char fruname[16];

  // Usually the pthread cancel state are enable by default but
  // here we explicitly would like to enable them
  PTHREAD_SET_CANCEL_ENABLE();

  fby2_common_fru_name(fru, fruname);

  memset(fname, 0, sizeof(fname));
  sprintf(fname, CPLDDUMP_PID, fru);
  if (access(fname, F_OK) == 0) {
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd,"rm %s",fname);
    log_system(cmd);
  }

  // Execute automatic CPLD dump
  memset(cmd, 0, 128);
  sprintf(cmd, "%s %s", CPLDDUMP_BIN, fruname);
  log_system(cmd);

  t_cpld_dump[fru-1].is_running = 0;

  pthread_exit(NULL);
}

void *
generate_sboot_cpld_dump(void *arg) {

  uint8_t fru = *(uint8_t *) arg;
  char cmd[128];
  char fname[120];
  char fruname[16];

  // Usually the pthread cancel state are enable by default but
  // here we explicitly would like to enable them
  PTHREAD_SET_CANCEL_ENABLE();

  fby2_common_fru_name(fru, fruname);

  memset(fname, 0, sizeof(fname));
  sprintf(fname, SBOOT_CPLDDUMP_PID, fru);
  if (access(fname, F_OK) == 0) {
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd,"rm %s",fname);
    log_system(cmd);
  }

  // Execute automatic CPLD dump for slow boot
  memset(cmd, 0, 128);
  sprintf(cmd, "%s %s", SBOOT_CPLDDUMP_BIN, fruname);
  log_system(cmd);

  t_sboot_cpld_dump[fru-1].is_running = 0;

  pthread_exit(NULL);
}

int
fby2_common_cpld_dump(uint8_t fru) {

  int ret;
  char cmd[100];

  // Check if the CPLD dump script exist
  if (access(CPLDDUMP_BIN, F_OK) == -1) {
    syslog(LOG_CRIT, "fby2_common_cpld_dump:CPLD dump for FRU: %d failed : "
        "cpld dump binary is not present", fru);
    return 0;
  }

  // Check if CPLD dump for that fru is already running.
  // If yes, kill that thread and start a new one.
  if (t_cpld_dump[fru-1].is_running) {
    ret = pthread_cancel(t_cpld_dump[fru-1].pt);
    if (ret == ESRCH) {
      syslog(LOG_INFO, "fby2_common_cpld_dump: No CPLD dump pthread exists");
    } else {
      pthread_join(t_cpld_dump[fru-1].pt, NULL);
      sprintf(cmd, "ps | grep '{cpld-dump.sh}' | grep 'slot%d' | awk '{print $1}'| xargs kill", fru);
      log_system(cmd);
#ifdef DEBUG
      syslog(LOG_INFO, "fby2_common_cpld_dump: Previous CPLD dump thread is cancelled");
#endif
    }
  }

  // Start a thread to generate the CPLD dump
  t_cpld_dump[fru-1].fru = fru;

  if (pthread_create(&(t_cpld_dump[fru-1].pt), NULL, generate_cpld_dump, (void*) &t_cpld_dump[fru-1].fru) < 0) {
    syslog(LOG_WARNING, "fby2_common_cpld_dump: pthread_create for"
      " FRU %d failed\n", fru);
    return -1;
  }

  t_cpld_dump[fru-1].is_running = 1;

  syslog(LOG_INFO, "fby2_common_cpld_dump: CPLD dump for FRU: %d is being generated.", fru);

  return 0;
}

int
fby2_common_sboot_cpld_dump(uint8_t fru) {

  int ret;
  char cmd[100];

  // Check if the CPLD dump script exist for slow boot
  if (access(SBOOT_CPLDDUMP_BIN, F_OK) == -1) {
    syslog(LOG_CRIT, "fby2_common_sboot_cpld_dump:CPLD dump for FRU: %d failed : "
        "cpld dump binary for slow boot is not present", fru);
    return 0;
  }

  // Check if CPLD dump of slow boot for that fru is already running.
  // If yes, kill that thread and start a new one.
  if (t_sboot_cpld_dump[fru-1].is_running) {
    ret = pthread_cancel(t_sboot_cpld_dump[fru-1].pt);
    if (ret == ESRCH) {
      syslog(LOG_INFO, "fby2_common_sboot_cpld_dump: No CPLD dump pthread exists for slow boot");
    } else {
      pthread_join(t_sboot_cpld_dump[fru-1].pt, NULL);
      sprintf(cmd, "ps | grep '{sboot-cpld-dump}' | grep 'slot%d' | awk '{print $1}'| xargs kill", fru);
      log_system(cmd);
#ifdef DEBUG
      syslog(LOG_INFO, "fby2_common_sboot_cpld_dump: Previous CPLD dump thread is cancelled for slow boot");
#endif
    }
  }

  // Start a thread to generate the CPLD dump for slow boot
  t_sboot_cpld_dump[fru-1].fru = fru;

  if (pthread_create(&(t_sboot_cpld_dump[fru-1].pt), NULL, generate_sboot_cpld_dump, (void*) &t_sboot_cpld_dump[fru-1].fru) < 0) {
    syslog(LOG_WARNING, "fby2_common_sboot_cpld_dump: pthread_create for"
      " FRU %d failed\n", fru);
    return -1;
  }

  t_sboot_cpld_dump[fru-1].is_running = 1;

  syslog(LOG_INFO, "fby2_common_sboot_cpld_dump: CPLD dump for slow boot for FRU: %d is being generated.", fru);

  return 0;
}

int
fby2_common_get_fan_config(void) {
  FILE *fp;
  int type = 0;

  fp = fopen(FAN_CONFIG_FILE, "r");
  if (fp != NULL) {
    if (fscanf(fp, "%d", &type) != 1)
      goto error_out;
    fclose(fp);
  }

  return (type)?TYPE_15K_FAN:TYPE_10K_FAN;
error_out:
  if (fp != NULL)
    fclose(fp);
  return -1;
}

int
fby2_common_set_gpio_val(char *shadow, int val) {
  int ret = 0;
  gpio_desc_t *gdesc = NULL;

  gdesc = gpio_open_by_shadow(shadow);
  if (gdesc == NULL){
    syslog(LOG_WARNING, "Fail to open GPIO shadow %s", shadow);
    return -1;
  }
  ret = gpio_set_value(gdesc, val);
  if (ret != 0) {
    syslog(LOG_WARNING, "Fail to set GPIO %s", shadow);
    ret = -1;
  }
  gpio_close(gdesc);

  return ret;
}

int
fby2_common_get_gpio_val(char *shadow, int *val) {
  int ret = 0;
  gpio_desc_t *gdesc = NULL;

  gdesc = gpio_open_by_shadow(shadow);
  if (gdesc == NULL){
    syslog(LOG_WARNING, "Fail to open GPIO shadow %s", shadow);
    return -1;
  }
  ret = gpio_get_value(gdesc, val);
  if (ret != 0) {
    syslog(LOG_WARNING, "Fail to get GPIO %s", shadow);
    ret = -1;
  }
  gpio_close(gdesc);

  return ret;
}
