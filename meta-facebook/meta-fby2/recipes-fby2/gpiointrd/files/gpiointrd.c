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
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <openbmc/gpio.h>
#include <openbmc/fruid.h>
#include <facebook/bic.h>
#include <facebook/fby2_gpio.h>
#include <facebook/fby2_sensor.h>
#include <facebook/fby2_common.h>

#define POLL_TIMEOUT -1 /* Forever */
#define MAX_NUM_SLOTS       4

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"

#define SLOT_RECORD_FILE "/tmp/slot%d.rc"
#define SV_TYPE_RECORD_FILE "/tmp/server_type%d.rc"

#define HOTSERVICE_SCRPIT "/usr/local/bin/hotservice-reinit.sh"
#define HOTSERVICE_FILE "/tmp/slot%d_reinit"
#define HSLOT_PID  "/tmp/slot%u_reinit.pid"
#define PWR_UTL_LOCK "/var/run/power-util_%d.lock"
#define POST_FLAG_FILE "/tmp/cache_store/slot%d_post_flag"
#define SYS_CONFIG_FILE "/mnt/data/kv_store/sys_config/fru%d_*"
#define SENSORDUMP_BIN "/usr/local/bin/sensordump.sh"


#define DEBUG_ME_EJECTOR_LOG 0 // Enable log "GPIO_SLOTX_EJECTOR_LATCH_DETECT_N is 1 and SLOT_12v is ON" before mechanism issue is fixed

static uint8_t IsHotServiceStart[MAX_NODES + 1] = {0};
static void *hsvc_event_handler(void *ptr);
static pthread_mutex_t hsvc_mutex[MAX_NODES + 1];
static struct timespec last_ejector_ts[MAX_NODES + 1];
static uint8_t IsLatchOpenStart[MAX_NODES + 1] = {0};
static void *latch_open_handler(void *ptr);
static pthread_mutex_t latch_open_mutex[MAX_NODES + 1];
static uint8_t dev_fru_complete[MAX_NODES + 1][MAX_NUM_DEVS + 1] = {DEV_FRU_NOT_COMPLETE};

char *fru_prsnt_log_string[3 * MAX_NUM_FRUS] = {
  // slot1, slot2, slot3, slot4
 "", "Slot1 Removal", "Slot2 Removal", "Slot3 Removal", "Slot4 Removal", "",
 "", "Slot1 Insertion", "Slot2 Insertion", "Slot3 Insertion", "Slot4 Insertion", "",
 "", "Slot1 Removal Without 12V-OFF", "Slot2 Removal Without 12V-OFF", "Slot3 Removal Without 12V-OFF", "Slot4 Removal Without 12V-OFF",
};

const static uint8_t gpio_12v[] = { 0, GPIO_P12V_STBY_SLOT1_EN, GPIO_P12V_STBY_SLOT2_EN, GPIO_P12V_STBY_SLOT3_EN, GPIO_P12V_STBY_SLOT4_EN };

const static uint8_t gpio_slot_latch[] = { 0, GPIO_SLOT1_EJECTOR_LATCH_DETECT_N, GPIO_SLOT2_EJECTOR_LATCH_DETECT_N, GPIO_SLOT3_EJECTOR_LATCH_DETECT_N, GPIO_SLOT4_EJECTOR_LATCH_DETECT_N };

struct threadinfo {
  uint8_t is_running;
  uint8_t fru;
  pthread_t pt;
};
static struct threadinfo t_fru_cache[MAX_NUM_FRUS] = {0, };

typedef struct {
  uint8_t def_val;
  char name[64];
  uint8_t num;
  char log[256];
} def_chk_info;

struct delayed_log {
  useconds_t usec;
  char msg[256];
};

typedef struct {
  uint8_t slot_id;
  uint8_t action;
} hot_service_info;

enum {
  REMOVAl = 0,
  INSERTION = 1,
};

slot_kv_st slot_kv_list[] = {
  // {slot_key, slot_def_val}
  {"pwr_server%d_last_state", "on"},
  {"sysfw_ver_slot%d",         "0"},
  {"slot%d_por_cfg",         "lps"},
  {"slot%d_sensor_health",     "1"},
  {"slot%d_sel_error",         "1"},
  {"slot%d_boot_order",  "0000000"},
  {"slot%d_cpu_ppin",          "0"},
  {"fru%d_restart_cause",      "3"},
};

static char *gpv2_m2_list[MAX_NUM_DEVS] = {
  "m2a",
  "m2b",
  "m2c",
  "m2d",
  "m2e",
  "m2f",
  "m2g",
  "m2h",
  "m2i",
  "m2j",
  "m2k",
  "m2l",
};

