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
#include <pthread.h>
#include <time.h>
#include <sys/un.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <openbmc/pal.h>
#include <openbmc/fruid.h>
#include <facebook/fby3_common.h>
#include <facebook/fby3_gpio.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/kv.h>

#define MAX_NUM_SLOTS       4
#define DELAY_GPIOD_READ    1000000
#define MAX_NUM_GPV3_DEVS 12


/* To hold the gpio info and status */
typedef struct {
  uint8_t flag;
  uint8_t status;
  uint8_t ass_val;
  char name[32];
} gpio_pin_t;

static gpio_pin_t gpio_slot1[MAX_GPIO_PINS] = {0};
static gpio_pin_t gpio_slot2[MAX_GPIO_PINS] = {0};
static gpio_pin_t gpio_slot3[MAX_GPIO_PINS] = {0};
static gpio_pin_t gpio_slot4[MAX_GPIO_PINS] = {0};

static uint8_t cpld_io_sts[MAX_NUM_SLOTS+1] = {0x10, 0};
static long int pwr_on_sec[MAX_NUM_SLOTS] = {0};
static long int retry_sec[MAX_NUM_SLOTS] = {0};
static bool bios_post_cmplt[MAX_NUM_SLOTS] = {false, false, false, false};
static bool is_pwrgd_cpu_chagned[MAX_NUM_SLOTS] = {false, false, false, false};
static uint8_t SLOTS_MASK = 0x0;
static bool dp_hsm_check = false;
static uint8_t bmc_location = 0;

pthread_mutex_t pwrgd_cpu_mutex[MAX_NUM_SLOTS] = {PTHREAD_MUTEX_INITIALIZER,
                                                  PTHREAD_MUTEX_INITIALIZER,
                                                  PTHREAD_MUTEX_INITIALIZER,
                                                  PTHREAD_MUTEX_INITIALIZER};
#define SET_BIT(list, index, bit) \
           if ( bit == 0 ) {      \
             (((uint8_t*)&list)[index/8]) &= ~(0x1 << (index % 8)); \
           } else {                                                 \
             (((uint8_t*)&list)[index/8]) |= 0x1 << (index % 8);    \
           }                                                        \

#define GET_BIT(list, index) \
           (((((uint8_t*)&list)[index/8]) >> (index % 8)) & 0x1) \

bic_gpio_t gpio_ass_val = {
  .gpio[0] = 0,
  .gpio[1] = 0,
  .gpio[2] = 0,
};

char *host_key[] = {"fru1_host_ready",
                    "fru2_host_ready",
                    "fru3_host_ready",
                    "fru4_host_ready"};

static inline void set_pwrgd_cpu_flag(uint8_t fru, bool val){
  pthread_mutex_lock(&pwrgd_cpu_mutex[fru-1]);
  is_pwrgd_cpu_chagned[fru-1] = val;
  pthread_mutex_unlock(&pwrgd_cpu_mutex[fru-1]);
}

static inline bool get_pwrgd_cpu_flag(uint8_t fru) {
  return is_pwrgd_cpu_chagned[fru-1];
}

static inline long int get_timer_tick(uint8_t fru) {
  return pwr_on_sec[fru-1];
}

static inline void rst_timer(uint8_t fru) {
  pwr_on_sec[fru-1] = 0;
}

static inline void incr_timer(uint8_t fru) {
  pwr_on_sec[fru-1]++;
}

static inline void decr_timer(uint8_t fru) {
  pwr_on_sec[fru-1]--;
}

struct threadinfo {
  uint8_t is_running;
  uint8_t fru;
  pthread_t pt;
};
static struct threadinfo t_fru_cache[MAX_NUM_FRUS] = {0, };
static uint8_t dev_fru_complete[MAX_NODES + MAX_NUM_EXPS + 1][MAX_NUM_GPV3_DEVS + 1] = {DEV_FRU_NOT_COMPLETE};

