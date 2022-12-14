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
#include <sys/file.h>
#include <sys/stat.h>
#include <openbmc/ipmi.h>
#include <openbmc/sdr.h>
#include <openbmc/pal.h>
#include <openbmc/pal_sensors.h>
#include <openbmc/aggregate-sensor.h>
#include <openbmc/kv.h>

#define MIN_POLL_INTERVAL 2
#define STOP_PERIOD 10
#define MAX_SENSOR_CHECK_RETRY 3
#define MAX_ASSERT_CHECK_RETRY 1
#define MAX_SENSORD_FRU MAX_NUM_FRUS

static thresh_sensor_t g_snr[MAX_SENSORD_FRU][MAX_SENSOR_NUM + 1] = {0};
static thresh_sensor_t g_aggregate_snr[MAX_SENSOR_NUM + 1] = {0};
static bool hotswap_support = false;

static void
print_usage() {
    printf("Usage: sensord <options>\n");
    printf("Options: [ %s ]\n", pal_fru_list);
}

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

static void sensor_fail_assert_check(uint8_t *fail_cnt UNUSED, uint8_t fru UNUSED, int snr_num UNUSED, const char *name UNUSED)
{
#ifdef SENSOR_FAIL_DETECT
  if (*fail_cnt < MAX_SENSOR_CHECK_RETRY) {
    (*fail_cnt)++;
    if (*fail_cnt == MAX_SENSOR_CHECK_RETRY)
      syslog(LOG_ERR, "FRU: %d, num: 0x%X, snr:%-16s, read failed",
          fru, snr_num, name);
  }
#endif
}

static void sensor_fail_assert_clear(uint8_t *fail_cnt UNUSED, uint8_t fru UNUSED, int snr_num UNUSED, const char *name UNUSED)
{
#ifdef SENSOR_FAIL_DETECT
  if (*fail_cnt == MAX_SENSOR_CHECK_RETRY) {
    syslog(LOG_ERR, "FRU: %d, num: 0x%X, snr:%-16s, read recovered",
        fru, snr_num, name);
  }
  *fail_cnt = 0;
#endif
}

/*
 * Returns the pointer to the struct holding all sensor info and
 * calculated threshold values for the fru#
 */
static thresh_sensor_t *
get_struct_thresh_sensor(uint8_t fru) {

  thresh_sensor_t *snr;

  if (fru == AGGREGATE_SENSOR_FRU_ID) {
    return g_aggregate_snr;
  }

  if (fru < 1 || fru > MAX_SENSORD_FRU) {
    syslog(LOG_WARNING, "get_struct_thresh_sensor: Wrong FRU ID %d\n", fru);
    return NULL;
  }
  snr = g_snr[fru-1];
  return snr;
}

/* Initialize all thresh_sensor_t structs for all the Yosemite sensors */
static int
init_fru_snr_thresh(uint8_t fru) {

  int i;
  int ret;
  uint8_t snr_num;
  int sensor_cnt;
  uint8_t *sensor_list;
  thresh_sensor_t *snr;
  uint8_t fruNb = fru;

  snr = get_struct_thresh_sensor(fru);
  if (snr == NULL) {
#ifdef DEBUG
    syslog(LOG_WARNING, "init_fru_snr_thresh: get_struct_thresh_sensor failed");
#endif /* DEBUG */
    return -1;
  }

  ret = pal_get_fru_sensor_list(fruNb, &sensor_list, &sensor_cnt);
  if (ret < 0) {
    return ret;
  }

  for (i = 0; i < sensor_cnt; i++) {
    snr_num = sensor_list[i];

    ret = sdr_get_snr_thresh(fruNb, snr_num, &snr[snr_num]);
    if (ret < 0) {
#ifdef DEBUG
      syslog(LOG_WARNING, "init_fru_snr_thresh: sdr_get_snr_thresh for FRU: %d", fru);
#endif /* DEBUG */
      continue;
    }
    pal_alter_sensor_poll_interval(fruNb, snr_num, &(snr[snr_num].poll_interval));

    pal_init_sensor_check(fruNb, snr_num, (void *)&snr[snr_num]);
  }

  if (access(THRESHOLD_PATH, F_OK) == -1) {
        mkdir(THRESHOLD_PATH, 0777);
  }
  ret = pal_copy_all_thresh_to_file(fruNb, snr);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: Fail to copy thresh to file for FRU: %d", __func__, fru);
    return ret;
  }
  return 0;
}