// Thread for delay event
static void *
delay_log(void *arg)
{
  struct delayed_log* log = (struct delayed_log*)arg;

  pthread_detach(pthread_self());

  if (arg) {
    usleep(log->usec);
    syslog(LOG_CRIT, log->msg);

    free(arg);
  }

  pthread_exit(NULL);
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

static void log_gpio_change(gpio_poll_st *gp, useconds_t log_delay)
{
  if (log_delay == 0) {
    syslog(LOG_CRIT, "%s: %s - %s\n", gp->value ? "DEASSERT": "ASSERT", gp->name, gp->desc);
  } else {
    pthread_t tid_delay_log;
    struct delayed_log *log = (struct delayed_log *)malloc(sizeof(struct delayed_log));
    if (log) {
      log->usec = log_delay;
      snprintf(log->msg, 256, "%s: %s - %s\n", gp->value ? "DEASSERT" : "ASSERT", gp->name, gp->value);
      if (pthread_create(&tid_delay_log, NULL, delay_log, (void *)log)) {
        free(log);
        log = NULL;
      }
    }
    if (!log) {
      syslog(LOG_CRIT, "%s: %s - %s\n", gp->value ? "DEASSERT": "ASSERT", gp->name, gp->desc);
    }
  }
}

static void
create_sensordump(void) {
  system(SENSORDUMP_BIN);
}

static void *
hsc_alert_handler(void *arg) {
  pthread_detach(pthread_self());

  if (fby2_check_hsc_sts_iout(0x20) == 1) {  // bit5: IOUT_OC_WARN
    syslog(LOG_CRIT, "ASSERT: OC_warning triggered (falling edge detected for SMB_HOTSWAP_ALERT_N)");
    create_sensordump();
    sleep(1);
    fby2_check_hsc_fault();
  }

  pthread_exit(NULL);
}

static int
fruid_cache_init(uint8_t slot_id, uint8_t fru_id) {
  int ret;
  int fru_size = 0;
  char fruid_temp_path[64] = {0};
  char fruid_path[64] = {0};

  if (fru_id == 0) {
    sprintf(fruid_temp_path, "/tmp/tfruid_slot%d.bin", slot_id);
    sprintf(fruid_path, "/tmp/fruid_slot%d.bin", slot_id);
  } else {
    sprintf(fruid_temp_path, "/tmp/tfruid_slot%d_dev%d.bin", slot_id, fru_id);
    sprintf(fruid_path, "/tmp/fruid_slot%d_dev%d.bin", slot_id, fru_id);
  }

  ret = bic_read_fruid(slot_id, fru_id, fruid_temp_path, &fru_size);
  if (ret) {
    syslog(LOG_WARNING, "fruid_cache_init: bic_read_fruid slot%d dev=%d returns %d, fru_size: %d\n", slot_id, fru_id, ret, fru_size);
  }

  rename(fruid_temp_path, fruid_path);

  return ret;
}

static void *
fru_cache_dump(void *arg) {
  uint8_t fru = *(uint8_t *) arg;
  uint8_t self_test_result[2] = {0};
  char key[MAX_KEY_LEN];
  char buf[MAX_VALUE_LEN];
  int ret;
  int retry;
  uint8_t status = DEVICE_POWER_OFF;
  uint8_t type = DEV_TYPE_UNKNOWN;
  uint8_t dev_id;
  int max_retry = 3;
  int oldstate;
  fruid_info_t fruid;

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  // Check BIC Self Test Result
  do {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    ret = bic_get_self_test_result(fru, self_test_result);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    if (ret == 0) {
      syslog(LOG_INFO, "bic_get_self_test_result: %X %X\n", self_test_result[0], self_test_result[1]);
      break;
    }
    sleep(5);
  } while (ret != 0);

  sleep(2); //wait for BIC poll at least one cycle

  // Get GPV2 devices' FRU
  for (dev_id = 1; dev_id <= MAX_NUM_DEVS; dev_id++) {
    if (dev_fru_complete[fru][dev_id] != DEV_FRU_NOT_COMPLETE)
      continue;

    //check for power status
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    ret = pal_get_device_power(fru, dev_id, &status, &type);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    syslog(LOG_WARNING, "fru_cache_dump: Slot%u Dev%d power=%u type=%u", fru, dev_id, status, type);

    if (ret == 0) {
      if (status == DEVICE_POWER_ON) {
        if (type == DEV_TYPE_POC) {
          retry = 0;
          do {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
            ret = fruid_cache_init(fru, dev_id);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);

            if (ret == 0)
              break;

            retry++;
            sleep(1);
          } while (retry < max_retry);

          if (retry >= max_retry) {
            syslog(LOG_WARNING, "fru_cache_dump: Fail on getting Slot%u Dev%d FRU", fru, dev_id);
          } else {
            pal_get_dev_fruid_path(fru, dev_id, buf);
            //check file's checksum
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
            ret = fruid_parse(buf, &fruid);
            if (ret != 0) { // Fail
              syslog(LOG_WARNING, "fru_cache_dump: Slot%u Dev%d FRU data checksum is invalid", fru, dev_id);
            } else { // Success
              free_fruid_info(&fruid);
              dev_fru_complete[fru][dev_id] = DEV_FRU_COMPLETE;
              syslog(LOG_WARNING, "fru_cache_dump: Finish getting Slot%u Dev%d FRU", fru, dev_id);
            }
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
          }
        } else { // SSD or unknown
          syslog(LOG_WARNING, "fru_cache_dump: Slot%u Dev%d ignore FRU", fru, dev_id);
          dev_fru_complete[fru][dev_id] = DEV_FRU_IGNORE;
        }
      }

      sprintf(key, "slot%u_%s_pres", fru, gpv2_m2_list[dev_id-1]);
      sprintf(buf, "%u", status);
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
      if (kv_set(key, buf, 0, 0) < 0) {
        syslog(LOG_WARNING, "fru_cache_dump: kv_set Slot%u Dev%d present status failed", fru, dev_id);
      }
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    } else { // Device Status Unknown
      syslog(LOG_WARNING, "fru_cache_dump: Fail to access Slot%u Dev%d power status", fru, dev_id);
    }
  }

  t_fru_cache[fru-1].is_running = 0;
  syslog(LOG_INFO, "%s: FRU %d cache is finished.", __func__, fru);

  pthread_detach(pthread_self());
  pthread_exit(0);
}

