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
#include <openbmc/pal.h>
#include <facebook/fby3_gpio.h>
#include <openbmc/obmc-i2c.h>

#define MAX_NUM_SLOTS       4
#define DELAY_GPIOD_READ    1000000

#define PWR_UTL_LOCK "/var/run/power-util_%d.lock"

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
static bool is_pwrgd_cpu_chagned[MAX_NUM_SLOTS] = {false, false, false, false};
static uint8_t SLOTS_MASK = 0x0;
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

err_t last_recovery_err[] = {
  /* Value indicating last FW Recovery reason. */
  {0x01, "LAST_RECOVERY_PCH_ACTIVE"},
  {0x02, "LAST_RECOVERY_PCH_RECOVERY"},
  {0x03, "LAST_RECOVERY_ME_LAUNCH_FAIL"},
  {0x04, "LAST_RECOVERY_ACM_LAUNCH_FAIL"},
  {0x05, "LAST_RECOVERY_IBB_LAUNCH_FAIL"},
  {0x06, "LAST_RECOVERY_OBB_LAUNCH_FAIL"},
  {0x07, "LAST_RECOVERY_BMC_ACTIVE"},
  {0x08, "LAST_RECOVERY_BMC_RECOVERY"},
  {0x09, "LAST_RECOVERY_BMC_LAUNCH_FAIL"},
  {0x0A, "LAST_RECOVERY_FORCED_ACTIVE_FW_RECOVERY"},
};

err_t last_panic_err[] = {
  /* Value indicating last Panic reason. */
  {0x00, "LAST_PANIC_DEFAULT"},
  {0x01, "LAST_PANIC_PCH_UPDATE_INTENT"},
  {0x02, "LAST_PANIC_BMC_UPDATE_INTENT"},
  {0x03, "LAST_PANIC_BMC_RESET_DETECTED"},
  {0x04, "LAST_PANIC_BMC_WDT_EXPIRED"},
  {0x05, "LAST_PANIC_ME_WDT_EXPIRED"},
  {0x06, "LAST_PANIC_ACM_BIOS_WDT_EXPIRED"},
  {0x07, "LAST_PANIC_RESERVED_1"},
  {0x08, "LAST_PANIC_RESERVED_2"},
  {0x09, "LAST_PANIC_ACM_BIOS_AUTH_FAILED"},
};

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

  // Only monitor RST_PLTRST_BMC_N & RST_RSMRST_BMC_N
  gpios[RST_RSMRST_BMC_N].flag = 1; // CPLD PFR alert pin
  gpios[RST_PLTRST_BMC_N].flag = 1; // Platform reset pin
  for (i = 0; i < MAX_GPIO_PINS; i++) {
    if (gpios[i].flag) {
      gpios[i].ass_val = GET_BIT(gpio_ass_val, i);
      ret = fby3_get_gpio_name(fru, i, gpios[i].name);
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

void
check_pfr_mailbox(uint8_t fru) {
  char path[128];
  int ret = 0, i2cfd = 0, retry=0, index = 0;
  uint8_t tbuf[1] = {0}, rbuf[1] = {0};
  uint8_t tlen = 1, rlen = 1;
  uint8_t rcvy_err = 0, panic_err = 0, major_err = 0, minor_err = 0;
  uint8_t rcvy_cnt = 0, panic_cnt = 0;
  char *rcvy_str = "NA", *panic_str = "NA", *major_str = "NA", *minor_str = "NA";
  // Check SB PFR status
  snprintf(path, sizeof(path), "/dev/i2c-%d", (fru+SLOT_BUS_BASE));
  i2cfd = open(path, O_RDWR);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %s", __func__, path);
  }

  tbuf[0] = RCVY_CNT_OFFSET;
  retry = 0;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      rcvy_cnt = rbuf[0];
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
  }

  tbuf[0] = LAST_RCVY_OFFSET;
  retry = 0;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      rcvy_err = rbuf[0];
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
  }

  tbuf[0] = PANIC_CNT_OFFSET;
  retry = 0;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      panic_cnt = rbuf[0];
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
  }

  tbuf[0] = LAST_PANIC_OFFSET;
  retry = 0;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      panic_err = rbuf[0];
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
  }

  tbuf[0] = MAJOR_ERR_OFFSET;
  retry = 0;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      major_err = rbuf[0];
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
  }

  tbuf[0] = MINOR_ERR_OFFSET;
  retry = 0;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      minor_err = rbuf[0];
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
  }
  if ( i2cfd > 0 ) close(i2cfd);

  if ( rcvy_cnt != 0 ) {
    syslog(LOG_CRIT, "FRU: %d, PFR - Recovery count: %d", fru, rcvy_cnt);
  }

  if ( rcvy_err != 0 ) {
    for (index = 0; index < (sizeof(last_recovery_err)/sizeof(err_t)); index++) {
      if (rcvy_err == last_recovery_err[index].err_id) {
        rcvy_str = last_recovery_err[index].err_des;
        break;
      }
    }
    syslog(LOG_CRIT, "FRU: %d, PFR - Last recovery reason: %s (0x%02X)", fru, rcvy_str, rcvy_err);
  }

  if ( panic_cnt != 0 ) {
    syslog(LOG_CRIT, "FRU: %d, PFR - Panic event count: %d", fru, panic_cnt);
  }

  if ( panic_err != 0 ) {
    for (index = 0; index < (sizeof(last_panic_err)/sizeof(err_t)); index++) {
      if (panic_err == last_panic_err[index].err_id) {
        panic_str = last_panic_err[index].err_des;
        break;
      }
    }
    syslog(LOG_CRIT, "FRU: %d, PFR - Last panic reason: %s (0x%02X)", fru, panic_str, panic_err);
  }

  if ( (major_err != 0) || (minor_err != 0) ) {
    if ( major_err == MAJOR_ERROR_PCH_AUTH_FAILED ) {
      major_str = "MAJOR_ERROR_PCH_AUTH_FAILED";
      for (index = 0; index < minor_auth_size; index++) {
        if (minor_err == minor_auth_error[index].err_id) {
          minor_str = minor_auth_error[index].err_des;
          break;
        }
      }
    } else if ( major_err == MAJOR_ERROR_UPDATE_FROM_PCH_FAILED ) {
      major_str = "MAJOR_ERROR_UPDATE_FROM_PCH_FAILED";
      for (index = 0; index < minor_update_size; index++) {
        if (minor_err == minor_update_error[index].err_id) {
          minor_str = minor_update_error[index].err_des;
          break;
        }
      }
    } else {
      major_str = "unknown major error";
    }

    syslog(LOG_CRIT, "FRU: %d, PFR - Major error: %s (0x%02X), Minor error: %s (0x%02X)", fru, major_str, major_err, minor_str, minor_err);
  }

  if ( i2cfd > 0 ) close(i2cfd);
}