static int
sensor_raw_read_helper(uint8_t fru, uint8_t snr_num, float *val)
{
  int ret = 0, cache_write_ret = 0;

  if (fru == AGGREGATE_SENSOR_FRU_ID) {
    ret = aggregate_sensor_read(snr_num, val);
    if (ret == 0) {
      cache_write_ret = sensor_cache_write(fru, snr_num, true, *val);
    } else {
      cache_write_ret = sensor_cache_write(fru, snr_num, false, 0.0);
    }

  } else if (pal_sensor_is_source_host(fru, snr_num)) {
    if (pal_is_host_snr_available(fru, snr_num) == true) {
      ret = sensor_cache_read(fru, snr_num, val);
      return ret;
    } else {
      cache_write_ret = sensor_cache_write(fru, snr_num, false, 0.0);
    }
  } else {
    ret = sensor_raw_read(fru, snr_num, val);
    return ret;
  }

  ret = cache_write_ret;

  return ret;
}

static float
get_snr_thresh_val(uint8_t fru, uint8_t snr_num, uint8_t thresh) {

  float val;
  thresh_sensor_t *snr;

  snr = get_struct_thresh_sensor(fru);

  switch (thresh) {
    case UCR_THRESH:
      val = snr[snr_num].ucr_thresh;
      break;
    case UNC_THRESH:
      val = snr[snr_num].unc_thresh;
      break;
    case UNR_THRESH:
      val = snr[snr_num].unr_thresh;
      break;
    case LCR_THRESH:
      val = snr[snr_num].lcr_thresh;
      break;
    case LNC_THRESH:
      val = snr[snr_num].lnc_thresh;
      break;
    case LNR_THRESH:
      val = snr[snr_num].lnr_thresh;
      break;
    default:
      syslog(LOG_WARNING, "get_snr_thresh_val: wrong thresh enum value");
      exit(-1);
  }

  return val;
}

/*
 * Check the curr sensor values against the threshold and
 * if the curr val has deasserted, log it.
 */
static int
check_thresh_deassert(uint8_t fru, uint8_t snr_num, uint8_t thresh,
  float *curr_val) {
  uint8_t curr_state = 0;
  float thresh_val;
  char thresh_name[100];
  thresh_sensor_t *snr;
  uint8_t retry = 0;
  int ret;
  uint8_t fruNb = fru;

  snr = get_struct_thresh_sensor(fru);

  if (!GETBIT(snr[snr_num].flag, thresh) ||
      !GETBIT(snr[snr_num].curr_state, thresh))
    return 0;

  thresh_val = get_snr_thresh_val(fru, snr_num, thresh);

  while (retry <= MAX_SENSOR_CHECK_RETRY) {
    switch (thresh) {

      case UNR_THRESH:
      case UCR_THRESH:
      case UNC_THRESH:
        if (FORMAT_CONV(*curr_val) >= FORMAT_CONV((thresh_val - snr[snr_num].neg_hyst)))
          return 0;
        break;

      case LNR_THRESH:
      case LCR_THRESH:
      case LNC_THRESH:
        if (FORMAT_CONV(*curr_val) <= FORMAT_CONV((thresh_val + snr[snr_num].pos_hyst)))
          return 0;
    }

    if (retry < MAX_SENSOR_CHECK_RETRY) {
      msleep(50);
      ret = sensor_raw_read_helper(fruNb, snr_num, curr_val);
      if (ret < 0)
        return -1;
    }
    retry++;
  }

  switch (thresh) {
    case UNC_THRESH:
        curr_state = ~(SETBIT(curr_state, UNR_THRESH) |
            SETBIT(curr_state, UCR_THRESH) |
            SETBIT(curr_state, UNC_THRESH));
        sprintf(thresh_name, "Upper Non Critical");
      break;

    case UCR_THRESH:
        curr_state = ~(SETBIT(curr_state, UCR_THRESH) |
            SETBIT(curr_state, UNR_THRESH));
        sprintf(thresh_name, "Upper Critical");
      break;

    case UNR_THRESH:
        curr_state = ~(SETBIT(curr_state, UNR_THRESH));
        sprintf(thresh_name, "Upper Non Recoverable");
      break;

    case LNC_THRESH:
        curr_state = ~(SETBIT(curr_state, LNR_THRESH) |
            SETBIT(curr_state, LCR_THRESH) |
            SETBIT(curr_state, LNC_THRESH));
        sprintf(thresh_name, "Lower Non Critical");
      break;

    case LCR_THRESH:
        curr_state = ~(SETBIT(curr_state, LCR_THRESH) |
            SETBIT(curr_state, LNR_THRESH));
        sprintf(thresh_name, "Lower Critical");
      break;

    case LNR_THRESH:
        curr_state = ~(SETBIT(curr_state, LNR_THRESH));
        sprintf(thresh_name, "Lower Non Recoverable");
      break;

    default:
      syslog(LOG_WARNING, "get_snr_thresh_val: wrong thresh enum value");
      exit(-1);
  }

  if (curr_state) {
    snr[snr_num].curr_state &= curr_state;
    pal_update_ts_sled();
    syslog(LOG_CRIT, "DEASSERT: %s threshold - settled - FRU: %d, num: 0x%X "
        "curr_val: %.2f %s, thresh_val: %.2f %s, snr: %-16s",thresh_name,
        fruNb, snr_num, *curr_val, snr[snr_num].units, thresh_val,
        snr[snr_num].units, snr[snr_num].name);
    pal_sensor_deassert_handle(fru, snr_num, *curr_val, thresh);
  }

  return 0;
}