int
fru_cahe_init(uint8_t fru) {
  int ret;
  uint8_t idx;

  if (fru != FRU_SLOT1 && fru != FRU_SLOT3) {
    return -1;
  }
  idx = fru - 1;

  // If yes, kill that thread and start a new one
  if (t_fru_cache[idx].is_running) {
    ret = pthread_cancel(t_fru_cache[idx].pt);
    if (ret == ESRCH) {
      syslog(LOG_INFO, "%s: No dump FRU cache pthread exists", __func__);
    } else {
      pthread_join(t_fru_cache[idx].pt, NULL);
      syslog(LOG_INFO, "%s: Previous dump FRU cache thread is cancelled", __func__);
    }
  }

  // Start a thread to generate the FRU
  t_fru_cache[idx].fru = fru;
  if (pthread_create(&t_fru_cache[idx].pt, NULL, fru_cache_dump, (void *)&t_fru_cache[idx].fru) < 0) {
    syslog(LOG_WARNING, "%s: pthread_create for FRU %d failed", __func__, fru);
    return -1;
  }
  t_fru_cache[idx].is_running = 1;
  syslog(LOG_INFO, "%s: FRU %d cache is being generated.", __func__, fru);

  return 0;
}


// Generic Event Handler for GPIO changes
static void gpio_event_handle(gpio_poll_st *gp)
{
  char cmd[128] = {0};
  uint8_t slot_id;
  uint8_t slot_12v = 1;
  int value;
  char vpath[80] = {0};
  char locstr[MAX_VALUE_LEN];
  static bool prsnt_assert[MAX_NODES + 1]={0};
  static pthread_t hsvc_action_tid[MAX_NODES + 1];
  static pthread_t latch_open_tid[MAX_NODES + 1];
  hot_service_info hsvc_info[MAX_NODES + 1];
  struct timespec ts;
  pthread_t hsc_alert_tid;

  if (gp->gs.gs_gpio == gpio_num("GPIOH5")) { // GPIO_FAN_LATCH_DETECT
    if (gp->value == 1) { // low to high
      syslog(LOG_CRIT, "ASSERT: SLED is not seated");
      memset(cmd, 0, sizeof(cmd));
      sprintf(cmd, "sv stop fscd ; /usr/local/bin/fan-util --set 100");
      system(cmd);
    }
    else { // high to low
      syslog(LOG_CRIT, "DEASSERT: SLED is seated");
      memset(cmd, 0, sizeof(cmd));
      sprintf(cmd, "/etc/init.d/setup-fan.sh ; sv start fscd");
      system(cmd);
    }
  }
  else if (gp->gs.gs_gpio == gpio_num("GPIOP0") || gp->gs.gs_gpio == gpio_num("GPIOP1") ||
           gp->gs.gs_gpio == gpio_num("GPIOP2") || gp->gs.gs_gpio == gpio_num("GPIOP3")
          ) //  GPIO_SLOT1/2/3/4_EJECTOR_LATCH_DETECT_N
  {
    slot_id = (gp->gs.gs_gpio - GPIO_SLOT1_EJECTOR_LATCH_DETECT_N) + 1;
    if (gp->value == 1) { // low to high
      clock_gettime(CLOCK_MONOTONIC, &ts);
      if (last_ejector_ts[slot_id].tv_sec && ((ts.tv_sec - last_ejector_ts[slot_id].tv_sec) < 5)) {
        return;
      }
      last_ejector_ts[slot_id].tv_sec = ts.tv_sec;

      sprintf(vpath, GPIO_VAL, GPIO_FAN_LATCH_DETECT);
      read_device(vpath, &value);

      // 12V action would be triggered when SLED is pulled out
      // if SLED is seated, log the event that slot latch is open
      if (!value) {
        syslog(LOG_CRIT,"FRU: %u, SLOT%u_EJECTOR_LATCH is not closed while in VCubby", slot_id, slot_id);
      } else {
        syslog(LOG_CRIT,"FRU: %u, SLOT%u_EJECTOR_LATCH is not closed", slot_id, slot_id);
        pthread_mutex_lock(&latch_open_mutex[slot_id]);
        if ( IsLatchOpenStart[slot_id] )
        {
#ifdef DEBUG
          syslog(LOG_WARNING, "[%s] Close the previous thread for slot%x\n", __func__, slot_id);
#endif
          pthread_cancel(latch_open_tid[slot_id]);
        }
        //Create thread for latch open detect
        value = slot_id;
        if (pthread_create(&latch_open_tid[slot_id], NULL, latch_open_handler, (void *)value) < 0) {
          syslog(LOG_WARNING, "[%s] Create latch_open_handler thread failed for slot%u", __func__, slot_id);
          pthread_mutex_unlock(&latch_open_mutex[slot_id]);
          return;
        }
        IsLatchOpenStart[slot_id] = true;
        pthread_mutex_unlock(&latch_open_mutex[slot_id]);
      } // end of SLED seated check
    } //End of low to high
  } //End of GPIO_SLOT1/2/3/4_EJECTOR_LATCH_DETECT_N
  else if (gp->gs.gs_gpio == gpio_num("GPIOZ0") || gp->gs.gs_gpio == gpio_num("GPIOZ1")   ||
           gp->gs.gs_gpio == gpio_num("GPIOZ2") || gp->gs.gs_gpio == gpio_num("GPIOZ3")   ||
           gp->gs.gs_gpio == gpio_num("GPIOAA0") || gp->gs.gs_gpio == gpio_num("GPIOAA1") ||
           gp->gs.gs_gpio == gpio_num("GPIOAA2") || gp->gs.gs_gpio == gpio_num("GPIOAA3")
          ) // GPIO_SLOT1/2/3/4_PRSNT_B_N, GPIO_SLOT1/2/3/4_PRSNT_N

  {
    if (gp->gs.gs_gpio >= GPIO_SLOT1_PRSNT_B_N && gp->gs.gs_gpio <= GPIO_SLOT4_PRSNT_B_N)
       slot_id = (gp->gs.gs_gpio - GPIO_SLOT1_PRSNT_B_N) + 1;
    else if (gp->gs.gs_gpio >= GPIO_SLOT1_PRSNT_N && gp->gs.gs_gpio <= GPIO_SLOT4_PRSNT_N)
       slot_id = (gp->gs.gs_gpio - GPIO_SLOT1_PRSNT_N) + 1;

    // Use flag to ignore the interrupt of pair present pin
    if (prsnt_assert[slot_id])
    {
       prsnt_assert[slot_id] = false;
       return;
    }

    sprintf(vpath, GPIO_VAL, GPIO_FAN_LATCH_DETECT);
    read_device(vpath, &value);

    // HOT SERVER event would be detected when SLED is pulled out
    if (value) {
       prsnt_assert[slot_id] = true;

       if (gp->value == 1) { // SLOT Removal
          hsvc_info[slot_id].slot_id = slot_id;
          hsvc_info[slot_id].action = REMOVAl;

          pthread_mutex_lock(&hsvc_mutex[slot_id]);
          if ( IsHotServiceStart[slot_id] )
          {
#ifdef DEBUG
            syslog(LOG_WARNING, "[%s] Close the previous thread for slot%x\n", __func__, slot_id);
#endif
            pthread_cancel(hsvc_action_tid[slot_id]);
          }

          if (IsLatchOpenStart[slot_id]) {
            pthread_cancel(latch_open_tid[slot_id]);
            pthread_mutex_lock(&latch_open_mutex[slot_id]);
            IsLatchOpenStart[slot_id] = false;
            pthread_mutex_unlock(&latch_open_mutex[slot_id]);
          }

          //Create thread for hsvc event detect
          if (pthread_create(&hsvc_action_tid[slot_id], NULL, hsvc_event_handler, &hsvc_info[slot_id]) < 0) {
            syslog(LOG_WARNING, "[%s] Create hsvc_event_handler thread failed for slot%x\n",__func__, slot_id);
            pthread_mutex_unlock(&hsvc_mutex[slot_id]);
            return;
          }
          IsHotServiceStart[slot_id] = true;
          pthread_mutex_unlock(&hsvc_mutex[slot_id]);
       }
       else { // SLOT INSERTION
          hsvc_info[slot_id].slot_id = slot_id;
          hsvc_info[slot_id].action = INSERTION;

          pthread_mutex_lock(&hsvc_mutex[slot_id]);
          if ( IsHotServiceStart[slot_id] )
          {
#ifdef DEBUG
            syslog(LOG_WARNING, "[%s] Close the previous thread for slot%x\n", __func__, slot_id);
#endif
            pthread_cancel(hsvc_action_tid[slot_id]);
          }

          //Create thread for hsvc event detect
          if (pthread_create(&hsvc_action_tid[slot_id], NULL, hsvc_event_handler, &hsvc_info[slot_id]) < 0) {
            syslog(LOG_WARNING, "[%s] Create hsvc_event_handler thread failed for slot%x\n",__func__, slot_id);
            pthread_mutex_unlock(&hsvc_mutex[slot_id]);
            return;
          }
          IsHotServiceStart[slot_id] = true;
          pthread_mutex_unlock(&hsvc_mutex[slot_id]);
       }
    }
  } // End of GPIO_SLOT1/2/3/4_PRSNT_B_N, GPIO_SLOT1/2/3/4_PRSNT_N
  else if (gp->gs.gs_gpio == gpio_num("GPIOO3")) {
    if (pal_get_hand_sw(&slot_id)) {
      slot_id = HAND_SW_BMC;
    }

    slot_id = (slot_id >= HAND_SW_BMC) ? HAND_SW_SERVER1 : (slot_id + 1);
    sprintf(locstr, "%u", slot_id);
    kv_set("spb_hand_sw", locstr, 0, 0);
    syslog(LOG_INFO, "change hand_sw location to FRU %s by button", locstr);
  }
  else if (gp->gs.gs_gpio == gpio_num("GPIOI0") || gp->gs.gs_gpio == gpio_num("GPIOI1") ||
           gp->gs.gs_gpio == gpio_num("GPIOI2") || gp->gs.gs_gpio == gpio_num("GPIOI3")
          ) // SLOT1/2/3/4_POWER_EN
  {
    slot_id = (gp->gs.gs_gpio - GPIO_SLOT1_POWER_EN) + 1;
    if (gp->value == 1) { // low to high
      syslog(LOG_WARNING, "[%s] slot%d power enable pin on", __func__,slot_id);
#if defined(CONFIG_FBY2_GPV2)
      if (fby2_get_slot_type(slot_id) == SLOT_TYPE_GPV2) {
        if (pal_is_server_12v_on(slot_id, &slot_12v))
          syslog(LOG_WARNING, "%s : pal_is_server_12v_on failed for slot: %u", __func__, slot_id);
        if (slot_12v)
          fru_cahe_init(slot_id);
      }
#endif
    } else { // high to low
      syslog(LOG_WARNING, "[%s] slot%d power enable pin off", __func__,slot_id);
      fby2_common_set_ierr(slot_id,false);
    }
  }
  else if (gp->gs.gs_gpio == gpio_num("GPION7")) {
    if (gp->value == 0) {
      if (pthread_create(&hsc_alert_tid, NULL, hsc_alert_handler, NULL)) {
        syslog(LOG_WARNING, "[%s] Create hsc_alert_handler thread failed", __func__);
      }
    }
  }
}

