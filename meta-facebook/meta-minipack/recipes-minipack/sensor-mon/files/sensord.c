/*
 * sensord.c - Fetch COMe data and write data to virtual driver
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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <pthread.h>
#include <openbmc/pal.h>

#define COM_E_SYSFS         "/sys/class/i2c-adapter/i2c-5/5-0033/"
#define TEMP_SYSFS          COM_E_SYSFS"temp%d_input"
#define VOL_SYSFS           COM_E_SYSFS"in%d_input"
#define PWR_SYSFS           COM_E_SYSFS"power%d_input"
#define CURR_SYSFS          COM_E_SYSFS"curr%d_input"
#define BYTES_ENTIRE_RECORD 0xFF
#define LAST_RECORD_ID      0xFFFF
#define MIN_POLL_INTERVAL   2
#define STOP_PERIOD         10

sensor_list_t bic_sensor_table[] = {
  /* Threshold sensors */
  {BIC_SENSOR_MB_OUTLET_TEMP, BIC_SENSOR_TEMP},
  {BIC_SENSOR_MB_INLET_TEMP, BIC_SENSOR_TEMP},
  {BIC_SENSOR_PCH_TEMP, BIC_SENSOR_TEMP},
  {BIC_SENSOR_VCCIN_VR_TEMP, BIC_SENSOR_TEMP},
  {BIC_SENSOR_1V05MIX_VR_TEMP, BIC_SENSOR_TEMP},
  {BIC_SENSOR_SOC_TEMP, BIC_SENSOR_TEMP},
  {BIC_SENSOR_SOC_THERM_MARGIN, BIC_SENSOR_TEMP},
  {BIC_SENSOR_VDDR_VR_TEMP, BIC_SENSOR_TEMP},
  {BIC_SENSOR_SOC_DIMMA0_TEMP, BIC_SENSOR_TEMP},
  {BIC_SENSOR_SOC_DIMMB0_TEMP, BIC_SENSOR_TEMP},
  {BIC_SENSOR_SOC_PACKAGE_PWR, BIC_SENSOR_PWR},
  {BIC_SENSOR_VCCIN_VR_POUT, BIC_SENSOR_PWR},
  {BIC_SENSOR_VDDR_VR_POUT, BIC_SENSOR_PWR},
  {BIC_SENSOR_SOC_TJMAX, BIC_SENSOR_TEMP},
  {BIC_SENSOR_P3V3_MB, BIC_SENSOR_VOL},
  {BIC_SENSOR_P12V_MB, BIC_SENSOR_VOL},
  {BIC_SENSOR_P1V05_PCH, BIC_SENSOR_VOL},
  {BIC_SENSOR_P3V3_STBY_MB, BIC_SENSOR_VOL},
  {BIC_SENSOR_P5V_STBY_MB, BIC_SENSOR_VOL},
  {BIC_SENSOR_PV_BAT, BIC_SENSOR_VOL},
  {BIC_SENSOR_PVDDR, BIC_SENSOR_VOL},
  {BIC_SENSOR_P1V05_MIX, BIC_SENSOR_VOL},
  {BIC_SENSOR_1V05MIX_VR_CURR, BIC_SENSOR_CURR},
  {BIC_SENSOR_VDDR_VR_CURR, BIC_SENSOR_CURR},
  {BIC_SENSOR_VCCIN_VR_CURR, BIC_SENSOR_CURR},
  {BIC_SENSOR_VCCIN_VR_VOL, BIC_SENSOR_VOL},
  {BIC_SENSOR_VDDR_VR_VOL, BIC_SENSOR_VOL},
  {BIC_SENSOR_P1V05MIX_VR_VOL, BIC_SENSOR_VOL},
  {BIC_SENSOR_P1V05MIX_VR_Pout, BIC_SENSOR_PWR},
  {BIC_SENSOR_INA230_POWER, BIC_SENSOR_PWR},
  {BIC_SENSOR_INA230_VOL, BIC_SENSOR_VOL},
};

static const int bic_sensor_cnt =
    sizeof(bic_sensor_table) / sizeof(bic_sensor_table[0]);

static void
print_usage() {
    printf("Usage: sensord scm\n");
}