/*
 * Check the curr sensor values against the threshold and
 * if the curr val has asserted, log it.
 */
static int
check_thresh_assert(uint8_t fru, uint8_t snr_num, uint8_t thresh,
  float *curr_val) {
  uint8_t curr_state = 0;
  float thresh_val;
  char thresh_name[100];
  thresh_sensor_t *snr;
  uint8_t retry = 0;
  int ret;
  uint8_t fruNb = fru;

  snr = get_struct_thresh_sensor(fru);

  if (pal_ignore_thresh(fru,snr_num,thresh))
    return 0;

  if (!GETBIT(snr[snr_num].flag, thresh) ||
      GETBIT(snr[snr_num].curr_state, thresh))
    return 0;

  thresh_val = get_snr_thresh_val(fru, snr_num, thresh);

  while (retry <= MAX_ASSERT_CHECK_RETRY) {
    switch (thresh) {
      case UNR_THRESH:
      case UCR_THRESH:
      case UNC_THRESH:
        if (FORMAT_CONV(*curr_val) < FORMAT_CONV(thresh_val)) {
          return 0;
        }
        break;
      case LNR_THRESH:
      case LCR_THRESH:
      case LNC_THRESH:
        if (FORMAT_CONV(*curr_val) > FORMAT_CONV(thresh_val)) {
          return 0;
        }
        break;
    }

    if (retry < MAX_ASSERT_CHECK_RETRY) {
      msleep(50);
      ret = sensor_raw_read_helper(fruNb, snr_num, curr_val);
      if (ret < 0)
        return -1;
    }
    retry++;
  }

  switch (thresh) {
    case UNR_THRESH:
        curr_state = (SETBIT(curr_state, UNR_THRESH) |
            SETBIT(curr_state, UCR_THRESH) |
            SETBIT(curr_state, UNC_THRESH));
        sprintf(thresh_name, "Upper Non Recoverable");
      break;

    case UCR_THRESH:
        curr_state = (SETBIT(curr_state, UCR_THRESH) |
            SETBIT(curr_state, UNC_THRESH));
        sprintf(thresh_name, "Upper Critical");
      break;

    case UNC_THRESH:
        curr_state = (SETBIT(curr_state, UNC_THRESH));
        sprintf(thresh_name, "Upper Non Critical");
      break;

    case LNR_THRESH:
        curr_state = (SETBIT(curr_state, LNR_THRESH) |
            SETBIT(curr_state, LCR_THRESH) |
            SETBIT(curr_state, LNC_THRESH));
        sprintf(thresh_name, "Lower Non Recoverable");
      break;

    case LCR_THRESH:
        curr_state = (SETBIT(curr_state, LCR_THRESH) |
            SETBIT(curr_state, LNC_THRESH));
        sprintf(thresh_name, "Lower Critical");
      break;

    case LNC_THRESH:
        curr_state = (SETBIT(curr_state, LNC_THRESH));
        sprintf(thresh_name, "Lower Non Critical");
      break;

    default:
      syslog(LOG_WARNING, "get_snr_thresh_val: wrong thresh enum value");
      exit(-1);
  }

  if (curr_state) {
    curr_state &= snr[snr_num].flag;
    snr[snr_num].curr_state |= curr_state;
    pal_update_ts_sled();
    syslog(LOG_CRIT, "ASSERT: %s threshold - raised - FRU: %d, num: 0x%X"
        " curr_val: %.2f %s, thresh_val: %.2f %s, snr: %-16s", thresh_name,
        fruNb, snr_num, *curr_val, snr[snr_num].units, thresh_val,
        snr[snr_num].units, snr[snr_num].name);
    pal_sensor_assert_handle(fru, snr_num, *curr_val, thresh);
  }

  return 0;
}