static void *
latch_open_handler(void *ptr) {
  uint8_t slot_id = (int)ptr;
  uint8_t pair_slot_id;
  uint8_t slot_12v = 1;
  int ret;
  int value;
  char path_slot[128];
  char path_pair_slot[128];
  int pair_set_type;
  char vpath[80] = {0};

  pthread_detach(pthread_self());
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  sleep(2);

  pair_set_type = pal_get_pair_slot_type(slot_id);
  switch(pair_set_type) {
    case TYPE_CF_A_SV:
    case TYPE_GP_A_SV:
    case TYPE_GPV2_A_SV:
      if (0 == slot_id%2)
        pair_slot_id = slot_id - 1;
      else
        pair_slot_id = slot_id + 1;

      sprintf(path_slot, PWR_UTL_LOCK, slot_id);
      sprintf(path_pair_slot, PWR_UTL_LOCK, pair_slot_id);
      if ((access(path_slot, F_OK) == 0) || (access(path_pair_slot, F_OK) == 0)) {
        pthread_mutex_lock(&latch_open_mutex[slot_id]);
        IsLatchOpenStart[slot_id] = false;
        pthread_mutex_unlock(&latch_open_mutex[slot_id]);
        pthread_exit(0);
      }
      break;
    default:
      sprintf(path_slot, PWR_UTL_LOCK, slot_id);
      if (access(path_slot, F_OK) == 0) {
        pthread_mutex_lock(&latch_open_mutex[slot_id]);
        IsLatchOpenStart[slot_id] = false;
        pthread_mutex_unlock(&latch_open_mutex[slot_id]);
        pthread_exit(0);
      }
      break;
  }

  ret = pal_is_server_12v_on(slot_id, &slot_12v);
  if (ret < 0)
    syslog(LOG_WARNING, "%s : pal_is_server_12v_on failed for slot: %u", __func__, slot_id);
  if (slot_12v) {
    sprintf(vpath, GPIO_VAL, GPIO_FAN_LATCH_DETECT);
    read_device(vpath, &value);

    // 12V action would be triggered when SLED is pulled out
    if (value) {
      // Active 12V off to protect server/device board
      if(pal_set_server_power(slot_id, SERVER_12V_OFF) < 0)
        syslog(LOG_WARNING,"%s : server_12v_off failed for slot: %u", __func__, slot_id);
    }
  }

  pthread_mutex_lock(&latch_open_mutex[slot_id]);
  IsLatchOpenStart[slot_id] = false;
  pthread_mutex_unlock(&latch_open_mutex[slot_id]);

  pthread_exit(0);
}

