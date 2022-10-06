/*
 *
 * Copyright 2021-present Facebook. All Rights Reserved.
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
#include <openbmc/pal.h>
#include <openbmc/pal_sensors.h>
#include <openbmc/misc-utils.h>
#include <openbmc/sdr.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/libgpio.h>

#define PIM_RETRY           10
#define PATH_MAX_SIZE       100
#define PIM_DPM_ADDR        0x4e
#define SEC_CMD             0xf1
#define SEC_MASK_CMD        0xf2
#define WRITE_PROTECT_REG   0x10
#define PIM_FPGA_INIT_DELAY 500000
#define PIM_POWER_ON_DELAY  100000
#define PIM_ISL_ADDR        0x54
#define TPS546D24_UPPER     0x16
#define TPS546D24_LOWER     0x18
#define WRITE_PROT_VAL      0x80

#define RETRY_COUNT 3
#define RETRY( func )                 \
  ({                                  \
    int attempt = RETRY_COUNT;        \
    int ret;                          \
    while (attempt-- )                \
      if ( (ret = func) == 0 ) break; \
    ret;                              \
  })

static int
pim_thresh_init_file_check(uint8_t fru) {
  char fru_name[32];
  char fpath[PATH_MAX_SIZE];

  pal_get_fru_name(fru, fru_name);
  snprintf(fpath, sizeof(fpath), INIT_THRESHOLD_BIN, fru_name);

  return access(fpath, F_OK);
}

int
pim_thresh_init(uint8_t fru) {
  int i, sensor_cnt;
  uint8_t snr_num;
  uint8_t *sensor_list;
  thresh_sensor_t snr[MAX_NUM_FRUS][MAX_SENSOR_NUM + 1] = {0};

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

static int
pim_ispresent(uint8_t fru, uint8_t *status){
  char path[PATH_MAX_SIZE];
  int num = fru - FRU_PIM2 + 2;

  snprintf(path, PATH_MAX_SIZE, "/tmp/.pim%d_reset", num);
  if (access(path, F_OK) == 0) {
    if (unlink(path) != 0) {
      syslog(LOG_CRIT, "Failed to delete PIM %d power cycle cookie file", num);
    }
    *status = 0;
    return 0;
  }
  return pal_is_fru_prsnt(fru, status);
}

// returns 1 when PIM is ready to be powered on
static int
pim_ready_status(uint8_t num, uint8_t pim_type) {
  gpio_value_t old_rst_value;
  gpio_value_t fpga_done_value;
  char fpga_reset_l_shadow[24];
  char fpga_done_shadow[24];
  uint8_t ready = 0;
  int ret;

  // 16Q and 8DDM are the only PIMs with GPIOs
  if (pim_type != PIM_TYPE_16Q && pim_type != PIM_TYPE_8DDM)
    return 0;

  // skip if already powered on
  snprintf(fpga_reset_l_shadow, sizeof(fpga_reset_l_shadow), "PIM%d_FPGA_RESET_L", num);
  old_rst_value = gpio_get_value_by_shadow(fpga_reset_l_shadow);
  if (old_rst_value == 1)
    return 0;

  ret = pal_is_pim_reset(num + FRU_PIM2 - 2, &ready);
  if (ret) {
    syslog(LOG_INFO, "Failed checking the status of PIM_RESET of PIM %d", num);
    return 0;
  }
  if (!ready) {
    syslog(LOG_INFO, "Skipping PIM %d power on. It's still powercycling", num);
    return 0;
  }

  ret = pal_is_pim_fpga_rev_valid(num + FRU_PIM2 - 2, &ready);
  if (ret) {
    syslog(LOG_INFO, "Failed checking the status of PIM_FPGA_REV_MAJOR of PIM %d", num);
    return 0;
  }
  if (!ready) {
    syslog(LOG_INFO, "Skipping PIM %d power on. FPGA loading is not complete", num);
    return 0;
  }

  snprintf(fpga_done_shadow, sizeof(fpga_done_shadow), "PIM%d_FPGA_DONE", num);
  fpga_done_value = gpio_get_value_by_shadow(fpga_done_shadow);
  if (fpga_done_value == 0) {
    syslog(LOG_INFO, "Skipping PIM %d power on. FPGA_DONE is not ready", num);
    return 0;
  }
  return 1;
}

static int
enable_power_security_mode(uint8_t bus) {
  int dev, locked, ret;
  static uint8_t block[32] = {0x6f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xfe, 0xff, 0xff, 0xc2, 0x1f, 0xc7};
  static uint8_t read_data[6];

  dev = i2c_cdev_slave_open(bus, PIM_DPM_ADDR, I2C_SLAVE_FORCE_CLAIM);
  if (dev < 0) {
    syslog(LOG_ERR, "i2c_cdev_slave_open bus %d addr 0x%x failed", bus, PIM_DPM_ADDR);
    return dev;
  }

  // Set security bitmask
  ret = i2c_smbus_write_block_data(dev, SEC_MASK_CMD, 32, block);
  if (ret < 0){
    syslog(LOG_ERR, "Failed to write security bit mask on bus %d addr 0x%x", bus, PIM_DPM_ADDR);
    return ret;
  }

  // Get security
  ret = i2c_smbus_read_block_data(dev, SEC_CMD, read_data);
  if (ret < 0){
    syslog(LOG_ERR, "Failed read security mode status on bus %d", bus);
    return ret;
  }
  locked = read_data[5];

  if (!locked) {
    // Write a new 6-byte password, enabling security
    ret = i2c_smbus_write_i2c_block_data(dev, SEC_CMD, 7, (uint8_t[]){0x06, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36});
    if (ret < 0){
      syslog(LOG_ERR, "Failed to write 6-byte password to %d on bus %d", SEC_CMD, bus);
      return ret;
    }

    i2c_smbus_read_block_data(dev, SEC_CMD, read_data);
    locked = read_data[5];
    if (!locked) {
      syslog(LOG_ERR, "Failed to lock UCD9090B, bus %d addr 0x%x", bus, PIM_DPM_ADDR);
      return 1;
    } else {
      syslog(LOG_INFO, "UCD bus %d addr 0x%x locked successfully", bus, PIM_DPM_ADDR);
      return 0;
    }
  }
  syslog(LOG_INFO, "UCD bus %d addr 0x%x is already locked", bus, PIM_DPM_ADDR);
  return 0;
}

static int
write_protect_reg_write(uint8_t bus, uint8_t addr, uint8_t val) {
  int dev, ret;
  dev = i2c_cdev_slave_open(bus, addr, I2C_SLAVE_FORCE_CLAIM);
  if (dev < 0) {
    syslog(LOG_ERR, "i2c_cdev_slave_open bus %d addr 0x%x failed", bus, addr);
    return dev;
  }

  ret = i2c_smbus_write_byte_data(dev, WRITE_PROTECT_REG, val);
  if(ret < 0){
    syslog(LOG_ERR, "Write to bus %d addr 0x%x failed", bus, addr);
    return ret;
  }
  syslog(LOG_INFO, "Write to bus %d addr 0x%x successed", bus, addr);
  return 0;
}

static int
enable_tps546d24_wp(uint8_t bus, uint8_t addr) {
  return write_protect_reg_write(bus, addr, WRITE_PROT_VAL);
}

static int
maybe_enable_isl_wp(uint8_t bus) {
  char path[PATH_MAX_SIZE];

  /* Check if device was probed and driver was installed. Skip if not */
  snprintf(
      path,
      sizeof(path),
      "/sys/bus/i2c/devices/%d-%04x",
      bus, PIM_ISL_ADDR);

  if (access(path, F_OK) != 0)
    return 0;

  // writing 0x40 disables all writes excep for WRITE_PROTECT, OPERATION, and PAGE
  return write_protect_reg_write(bus, PIM_ISL_ADDR, 0x40);
}

