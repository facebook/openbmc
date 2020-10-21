/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>
#include <openbmc/sdr.h>
#include <openbmc/obmc-i2c.h>

#define ADM1278_NAME  "adm1278"
#define ADM1278_ADDR 0x10
#define SCM_ADM1278_BUS 16

#define INTERVAL_MAX  5
#define PIM_RETRY     10

/* Dynamic change lmsensors config for different PIM card type */
static void
add_pim_lmsensor_conf(uint8_t num, uint8_t bus, uint8_t pim_type) {
  char cmd[1024];

  snprintf(cmd, sizeof(cmd),
           "cat /etc/sensors.d/custom/%s.conf"
           " | sed 's/{pimid}/%d/g'"
           " | sed 's/{i2cbus0}/%d/g'"
           " | sed 's/{i2cbus1}/%d/g'"
           " | sed 's/{i2cbus2}/%d/g'"
           " | sed 's/{i2cbus3}/%d/g'"
           " | sed 's/{i2cbus4}/%d/g'"
           " | sed 's/{i2cbus5}/%d/g'"
           " | sed 's/{i2cbus6}/%d/g'"
           " | sed 's/{i2cbus7}/%d/g'"
           " > /etc/sensors.d/pim%d.conf",
           pim_type == PIM_TYPE_16O ? "PIM16O" : "PIM16Q", num,
           bus, bus + 1, bus + 2, bus + 3,
           bus + 4, bus + 5, bus + 6, bus + 7, num);

  run_command(cmd);
}

static void
remove_pim_lmsensor_conf(uint8_t num) {
  char path[PATH_MAX];

  snprintf(path, sizeof(path), "/etc/sensors.d/pim%d.conf", num);
  if (unlink(path))
    syslog(LOG_ERR, "%s: Failed to remove %s", __func__, path);
}

static int
write_adm1278_conf(uint8_t bus, uint8_t addr, uint8_t cmd, uint16_t data) {
  int dev = -1, read_back = -1;

  dev = i2c_cdev_slave_open(bus, addr, I2C_SLAVE_FORCE_CLAIM);
  if (dev < 0) {
    syslog(LOG_ERR, "%s: i2c_cdev_slave_open bus %d addr 0x%x failed", __func__,
           bus, addr);
    return dev;
  }

  i2c_smbus_write_word_data(dev, cmd, data);
  read_back = i2c_smbus_read_word_data(dev, cmd);
  if (read_back < 0) {
    close(dev);
    syslog(LOG_ERR, "%s: i2c_smbus_read_word_data failed", __func__);
    return read_back;
  }

  if (((uint16_t) read_back) != data) {
    syslog(LOG_WARNING, "%s: data isn't expectd value (%04X, %04X)",
                        __func__, data, read_back);
    close(dev);
    return -1;
  }

  close(dev);
  return 0;
}

static void
pim_device_detect_log(uint8_t num, uint8_t bus, uint8_t addr,
                      char* device, int state) {
  switch (state) {
    case I2C_BUS_ERROR:
    case I2C_FUNC_ERROR:
      syslog(LOG_CRIT, "PIM %d Bus:%d get fail", num, bus);
      break;
    case I2C_DEVICE_ERROR:
      syslog(LOG_CRIT, "PIM %d Bus:%d Addr:0x%x Device:%s get fail",
             num, bus, addr, device);
      break;
    case I2c_DRIVER_EXIST:
      syslog(LOG_WARNING, "PIM %d Bus:%d Addr:0x%x Device:%s driver exist",
             num, bus, addr, device);
      break;
  }
}