static void *
hsvc_event_handler(void *ptr) {

  int ret=-1;
  uint8_t value;
  uint8_t status;
  char vpath[80] = {0};
  char hspath[80] = {0};
  char postpath[80] = {0};
  char sys_config_path[80] = {0};
  char cmd[128] = {0};
  char slotrcpath[80] = {0};
  char slot_kv[80] = {0};
  int i = 0, slot_type;
  uint8_t server_type;
  char event_log[80] = {0};
  hot_service_info *hsvc_info = (hot_service_info *)ptr;
  pthread_detach(pthread_self());

  switch(hsvc_info->action)
  {
    case REMOVAl :
      usleep(250000);   //Delay 250ms to wait for slot present pin status stable
      ret = pal_is_fru_prsnt(hsvc_info->slot_id, &value);
      if (ret < 0) {
        syslog(LOG_ERR, "%s pal_is_fru_prsnt failed for fru: %d\n", __func__, hsvc_info->slot_id);
        break;
      }
      if (value) {
        printf("Not remove entirely\n");
      }
      else {   //Card has been removed
        pal_baseboard_clock_control(hsvc_info->slot_id, "1"); // Disable baseboard clock passing buffer to prevent voltage leakage
        ret = pal_is_server_12v_on(hsvc_info->slot_id, &value);    /* Check whether the system is 12V off or on */
        if (ret < 0) {
          syslog(LOG_ERR, "pal_get_server_power: pal_is_server_12v_on failed");
          break;
        }
        if (value) {
          sprintf(event_log, "FRU: %d, %s", hsvc_info->slot_id, fru_prsnt_log_string[2*MAX_NUM_FRUS + hsvc_info->slot_id]);
          syslog(LOG_CRIT, event_log);     //Card removal without 12V-off
          memset(vpath, 0, sizeof(vpath));
          sprintf(vpath, GPIO_VAL, gpio_12v[hsvc_info->slot_id]);
          if (write_device(vpath, "0")) {        /* Turn off 12V to given slot when Server/GP/CF be removed brutally */
            break;
          }
          ret = pal_slot_pair_12V_off(hsvc_info->slot_id);  /* Turn off 12V to pair of slots when Server/GP/CF be removed brutally with pair config */
          if (0 != ret)
            printf("pal_slot_pair_12V_off failed for fru: %d\n", hsvc_info->slot_id);
        }
        else {
          sprintf(event_log, "FRU: %d, %s", hsvc_info->slot_id, fru_prsnt_log_string[hsvc_info->slot_id]);
          syslog(LOG_CRIT, event_log);     //Card removal with 12V-off
        }

        // Re-init kv list
        for(i=0; i < sizeof(slot_kv_list)/sizeof(slot_kv_st); i++) {
          memset(slot_kv, 0, sizeof(slot_kv));
          sprintf(slot_kv, slot_kv_list[i].slot_key, hsvc_info->slot_id);
          if ((ret = pal_set_key_value(slot_kv, slot_kv_list[i].slot_def_val)) < 0) {
            syslog(LOG_WARNING, "%s pal_set_def_key_value: kv_set failed. %d", __func__, ret);
          }
        }

        // Remove post flag file when board has been removed
        sprintf(postpath, POST_FLAG_FILE, hsvc_info->slot_id);
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd,"rm %s",postpath);
        system(cmd);

        // Remove DIMM and CPU related file when board has been removed
        sprintf(sys_config_path, SYS_CONFIG_FILE, hsvc_info->slot_id);
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd,"rm %s",sys_config_path);
        system(cmd);

        // Create file for 12V-on re-init
        sprintf(hspath, HOTSERVICE_FILE, hsvc_info->slot_id);
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd,"touch %s",hspath);
        system(cmd);

        // Assign slot type
        sprintf(slotrcpath, SLOT_RECORD_FILE, hsvc_info->slot_id);
        slot_type = fby2_get_slot_type(hsvc_info->slot_id);
        sprintf(cmd, "echo %d > %s", slot_type, slotrcpath);
        system(cmd);

        if (slot_type == SLOT_TYPE_SERVER) {  // Assign server type
          server_type = 0xFF;
          ret = fby2_get_server_type(hsvc_info->slot_id, &server_type);
          if (ret) {
            syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
          }
          sprintf(slotrcpath, SV_TYPE_RECORD_FILE, hsvc_info->slot_id);
          sprintf(cmd, "echo %d > %s", server_type, slotrcpath);
          system(cmd);
        }
      }
      break;
    case INSERTION :
      sleep(5);   //Delay 5 seconds to wait for slot present pin status stable
      ret = pal_is_fru_prsnt(hsvc_info->slot_id, &value);
      if (ret < 0) {
        syslog(LOG_ERR, "%s pal_is_fru_prsnt failed for fru: %d\n", __func__, hsvc_info->slot_id);
        break;
      }
      if (value) {      //Card has been inserted
        sprintf(event_log, "FRU: %d, %s", hsvc_info->slot_id, fru_prsnt_log_string[MAX_NUM_FRUS + hsvc_info->slot_id]);
        syslog(LOG_CRIT, event_log);

        // Create file for 12V-on re-init
        sprintf(hspath, HOTSERVICE_FILE, hsvc_info->slot_id);
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd,"touch %s",hspath);
        system(cmd);

        // Assign slot type
        sprintf(slotrcpath, SLOT_RECORD_FILE, hsvc_info->slot_id);
        slot_type = fby2_get_slot_type(hsvc_info->slot_id);
        sprintf(cmd, "echo %d > %s", slot_type, slotrcpath);
        system(cmd);

        if (slot_type == SLOT_TYPE_SERVER) {  // Assign server type
          server_type = 0xFF;
          ret = fby2_get_server_type(hsvc_info->slot_id, &server_type);
          if (ret) {
            syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
          }
          sprintf(slotrcpath, SV_TYPE_RECORD_FILE, hsvc_info->slot_id);
          sprintf(cmd, "echo %d > %s", server_type, slotrcpath);
          system(cmd);
        } else if (slot_type == SLOT_TYPE_GPV2) {
          for (i=1; i<=MAX_NUM_DEVS; i++)
            dev_fru_complete[hsvc_info->slot_id][i] = DEV_FRU_NOT_COMPLETE;
        }

        /* Check whether the system is 12V off or on */
        ret = pal_is_server_12v_on(hsvc_info->slot_id, &status);
        if (ret < 0) {
          syslog(LOG_ERR, "pal_get_server_power: pal_is_server_12v_on failed");
          break;
        }
        if (!status) {
          sprintf(cmd, "/usr/local/bin/power-util slot%u 12V-on &",hsvc_info->slot_id);
          system(cmd);
        }
      }
      else {
        printf("Not insert entirely\n");
      }
      break;
  }

  pthread_mutex_lock(&hsvc_mutex[hsvc_info->slot_id]);
  IsHotServiceStart[hsvc_info->slot_id] = false;
  pthread_mutex_unlock(&hsvc_mutex[hsvc_info->slot_id]);

  pthread_exit(0);
}