static int
reinit_snr_threshold(uint8_t fru, int mode) {
  int ret = 0;
  thresh_sensor_t *snr;
  uint8_t fruNb = fru;

  snr = get_struct_thresh_sensor(fru);
  if (snr == NULL) {
#ifdef DEBUG
    syslog(LOG_WARNING, "%s: get_struct_thresh_sensor failed",__func__);
#endif /* DEBUG */
  }
  ret = pal_get_all_thresh_from_file(fruNb, snr, mode);
  if (0 != ret) {
    syslog(LOG_WARNING, "%s: Fail to get threshold from file for slot%d", __func__, fru);
    return -1;
  }

  return 0;
}

static int
thresh_reinit_chk(uint8_t fru) {
  int ret;
  char fpath[128] = {0};
  char initpath[128] = {0};
  char fru_name[32];
  uint8_t fruNb = fru;

  ret = pal_get_fru_name(fruNb, fru_name);
  if (ret < 0) {
    printf("%s: Fail to get fru%d name\n", __func__, fru);
    return -1;
  }

  snprintf(fpath, sizeof(fpath), THRESHOLD_BIN, fru_name);
  snprintf(initpath, sizeof(initpath), THRESHOLD_RE_FLAG, fru_name);
  if (0 != access(fpath, F_OK)) {
    // If there is no THRESHOLD_BIN file but INIT_FLAG exist, it means threshold-util --clear is triggered.
    // And snr info should be loaded default.
    if (0 == access(initpath, F_OK)) {
      ret = reinit_snr_threshold(fru, SENSORD_MODE_NORMAL);
    }
  } else {
    // If THRESHOLD_BIN file exist and INIT_FLAG also exist, it means threshold-util --set is triggered.
    // And snr info should be updated.
    if (0 == access(initpath, F_OK))
      ret = reinit_snr_threshold(fru, SENSORD_MODE_TESTING);
  }

  return ret;
}

/*
 * Starts monitoring all the sensors on a fru for all the threshold/discrete values.
 * Each pthread runs this monitoring for a different fru.
 */