static int
fruid_cache_init(uint8_t slot_id, uint8_t fru_id, uint8_t less_retry) {
  int ret;
  int fru_size = 0;
  char fruid_temp_path[64] = {0};
  char fruid_path[64] = {0};
  uint8_t offset = 0;
  struct stat st;
  uint8_t intf = 0;

  fru_id += DEV_ID0_2OU - 1;
  offset = DEV_ID0_2OU - 1;

  if (slot_id == FRU_2U_TOP || slot_id == FRU_2U_BOT) {
    intf = slot_id == FRU_2U_TOP ? RREXP_BIC_INTF1 : RREXP_BIC_INTF2;
    pal_get_dev_fruid_path(slot_id, fru_id, fruid_path);
    sprintf(fruid_temp_path, "/tmp/tfruid_slot%d.%d.bin", slot_id, intf);
    ret = bic_read_fruid(FRU_SLOT1, fru_id - offset , fruid_temp_path, &fru_size, intf, less_retry);
  } else {
    sprintf(fruid_path, "/tmp/fruid_slot%d_dev%d.bin", slot_id, fru_id);
    sprintf(fruid_temp_path, "/tmp/tfruid_slot%d.%d.bin", slot_id, REXP_BIC_INTF);
    ret = bic_read_fruid(slot_id, fru_id - offset , fruid_temp_path, &fru_size, REXP_BIC_INTF, less_retry);
  }
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() slot%d dev%d is not present, fru_size: %d\n", __func__, slot_id, fru_id - offset, fru_size);
  }

  if (stat(fruid_temp_path, &st) == 0 && st.st_size == 0 ) {
    remove(fruid_temp_path);
  } else {
    rename(fruid_temp_path, fruid_path);
  }

  return ret;
}