static gpio_poll_st g_gpios[] = {
  // {{gpio, fd}, edge, gpioValue, call-back function, GPIO description}
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOH5",  "GPIO_FAN_LATCH_DETECT"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOP0",  "GPIO_SLOT1_EJECTOR_LATCH_DETECT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOP1",  "GPIO_SLOT2_EJECTOR_LATCH_DETECT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOP2",  "GPIO_SLOT3_EJECTOR_LATCH_DETECT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOP3",  "GPIO_SLOT4_EJECTOR_LATCH_DETECT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOZ0",  "GPIO_SLOT1_PRSNT_B_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOZ1",  "GPIO_SLOT2_PRSNT_B_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOZ2",  "GPIO_SLOT3_PRSNT_B_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOZ3",  "GPIO_SLOT4_PRSNT_B_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOAA0", "GPIO_SLOT1_PRSNT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOAA1", "GPIO_SLOT2_PRSNT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOAA2", "GPIO_SLOT3_PRSNT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOAA3", "GPIO_SLOT4_PRSNT_N"},
  {{0, 0}, GPIO_EDGE_FALLING, 0, gpio_event_handle, "GPIOO3",  "GPIO_UART_SEL"},
  {{0, 0}, GPIO_EDGE_BOTH, 0, gpio_event_handle, "GPIOI0",  "GPIO_SLOT1_POWER_EN"},
  {{0, 0}, GPIO_EDGE_BOTH, 0, gpio_event_handle, "GPIOI1",  "GPIO_SLOT2_POWER_EN"},
  {{0, 0}, GPIO_EDGE_BOTH, 0, gpio_event_handle, "GPIOI2",  "GPIO_SLOT3_POWER_EN"},
  {{0, 0}, GPIO_EDGE_BOTH, 0, gpio_event_handle, "GPIOI3",  "GPIO_SLOT4_POWER_EN"},
  {{0, 0}, GPIO_EDGE_FALLING, 0, gpio_event_handle, "GPION7",  "GPIO_SMB_HOTSWAP_ALERT_N"},
};