/* Monitor the gpio pins */
static void *
gpio_monitor_poll(void *ptr) {

  int i, ret = 0;
  uint8_t fru = (int)ptr;
  bool is_bmc_ready = false;
  bic_gpio_t revised_pins, n_pin_val, o_pin_val;
  gpio_pin_t *gpios;
  uint8_t chassis_sts[8] = {0};
  uint8_t chassis_sts_len;
  uint8_t power_policy = POWER_CFG_UKNOWN;
  uint8_t bmc_location = 0;
  char pwr_state[256] = {0};

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

  //Init POST status
  gpios[RST_RSMRST_BMC_N].status = GET_BIT(o_pin_val, RST_RSMRST_BMC_N);

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

      //rst old & new pin val
      memset(&o_pin_val, 0, sizeof(o_pin_val));
      memset(&n_pin_val, 0, sizeof(n_pin_val));

      //Normally, BIC can always be accessed except 12V-off or 12V-cycle
      is_bmc_ready = false;
      usleep(DELAY_GPIOD_READ);
      continue;
    }

    //Inform BIOS that BMC is ready
    if ( is_bmc_ready == false ) {
      bic_set_gpio(fru, BMC_READY, 1);
      is_bmc_ready = true;
    }

    // Get CPLD io sts
    cpld_io_sts[fru] = (GET_BIT(n_pin_val, PWRGD_SYS_PWROK)  << 0x0) | \
                       (GET_BIT(n_pin_val, RST_PLTRST_BMC_N) << 0x1) | \
                       (GET_BIT(n_pin_val, RST_RSMRST_BMC_N) << 0x2) | \
                       (GET_BIT(n_pin_val, FM_CPU_MSMI_CATERR_LVT3_N ) << 0x3) | \
                       (GET_BIT(n_pin_val, FM_SLPS3_R_N) << 0x4);

    //check PWRGD_CPU_LVC3_R is changed
    if ( (get_pwrgd_cpu_flag(fru) == false) && 
         (GET_BIT(n_pin_val, PWRGD_CPU_LVC3_R) != GET_BIT(o_pin_val, PWRGD_CPU_LVC3_R))) {
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
    
          if (i == RST_RSMRST_BMC_N) {
            printf("RST_RSMRST_BMC_N is ASSERT !\n");
          } else if (i == RST_PLTRST_BMC_N) {
            rst_timer(fru);
          } 
          
        } else {
          if (i == RST_RSMRST_BMC_N) {
            printf("RST_RSMRST_BMC_N is DEASSERT !\n");

            //get power restore policy
            //defined by IPMI Spec/Section 28.2.
            pal_get_chassis_status(fru, NULL, chassis_sts, &chassis_sts_len);

            //byte[1], bit[6:5]: power restore policy
            power_policy = (*chassis_sts >> 5);

            //Check power policy and last power state
            if (power_policy == POWER_CFG_LPS) {
              //if (!last_ps) {
              pal_get_last_pwr_state(fru, pwr_state);
              //last_ps = pwr_state;
              //}
              if (!(strcmp(pwr_state, "on"))) {
                sleep(3);
                if ( bmc_location != NIC_BMC) {
                  pal_server_set_nic_power(SERVER_POWER_ON);
                }
                pal_set_server_power(fru, SERVER_POWER_ON);
              }
            }
            else if (power_policy == POWER_CFG_ON) {
              sleep(3);
              if ( bmc_location != NIC_BMC) {
                pal_server_set_nic_power(SERVER_POWER_ON);
              }
              pal_set_server_power(fru, SERVER_POWER_ON);
            }

            check_pfr_mailbox(fru);
          } else if (i == RST_PLTRST_BMC_N) {
            rst_timer(fru);
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
host_pwr_mon() {
#define MAX_NIC_PWR_RETRY   15
#define POWER_ON_DELAY       2
#define NON_PFR_POWER_OFF_DELAY  -2
#define PFR_POWER_OFF_DELAY     -60
  char path[64] = {0};
  uint8_t host_off_flag = 0;
  uint8_t is_util_run_flag = 0;
  uint8_t fru = 0;
  uint8_t bmc_location = 0;
  bool nic_pwr_set_off = false;
  int i = 0;
  int retry = 0;
  long int tick = 0;
  int power_off_delay = NON_PFR_POWER_OFF_DELAY;

  pthread_detach(pthread_self());

  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    syslog(LOG_WARNING, "Failed to get the location of BMC");
    bmc_location = NIC_BMC;//default value
  }

  for ( i = 0; i < MAX_NUM_SLOTS; i++ ) {
    if ( ((SLOTS_MASK >> i) & 0x1) != 0x1) continue; // skip since fru${i} is not present
    fru = i + 1;
    if (pal_is_slot_pfr_active(fru) == PFR_ACTIVE) {
      power_off_delay = PFR_POWER_OFF_DELAY;
    }
  }

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
          syslog(LOG_CRIT, "FRU: %d, System powered OFF", fru);
          pal_set_last_pwr_state(fru, "off");
        }
        host_off_flag |= 0x1 << i;
      } else if ( tick >= POWER_ON_DELAY ) {
        if ( (get_pwrgd_cpu_flag(fru) == true) && (tick == POWER_ON_DELAY) ) {
          syslog(LOG_CRIT, "FRU: %d, System powered ON", fru);
          pal_set_last_pwr_state(fru, "on");
        }
        host_off_flag &= ~(0x1 << i);
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
        syslog(LOG_CRIT, "NIC Power is set to VAUX");
        nic_pwr_set_off = true;
      }
    } else if ( retry < MAX_NIC_PWR_RETRY && nic_pwr_set_off == true) {
      syslog(LOG_CRIT, "NIC Power is set to VMAIN");
      nic_pwr_set_off = false;
      //Don't change NIC Power to normal here.
      //Because the power seq. of NIC will be affected. We should change it before turning on any one of the slots.
    }

    usleep(DELAY_GPIOD_READ);
  }
}

static void
print_usage() {
  printf("Usage: gpiod [ %s ]\n", pal_server_list);
}

/* Spawns a pthread for each fru to monitor all the sensors on it */
static void
run_gpiod(int argc, void **argv) {
  int i, ret, slot;
  uint8_t fru_flag, fru;
  pthread_t tid_host_pwr_mon;
  pthread_t tid_gpio[MAX_NUM_SLOTS];
  pthread_t tid_cpld_io_mon;

  /* Check for which fru do we need to monitor the gpio pins */
  fru_flag = 0;
  for (i = 1; i < argc; i++) {
    ret = pal_get_fru_id(argv[i], &fru);
    if (ret < 0) {
      print_usage();
      exit(-1);
    }

    SLOTS_MASK |= 0x1 << (fru - 1);

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
