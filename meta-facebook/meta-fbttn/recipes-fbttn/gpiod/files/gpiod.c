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
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <facebook/bic.h>
#include <facebook/fbttn_gpio.h>

#define MAX_NUM_SLOTS       1
#define POLLING_INTERVAL    500
#define SOCK_PATH_GPIO      "/tmp/gpio_socket"

#define GPIO_BMC_READY_N    28
#define GPIO_ML_INS         472
#define GPIO_SCC_A_INS      478
#define GPIO_SCC_B_INS      479
#define GPIO_NIC_INS        209
#define GPIO_COMP_PWR_EN    119   // Mono Lake 12V
#define GPIO_PCIE_RESET      203
#define GPIO_IOM_FULL_PWR_EN 215
#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define PWR_UTL_LOCK "/var/run/power-util_%d.lock"

#define SERVER_IS_PRESENT 0
#define SERVER_IS_ABSENT 1
#define SCC_IS_PRESENT 0
#define SCC_IS_ABSENT 1
#define NIC_IS_PRESENT 1
#define NIC_IS_ABSENT 0
#define MAX_ASSERT_CHECK_RETRY 3
#define MAX_DEASSERT_CHECK_RETRY 3

/* To hold the gpio info and status */
typedef struct {
  uint8_t flag;
  uint8_t status;
  uint8_t ass_val;
  char name[32];
} gpio_pin_t;

static gpio_pin_t gpio_slot1[MAX_GPIO_PINS] = {0};

char *fru_prsnt_log_string[2 * MAX_NUM_FRUS] = {
  // slot1, iom, dpb, scc, nic
 "", "ASSERT: Mono Lake missing", "", "", "ASSERT: SCC missing", "ASSERT: NIC is plugged out",
 "DEASSERT: Mono Lake missing", "", "", "DEASSERT: SCC missing", "DEASSERT: NIC is plugged out",
};

/* To get Mono Lake, SCC, and NIC present status */
int get_fru_prsnt(int chassis_type, uint8_t fru) {
  FILE *fp = NULL;
  char path[64] = {0};
  int gpio_num;
  int rc;
  int val;
  int i;

  for (i = 0; i <= chassis_type; i++) {
    switch(fru) {
      case FRU_SLOT1:
        gpio_num = GPIO_ML_INS;
        break;
      case FRU_SCC:
        // Type 5 need to recognize BMC is in which side

        if ( i == 0 ) {  // Type 5
          if (pal_get_iom_location() == IOM_SIDEA) {          // IOMA
            gpio_num = GPIO_SCC_A_INS;
          } else if (pal_get_iom_location() == IOM_SIDEB) {   // IOMB
            gpio_num = GPIO_SCC_B_INS;
          } else {
            gpio_num = GPIO_SCC_A_INS;
          }
        } else {    // Type 7 only
          gpio_num = GPIO_SCC_B_INS;
        }
        break;
      case FRU_NIC:
        gpio_num = GPIO_NIC_INS;
        break;
      default:
        return -2;
    }

    sprintf(path, GPIO_VAL, gpio_num);

    fp = fopen(path, "r");
    if (!fp) {
      return errno;
    }

    rc = fscanf(fp, "%d", &val);
    fclose(fp);
    if (rc != 1) {
      val = ENOENT;
    }
    if ((fru != FRU_SCC) || ((fru == FRU_SCC) && (val != 0))) {
      break;
    }
  }
  return val;
}

/* Retry for FRU present assert */
int check_fru_present_assert (int chassis_type, uint8_t fru, int fru_present_val, int fru_absent_val) {
  int is_present = 0;
  int retry = 0;

  while (retry <= MAX_ASSERT_CHECK_RETRY) {
    is_present = get_fru_prsnt(chassis_type, fru);
    if (is_present == fru_present_val) {
      break;
    }

    msleep(50);
    retry++;
  }

  return is_present;
}

/* Retry for FRU present deassert */
int check_fru_present_deassert (int chassis_type, uint8_t fru, int fru_present_val, int fru_absent_val) {
  int is_present = 0;
  int retry = 0;

  retry = 0;
  while (retry <= MAX_DEASSERT_CHECK_RETRY) {
    is_present = get_fru_prsnt(chassis_type, fru);
    if (is_present == fru_absent_val) {
      break;
    }

    msleep(50);
    retry++;
  }

  return is_present;
}