static void
pim_driver_add(uint8_t num, uint8_t pim_type) {
/*         PIM16Q      PIM16O
 * CH1     DOMFPGA     DOMFPGA
 * CH2     EEPROM      EEPROM
 * CH3     LM75        LM75
 * CH4     LM75        LM75
 * CH5     ADM1278     ADM1278
           LM75         -----
 * CH6     DOMFPGA     DOMFPGA
 * CH7     UCD90160    UCD90160
 * CH8      -----      SI5391B
*/
  int state;
  uint8_t bus = ((num - 1) * 8) + 80;
  add_pim_lmsensor_conf(num, bus, pim_type);

  state = pal_detect_i2c_device(bus + 4, ADM1278_ADDR, MODE_AUTO, 1);
  if (state) {
    pim_device_detect_log(num, bus + 4, ADM1278_ADDR, ADM1278_NAME, state);
  }

  /* Config ADM1278 power monitor averaging */
  if (write_adm1278_conf(bus + 4, ADM1278_ADDR, 0xD4, 0x3F1E)) {
    syslog(LOG_CRIT, "PIM %d Bus:%d Addr:0x%x Device:%s "
                     "can't config register",
                      num, bus + 4, ADM1278_ADDR, ADM1278_NAME);
  }

  sleep(1);
}

static void
pim_driver_del(uint8_t num, uint8_t pim_type) {
  remove_pim_lmsensor_conf(num);
}

static int
pim_thresh_init_file_check(uint8_t fru) {
  char fru_name[32];
  char fpath[64];

  pal_get_fru_name(fru, fru_name);
  snprintf(fpath, sizeof(fpath), INIT_THRESHOLD_BIN, fru_name);

  return access(fpath, F_OK);
}

int
pim_thresh_init(uint8_t fru) {
  int i, sensor_cnt;
  uint8_t snr_num;
  uint8_t *sensor_list;
  thresh_sensor_t snr[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

  pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
  for (i = 0; i < sensor_cnt; i++) {
    snr_num = sensor_list[i];
    if (sdr_get_snr_thresh(fru, snr_num, &snr[fru - 1][snr_num]) < 0) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pim_thresh_init: sdr_get_snr_thresh for FRU: %d",
             fru);
#endif
      continue;
    }
  }

  if (access(THRESHOLD_PATH, F_OK) == -1) {
    mkdir(THRESHOLD_PATH, 0777);
  }

  if (pal_copy_all_thresh_to_file(fru, snr[fru - 1]) < 0) {
    syslog(LOG_WARNING, "%s: Fail to copy thresh to file for FRU: %d",
           __func__, fru);
    return -1;
  }

  return 0;
}

//clear the pimserial cache in /tmp
static void
clear_pimserial_cache(int pim){
  char pimserial_file[PATH_MAX];
  int ret;

  snprintf(pimserial_file, sizeof(pimserial_file),"/tmp/pim%d_serial.txt", pim);
  if (!access(pimserial_file, F_OK)) {
    syslog(LOG_INFO, "found %s", pimserial_file);
    ret = unlink(pimserial_file);
    if(ret != 0){
      syslog(LOG_CRIT, "deleting  %s failed with errno %s", pimserial_file, strerror(errno));
    }else{
      syslog(LOG_INFO, "%s has been deleted", pimserial_file);
    }
  }
}

