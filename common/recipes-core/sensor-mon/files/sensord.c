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
#include <openbmc/ipmi.h>
#include <openbmc/sdr.h>
#include <openbmc/pal.h>

#define DELAY 2
#define MAX_SENSOR_CHECK_RETRY 3

static thresh_sensor_t g_snr[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

static void
print_usage() {
    printf("Usage: sensord <options>\n");
    printf("Options: [ %s ]\n", pal_fru_list);
}

/*
 * Returns the pointer to the struct holding all sensor info and
 * calculated threshold values for the fru#
 */
static thresh_sensor_t *
get_struct_thresh_sensor(uint8_t fru) {

  thresh_sensor_t *snr;

  if (fru < 1 || fru > MAX_NUM_FRUS) {
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

  snr = get_struct_thresh_sensor(fru);
  if (snr == NULL) {
#ifdef DEBUG
    syslog(LOG_WARNING, "init_fru_snr_thresh: get_struct_thresh_sensor failed");
#endif /* DEBUG */
    return -1;
  }

  ret = pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
  if (ret < 0) {
    return ret;
  }

  for (i < 0; i < sensor_cnt; i++) {
    snr_num = sensor_list[i];

    ret = sdr_get_snr_thresh(fru, snr_num,
        GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH), &snr[snr_num]);
    if (ret < 0) {
#ifdef DEBUG
      syslog(LOG_WARNING, "init_fru_snr_thresh: sdr_get_snr_thresh for FRU: %d", fru);
#endif /* DEBUG */
      continue;
    }
    pal_sensor_threshold_flag(fru, snr_num, &snr[snr_num].flag);
  }

  return 0;
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

  snr = get_struct_thresh_sensor(fru);

  if (!GETBIT(snr[snr_num].flag, thresh) ||
      !GETBIT(snr[snr_num].curr_state, thresh))
    return 0;

  thresh_val = get_snr_thresh_val(fru, snr_num, thresh);

  while (retry < MAX_SENSOR_CHECK_RETRY) {
    switch (thresh) {

      case UNR_THRESH:
      case UCR_THRESH:
      case UNC_THRESH:
        if (*curr_val < (thresh_val - snr[snr_num].pos_hyst))
          retry++;
         else
          return 0;
        break;

      case LNR_THRESH:
      case LCR_THRESH:
      case LNC_THRESH:
        if (*curr_val > (thresh_val + snr[snr_num].neg_hyst))
          retry++;
        else
          return 0;
        break;
    }

    if (retry < MAX_SENSOR_CHECK_RETRY) {
      msleep(50);
      ret = pal_sensor_read(fru, snr_num, curr_val);
      if (ret < 0)
        return -1;
    }
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
    syslog(LOG_CRIT, "DEASSERT: %s threshold - settled - FRU: %d, num: 0x%X "
        "curr_val: %.2f %s, thresh_val: %.2f %s, snr: %-16s",thresh_name,
        fru, snr_num, *curr_val, snr[snr_num].units, thresh_val,
        snr[snr_num].units, snr[snr_num].name);
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

  snr = get_struct_thresh_sensor(fru);

  if (!GETBIT(snr[snr_num].flag, thresh) ||
      GETBIT(snr[snr_num].curr_state, thresh))
    return 0;

  thresh_val = get_snr_thresh_val(fru, snr_num, thresh);

  while (retry < MAX_SENSOR_CHECK_RETRY) {
    switch (thresh) {
      case UNR_THRESH:
      case UCR_THRESH:
      case UNC_THRESH:
        if (*curr_val >= thresh_val) {
          retry++;
        } else {
          return 0;
        }
        break;
      case LNR_THRESH:
      case LCR_THRESH:
      case LNC_THRESH:
        if (*curr_val <= thresh_val) {
          retry++;
        } else {
          return 0;
        }
        break;
    }

    msleep(50);
    if (retry < MAX_SENSOR_CHECK_RETRY) {
      ret = pal_sensor_read(fru, snr_num, curr_val);
      if (ret < 0)
        return -1;
    }
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
    syslog(LOG_CRIT, "ASSERT: %s threshold - raised - FRU: %d, num: 0x%X"
        " curr_val: %.2f %s, thresh_val: %.2f %s, snr: %-16s", thresh_name,
        fru, snr_num, *curr_val, snr[snr_num].units, thresh_val,
        snr[snr_num].units, snr[snr_num].name);
  }

  return 0;
}


/*
 * Starts monitoring all the sensors on a fru for all discrete sensors.
 * Each pthread runs this monitoring for a different fru.
 */
static void *
snr_discrete_monitor(void *arg) {

#ifdef DEBUG2
  char tmplog[1000];
#endif /* DEBUG2 */
  uint8_t fru = *(uint8_t *) arg;
  int i, ret;
  uint8_t discrete_cnt;
  uint8_t snr_num;
  thresh_sensor_t *snr;
  float normal_val, curr_val;

  /* HACK */
  // TODO: Need to resolve the SEGFAULT in the call below and replace HACK here.
#ifdef CONFIG_YOSEMITE
  uint8_t discrete_list[3];
  discrete_cnt = 3;
  if (fru != FRU_SPB && fru != FRU_NIC) {
    discrete_list[0] = BIC_SENSOR_SYSTEM_STATUS;
    discrete_list[1] = BIC_SENSOR_VR_HOT;
    discrete_list[2] = BIC_SENSOR_CPU_DIMM_HOT;
  } else {
    return;
  }
#else
  uint8_t *discrete_list;
  ret = pal_get_fru_discrete_list(fru, &discrete_list, &discrete_cnt);
  if (ret < 0) {
    return;
  }
#endif /* CONFIG_YOSEMITE */

  snr = get_struct_thresh_sensor(fru);
  if (snr == NULL) {
    syslog(LOG_WARNING, "snr_thresh_monitor: get_struct_thresh_sensor failed");
    exit(-1);
  }

  for (i = 0; i < discrete_cnt; i++) {
    snr_num = discrete_list[i];
    pal_get_sensor_name(fru, snr_num, snr[snr_num].name);
  }

#if defined(TESTING) || defined(DEBUG)
  int cnt = 0;
#endif
  while(1) {
    for (i = 0; i < discrete_cnt; i++) {
      snr_num = discrete_list[i];
      ret = pal_sensor_read(fru, snr_num, &curr_val);

#ifdef DEBUG2
      sprintf(tmplog, "echo 0x%X %s %d >> /tmp/discretetest%d", snr_num, snr[snr_num].name, (int) curr_val, fru);
      system(tmplog);
#endif /* DEBUG2 */

#if defined(TESTING) || defined(DEBUG)
      if (cnt == 2 && snr_num == BIC_SENSOR_SYSTEM_STATUS) {
        curr_val = ((int) curr_val) | 0x1;
      } else if (cnt == 6 && snr_num == BIC_SENSOR_SYSTEM_STATUS) {
        curr_val = ((int) curr_val) & 0x0;
      }
#endif
      if ((snr[snr_num].curr_state != (int) curr_val) && !ret) {
        pal_sensor_discrete_check(fru, snr_num, snr[snr_num].name,
            snr[snr_num].curr_state, (int) curr_val);
        snr[snr_num].curr_state = (int) curr_val;
      }
    }
#if defined(TESTING) || defined(DEBUG)
    cnt ++;
#endif
    sleep(DELAY);
  }
}

/*
 * Starts monitoring all the sensors on a fru for all the threshold values.
 * Each pthread runs this monitoring for a different fru.
 */
static void *
snr_thresh_monitor(void *arg) {

  uint8_t fru = *(uint8_t *) arg;
  int f, ret, snr_num;
  float normal_val, curr_val;
  thresh_sensor_t *snr;

#if defined(TESTING) || defined(DEBUG)
  float temp_thresh;
  int cnt = 0;
#endif /* TESTING */

  snr = get_struct_thresh_sensor(fru);
  if (snr == NULL) {
    syslog(LOG_WARNING, "snr_thresh_monitor: get_struct_thresh_sensor failed");
    exit(-1);
  }

#ifdef DEBUG2
  char tmplog[1000];
#endif /* DEBUG2 */

  while(1) {

#ifdef TESTING
    cnt++;
#endif /* TESTING */

    for (snr_num = 0; snr_num < MAX_SENSOR_NUM; snr_num++) {
      curr_val = 0;
      if (snr[snr_num].flag) {
        if (!(ret = pal_sensor_read(fru, snr_num, &curr_val))) {
#ifdef DEBUG2
          sprintf(tmplog, "echo 0x%X %s %.2f >> /tmp/analog%d", snr_num, snr[snr_num].name, curr_val, fru);
      system(tmplog);
#endif /* DEBUG2 */

#ifdef TESTING
          /*
           * The curr_val crosses UCR and then return to a state
           * where UNC < curr_val < UCR and then eventually back to normal.
           */
          if (cnt == 5 && snr_num == BIC_SENSOR_MB_INLET_TEMP) {
            snr[snr_num].flag |= SETBIT(snr[snr_num].flag, UNC_THRESH);
            temp_thresh = snr[snr_num].ucr_thresh;
            snr[snr_num].ucr_thresh = 20.0;
            snr[snr_num].unc_thresh = 10.0;
          } else if (cnt == 8 && snr_num == BIC_SENSOR_MB_INLET_TEMP) {
            snr[snr_num].ucr_thresh = temp_thresh;
          } else if (cnt == 10 && snr_num == BIC_SENSOR_MB_INLET_TEMP) {
            snr[snr_num].unc_thresh = 50.0;
          } else if (cnt == 11 && snr_num == BIC_SENSOR_MB_INLET_TEMP) {
            snr[snr_num].unc_thresh = 0.0;
            snr[snr_num].flag &= CLEARBIT(snr[snr_num].flag, UNC_THRESH);

          }
#endif /* TESTING */

#ifdef DEBUG
          if (cnt == 2) {
            syslog(LOG_INFO, "pthread %d, cnt: %d, num: 0x%X name: %-16s"
                " units:%s", fru, cnt, snr_num, snr[snr_num].name,
                snr[snr_num].units);
          }
#endif /* DEBUG */

          check_thresh_assert(fru, snr_num, UNR_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, UCR_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, UNC_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, LNR_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, LCR_THRESH, &curr_val);
          check_thresh_assert(fru, snr_num, LNC_THRESH, &curr_val);

          check_thresh_deassert(fru, snr_num, UNC_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, UCR_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, UNR_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, LNC_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, LCR_THRESH, &curr_val);
          check_thresh_deassert(fru, snr_num, LNR_THRESH, &curr_val);
        } else {
#ifdef DEBUG
          syslog(LOG_ERR, "FRU: %d, num: 0x%X, snr:%-16s, read failed",
              fru, snr_num, snr[snr_num].name);
#endif
        } /* pal_sensor_read return check */
      } /* flag check */
    } /* loop for all sensors */
    sleep(DELAY);
  } /* while loop*/
} /* function definition */

#ifdef DEBUG
void print_snr_thread(uint8_t fru, thresh_sensor_t *snr)
{
  int i,j;
  float curr_val;
  char tmplog[1000];
  char print[1000];

  for (i = 0; i <= MAX_SENSOR_NUM; i++) {
    if (snr[i].flag) {
      curr_val = 0;
      if(!(pal_sensor_read(fru, i, &curr_val))) {
        sprintf(tmplog, "%-30s: %s  %.2f  %.2f  %.2f  %.2f\n",
        snr[i].name, snr[i].units,
        snr[i].ucr_thresh,
        snr[i].lcr_thresh,
        snr[i].pos_hyst,
        snr[i].neg_hyst);
        sprintf(print, "echo %s >> /tmp/print%d", tmplog, fru);
        system(print);
      }
    }
  }
}
#endif /* DEBUG */


static void *
snr_health_monitor() {

  int fru;
  thresh_sensor_t *snr;
  uint8_t value = 0;
  int num;

  while (1) {
    for (fru = 1; fru <= MAX_NUM_FRUS; fru++) {

      value = 0;

      snr = get_struct_thresh_sensor(fru);
      if (snr == NULL) {
        syslog(LOG_WARNING, "snr_health_monitor: get_struct_thresh_sensor failed");
         exit(-1);
      }

      for (num = 0; num <= MAX_SENSOR_NUM; num++) {
        value |= snr[num].curr_state;
      }

      value = (value > 0) ? FRU_STATUS_BAD: FRU_STATUS_GOOD;

      pal_set_sensor_health(fru, value);

#ifdef DEBUG
      pal_get_sensor_health(fru, &value);
      syslog(LOG_WARNING, "fru %d : health %d", fru, value);
#endif

    } /* for loop for frus*/
    sleep(DELAY);
  } /* while loop */
}

/* Spawns a pthread for each fru to monitor all the sensors on it */
static int
run_sensord(int argc, char **argv) {

  int ret, arg;
  uint8_t fru;
  uint8_t fru_flag = 0;
  pthread_t thread_snr[MAX_NUM_FRUS];
  pthread_t discrete_snr[MAX_NUM_FRUS];
  pthread_t sensor_health;

  arg = 1;
  while(arg < argc) {

    ret = pal_get_fru_id(argv[arg], &fru);
    if (ret < 0)
      return ret;

    fru_flag = SETBIT(fru_flag, fru);
    arg++;
  }

  for (fru = 1; fru <= MAX_NUM_FRUS; fru++) {

    if (GETBIT(fru_flag, fru)) {

      if (init_fru_snr_thresh(fru))
        return ret;

      /* Threshold Sensors */
      if (pthread_create(&thread_snr[fru-1], NULL, snr_thresh_monitor,
          (void*) &fru) < 0) {
        syslog(LOG_WARNING, "pthread_create for FRU %d failed\n", fru);
      }
#ifdef DEBUG
      else {
        syslog(LOG_WARNING, "pthread_create for FRU %d succeed\n", fru);
      }
#endif /* DEBUG */
      sleep(1);

      /* Discrete Sensors */
      if (pthread_create(&discrete_snr[fru-1], NULL, snr_discrete_monitor,
            (void*) &fru) < 0) {
        syslog(LOG_WARNING, "pthread_create for FRU %d failed\n", fru);
      }
#ifdef DEBUG
      else {
        syslog(LOG_WARNING, "pthread_create for discrete FRU %d succeed\n", fru);
      }
#endif /* DEBUG */
      sleep(1);

    }
  }
  /* Sensor Health */
  if (pthread_create(&sensor_health, NULL, snr_health_monitor, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for sensor health failed\n");
  }
#ifdef DEBUG
  else {
    syslog(LOG_WARNING, "pthread_create for sensor health succeed\n");
  }
#endif /* DEBUG */

#ifdef DEBUG
    int i;
    for (i = 1; i <= MAX_NUM_FRUS; i++)
      print_snr_thread(i, g_snr[i-1]);
#endif /* DEBUG */

  pthread_join(sensor_health, NULL);

  for (fru = 1; fru <= MAX_NUM_FRUS; fru++) {

    if (GETBIT(fru_flag, fru))
      pthread_join(discrete_snr[fru-1], NULL);
  }

  for (fru = 1; fru <= MAX_NUM_FRUS; fru++) {

    if (GETBIT(fru_flag, fru))
      pthread_join(thread_snr[fru-1], NULL);
  }
}


int
main(int argc, void **argv) {
  int dev, rc, pid_file;

  if (argc < 2) {
    print_usage();
    exit(1);
  }

  pid_file = open("/var/run/sensord.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another sensord instance is running...\n");
      exit(-1);
    }
  } else {

    daemon(0,1);
    openlog("sensord", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "sensord: daemon started");

    rc = run_sensord(argc, (char **) argv);
    if (rc < 0) {
      print_usage();
      return -1;
    }
  }
  return 0;
}