/* Returns the pointer to the struct holding all gpio info for the fru#. */
static gpio_pin_t *
get_struct_gpio_pin(uint8_t fru) {

  gpio_pin_t *gpios;

  switch (fru) {
    case FRU_SLOT1:
      gpios = gpio_slot1;
      break;
    default:
      syslog(LOG_WARNING, "get_struct_gpio_pin: Wrong SLOT ID %d\n", fru);
      return NULL;
  }

  return gpios;
}

int
enable_gpio_intr_config(uint8_t fru, uint8_t gpio) {
  int ret;

  bic_gpio_config_t cfg = {0};
  bic_gpio_config_t verify_cfg = {0};


  ret =  bic_get_gpio_config(fru, gpio, &cfg);
  if (ret < 0) {
    syslog(LOG_ERR, "enable_gpio_intr_config: bic_get_gpio_config failed"
        "for slot_id: %u, gpio pin: %u", fru, gpio);
    return -1;
  }

  cfg.ie = 1;

  ret = bic_set_gpio_config(fru, gpio, &cfg);
  if (ret < 0) {
    syslog(LOG_ERR, "enable_gpio_intr_config: bic_set_gpio_config failed"
        "for slot_id: %u, gpio pin: %u", fru, gpio);
    return -1;
  }

  ret =  bic_get_gpio_config(fru, gpio, &verify_cfg);
  if (ret < 0) {
    syslog(LOG_ERR, "enable_gpio_intr_config: verification bic_get_gpio_config"
        "for slot_id: %u, gpio pin: %u", fru, gpio);
    return -1;
  }

  if (verify_cfg.ie != cfg.ie) {
    syslog(LOG_WARNING, "Slot_id: %u,Interrupt enabling FAILED for GPIO pin# %d",
        fru, gpio);
    return -1;
  }

  return 0;
}