// Put the PIM's FPGA out of reset
static int
power_pim_on(uint8_t num, uint8_t bus) {
  char fpga_reset_l_shadow[24];
  int ret;

  snprintf(fpga_reset_l_shadow, sizeof(fpga_reset_l_shadow), "PIM%d_FPGA_RESET_L", num);
  ret = gpio_set_init_value_by_shadow(fpga_reset_l_shadow, GPIO_VALUE_HIGH); //sets FPGA out of reset
  if (ret != 0){
    syslog(LOG_CRIT, "Failed to write to FPGA_RESET_L of PIM %d", num);
    return 1;
  }

  syslog(LOG_INFO, "Locking PIM %d UCD9090B security", num);
  RETRY(enable_power_security_mode(bus));

  syslog(LOG_INFO, "Enable WRITE_PROTECT on PIM %d TPS/ISL chips", num);
  RETRY(enable_tps546d24_wp(bus, TPS546D24_UPPER));
  RETRY(enable_tps546d24_wp(bus, TPS546D24_LOWER));
  RETRY(maybe_enable_isl_wp(bus));

  syslog(LOG_INFO, "Powered on PIM %d", num);
  return 0;
}

// Exports GPIO if it doesn't already exist
static void
export_pim_gpio(
  uint8_t num,
  const char* chip,
  int offset,
  const char* shadow_suffix) {

  int ret = 0;
  char shadow[32];

  snprintf(shadow, sizeof(shadow), "PIM%d_%s", num, shadow_suffix);

  if (gpio_is_exported(shadow) == false) {
    ret = gpio_export_by_offset(chip, offset, shadow);
  }

  if (ret != 0)
    syslog(LOG_ERR, "Failed to export %s\n", shadow);

}