static void *
fru_cache_dump(void *arg) {
  uint8_t fru = *(uint8_t *) arg;
  uint8_t self_test_result[2] = {0};
  // char key[MAX_KEY_LEN];
  char buf[MAX_VALUE_LEN];
  int ret;
  int retry;
  uint8_t status[MAX_NUM_GPV3_DEVS+1] = {DEVICE_POWER_OFF};
  uint8_t type = DEV_TYPE_UNKNOWN;
  uint8_t nvme_ready = 0;
  uint8_t all_nvme_ready = 0;
  uint8_t dev_id;
  uint8_t flagIdx = fru;
  uint8_t fruIdx = fru - 1;
  uint8_t keepPoll = 1;
  uint8_t root = fru;
  const int max_retry = 3;
  int oldstate;
  int finish_count = 0; // fru finish
  int nvme_ready_count = 0;
  fruid_info_t fruid;
  struct timespec slp_time;

  pal_get_root_fru(fru, &root);

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  // pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
  // pal_set_nvme_ready(fru,all_nvme_ready);
  // pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);

  // Check 2OU BIC Self Test Result
  do {
    if ( pal_is_fw_update_ongoing(root) == true ) {
      slp_time.tv_sec = 5;
      slp_time.tv_nsec = 0;
      nanosleep(&slp_time, NULL);
      continue;
    }

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    if (fru == FRU_2U_TOP) {
      ret = bic_get_self_test_result(FRU_SLOT1, (uint8_t *)&self_test_result, RREXP_BIC_INTF1);
      flagIdx = MAX_NODES + FRU_2U_TOP - FRU_EXP_BASE;
    } else if (fru == FRU_2U_BOT) {
      ret = bic_get_self_test_result(FRU_SLOT1, (uint8_t *)&self_test_result, RREXP_BIC_INTF2);
      flagIdx = MAX_NODES + FRU_2U_BOT - FRU_EXP_BASE;
    } else {
      ret = bic_get_self_test_result(fru, (uint8_t *)&self_test_result, REXP_BIC_INTF);
    }
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    if (ret == 0) {
      syslog(LOG_INFO, "bic_get_self_test_result of slot%u: %X %X", fru, self_test_result[0], self_test_result[1]);
      break;
    }
    sleep(5);
  } while (ret != 0);

  sleep(2); //wait for BIC poll at least one cycle

  if (keepPoll) {
    for (dev_id = 1; dev_id <= MAX_NUM_GPV3_DEVS; dev_id++) {
      if (dev_fru_complete[flagIdx][dev_id] != DEV_FRU_NOT_COMPLETE) {
        finish_count++;
      }
    }
  }

  while (finish_count < MAX_NUM_GPV3_DEVS) {
    // Get GPV3 devices' FRU
    for (dev_id = 1; dev_id <= MAX_NUM_GPV3_DEVS; dev_id++) {
      if ( pal_is_fw_update_ongoing(root) == true ) {
        slp_time.tv_sec = 5;
        slp_time.tv_nsec = 0;
        nanosleep(&slp_time, NULL);
        continue;
      }

      //check for power status
      ret = pal_get_dev_info(fru, dev_id, &nvme_ready ,&status[dev_id], &type);

      //syslog(LOG_WARNING, "fru_cache_dump1: Slot%u Dev%d power=%u nvme_ready=%u type=%u", fru, dev_id-1, status[dev_id], nvme_ready, type);

      if (dev_fru_complete[flagIdx][dev_id] != DEV_FRU_NOT_COMPLETE) {
        if (keepPoll) {
          sleep(1);
        } else {
          finish_count++;
        }
        continue;
      }

      if (ret == 0) {
        if (status[dev_id] == DEVICE_POWER_ON) {
          retry = 0;
          while (1) {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
            ret = fruid_cache_init(fru, dev_id, keepPoll);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
            retry++;

            if ((ret == 0) || (retry == max_retry) || keepPoll)
              break;

            msleep(50);
          }

          if (ret != 0) {
            syslog(LOG_WARNING, "fru_cache_dump: Fail on getting Slot%u Dev%d FRU", fru, dev_id-1);
          } else {
            pal_get_dev_fruid_path(fru, dev_id + DEV_ID0_2OU - 1, buf);
            //check file's checksum
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
            ret = fruid_parse(buf, &fruid);
            if (ret != 0) { // Fail
              syslog(LOG_WARNING, "fru_cache_dump: Slot%u Dev%d FRU data checksum is invalid", fru, dev_id-1);
            } else { // Success
              free_fruid_info(&fruid);
              dev_fru_complete[flagIdx][dev_id] = DEV_FRU_COMPLETE;
              finish_count++;
              syslog(LOG_WARNING, "fru_cache_dump: Finish getting Slot%u Dev%d FRU", fru, dev_id-1);
            }
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
          }
        } else { // DEVICE_POWER_OFF
          if (!keepPoll) {
            finish_count++;
          }
        }

        // sprintf(key, "slot%u_dev%u_pres", fru, dev_id-1);
        // sprintf(buf, "%u", status[dev_id]);
        // pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
        // if (kv_set(key, buf, 0, 0) < 0) {
        //   syslog(LOG_WARNING, "fru_cache_dump: kv_set Slot%u Dev%d present status failed", fru, dev_id-1);
        // }
        // pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
      } else { // Device Status Unknown
        if (!keepPoll) {
          finish_count++;
        }
        syslog(LOG_WARNING, "fru_cache_dump: Fail to access Slot%u Dev%d power status", fru, dev_id-1);
      }

      if (keepPoll) {
        slp_time.tv_sec = 3;
        slp_time.tv_nsec = 0;
        nanosleep(&slp_time, NULL);
      }
    }
  }

  // If NVMe is ready, try to get the FRU which was failed to get and
  // update the fan speed control table according to the device type
  while (((finish_count < MAX_NUM_GPV3_DEVS) || (nvme_ready_count < MAX_NUM_GPV3_DEVS)) && !keepPoll) {
    nvme_ready_count = 0;
    if ( pal_is_fw_update_ongoing(root) == true ) {
      sleep(5);
      continue;
    }

    for (dev_id = 1; dev_id <= MAX_NUM_GPV3_DEVS; dev_id++) {
      if (status[dev_id] == DEVICE_POWER_OFF) {// M.2 device is present or not
        nvme_ready_count++;
        continue;
      }

      // check for device type
      ret = pal_get_dev_info(fru, dev_id, &nvme_ready, &status[dev_id], &type);
      //syslog(LOG_WARNING, "fru_cache_dump2: Slot%u Dev%d power=%u nvme_ready=%u type=%u", fru, dev_id-1, status[dev_id], nvme_ready, type);

      if (ret || (!nvme_ready))
        continue;

      nvme_ready_count++;

      if (dev_fru_complete[flagIdx][dev_id] == DEV_FRU_NOT_COMPLETE) { // try to get fru or not
        if (type == DEV_TYPE_BRCM_ACC) { // device type has FRU
          retry = 0;
          while (1) {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
            ret = fruid_cache_init(fru, dev_id, keepPoll);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
            retry++;

            if ((ret == 0) || (retry == max_retry))
              break;

            msleep(50);
          }

          if (retry >= max_retry) {
            syslog(LOG_WARNING, "fru_cache_dump: Fail on getting Slot%u Dev%d FRU", fru, dev_id-1);
          } else {
            pal_get_dev_fruid_path(fru, dev_id + DEV_ID0_2OU - 1, buf);
            // check file's checksum
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
            ret = fruid_parse(buf, &fruid);
            if (ret != 0) { // Fail
              syslog(LOG_WARNING, "fru_cache_dump: Slot%u Dev%d FRU data checksum is invalid", fru, dev_id-1);
            } else { // Success
              free_fruid_info(&fruid);
              dev_fru_complete[flagIdx][dev_id] = DEV_FRU_COMPLETE;
              finish_count++;
              syslog(LOG_WARNING, "fru_cache_dump: Finish getting Slot%u Dev%d FRU", fru, dev_id-1);
            }
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
          }
        } else {
          dev_fru_complete[flagIdx][dev_id] = DEV_FRU_IGNORE;
          finish_count++;
          syslog(LOG_WARNING, "fru_cache_dump: Slot%u Dev%d ignore FRU", fru, dev_id-1);
        }
      }
    }

    if (!all_nvme_ready && (nvme_ready_count == MAX_NUM_GPV3_DEVS)) {
      // set nvme is ready
      all_nvme_ready = 1;
      // pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
      // pal_set_nvme_ready(fru,all_nvme_ready);
      // pal_set_nvme_ready_timestamp(fru);
      // pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
      syslog(LOG_WARNING, "fru_cache_dump: Slot%u all devices' NVMe are ready", fru);
    }
    sleep(10);

  }

  t_fru_cache[fruIdx].is_running = 0;
  syslog(LOG_INFO, "%s: FRU %d cache is finished.", __func__, fru);

  pthread_detach(pthread_self());
  pthread_exit(0);
}