static void *
snr_monitor(void *arg) {

  uint8_t fru = (uint8_t)(uintptr_t)arg;
  int i, ret, snr_num, sensor_cnt, discrete_cnt;
  float curr_val;
  uint8_t *sensor_list, *discrete_list;
  thresh_sensor_t *snr;
  uint32_t snr_poll_interval[MAX_SENSOR_NUM + 1] = {0};
  uint8_t snr_read_fail[MAX_SENSOR_NUM + 1] = {0};
  uint8_t fruNb = fru;
  uint8_t slot = fru;

  ret = pal_get_fru_sensor_list(fruNb, &sensor_list, &sensor_cnt);
  if (ret < 0) {
    pthread_detach(pthread_self());
    pthread_exit(NULL);
  }

  ret = pal_get_fru_discrete_list(fruNb, &discrete_list, &discrete_cnt);
  if (ret < 0) {
    pthread_detach(pthread_self());
    pthread_exit(NULL);
  }

  /*
    Register sensor failure tolerance policy
  */
  ret = pal_register_sensor_failure_tolerance_policy(fruNb);
  if (ret < 0) {
    pthread_detach(pthread_self());
    pthread_exit(NULL);
  }

  /*
    ignore check sensor_cnt when enable hotswap_support
  */
  if ((sensor_cnt == 0) && (discrete_cnt == 0) && !hotswap_support) {
    pthread_detach(pthread_self());
    pthread_exit(NULL);
  }

  snr = get_struct_thresh_sensor(fru);
  if (snr == NULL) {
    syslog(LOG_WARNING, "snr_monitor: get_struct_thresh_sensor failed");
    exit(-1);
  }

  for (i = 0; i < discrete_cnt; i++) {
    snr_num = discrete_list[i];
    pal_get_sensor_name(fruNb, snr_num, snr[snr_num].name);
  }

  // set flag to notice BMC sensord snr_monitor  is ready
  kv_set("flag_sensord_monitor", "1", 0, 0);

  while(1) {
    uint8_t fru_presence;

    // Delay and retry later if the FRU is not detected.
    // XXX Shall we exit the loop/thread? Or perhaps we shouldn't even
    // create the thread if FRU is not detected when "sensord" starts?
    // In other word, how do we swap FRUs in the field? Can we expect
    // sensord, or even OpenBMC to be restarted after swapping FRUs?
    ret = pal_is_fru_prsnt(fru, &fru_presence);
    if (ret != 0 || fru_presence == 0) {
      sleep(STOP_PERIOD);
      continue;
    }

    if (pal_is_fw_update_ongoing(slot)) {
      sleep(STOP_PERIOD);
      continue;
    }

    if (pal_get_sdr_update_flag(fru)) {
      /*
        If hotswap_support enabled
        re-getting sensor list when sdr_update change
      */
      if (hotswap_support) {
        ret = pal_get_fru_sensor_list(fruNb, &sensor_list, &sensor_cnt);
        if (ret < 0) {
          syslog(LOG_DEBUG, "%s : slot%u pal_get_fru_sensor_list fail", __func__, fru);
        }
        ret = pal_get_fru_discrete_list(fruNb, &discrete_list, &discrete_cnt);
        if (ret < 0) {
          syslog(LOG_DEBUG, "%s : slot%u pal_get_fru_discrete_list fail", __func__, fru);
        }
      }

      if (init_fru_snr_thresh(fru) < 0 || pal_update_sensor_reading_sdr(fru) < 0) {
        syslog(LOG_DEBUG, "%s : slot%u SDR update fail", __func__, fru);
        sleep(STOP_PERIOD);
        continue;
      } else {
        syslog(LOG_DEBUG, "%s : slot%u SDR update successfully", __func__, fru);
        pal_set_sdr_update_flag(fru,0);
      }
    }

    ret = thresh_reinit_chk(fru);
    if (ret < 0)
      syslog(LOG_ERR, "%s: Fail to reinit sensor threshold for fru%d",__func__,fru);

    for (i = 0; i < sensor_cnt; i++) {
      snr_num = sensor_list[i];
      curr_val = 0;
      if (snr[snr_num].flag) {
        // granular the sensor via assigning the poll_interval
        if (snr_poll_interval[snr_num] > MIN_POLL_INTERVAL) {
          snr_poll_interval[snr_num] -= MIN_POLL_INTERVAL;
          continue;
        }
        snr_poll_interval[snr_num] = snr[snr_num].poll_interval;
        if (!(ret = sensor_raw_read_helper(fruNb, snr_num, &curr_val))) {
          sensor_fail_assert_clear(&snr_read_fail[snr_num], fru, snr_num, snr[snr_num].name);
          check_thresh_assert(fru, snr_num, UNC_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, UCR_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, UNR_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, LNC_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, LCR_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, LNR_THRESH, &curr_val);

          check_thresh_deassert(fru, snr_num, UNR_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, UCR_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, UNC_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, LNR_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, LCR_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, LNC_THRESH, &curr_val);
        } else if (ret < 0) {
          sensor_fail_assert_check(&snr_read_fail[snr_num], fru, snr_num, snr[snr_num].name);
        } /* pal_sensor_read return check */
      } /* flag check */
    } /* loop for all sensors */

    for (i = 0; i < discrete_cnt; i++) {
      snr_num = discrete_list[i];
      ret = sensor_raw_read_helper(fruNb, snr_num, &curr_val);
      if (!ret && (snr[snr_num].curr_state != (int) curr_val)) {
        pal_sensor_discrete_check(fru, snr_num, snr[snr_num].name,
            snr[snr_num].curr_state, (int) curr_val);
        snr[snr_num].curr_state = (int) curr_val;
      }
    }

#ifdef DYN_THRESH_FRU1
    // Handle dynamic threshold changes for FRU1
    if (fru == 1) {
      init_fru_snr_thresh(1);
    }
#endif

    sleep(MIN_POLL_INTERVAL);
  } /* while loop*/
} /* function definition */