// Thread for monitoring pim plug
static void *
pim_monitor_handler(void *unused) {
  uint8_t fru;
  uint8_t num;
  uint8_t ret;
  uint8_t prsnt = 0;
  uint8_t prsnt_ori = 0;
  uint8_t curr_state = 0x00;
  uint8_t pim_type;
  uint8_t pim_pedigree;
  uint8_t pim_type_old[10] = {PIM_TYPE_UNPLUG};
  uint8_t interval[10];
  bool thresh_first[10];

  memset(interval, INTERVAL_MAX, sizeof(interval));
  memset(thresh_first, true, sizeof(thresh_first));
  while (1) {
    for (fru = FRU_PIM1; fru <= FRU_PIM8; fru++) {
      ret = pal_is_fru_prsnt(fru, &prsnt);
      if (ret) {
        goto pim_mon_out;
      }
      /* Get original prsnt state PIM1 @bit0, PIM2 @bit1, ..., PIM8 @bit7 */
      num = fru - FRU_PIM1 + 1;
      prsnt_ori = GETBIT(curr_state, (num - 1));
      /* 1 is prsnt, 0 is not prsnt. */
      if (prsnt != prsnt_ori) {
        if (prsnt) {
          syslog(LOG_WARNING, "PIM %d is plugged in.", num);
          pim_type = pal_get_pim_type(fru, PIM_RETRY);
          pim_driver_add(num, pim_type);

          if (pim_type != pim_type_old[num]) {
            if (pim_type == PIM_TYPE_16Q) {
              if (!pal_set_pim_type_to_file(fru, "16q")) {
                syslog(LOG_INFO, "PIM %d type is 16Q", num);
                pim_type_old[num] = PIM_TYPE_16Q;
              } else {
                syslog(LOG_WARNING,
                       "pal_set_pim_type_to_file: PIM %d set 16Q failed", num);
              }
              if (!pal_set_pim_pedigree_to_file(fru, "none")) {
                syslog(LOG_WARNING,
                      "pal_set_pim_pedigree_to_file: PIM %d For PIM16Q set pedigree to none", num);
              }else{
                syslog(LOG_WARNING,
                      "pal_set_pim_pedigree_to_file: PIM %d set none failed", num);
              }
            } else if (pim_type == PIM_TYPE_16O) {
              if (!pal_set_pim_type_to_file(fru, "16o")) {
                syslog(LOG_INFO, "PIM %d type is 16O", num);
                pim_type_old[num] = PIM_TYPE_16O;
              } else {
                syslog(LOG_WARNING,
                       "pal_set_pim_type_to_file: PIM %d set 16O failed", num);
              }
              pim_pedigree = pal_get_pim_pedigree(fru, PIM_RETRY);
              if(pim_pedigree == PIM_16O_SIMULATE){
                if (!pal_set_pim_pedigree_to_file(fru, "simulate")) {
                  syslog(LOG_INFO, "PIM %d pedigree is simulate version", num);
                }else{
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d set simulate version failed", num);
                }
              }else if(pim_pedigree == PIM_16O_ALPHA1){
                if (!pal_set_pim_pedigree_to_file(fru, "alpha1")) {
                  syslog(LOG_INFO, "PIM %d pedigree is alpha1 version", num);
                }else{
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d set alpha1 version failed", num);
                }
              }else if(pim_pedigree == PIM_16O_ALPHA2){
                if (!pal_set_pim_pedigree_to_file(fru, "alpha2")) {
                  syslog(LOG_INFO, "PIM %d pedigree is alpha2 version", num);
                }else{
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d set alpha2 version failed", num);
                }
              }else{
                if (!pal_set_pim_pedigree_to_file(fru, "none")) {
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d set pedigree none", num);
                }else{
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d fail to set pedigree", num);
                }
              }
            } else {
              if (!pal_set_pim_type_to_file(fru, "none")) {
                syslog(LOG_CRIT,
                        "PIM %d type cannot detect, DOMFPGA get fail", num);
                pim_type_old[num] = PIM_TYPE_NONE;
              } else {
                syslog(LOG_WARNING,
                      "pal_set_pim_type_to_file: PIM %d set none failed", num);
              }
              if (!pal_set_pim_pedigree_to_file(fru, "none")) {
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d set pedigree none", num);
                }else{
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d set none failed", num);
                }
            }
            pal_set_sdr_update_flag(fru,1);
            if (thresh_first[num] == true) {
              thresh_first[num] = false;
            } else {
              if (pim_thresh_init_file_check(fru)) {
                pim_thresh_init(fru);
              } else {
                pal_set_pim_thresh(fru);
              }
            }
          }
        } else {
          syslog(LOG_WARNING, "PIM %d is unplugged.", num);
          clear_pimserial_cache(num);
          pal_clear_thresh_value(fru);
          pal_set_sdr_update_flag(fru,1);
          pim_driver_del(num, pim_type_old[num]);
          pim_type_old[num] = PIM_TYPE_UNPLUG;
          if (pal_set_pim_type_to_file(fru, "unplug")) {
            syslog(LOG_WARNING,
                  "pal_set_pim_type_to_file: PIM %d set unplug failed", num);
          }
          if (pal_set_pim_pedigree_to_file(fru, "none")) {
            syslog(LOG_WARNING,
                  "pal_get_pim_pedigree: PIM %d set pedigree failed", num);
          }
        }
        /* Set PIM1 prsnt state @bit0, PIM2 @bit1, ..., PIM8 @bit7 */
        curr_state = prsnt ? SETBIT(curr_state, (num - 1))
                           : CLEARBIT(curr_state, (num - 1));
      }
      /* 1 is prsnt, 0 is not prsnt. */
      if (prsnt) {
        if (interval[num] == 0) {
          interval[num] = INTERVAL_MAX;
          pal_set_pim_sts_led(fru);
          msleep(50);
        } else {
          interval[num]--;
        }
      }
    }
pim_mon_out:
    sleep(1);
  }
  return NULL;
}