int
fru_cahe_init(uint8_t fru) {
  int ret, i;
  uint8_t idx;
  uint8_t type_2ou = UNKNOWN_BOARD;
  uint8_t topbot = 0;

  if (fru != FRU_SLOT1 && fru != FRU_SLOT3) {
    return -1;
  }

  ret = bic_is_m2_exp_prsnt(fru);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to get 1OU & 2OU present status, return val: %d", __func__, ret);
    return -1;
  } 
  if ( (ret & PRESENT_2OU) != PRESENT_2OU ) {
    return -1;
  }
  if ( fby3_common_get_2ou_board_type(fru, &type_2ou) < 0 ) {
    syslog(LOG_WARNING, "%s() slot%u Failed to get 2OU board type", __func__,fru);
    return -1;
  }
  if ( type_2ou != GPV3_MCHP_BOARD && type_2ou != GPV3_BRCM_BOARD &&
      type_2ou != CWC_MCHP_BOARD) {
    syslog(LOG_WARNING, "%s() slot%u 2OU board type = %u (not GPv3)", __func__,fru,type_2ou);
    return -1;
  }

  if (type_2ou == CWC_MCHP_BOARD) {
    topbot = bic_is_2u_top_bot_prsnt(fru);
    fru = FRU_2U_TOP;
    if ((topbot & PRESENT_2U_TOP) == 0) {
      goto fru_cache_ends;
    }
  }
fru_cache_starts:
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

fru_cache_ends:
  if (type_2ou == CWC_MCHP_BOARD && fru == FRU_2U_TOP) {
    fru = FRU_2U_BOT;

    if ((topbot & PRESENT_2U_BOT) > 0) {
      goto fru_cache_starts;
    }
  }

  return 0;
}

/* Returns the pointer to the struct holding all gpio info for the fru#. */
static gpio_pin_t *
get_struct_gpio_pin(uint8_t fru) {

  gpio_pin_t *gpios;

  switch (fru) {
    case FRU_SLOT1:
      gpios = gpio_slot1;
      break;
    case FRU_SLOT2:
      gpios = gpio_slot2;
      break;
    case FRU_SLOT3:
      gpios = gpio_slot3;
      break;
    case FRU_SLOT4:
      gpios = gpio_slot4;
      break;
    default:
      syslog(LOG_WARNING, "get_struct_gpio_pin: Wrong SLOT ID %d\n", fru);
      return NULL;
  }

  return gpios;
}


static void
populate_gpio_pins(uint8_t fru) {

  int i, ret;
  gpio_pin_t *gpios;

  gpios = get_struct_gpio_pin(fru);
  if (gpios == NULL) {
    syslog(LOG_WARNING, "populate_gpio_pins: get_struct_gpio_pin failed.");
    return;
  }

  // monitor RST_PLTRST_BMC_N & FM_BMC_DEBUG_ENABLE_N

  gpios[RST_PLTRST_BMC_N].flag = 1; // Platform reset pin
  gpios[FM_BMC_DEBUG_ENABLE_N].flag = 1; // Debug enable pin
  for (i = 0; i < MAX_GPIO_PINS; i++) {
    if (gpios[i].flag) {
      gpios[i].ass_val = GET_BIT(gpio_ass_val, i);
      ret = fby3_get_gpio_name(fru, i, gpios[i].name, NONE_INTF);
      if (ret < 0)
        continue;
    }
  }
}