// Configures GPIO based on PIM type
static void
add_pim_gpios(uint8_t num, uint8_t bus, uint8_t pim_type) {
  char chip[64];
  char drv_path[PATH_MAX_SIZE];

  /* Check if device was probed and driver was installed. Skip if not */
  snprintf(
      drv_path,
      sizeof(drv_path),
      "/sys/bus/i2c/drivers/ucd9000/%d-%04x/gpio",
      bus, PIM_DPM_ADDR);
  if (access(drv_path, F_OK) != 0)
    return;

  /* construct chip name */
  snprintf(chip, sizeof(chip), "%d-%04x", bus, PIM_DPM_ADDR);

  if (pim_type == PIM_TYPE_16Q) {
    export_pim_gpio(num, chip, 22, "FPGA_RESET_L"); // GPIO17 PIN26
    export_pim_gpio(num, chip, 4, "FULL_POWER_EN"); // GPIO9  PIN14
    export_pim_gpio(num, chip, 20, "FPGA_DONE"); // GPIO15 PIN24
    export_pim_gpio(num, chip, 21, "FPGA_INIT"); // GPIO16 PIN25
  } else if (pim_type == PIM_TYPE_8DDM) {
    export_pim_gpio(num, chip, 22, "FPGA_RESET_L"); // GPIO17 PIN26
    export_pim_gpio(num, chip, 8, "FULL_POWER_EN"); // GPIO1  PIN22
    export_pim_gpio(num, chip, 20, "FPGA_DONE"); // GPIO15 PIN24
    export_pim_gpio(num, chip, 21, "FPGA_INIT"); // GPIO16 PIN25
  } else if (pim_type == PIM_TYPE_16Q2) {
    syslog(LOG_INFO, "PIM %d has no GPIOs", num);
    return;
  } else {
    syslog(LOG_CRIT, "Could not configure PIM %d GPIOs as type is none", num);
    return;
  }
  syslog(LOG_INFO, "Configured PIM %d GPIOs with GPIO chip %s", num, chip);
}

// Dynamic change lmsensors config for different PIM card type
static void
add_pim_lmsensor_conf(uint8_t num, uint8_t bus, uint8_t pim_type) {
  char cmd[256];
  char template_path[PATH_MAX_SIZE];
  char pattern[PATH_MAX_SIZE];

  if (pim_type == PIM_TYPE_16Q) {
    sprintf(template_path, "/etc/sensors.d/.pim16q.conf" );
  } else if (pim_type == PIM_TYPE_16Q2) {
    sprintf(template_path, "/etc/sensors.d/.pim16q2.conf" );
  } else if (pim_type == PIM_TYPE_8DDM) {
    sprintf(template_path, "/etc/sensors.d/.pim8ddm.conf" );
  } else {
    return;
  }

  snprintf(pattern, sizeof(pattern), "s/{bus}/%d/g; s/{pim}/%d/g",
           bus, num);
  snprintf(cmd, sizeof(cmd), "sed '%s' '%s' > '/etc/sensors.d/pim%d.conf'",
           pattern, template_path, num);
  syslog(LOG_INFO, "Creating lmsensor configs file for PIM %d", num);
  run_command(cmd);

}

// Remove PIM sensor config file
static void
remove_pim_lmsensor_conf(uint8_t num) {
  char config_path[PATH_MAX_SIZE];
  snprintf(config_path, sizeof(config_path), "/etc/sensors.d/pim%d.conf", num);

  if (access(config_path, F_OK) == 0) {
    if (unlink(config_path) != 0) {
      syslog(LOG_CRIT, "Failed to delete PIM %d lmsensor configs", num);
    } else {
      syslog(LOG_INFO, "Removed lmsensor configs of PIM %d", num);
    }
  }
}