// Thread for monitoring scm plug
static void *
scm_monitor_handler(void *unused) {
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;
  uint8_t power;

  while (1) {
    ret = pal_is_fru_prsnt(FRU_SCM, &prsnt);
    if (ret) {
      goto scm_mon_out;
    }
    curr = prsnt;
    if (curr != prev) {
      if (curr) {
        // SCM was inserted
        syslog(LOG_WARNING, "SCM Insertion\n");

        ret = pal_get_server_power(FRU_SCM, &power);
        if (ret) {
          goto scm_mon_out;
        }
        if (power == SERVER_POWER_OFF) {
          sleep(3);
          syslog(LOG_WARNING, "SCM power on\n");
          pal_set_server_power(FRU_SCM, SERVER_POWER_ON);
          /* Setup management port LED */
          run_command("/usr/local/bin/setup_mgmt.sh led");
          /* Config ADM1278 power monitor averaging */
          if (write_adm1278_conf(SCM_ADM1278_BUS, ADM1278_ADDR, 0xD4, 0x3F1E)) {
            syslog(LOG_CRIT, "SCM Bus:%d Addr:0x%x Device:%s "
                             "can't config register",
                             SCM_ADM1278_BUS, ADM1278_ADDR, ADM1278_NAME);
          }
          goto scm_mon_out;
        }
      } else {
        // SCM was removed
        syslog(LOG_WARNING, "SCM Extraction\n");
      }
    }
scm_mon_out:
    prev = curr;
    sleep(1);
  }
  return NULL;
}


void
exithandler(int signum) {
  int brd_rev;
  pal_get_board_rev(&brd_rev);
  set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SMB);
  exit(0);
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_pim_monitor;
  pthread_t tid_scm_monitor;
  int rc;
  int pid_file;

  signal(SIGTERM, exithandler);
  pid_file = open("/var/run/front-paneld.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if (rc) {
    if (EWOULDBLOCK == errno) {
      printf("Another front-paneld instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }

  if ((rc = pthread_create(&tid_pim_monitor, NULL,
                           pim_monitor_handler, NULL))) {
    syslog(LOG_WARNING, "failed to create pim monitor thread: %s",
           strerror(rc));
    exit(1);
  }

  if ((rc = pthread_create(&tid_scm_monitor, NULL,
                           scm_monitor_handler, NULL))) {
    syslog(LOG_WARNING, "failed to create scm monitor thread: %s",
           strerror(rc));
    exit(1);
  }


  pthread_join(tid_pim_monitor, NULL);
  pthread_join(tid_scm_monitor, NULL);

  return 0;
}