/* Wrapper function to configure and get all gpio info */
static void
init_gpio_pins() {
  int fru;

  for (fru = FRU_SLOT1; fru < (FRU_SLOT1 + MAX_NUM_SLOTS); fru++) {
    populate_gpio_pins(fru);
  }

}

/* Monitor the gpio pins */
static void *
gpio_monitor_poll(void *ptr) {

  int i, ret = 0;
  uint8_t fru = (int)ptr;
  uint8_t pwr_sts = 0;
  bic_gpio_t revised_pins, n_pin_val, o_pin_val;
  gpio_pin_t *gpios;
  uint8_t chassis_sts[8] = {0};
  uint8_t chassis_sts_len;
  uint8_t power_policy = POWER_CFG_UKNOWN;
  char pwr_state[256] = {0};
  char sys_conf[16] = {0};

  /* Check for initial Asserts */
  gpios = get_struct_gpio_pin(fru);
  if (gpios == NULL) {
    syslog(LOG_WARNING, "gpio_monitor_poll: get_struct_gpio_pin failed for fru %u", fru);
    pthread_exit(NULL);
  }
 
  ret = bic_get_gpio(fru, &o_pin_val, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "gpio_monitor_poll: bic_get_gpio failed for fru %u", fru);
  }


  while (1) {
    //check the fw update is ongoing
    if ( pal_is_fw_update_ongoing(fru) == true ) {
      usleep(DELAY_GPIOD_READ);
      continue;
    }

    //when bic is unreachable, wait it anyway.
    if ( bic_get_gpio(fru, &n_pin_val, NONE_INTF) < 0 ) {
      //rst timer
      rst_timer(fru);
      kv_set(host_key[fru-1], "0", 0, 0);

      ret = pal_get_server_12v_power(fru, &pwr_sts);
      if (ret == PAL_EOK) {
        if (pwr_sts == SERVER_12V_OFF) {
          SET_BIT(o_pin_val, PWRGD_CPU_LVC3_R, 0);
          SET_BIT(o_pin_val, RST_PLTRST_BMC_N, 0);
        }
      }

      //rst new pin val
      memset(&n_pin_val, 0, sizeof(n_pin_val));

      //Normally, BIC can always be accessed except 12V-off or 12V-cycle

      usleep(DELAY_GPIOD_READ);
      continue;
    }

    // Inform BIOS that BMC is ready
    // handle case : BIC FW update & BIC resets unexpectedly
    if (GET_BIT(n_pin_val, BMC_READY) == 0) {
      bic_set_gpio(fru, BMC_READY, 1);
    }

    // Get CPLD io sts
    cpld_io_sts[fru] = (GET_BIT(n_pin_val, PWRGD_SYS_PWROK)  << 0x0) | \
                       (GET_BIT(n_pin_val, RST_PLTRST_BMC_N) << 0x1) | \
                       (GET_BIT(n_pin_val, RST_RSMRST_BMC_N) << 0x2) | \
                       (GET_BIT(n_pin_val, FM_CPU_MSMI_CATERR_LVT3_N ) << 0x3) | \
                       (GET_BIT(n_pin_val, FM_SLPS3_R_N) << 0x4);
    if (GET_BIT(n_pin_val, FM_BIOS_POST_CMPLT_BMC_N) == 0x0) {
      if (retry_sec[fru-1] == (MAX_READ_RETRY*12)) {
        kv_set(host_key[fru-1], "1", 0, 0);
        bios_post_cmplt[fru-1] = true;
        retry_sec[fru-1] = 0;
        // Check if it is DP-HSM when first boot
        if (!dp_hsm_check) {
          pal_dp_hba_fan_table_check();
          dp_hsm_check = true;
        }

        if ( kv_get("sled_system_conf", sys_conf, NULL, KV_FPERSIST) == 0 ) {
          if ( strcmp(sys_conf, "Type_DPB") == 0 ) {
            fby3_common_fscd_ctrl(FAN_AUTO_MODE);
          }
        }
      }
      retry_sec[fru-1]++;
    } else {
      if (bios_post_cmplt[fru-1] == true) {
        kv_set(host_key[fru-1], "0", 0, 0);
        bios_post_cmplt[fru-1] = false;
        retry_sec[fru-1] = 0;
        rst_timer(fru);
      }
    }

    //check PWRGD_CPU_LVC3_R is changed
    if ( (get_pwrgd_cpu_flag(fru) == false) && 
         (GET_BIT(n_pin_val, PWRGD_CPU_LVC3_R) != GET_BIT(o_pin_val, PWRGD_CPU_LVC3_R))) {
      rst_timer(fru);
      set_pwrgd_cpu_flag(fru, true);  
      //update the value since the bit is not monitored
      SET_BIT(o_pin_val, PWRGD_CPU_LVC3_R, GET_BIT(n_pin_val, PWRGD_CPU_LVC3_R));
    }

    //check the power status since the we need to set timer
    if ( GET_BIT(n_pin_val, PWRGD_CPU_LVC3_R) != gpios[PWRGD_CPU_LVC3_R].ass_val ) {
      incr_timer(fru);
    } else {
      decr_timer(fru);
    }

    if ( memcmp(&o_pin_val, &n_pin_val, sizeof(o_pin_val)) == 0 ) {
      usleep(DELAY_GPIOD_READ);
      continue;
    }

    revised_pins.gpio[0] = n_pin_val.gpio[0] ^ o_pin_val.gpio[0];
    revised_pins.gpio[1] = n_pin_val.gpio[1] ^ o_pin_val.gpio[1];
    revised_pins.gpio[2] = n_pin_val.gpio[2] ^ o_pin_val.gpio[2];

    for (i = 0; i < MAX_GPIO_PINS; i++) {
      if (GET_BIT(revised_pins, i) && (gpios[i].flag == 1)) {
        gpios[i].status = GET_BIT(n_pin_val, i);
    
        // Check if the new GPIO val is ASSERT
        if (gpios[i].status == gpios[i].ass_val) {
          if (i == RST_PLTRST_BMC_N) {
            rst_timer(fru);
          } else if (i == FM_BMC_DEBUG_ENABLE_N) {
            syslog(LOG_CRIT, "FRU: %d, FM_BMC_DEBUG_ENABLE_N is ASSERT: %d", fru, gpios[i].status);
          }
        } else {
          if (i == RST_PLTRST_BMC_N) {
            rst_timer(fru);
          } else if (i == FM_BMC_DEBUG_ENABLE_N) {
            syslog(LOG_CRIT, "FRU: %d, FM_BMC_DEBUG_ENABLE_N is DEASSERT: %d", fru, gpios[i].status);
          }
        }
      }
    }

    memcpy(&o_pin_val, &n_pin_val, sizeof(o_pin_val));
    
    usleep(DELAY_GPIOD_READ);
  } /* while loop */
} /* function definition*/