// Clear pimserial cache
void
clear_pimserial_cache(uint8_t num) {
  char pimserial_cache_path[PATH_MAX_SIZE];
  snprintf(pimserial_cache_path, sizeof(pimserial_cache_path), "/tmp/pim%d_serial.txt", num);

  if (access(pimserial_cache_path, F_OK) == 0) {
    // Pim device doesn't exist but pimserial cache txt file exist, so remove it
    if (unlink(pimserial_cache_path) != 0) {
      syslog(LOG_CRIT, "Failed to delete pimserial cache");
    }else{
      syslog(LOG_INFO, "Removed serial cache of PIM %d", num);
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
  uint8_t bus;
  uint8_t pim_type_old[10] = {PIM_TYPE_UNPLUG};
  uint8_t pim_status[10];
  bool thresh_first[10];

  memset(thresh_first, true, sizeof(thresh_first));
  while (1) {
    for (fru = FRU_PIM2; fru <= FRU_PIM9; fru++) {
      ret = pim_ispresent(fru, &prsnt);
      if (ret) {
        goto pim_mon_out;
      }
      /* Get original prsnt state PIM2 @bit0, PIM3 @bit1, ..., PIM9 @bit7 */
      num = fru - FRU_PIM2 + 2;
      prsnt_ori = GETBIT(curr_state, (num - 2));
      /* Get bus id of the pim */
      bus = get_pim_i2cbus(fru);
      /* 1 is prsnt, 0 is not prsnt. */
      if (prsnt != prsnt_ori) {
        if (prsnt) {
          syslog(LOG_WARNING, "PIM %d is plugged in.", num);
          pim_type = pal_get_pim_type(fru, PIM_RETRY);
          add_pim_lmsensor_conf(num, bus, pim_type);
          add_pim_gpios(num, bus, pim_type);

          if (pim_type != pim_type_old[num]) {
            if (pim_type == PIM_TYPE_16Q2) {
              if (!pal_set_pim_type_to_file(fru, "16q2")) {
                syslog(LOG_INFO, "PIM %d type is 16Q2", num);
                pim_type_old[num] = PIM_TYPE_16Q2;
              } else {
                syslog(LOG_WARNING,
                       "pal_set_pim_type_to_file: PIM %d set 16Q2 failed", num);
              }
            } else if (pim_type == PIM_TYPE_16Q) {
              if (!pal_set_pim_type_to_file(fru, "16q")) {
                syslog(LOG_INFO, "PIM %d type is 16Q", num);
                pim_type_old[num] = PIM_TYPE_16Q;
              } else {
                syslog(LOG_WARNING,
                       "pal_set_pim_type_to_file: PIM %d set 16Q failed", num);
              }
            } else if (pim_type == PIM_TYPE_8DDM) {
              if (!pal_set_pim_type_to_file(fru, "8ddm")) {
                syslog(LOG_INFO, "PIM %d type is 8DDM", num);
                pim_type_old[num] = PIM_TYPE_8DDM;
              } else {
                syslog(LOG_WARNING,
                       "pal_set_pim_type_to_file: PIM %d set 8DDM failed", num);
              }
            } else {
              if (!pal_set_pim_type_to_file(fru, "none")) {
                syslog(LOG_CRIT,
                        "PIM %d type cannot detect, EEPROM get fail", num);
                pim_type_old[num] = PIM_TYPE_NONE;
              } else {
                syslog(LOG_WARNING,
                      "pal_set_pim_type_to_file: PIM %d set none failed", num);
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
          syslog(LOG_WARNING, "PIM %d is unplugged", num);
          pal_clear_thresh_value(fru);
          pal_set_sdr_update_flag(fru,1);
          pim_type_old[num] = PIM_TYPE_UNPLUG;
          if (pal_set_pim_type_to_file(fru, "unplug")) {
            syslog(LOG_WARNING,
                  "pal_set_pim_type_to_file: PIM %d set unplug failed", num);
          }
          clear_pimserial_cache(num); // Clear pimserial endpoint cache
          remove_pim_lmsensor_conf(num);
          pim_status[num] = 0;
        }
        /* Set PIM2 prsnt state @bit0, PIM3 @bit1, ..., PIM9 @bit7 */
        curr_state = prsnt ? SETBIT(curr_state, (num - 2))
                           : CLEARBIT(curr_state, (num - 2));
      }

      /* 1 is prsnt, 0 is not prsnt. */
      if (prsnt) {
        if (pim_status[num] == 0 && pim_ready_status(num, pim_type_old[num]) ) {
          if (power_pim_on(num, bus) == 0)
            pim_status[num] = 1;
        }
      }
    }
pim_mon_out:
    sleep(1);
  }
  return NULL;
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_pim_monitor;
  int rc;
  int pid_file;

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

  pthread_join(tid_pim_monitor, NULL);

  return 0;
}