/* Enable the interrupt mode for all the gpio sensors */
static void
enable_gpio_intr(uint8_t fru) {

  int i, ret;
  gpio_pin_t *gpios;

  gpios = get_struct_gpio_pin(fru);
  if (gpios == NULL) {
    syslog(LOG_WARNING, "enable_gpio_intr: get_struct_gpio_pin failed.");
    return;
  }

  for (i = 0; i < gpio_pin_cnt; i++) {

    gpios[i].flag = 0;

    ret = enable_gpio_intr_config(fru, gpio_pin_list[i]);
    if (ret < 0) {
      syslog(LOG_WARNING, "enable_gpio_intr: Slot: %d, Pin %d interrupt enabling"
          " failed", fru, gpio_pin_list[i]);
      syslog(LOG_WARNING, "enable_gpio_intr: Disable check for Slot %d, Pin %d",
          fru, gpio_pin_list[i]);
    } else {
      gpios[i].flag = 1;
#ifdef DEBUG
      syslog(LOG_WARNING, "enable_gpio_intr: Enabled check for Slot: %d, Pin %d",
          fru, gpio_pin_list[i]);
#endif /* DEBUG */
    }
  }
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

  for(i = 0; i < gpio_pin_cnt; i++) {
    // Only monitor the PWRGOOD_CPU pin
    if (i == PWRGOOD_CPU)
      gpios[gpio_pin_list[i]].flag = 1;
  }


  for(i = 0; i < MAX_GPIO_PINS; i++) {
    if (gpios[i].flag) {
      gpios[i].ass_val = GETBIT(gpio_ass_val, i);
      ret = fbttn_get_gpio_name(fru, i, gpios[i].name);
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
static int
gpio_monitor_poll(uint8_t fru_flag) {
  FILE *fp = NULL;
  int i, ret;
  int is_fru_present = 0;
  uint8_t fru;
  uint32_t revised_pins, n_pin_val, o_pin_val[MAX_NUM_SLOTS + 1] = {0};
  gpio_pin_t *gpios;
  char last_pwr_state[MAX_VALUE_LEN];
  uint8_t curr_pwr_state = -1;
  uint8_t prev_pwr_state = -1;
  uint8_t server_12v_status = 0;

  uint32_t status;
  bic_gpio_t gpio = {0};

  int chassis_type = 0;
  bool is_fru_prsnt[MAX_NUM_FRUS] = {false};
  char vpath_comp_pwr_en[64] = {0};
  char vpath_iom_full_pwr_en[64] = {0};
  char vpath_pwr_util_lock[64] = {0};
  int val;
  int fru_health_last_state = 1;
  int fru_health_kv_state = 1;
  char tmp_health[MAX_VALUE_LEN];
  int is_fru_missing[2] = {0};
  int is_scc_hotplugged[2] = {1,1}; // if [1,1] = SCC is not present since sled-cycle
                                    // if [0,1] = SCC is hot plugged
  // Make is_scc_hotplugged defualt to [1,1], so if SCC is not present since sled-cycle, ML 12V will not be powered off

  /* Check for initial Asserts */
  for (fru = 1; fru <= MAX_NUM_SLOTS; fru++) {
    if (GETBIT(fru_flag, fru) == 0)
      continue;

    // Inform BIOS that BMC is ready
    bic_set_gpio(fru, GPIO_BMC_READY_N, 0);

    ret = bic_get_gpio(fru, &gpio);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "gpio_monitor_poll: bic_get_gpio failed for "
        " fru %u", fru);
#endif
      continue;
    }

    gpios = get_struct_gpio_pin(fru);
    if  (gpios == NULL) {
      syslog(LOG_WARNING, "gpio_monitor_poll: get_struct_gpio_pin failed for"
          " fru %u", fru);
      continue;
    }

    memcpy(&status, (uint8_t *) &gpio, sizeof(status));

    o_pin_val[fru] = 0;

    for (i = 0; i < MAX_GPIO_PINS; i++) {

      if (gpios[i].flag == 0)
        continue;

      gpios[i].status = GETBIT(status, i);

      if (gpios[i].status)
        o_pin_val[fru] = SETBIT(o_pin_val[fru], i);
    }
  }

  chassis_type = (pal_get_sku() >> 6) & 0x1;
  sprintf(vpath_comp_pwr_en, GPIO_VAL, GPIO_COMP_PWR_EN);
  sprintf(vpath_iom_full_pwr_en, GPIO_VAL, GPIO_IOM_FULL_PWR_EN);
  // For checking power-util is running or not
  sprintf(vpath_pwr_util_lock, PWR_UTL_LOCK, FRU_SLOT1);

  // Initialze pervious power status via GPIO PERST
  prev_pwr_state = get_perst_value();

  /* Keep monitoring each fru's gpio pins every 4 * GPIOD_READ_DELAY seconds */

//ML and SCC Board Present Cases
//1. In case the SCC is connected, the server will continue power-on as usual.
//2. In case the SCC is not inserted when AC on, BMC log ASSERT SCC absence and the server will continue power-on as usual.
//3. In case the SCC is hotplugged out (surprise removal), BMC log ASSERT SCC absence and turn off ML-12V.
//4. In case the SCC is hotplugged in (surprise install), BMC log DEASSERT SCC absence and turn on ML-12V and then turn on ML-DC depending on on/off/lps power policy.
//5. In case the Server is hotplugged out (surprise removal), BMC log ASSERT ML absence and turn off ML-12V and IOM-3V3.
//6. In case the Server is hotplugged in (surprise install), BMC log DEASSERT ML absence and turn on ML-12V.

  while(1) {
    // get current health status from kv_store
    memset(tmp_health, 0, MAX_VALUE_LEN);
    ret = pal_get_key_value("fru_prsnt_health", tmp_health);
    if (ret){
      syslog(LOG_ERR, " %s - kv get fru_prsnt_health status failed", __func__);
    }
    fru_health_kv_state = atoi(tmp_health);

    // To detect Mono Lake is present or not
    if (get_fru_prsnt(chassis_type, FRU_SLOT1) == SERVER_IS_ABSENT) {
      // Double check Mono Lake is absent
      is_fru_present = check_fru_present_assert(chassis_type, FRU_SLOT1, SERVER_IS_PRESENT, SERVER_IS_ABSENT);
      if (is_fru_present == SERVER_IS_ABSENT) {
        if (is_fru_prsnt[FRU_SLOT1 - 1] == false) {
          syslog(LOG_CRIT, fru_prsnt_log_string[FRU_SLOT1]);
          is_fru_prsnt[FRU_SLOT1 - 1] = true;
        }
        // Turn off ML HSC 12V and IOM 3V3 when Mono Lake was hot plugged
        read_device(vpath_comp_pwr_en, &val);
        if (val != 0) {
          syslog(LOG_CRIT, "Powering Off Server due to server is absent.");
          pal_set_server_power(FRU_SLOT1, SERVER_12V_OFF);
          write_device(vpath_iom_full_pwr_en, "0");
        }
        pal_err_code_enable(0xE4);
        pal_set_key_value("fru_prsnt_health", "0");
        is_fru_missing[0] = 1;
      }
    } else {
      // Double check Mono Lake is present
      is_fru_present = check_fru_present_deassert(chassis_type, FRU_SLOT1, SERVER_IS_PRESENT, SERVER_IS_ABSENT);
      if (is_fru_present == SERVER_IS_PRESENT) {
        if (is_fru_prsnt[FRU_SLOT1 - 1] == true) {
          
            syslog(LOG_CRIT, fru_prsnt_log_string[MAX_NUM_FRUS + FRU_SLOT1]);
            is_fru_prsnt[FRU_SLOT1 - 1] = false;

            read_device(vpath_comp_pwr_en, &val);
            if ((val != 1) && (is_fru_missing[0] == 1)) {
              syslog(LOG_CRIT, "Due to Server board were pushed in. Power On Server board 12V power.");
              pal_set_server_power(FRU_SLOT1, SERVER_12V_ON);
            }
            is_fru_missing[0] = 0;
        }
        pal_err_code_disable(0xE4);
      }
    }

    // To detect SCC is hotplugged or not
    if (get_fru_prsnt(chassis_type, FRU_SCC) == SCC_IS_ABSENT) {
      // Double check SCC is absent
      is_fru_present = check_fru_present_assert(chassis_type, FRU_SCC, SCC_IS_PRESENT, SCC_IS_ABSENT);
      if (is_fru_present == SCC_IS_ABSENT) {
        if (is_fru_prsnt[FRU_SCC - 1] == false) {
          syslog(LOG_CRIT, fru_prsnt_log_string[FRU_SCC]);
          is_fru_prsnt[FRU_SCC - 1] = true;
        }

        is_scc_hotplugged[0] = is_scc_hotplugged[1];
        is_scc_hotplugged[1] = 1;


        if (get_fru_prsnt(chassis_type, FRU_SCC) == SCC_IS_ABSENT &&  // SCC of current side is absent
            is_scc_hotplugged[0] == 0 &&  // [1,1] = SCC is not present since sled-cycle
            is_scc_hotplugged[1] == 1 ) { // [0,1] = SCC is hotplugged

          // Turn off ML HSC 12V and IOM 3V3 only when SCC is hotplugged
          // If SCC is not present since sled-cycle, keep ML 12V turned on
          read_device(vpath_comp_pwr_en, &val);
          if (val != 0) {
            syslog(LOG_CRIT, "Powering Off Server due to SCC is absent.");
            pal_set_server_power(FRU_SLOT1, SERVER_12V_OFF);
          }
        }
        pal_err_code_enable(0xE7);
        pal_set_key_value("fru_prsnt_health", "0");
        pal_set_key_value("scc_sensor_health", "0");
        is_fru_missing[1] = 1;
      }
    } else {
      // Double check SCC is present
      is_fru_present = check_fru_present_deassert(chassis_type, FRU_SCC, SCC_IS_PRESENT, SCC_IS_ABSENT);
      if (is_fru_present == SCC_IS_PRESENT) {
        is_scc_hotplugged[0] = is_scc_hotplugged[1];
        is_scc_hotplugged[1] = 0;
        if (is_fru_prsnt[FRU_SCC - 1] == true) {
          syslog(LOG_CRIT, fru_prsnt_log_string[MAX_NUM_FRUS + FRU_SCC]);
          is_fru_prsnt[FRU_SCC - 1] = false;

          read_device(vpath_comp_pwr_en, &val);
          if ((val != 1) && (is_fru_missing[1] == 1)) {
            syslog(LOG_CRIT, "Due to SCC were pushed in. Power On Server board 12V power and Power On Server by Power Restore Policy.");
            pal_set_server_power(FRU_SLOT1, SERVER_12V_ON);
            pal_power_policy_control(FRU_SLOT1, NULL);
          }
          is_fru_missing[1] = 0;
        }
        pal_err_code_disable(0xE7);
      }
    }

    // To detect NIC is present or not
    if (get_fru_prsnt(chassis_type, FRU_NIC) == NIC_IS_ABSENT) {
      // Double check NIC is absent
      is_fru_present = check_fru_present_assert(chassis_type, FRU_NIC, NIC_IS_PRESENT, NIC_IS_ABSENT);
      if (is_fru_present == NIC_IS_ABSENT) {
        if (is_fru_prsnt[FRU_NIC - 1] == false) {
          syslog(LOG_CRIT, fru_prsnt_log_string[FRU_NIC]);
          is_fru_prsnt[FRU_NIC - 1] = true;
        }
        pal_err_code_enable(0xE8);
        pal_set_key_value("fru_prsnt_health", "0");
      }
    } else {
      // Double check NIC is present
      is_fru_present = check_fru_present_deassert(chassis_type, FRU_NIC, NIC_IS_PRESENT, NIC_IS_ABSENT);
      if (is_fru_present == NIC_IS_PRESENT) {
        if (is_fru_prsnt[FRU_NIC - 1] == true) {
          syslog(LOG_CRIT, fru_prsnt_log_string[MAX_NUM_FRUS + FRU_NIC]);
          is_fru_prsnt[FRU_NIC - 1] = false;
        }
        pal_err_code_disable(0xE8);
      }
    }

    // If Mono Lake is present, monitor the server power status.
    if (is_fru_prsnt[FRU_SLOT1 - 1] == false) {
      // Get current server power status via GPIO PERST
      curr_pwr_state = get_perst_value();

      if ((prev_pwr_state == SERVER_POWER_ON) && (curr_pwr_state == SERVER_POWER_OFF)) {
        // Check server 12V power status, avoid changing the lps due to 12V-off
        ret = pal_is_server_12v_on(FRU_SLOT1, &server_12v_status);
        if ((ret >= 0) && (server_12v_status == CTRL_ENABLE)) { // 12V-on

          // Each time Server Power On and Reset, BIOS sends a 13~14ms PERST# low pulse to PCIe devices.
          // To filter-out the low pulse to prevent the false power status query,
          // we add recheck in 50ms (add some tolerance).
          msleep(50);
          curr_pwr_state = get_perst_value();
          if (curr_pwr_state == SERVER_POWER_ON) {
            // the signal is not a real power off signal but a power on PERST# pulse
            msleep(POLLING_INTERVAL);
            continue;
          }

          // Change lps only if power-util is NOT running.
          // If power-util is running, the power status might be changed soon.
          fp = fopen(vpath_pwr_util_lock, "r");
          if (fp == NULL) { // File is absent, means power-util is not running
            pal_set_last_pwr_state(FRU_SLOT1, "off");
          } else {
            fclose(fp);
          }
          syslog(LOG_CRIT, "FRU: %d, Server is powered off", FRU_SLOT1);
        } else {
          prev_pwr_state = SERVER_POWER_OFF;
          msleep(POLLING_INTERVAL);
          continue;
        }
      } else if ((prev_pwr_state == SERVER_POWER_OFF) && (curr_pwr_state == SERVER_POWER_ON)) {
        ret = pal_is_server_12v_on(FRU_SLOT1, &server_12v_status);
        if ((ret >= 0) && (server_12v_status == CTRL_ENABLE)) {
          fp = fopen(vpath_pwr_util_lock , "r");
          if (fp == NULL) {
            pal_set_last_pwr_state(FRU_SLOT1, "on");
          } else {
            fclose(fp);
          }
          syslog(LOG_CRIT, "FRU: %d, Server is powered on", FRU_SLOT1);

          //Run check_M2_nvme.sh only when any of the M2_NVMe file is not existing
          if( (access( "/tmp/cache_store/M2_1_NVMe", F_OK ) == -1) || (access( "/tmp/cache_store/M2_2_NVMe", F_OK ) == -1) ) {
            //Check M.2 NVMe surrpot, for fsc M2 sensor valid check
            system("nohup /etc/check_M2_nvme.sh &");
          }
          
          // Inform BIOS that BMC is ready
          bic_set_gpio(FRU_SLOT1, GPIO_BMC_READY_N, 0);
        } else {
          prev_pwr_state = SERVER_POWER_OFF;
          msleep(POLLING_INTERVAL);
          continue;
        }
      } else {
        ret = pal_is_server_12v_on(FRU_SLOT1, &server_12v_status);
        if ((ret >= 0) && (server_12v_status == CTRL_DISABLE)) {  // 12V-off
          curr_pwr_state = SERVER_POWER_OFF;
        }else{
          msleep(POLLING_INTERVAL);
          continue;
        }
      }
      prev_pwr_state = curr_pwr_state;
      msleep(POLLING_INTERVAL);
    } else {
      msleep(POLLING_INTERVAL);
    }

    // If log-util clear all fru, cleaning ML, SCC, and NIC present status
    // After doing it, gpiod will regenerate assert
    if ((fru_health_kv_state != fru_health_last_state) && (fru_health_kv_state == 1)) {
      is_fru_prsnt[FRU_SLOT1 - 1] = false;
      is_fru_prsnt[FRU_SCC - 1] = false;
      is_fru_prsnt[FRU_NIC - 1] = false;
    }
    fru_health_last_state = fru_health_kv_state;
  } /* while loop */
} /* function definition*/


static void
print_usage() {
  printf("Usage: gpiod [ %s ]\n", pal_server_list);
}

/* Spawns a pthread for each fru to monitor all the sensors on it */
static void
run_gpiod(int argc, void **argv) {

  //gpio_monitor();

  int i, ret;
  uint8_t fru_flag, fru;

  /* Check for which fru do we need to monitor the gpio pins */
  fru_flag = 0;
  for (i = 1; i < argc; i++) {
    ret = pal_get_fru_id(argv[i], &fru);
    if (ret < 0) {
      print_usage();
      exit(-1);
    }
    fru_flag = SETBIT(fru_flag, fru);
  }

  gpio_monitor_poll(fru_flag);
}

void iom_3v3_power_cycle (void) {
  char vpath[64] = {0};

  sprintf(vpath, GPIO_VAL, GPIO_IOM_FULL_PWR_EN);
  write_device(vpath, "1");
  write_device(vpath, "0");
  msleep(120);
  write_device(vpath, "1");
}

void iom_3v3_power_off (void) {
  char vpath[64] = {0};

  sprintf(vpath, GPIO_VAL, GPIO_IOM_FULL_PWR_EN);
  write_device(vpath, "0");
}

int get_perst_value (void) {
  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_PCIE_RESET);
  if (read_device(path, &val))
    return -1;

  return val;
}