static void *
cpld_io_mon() {
  uint8_t uart_pos = 0x00;
  uint8_t card_prsnt = 0x00;
  uint8_t prev_uart_pos = 0xff;
  uint8_t prev_cpld_io_sts[MAX_NUM_SLOTS+1] = {0x00};
  pthread_detach(pthread_self());

  while (1) {
    // we start updating IO sts when card is present
    if ( pal_is_debug_card_prsnt(&card_prsnt) < 0 || card_prsnt == 0 ) {
      usleep(DELAY_GPIOD_READ); //sleep
      continue;
    }

    // Get the uart position
    if ( pal_get_uart_select_from_kv(&uart_pos) < 0 ) {
      usleep(DELAY_GPIOD_READ); //sleep
      continue;
    }

    // If uart position is at slot1/2/3/4, cpld_io_sts[1/2/3/4] can't be 0.
    // If it is still 0, it means BMC hasn't gotten GPIO value from BIC.
    if ( cpld_io_sts[uart_pos] == 0 ) {
      usleep(DELAY_GPIOD_READ); //sleep
      continue;
    }

    //If uart_post or cpld_io_sts is updated, write the new value to cpld
    //Because cpld_io_sts[BMC_uart_position] is always 0x10
    //So, we need to two conditions to help
    if ( prev_uart_pos != uart_pos || cpld_io_sts[uart_pos] != prev_cpld_io_sts[uart_pos] ) {
      if ( pal_set_uart_IO_sts(uart_pos, cpld_io_sts[uart_pos]) < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to update uart IO sts\n", __func__);
      } else {
        prev_uart_pos = uart_pos;
        prev_cpld_io_sts[uart_pos] = cpld_io_sts[uart_pos];
      }
    }

    usleep(DELAY_GPIOD_READ);
  }
}