static int g_count = sizeof(g_gpios) / sizeof(gpio_poll_st);

static def_chk_info def_gpio_chk[] = {
  // { default value, gpio name, gpio num, log }
#if DEBUG_ME_EJECTOR_LOG
  { 0, "GPIO_SLOT1_EJECTOR_LATCH_DETECT_N", GPIO_SLOT1_EJECTOR_LATCH_DETECT_N, "FRU: 1, GPIO_SLOT1_EJECTOR_LATCH_DETECT_N is \"1\" and SLOT_12v is ON" },
  { 0, "GPIO_SLOT2_EJECTOR_LATCH_DETECT_N", GPIO_SLOT2_EJECTOR_LATCH_DETECT_N, "FRU: 2, GPIO_SLOT2_EJECTOR_LATCH_DETECT_N is \"1\" and SLOT_12v is ON" },
  { 0, "GPIO_SLOT3_EJECTOR_LATCH_DETECT_N", GPIO_SLOT3_EJECTOR_LATCH_DETECT_N, "FRU: 3, GPIO_SLOT3_EJECTOR_LATCH_DETECT_N is \"1\" and SLOT_12v is ON" },
  { 0, "GPIO_SLOT4_EJECTOR_LATCH_DETECT_N", GPIO_SLOT4_EJECTOR_LATCH_DETECT_N, "FRU: 4, GPIO_SLOT4_EJECTOR_LATCH_DETECT_N is \"1\" and SLOT_12v is ON" },
#endif
  { 0, "GPIO_FAN_LATCH_DETECT",             GPIO_FAN_LATCH_DETECT,             "ASSERT: SLED is not seated"                                    },
};