// It's used for monitoring BMC PERST pin to detect the server power status.
void
*OEM_PE_MON (void *ptr) {
  int value = 1;
  int cnt   = 0;
  int ret   = 0;
  bool is_3v3_power_cycled  = false;
  bool is_iom_full_power_on = false;
  char vpath_server_pwr_on_lock[64] = {0};

  // For checking server_power_on is running or not
  sprintf(vpath_server_pwr_on_lock, SERVER_PWR_ON_LOCK, FRU_SLOT1);
  while (1) {
    value = get_perst_value();
    // If PERST value is from high to low (server power status from on to off)
    if((value == 0) && (is_3v3_power_cycled == false)) {
      msleep(200);
      // Double check the PERST value.
      value = get_perst_value();
      if(value == 0) {
        iom_3v3_power_cycle();
        is_3v3_power_cycled = true;
        is_iom_full_power_on = true;
      }
    } else {
      if(value == 0) {
        // If server is power-off, increase the counter for power off IOM 3V3.
        if (is_iom_full_power_on == true) {
          cnt ++;
          // If PERST value keep low over than 20s, it means server has been power-off.
          // So we power off the IOM 3v3.
          if(cnt > 40) {  // 40 * 500ms = 20000ms = 20s
            cnt = 0;
            // Power off 3V3 only if server_power_on is NOT running.
            ret = access(vpath_server_pwr_on_lock, F_OK);
            if (ret == -1) {
              iom_3v3_power_off();
              is_iom_full_power_on = false;
            }
          }
        }
      } else {
        // If server is power-on, clear the counter.
        cnt = 0;
        is_3v3_power_cycled = false;
      }
    }
    msleep(500);
  }
}

int
main(int argc, void **argv) {
  char vpath[64] = {0};
  int iom_board_id = BOARD_MP;

  if (argc < 2) {
    print_usage();
    exit(-1);
  }

  init_gpio_pins();

  // It's a transition period from EVT to DVT
  // Support PERST monitor on DVT stage or later
  iom_board_id = pal_get_iom_board_id();
  if(iom_board_id != BOARD_EVT) { // except EVT
    pthread_t PE_RESET_MON_ID;
    if(pthread_create(&PE_RESET_MON_ID,NULL,OEM_PE_MON,NULL) != 0) {
      printf("Error creating thread \n");
    }
  }

  run_gpiod(argc, argv);

  return 0;
}