static void *
crit_act_mon() {
  int i2cfd = 0;
  int ret = 0;
  bool crit_act_is_asserted = false;
  bool is_pwr_lock_set = false;
  uint8_t cnt = 0;

  pthread_detach(pthread_self());

  if ( bmc_location != NIC_BMC ) {
    syslog(LOG_INFO, "%s() disable the crit_act_mon\n", __func__);
    pthread_exit(0);
  }

  i2cfd = i2c_cdev_slave_open(FRU_SLOT1 + SLOT_BUS_BASE, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, CPLD_ADDRESS);
    pthread_exit(0);
  }

  while (1) {
    // get the flag
    crit_act_is_asserted = pal_get_crit_act_status(i2cfd);
    if ( crit_act_is_asserted == true ) {
      // count it
      cnt++;

      // force lock
      if ( is_pwr_lock_set == false ) {
        kv_set("pwr_lock", "1", 0, 0);
        is_pwr_lock_set = true;
        syslog(LOG_WARNING, "pwr_lock is set");
      }
    }

    // sleep periodically
    sleep(1);

    // if is_pwr_lock_set is true and crit_act_is_asserted is false, we need to proceed
    if ( is_pwr_lock_set != true || crit_act_is_asserted != false ) continue;

    // run the corresponding action according to cnt
    if ( cnt >= 2 && cnt <= 4 ) { // fan mode ctrl - manuel
      kv_set("fan_manual", "1", 0, 0);
      syslog(LOG_WARNING, "fan_manual is set");

      if ( pal_set_fan_ctrl("sleep") < 0 ) syslog(LOG_WARNING, "fscd is still running based on fan_manual(1)\n");

    } else if ( cnt >= 5 && cnt <= 7 ) { // fan mode ctrl - auto
      if ( pal_set_fan_ctrl("wakeup") < 0 ) syslog(LOG_WARNING, "fscd is not running, try again\n");

      kv_set("fan_manual", "0", 0, 0);
      syslog(LOG_WARNING, "fan_manual is cancelled");
    }

    // recover pwr_lock
    kv_set("pwr_lock", "0", 0, 0);
    syslog(LOG_WARNING, "pwr_lock is cancelled");
    is_pwr_lock_set = false;
    cnt = 0;
  }

  close(i2cfd);
  pthread_exit(0);
}

static void *
host_pwr_mon() {
#define MAX_NIC_PWR_RETRY   15
#define POWER_ON_DELAY       2
#define NON_PFR_POWER_OFF_DELAY  -2
#define HOST_READY 500
  char path[64] = {0};
  uint8_t host_off_flag = 0;
  uint8_t is_util_run_flag = 0;
  uint8_t fru = 0;
  bool nic_pwr_set_off = false;
  int i = 0;
  int retry = 0;
  long int tick = 0;
  int power_off_delay = NON_PFR_POWER_OFF_DELAY;

  pthread_detach(pthread_self());

  while (1) {
    for ( i = 0; i < MAX_NUM_SLOTS; i++ ) {
      if ( ((SLOTS_MASK >> i) & 0x1) != 0x1) continue; // skip since fru${i} is not present

      fru = i + 1;
      tick = get_timer_tick(fru);
      sprintf(path, PWR_UTL_LOCK, fru);
      if (access(path, F_OK) == 0) {
        is_util_run_flag |= 0x1 << i;
      } else {
        is_util_run_flag &= ~(0x1 << i);
      }

      //record which slot is off
      if ( tick <= power_off_delay ) {
        if ( (get_pwrgd_cpu_flag(fru) == true) && (tick == power_off_delay) ) {
          sprintf(path, PWR_UTL_LOCK, fru);
          if (access(path, F_OK) != 0) {
            pal_set_last_pwr_state(fru, "off");
          }
          syslog(LOG_CRIT, "FRU: %d, System powered OFF", fru);
        }
        host_off_flag |= 0x1 << i;
      } else if ( tick >= POWER_ON_DELAY ) {
        if ( (get_pwrgd_cpu_flag(fru) == true) && (tick == POWER_ON_DELAY) ) {
          sprintf(path, PWR_UTL_LOCK, fru);
          if (access(path, F_OK) != 0) {
            pal_set_last_pwr_state(fru, "on");
          }
          syslog(LOG_CRIT, "FRU: %d, System powered ON", fru);
          // update fru if GPv3 dev fru flag (dev_fru_complete) does not set to complete
          // If dc on after ac/dc cycle, only get the fru which did not get last time
          // In the future, it will also update GPv3 device NVMe is ready or not for some implementations
          fru_cahe_init(fru); 
        }
        host_off_flag &= ~(0x1 << i);
        if ( tick == HOST_READY ) {
          kv_set(host_key[fru-1], "1", 0, 0);
        }
      } else {
        if ( tick < POWER_ON_DELAY ) {
          kv_set(host_key[fru-1], "0", 0, 0);
        }
      }

      if ( (tick == power_off_delay) || (tick == POWER_ON_DELAY) ) {
        set_pwrgd_cpu_flag(fru, false); //recover the flag
      }
    }

    if ( bmc_location == NIC_BMC ) {
      usleep(DELAY_GPIOD_READ);
      continue; //CPLD controls NIC directly on NIC_BMC
    }

    if ( host_off_flag == SLOTS_MASK ) {
      //Need to make sure the hosts are off instead of power cycle
      //delay to change the power mode of NIC
      if ( is_util_run_flag > 0 || access(SET_NIC_PWR_MODE_LOCK, F_OK) == 0) {
        retry = 0;
        usleep(DELAY_GPIOD_READ);
        continue;
      }

      if ( retry == MAX_NIC_PWR_RETRY ) retry = MAX_NIC_PWR_RETRY;
      else retry++;
    } else {
      //it means one of the slots is powered on
      retry = 0;
    }

    //reach the limit, change the power mode of NIC.
    if ( retry == MAX_NIC_PWR_RETRY && nic_pwr_set_off == false) {
      //set off here because users may power off the host from OS instead of BMC
      if ( pal_server_set_nic_power(SERVER_POWER_OFF) == 0 ) {
        nic_pwr_set_off = true;
      }
    } else if ( retry < MAX_NIC_PWR_RETRY && nic_pwr_set_off == true) {
      nic_pwr_set_off = false;
      //Don't change NIC Power to normal here.
      //Because the power seq. of NIC will be affected. We should change it before turning on any one of the slots.
    }

    usleep(DELAY_GPIOD_READ);
  }
}