static void *
snr_health_monitor() {

  int fru;
  thresh_sensor_t *snr;
  uint8_t value = 0;
  int num;
  int ret = 0;
  uint8_t fru_health_last_state[MAX_NUM_FRUS+1] = {0};
  uint8_t fru_health_kv_state[MAX_NUM_FRUS+1] = {0};

  // Initial fru health, default value is good.
  for (num = 0; num<=MAX_NUM_FRUS; num++){
      fru_health_last_state[num] = 1;
      fru_health_kv_state[num] = 1;
  }

  // set flag to notice BMC sensord snr_health_monitor is ready
  kv_set("flag_sensord_health", "1", 0, 0);

  while (1) {
    for (fru = 1; fru <= MAX_NUM_FRUS; fru++) {

      value = 0;

      snr = get_struct_thresh_sensor(fru);
      if (snr == NULL) {
         syslog(LOG_WARNING, "snr_health_monitor: get_struct_thresh_sensor failed, fru %d", fru);
         exit(-1);
      }

      // get current health status from kv_store
      ret = pal_get_fru_health(fru, &fru_health_kv_state[fru]);
      if (ret) {
        // If the FRU is not ready, do not log error about errors in its health reporting
        if (ret != ERR_SENSOR_NA)
          syslog(LOG_ERR, " %s - kv get health status failed, fru %d",__func__, fru);
        continue;
      }

      for (num = 0; num <= MAX_SENSOR_NUM; num++) {
        value |= snr[num].curr_state;
      }

      value = (value > 0) ? FRU_STATUS_BAD: FRU_STATUS_GOOD;

      // If log-util clear the fru, cleaning sensor status (After doing it, sensord will regenerate assert)
      if ( (fru_health_kv_state[fru] != fru_health_last_state[fru]) && (fru_health_kv_state[fru] == 1)) {
        for (num = 0; num <= MAX_SENSOR_NUM; num++) {
           snr[num].curr_state = 0;
        }
      }

      // keep last status
      fru_health_last_state[fru] = value;

      // set value to kv_store
      pal_set_sensor_health(fru, value);

    } /* for loop for frus */
    sleep(MIN_POLL_INTERVAL);
  } /* while loop */
}