static void
sdr_cache_init(uint8_t fru) {
  int ret;
  int fd;
  uint8_t rlen;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  char *path = NULL;
  char sdr_temp_path[64] = {0};
  char sdr_path[64] = {0};
  char fru_name[16] = {0};

  switch(fru) {
    case FRU_SCM:
      sprintf(fru_name, "%s", "scm");
      break;
    default:
  #ifdef DEBUG
      syslog(LOG_WARNING, "sdr_cache_init: Wrong Slot ID\n");
  #endif
      return;
  }

  sprintf(sdr_temp_path, "/tmp/tsdr_%s.bin", fru_name);
  sprintf(sdr_path, "/tmp/sdr_%s.bin", fru_name);

  ipmi_sel_sdr_req_t req;
  ipmi_sel_sdr_res_t *res = (ipmi_sel_sdr_res_t *) rbuf;

  req.rsv_id = 0;
  req.rec_id = 0;
  req.offset = 0;
  req.nbytes = BYTES_ENTIRE_RECORD;

  // Read Slot0's SDR records and store
  path = sdr_temp_path;
  unlink(path);
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s: open fails for path: %s\n", __func__, path);
    return;
  }

  ret = pal_flock_retry(fd);
  if (ret == -1) {
   syslog(LOG_WARNING, "%s: failed to flock on %s\n", __func__, path);
   close(fd);
   return;
  }

  while (1) {
    ret = bic_get_sdr(IPMB_BUS, &req, res, &rlen);
    if (ret) {
      syslog(LOG_WARNING, "%s: bic_get_sdr returns %d\n", __func__, ret);
      continue;
    }

    sdr_full_t *sdr = (sdr_full_t *)res->data;

    write(fd, sdr, sizeof(sdr_full_t));

    req.rec_id = res->next_rec_id;
    if (req.rec_id == LAST_RECORD_ID) {
      // syslog(LOG_INFO, "This record is LAST record\n");
      break;
    }
  }

  ret = pal_unflock_retry(fd);
  if (ret == -1) {
   syslog(LOG_WARNING, "%s: failed to unflock on %s\n", __func__, path);
   close(fd);
   return;
  }

  close(fd);
  rename(sdr_temp_path, sdr_path);
}

static int
write_to_driver(const char *sysfs, int value) {
  FILE *fp;
  int rc;
  char value_str[10] = {0};

  fp = fopen(sysfs, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open sysfs for write %s\n", sysfs);
#endif
    printf("failed to open sysfs for write %s\n", sysfs);
    return err;
  }

  snprintf(value_str, sizeof(value_str), "%d", value);
  rc = fputs(value_str, fp);
  fclose(fp);

  if (rc < 0) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to write sysfs %s\n", sysfs);
#endif
    printf("failed to write sysfs %s\n", sysfs);
    return ENOENT;
  } else {
    return 0;
  }
}

static void *
snr_monitor(void *arg) {

  uint8_t fru = (uint8_t)(uintptr_t)arg;
  uint8_t temp_cnt;
  uint8_t vol_cnt;
  uint8_t pwr_cnt;
  uint8_t curr_cnt;
  int i;
  int value = 0;
  float fvalue = 0;
  char path_temp[64] = {0};
  char path_vol[64] = {0};
  char path_pwr[64] = {0};
  char path_curr[64] = {0};

  while(1) {
    if (pal_is_fw_update_ongoing(fru)) {
      sleep(STOP_PERIOD);
      continue;
    }

    i = 0;
    temp_cnt = 1;
    vol_cnt = 0;
    pwr_cnt = 1;
    curr_cnt = 1;
    while (i < bic_sensor_cnt) {
      if (pal_sensor_read(fru, bic_sensor_table[i].sensor_num, &fvalue)) {
  #ifdef DEBUG
        syslog(LOG_INFO, "pal_sensor_read %d failed\n",
                bic_sensor_table[i].sensor_num);
  #endif
      } else { /* sensor read success */
        value = (int) (fvalue * 1000);

        switch (bic_sensor_table[i++].sensor_type) {
          case BIC_SENSOR_TEMP:
            snprintf(path_temp, sizeof(path_temp), TEMP_SYSFS, temp_cnt++);
            write_to_driver(path_temp, value);
            break;
          case BIC_SENSOR_VOL:
            snprintf(path_vol, sizeof(path_vol), VOL_SYSFS, vol_cnt++);
            write_to_driver(path_vol, value);
            break;
          case BIC_SENSOR_PWR:
            snprintf(path_pwr, sizeof(path_pwr), PWR_SYSFS, pwr_cnt++);
            write_to_driver(path_pwr, value);
            break;
          case BIC_SENSOR_CURR:
            snprintf(path_curr, sizeof(path_curr), CURR_SYSFS, curr_cnt++);
            write_to_driver(path_curr, value);
            break;
        }
      }
    } /* sensor read loop */
  sleep(MIN_POLL_INTERVAL);
  } /* while loop*/
} /* function definition */

static int
run_sensord(int argc, char **argv) {

  uint8_t fru = 0;
  pthread_t tid_snr_monitor;

  if (!strcmp(argv[1], "scm")) {
    fru = 1;
  }

  sdr_cache_init(fru);

  if (pthread_create(&tid_snr_monitor, NULL, snr_monitor,
      (void*)(uintptr_t)fru) < 0) {
    syslog(LOG_WARNING, "pthread_create for Threshold Sensors for FRU %d failed\n", fru);
#ifdef DEBUG
  } else {
    syslog(LOG_WARNING, "pthread_create for Threshold Sensors for FRU %d succeed\n", fru);
#endif
  }

  pthread_join(tid_snr_monitor, NULL);

  return 0;
}

int main(int argc, char **argv) {

  int rc, pid_file;

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
    syslog(LOG_INFO, "sensord: daemon started");
    rc = run_sensord(argc, argv);

    if (rc < 0) {
      print_usage();
      return -1;
    }
  }
  return 0;
}