static void
print_usage() {
  printf("Usage: gpiod [ slot1, slot2, slot3, slot4 ]\n");
}

/* Spawns a pthread for each fru to monitor all the sensors on it */
static void
run_gpiod(int argc, void **argv) {
  int i, ret, slot;
  uint8_t fru_flag, fru;
  pthread_t tid_host_pwr_mon;
  pthread_t tid_gpio[MAX_NUM_SLOTS];
  pthread_t tid_cpld_io_mon;
  pthread_t tid_crit_act_mon;

  // get the bmc location
 if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    syslog(LOG_WARNING, "Failed to get the location of BMC, default value = NIC_BMC\n");
    bmc_location = NIC_BMC;
  }

  /* Check for which fru do we need to monitor the gpio pins */
  fru_flag = 0;
  for (i = 1; i < argc; i++) {
    ret = pal_get_fru_id(argv[i], &fru);
    if (ret < 0) {
      print_usage();
      exit(-1);
    }

    SLOTS_MASK |= 0x1 << (fru - 1);
    //init GPv3 device FRU after BMC start or hot service
    fru_cahe_init(fru);

    if ((fru >= FRU_SLOT1) && (fru < (FRU_SLOT1 + MAX_NUM_SLOTS))) {
      fru_flag = SETBIT(fru_flag, fru);
      slot = fru;

      if (pthread_create(&tid_gpio[fru-1], NULL, gpio_monitor_poll, (void *)slot) < 0) {
        syslog(LOG_WARNING, "pthread_create for gpio_monitor_poll failed\n");
      }
    }
  }

  /* Monitor the host powers */
  if (pthread_create(&tid_host_pwr_mon, NULL, host_pwr_mon, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for host_pwr_mon fail\n");
  }

  if (pthread_create(&tid_cpld_io_mon, NULL, cpld_io_mon, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for cpld_io_mon fail\n");
  }

  if (pthread_create(&tid_crit_act_mon, NULL, crit_act_mon, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for crit_act_mon fail\n");
  }

  for (fru = FRU_SLOT1; fru < (FRU_SLOT1 + MAX_NUM_SLOTS); fru++) {
    if (GETBIT(fru_flag, fru)) {
      pthread_join(tid_gpio[fru-1], NULL);
    }
  }
}

int
main(int argc, void **argv) {
  int dev, rc, pid_file;

  if (argc < 2) {
    print_usage();
    exit(-1);
  }

  pid_file = open("/var/run/gpiod.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another gpiod instance is running...\n");
      exit(-1);
    }
  } else {

    init_gpio_pins();

    openlog("gpiod", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "gpiod: daemon started");
    run_gpiod(argc, argv);
  }

  return 0;
}