static void *
aggregate_snr_monitor(void *unused UNUSED)
{
  size_t cnt = 0, i;
  int ret;
  float curr_val;
  uint8_t fru = AGGREGATE_SENSOR_FRU_ID;
  uint8_t snr_num;
  thresh_sensor_t *snr;
  uint8_t snr_read_fail[MAX_SENSOR_NUM + 1] = {0};

  if(aggregate_sensor_init(NULL)) {
    syslog(LOG_WARNING, "Initializing aggregate sensors failed!");
  }

  aggregate_sensor_count(&cnt);
  if (cnt == 0) {
    pthread_exit(NULL);
    return NULL;
  }
  for(i = 0; i < cnt; i++) {
    aggregate_sensor_threshold(i, &g_aggregate_snr[i]);
  }
  snr = get_struct_thresh_sensor(fru);
  if (snr == NULL) {
    syslog(LOG_WARNING, "agg_snr_monitor: get_struct_thresh_sensor failed");
    pthread_exit(NULL);
  }

  while(1) {
    for (i = 0; i < cnt; i++) {
      snr_num = (uint8_t)i;
      curr_val = 0;
      if (snr[snr_num].flag) {
        if (!(ret = sensor_raw_read_helper(fru, snr_num, &curr_val))) {
          sensor_fail_assert_clear(&snr_read_fail[snr_num], fru, snr_num, snr[snr_num].name);
          check_thresh_assert(fru, snr_num, UNC_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, UCR_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, UNR_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, LNC_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, LCR_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, LNR_THRESH, &curr_val);

          check_thresh_deassert(fru, snr_num, UNR_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, UCR_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, UNC_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, LNR_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, LCR_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, LNC_THRESH, &curr_val);
        } else if (ret < 0) {
          sensor_fail_assert_check(&snr_read_fail[snr_num], fru, snr_num, snr[snr_num].name);
        } /* pal_sensor_read return check */
      } /* flag check */
    } /* loop for all sensors */
    sleep(MIN_POLL_INTERVAL);
  }
  pthread_exit(NULL);
  return NULL;
}

/* Spawns a pthread for each fru to monitor all the sensors on it */
static int
run_sensord(int argc, char **argv) {

  int ret, arg;
  uint8_t fru;
  int fru_flag = 0;
  pthread_t thread_snr[MAX_SENSORD_FRU];
  pthread_t sensor_health;
  pthread_t agg_sensor_mon;

  arg = 1;
  while(arg < argc) {

    ret = pal_get_fru_id(argv[arg], &fru);
    if (ret < 0) {
      return ret;
    }

    fru_flag = SETBIT(fru_flag, fru);
    arg++;
  }

  ret = pal_sensor_monitor_initial();
  for (fru = 1; fru <= MAX_SENSORD_FRU; fru++) {

    if (GETBIT(fru_flag, fru)) {

      if (init_fru_snr_thresh(fru) < 0)
        continue;

      /* Threshold Sensors */
      ret = pthread_create(&thread_snr[fru-1], NULL, snr_monitor,
                           (void*)(uintptr_t)fru);
      if (ret != 0) {
        syslog(LOG_WARNING,
               "failed to create sensor monitor thread for FRU %d: %s\n",
               fru, strerror(ret));
      } else {
        syslog(LOG_INFO, "created sensor monitor thread for FRU %d\n", fru);
      }
      sleep(1);
    }
  }

  /* Sensor Health */
  ret = pthread_create(&sensor_health, NULL, snr_health_monitor, NULL);
  if (ret != 0) {
    syslog(LOG_WARNING, "failed to create sensor health thread: %s\n",
           strerror(ret));
  } else {
    syslog(LOG_INFO, "created sensor health thread\n");
  }

  /* Aggregate sensor monitor */
  ret = pthread_create(&agg_sensor_mon, NULL, aggregate_snr_monitor, NULL);
  if (ret != 0) {
    syslog(LOG_WARNING, "failed to create aggregate sensor thread: %s\n",
           strerror(ret));
  } else {
    syslog(LOG_INFO, "created aggregate sensor thread\n");
  }

  pthread_join(agg_sensor_mon, NULL);

  pthread_join(sensor_health, NULL);

  for (fru = 1; fru <= MAX_SENSORD_FRU; fru++) {

    if (GETBIT(fru_flag, fru))
      pthread_join(thread_snr[fru-1], NULL);
  }
  return 0;
}


int
main(int argc, char **argv) {
  int rc, pid_file;

  if (argc < 2) {
    print_usage();
    exit(1);
  }

  if (getenv("SENSORD_HOTSWAP_SUPPORT") != NULL) {
    hotswap_support = true;
    syslog(LOG_INFO, "sensord: SENSORD_HOTSWAP_SUPPORT is set");
  }

  pid_file = open("/var/run/sensord.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another sensord instance is running...\n");
      exit(-1);
    }
  } else {

    syslog(LOG_INFO, "sensord: daemon started");

    rc = run_sensord(argc, argv);
    if (rc < 0) {
      print_usage();
      return -1;
    }
  }
  return 0;
}