const static int power_enable[] = { 0, GPIO_SLOT1_POWER_EN,  GPIO_SLOT2_POWER_EN,  GPIO_SLOT3_POWER_EN,  GPIO_SLOT4_POWER_EN };

static void slot_latch_check(void) {
  int slot_id;
  int value;
  char vpath[80] = {0};
  int fd;
  char path[128] = {0};
  int ret;
  int def_slot_latch_val = 0;
  int retry_count = 0;
  int max_retry_count = 40;

  for (slot_id = 1; slot_id <= MAX_NUM_SLOTS; slot_id++) {
    sprintf(vpath, GPIO_VAL, gpio_slot_latch[slot_id]);
    read_device(vpath, &value);
    if (value != def_slot_latch_val) {
      sprintf(path, BIC_CACHED_PID, slot_id);
      while(retry_count < max_retry_count) {
        fd = open(path, O_WRONLY | O_CREAT, 0666);
        if (fd < 0) {
          syslog(LOG_WARNING, "%s: Open file failed for path: %s", __func__, path);
          retry_count++;
          continue;
        }
        ret = flock(fd, LOCK_EX | LOCK_NB);
        if (ret) {
          syslog(LOG_WARNING,"Waiting for bic-cached complete for slot%d",slot_id);
          retry_count++;
          sleep(1);
        } else {
          break;
        }
      }

      sprintf(vpath, GPIO_VAL, GPIO_FAN_LATCH_DETECT);
      read_device(vpath, &value);

      // 12V action would be triggered when SLED is pulled out
      // if SLED is seated, log the event that slot latch is open
      if (!value) {
        syslog(LOG_CRIT,"FRU: %u, SLOT%u_EJECTOR_LATCH is not closed while in VCubby", slot_id, slot_id);
      } else {
        syslog(LOG_CRIT,"FRU: %u, SLOT%u_EJECTOR_LATCH is not closed", slot_id, slot_id);
        if (pal_set_server_power(slot_id, SERVER_12V_OFF) < 0)
          syslog(LOG_WARNING,"%s : server_12v_off failed for slot: %d", __func__, slot_id);
      }

      flock(fd, LOCK_UN);
      close(fd);
      remove(path);
      retry_count = 0;
    }
  }
}

static void default_gpio_check(void) {
  int i;
  int value;
  char vpath[80] = {0};

  for (i=0; i<sizeof(def_gpio_chk)/sizeof(def_chk_info); i++) {
    sprintf(vpath, GPIO_VAL, def_gpio_chk[i].num);
    read_device(vpath, &value);
    if (value != def_gpio_chk[i].def_val) {
      syslog(LOG_CRIT, def_gpio_chk[i].log);
    }
  }
#if defined(CONFIG_FBY2_GPV2)
  for (i=FRU_SLOT1; i<=FRU_SLOT4; i+=2) {
    if (fby2_get_slot_type(i) == SLOT_TYPE_GPV2) {
      sprintf(vpath, GPIO_VAL, power_enable[i]);
      read_device(vpath, &value);
      if (value) {
        fru_cahe_init(i);
      }
    }
  }
#endif
}

int
main(int argc, void **argv) {
  int dev, rc, pid_file;
  uint8_t status = 0;
  int i;

  for(i=1 ;i<MAX_NODES + 1; i++)
  {
    pthread_mutex_init(&hsvc_mutex[i], NULL);
    pthread_mutex_init(&latch_open_mutex[i], NULL);
    last_ejector_ts[i].tv_sec = 0;
  }

  default_gpio_check();

  slot_latch_check();

  pid_file = open("/var/run/gpiointrd.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
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

  for(i=1; i<MAX_NODES + 1; i++)
  {
    pthread_mutex_destroy(&hsvc_mutex[i]);
    pthread_mutex_destroy(&latch_open_mutex[i]);
  }

  return 0;
}
