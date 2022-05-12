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
#include <pthread.h>
#include <assert.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/ipmi.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal.h>
#include <openbmc/ras.h>
#include <facebook/fby35_common.h>

#define POLL_TIMEOUT -1 /* Forever */
#define POC1_BOARD_ID 0x0
#define MAX_BLOCK_TIMEOUT 100
#define MAX_SLED_CYCLE_RETRY 3

static void
log_slot_present(uint8_t slot_id, gpio_value_t value)
{
  // no need to consider Config C, because the HW "PRSNT_MB_BMC_SLOTX_BB_N" is not connected to Config C BMC.

  if ( value == GPIO_VALUE_LOW ) {
    syslog(LOG_CRIT, "slot%d present", slot_id);
  } else if ( value == GPIO_VALUE_HIGH ) {
    syslog(LOG_CRIT, "Abnormal - slot%d not detected", slot_id);
    return;
  }
}

static void
slot_present(gpiopoll_pin_t *gpdesc, gpio_value_t value) {
  uint32_t slot_id;
  uint8_t bmc_location = 0;
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gpdesc);
  assert(cfg);
  sscanf(cfg->description, "GPIOH%u", &slot_id);
  slot_id -= 3;
  log_gpio_change(gpdesc, value, 0, true);
  log_slot_present(slot_id, value);
  pal_is_cable_connect_baseborad(slot_id, value);

  if (fby35_common_get_bmc_location(&bmc_location) < 0) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return;
  }

  if (value == GPIO_VALUE_LOW) {
    pal_check_sled_mgmt_cbl_id(slot_id, NULL, bmc_location);
    
    pal_check_slot_cpu_present(slot_id);
    pal_check_slot_fru(slot_id);
  }
}

static void 
slot_hotplug_setup(uint8_t slot_id) {
#define SCRIPT_PATH "sh /etc/init.d/server-init.sh %d &"
  char cmd[128] = {0};
  int cmd_len = sizeof(cmd);

  snprintf(cmd, cmd_len, SCRIPT_PATH, slot_id);
  if ( system(cmd) != 0 ) {
    syslog(LOG_CRIT, "Failed to run: %s", cmd);
  }
}

static void 
slot_hotplug_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  uint32_t slot_id;
  uint8_t bmc_location = 0;
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gp);
  assert(cfg);
  sscanf(cfg->description, "GPIOH%u", &slot_id);
  slot_id -= 3;
  slot_hotplug_setup(slot_id);
  log_gpio_change(gp, curr, 0, true);
  log_slot_present(slot_id, curr);
  sleep(1);
  pal_is_cable_connect_baseborad(slot_id, curr);

  if (fby35_common_get_bmc_location(&bmc_location) < 0) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return;
  }

  if (curr == GPIO_VALUE_LOW) {
    pal_check_sled_mgmt_cbl_id(slot_id, NULL, bmc_location);

    // Wait for IPMB ready
    sleep(6);
    pal_check_slot_cpu_present(slot_id);
    pal_check_slot_fru(slot_id);
  }
}

static void
issue_slot_ocp_fault_sel(uint8_t slot_id) {
  uint8_t tbuf[32] = {0x00};
  uint8_t rbuf[32] = {0x00};
  uint8_t tlen = 0;
  uint16_t rlen = 0;

  tbuf[0]  = slot_id;
  tbuf[1]  = NETFN_STORAGE_REQ << 2;
  tbuf[2]  = CMD_STORAGE_ADD_SEL;
  tbuf[5]  = 0x02; //record ID
  tbuf[10] = 0x20; //addr 
  tbuf[11] = 0x00;
  tbuf[12] = 0x04;
  tbuf[13] = 0xC9;
  tbuf[14] = 0x46;
  //When the OCP fault is asserted, the slot is shutdown.
  //So, only the assertion event is issued.
  tbuf[15] = 0x6F;
  tbuf[16] = 0x08; //fault event
  tbuf[17] = 0xff;
  tbuf[18] = 0xff; 
  tlen = 19;
  lib_ipmi_handle(tbuf, tlen, rbuf, &rlen);
}

static void
slot_ocp_fault_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  uint32_t slot_id;
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gp);
  assert(cfg);
  // because we don't have consistent shadow name here, we use description instead
  sscanf(cfg->description, "GPION%u", &slot_id);
  slot_id += 1;
  log_gpio_change(gp, curr, 0, true);
  issue_slot_ocp_fault_sel(slot_id);
}

static void
slot_power_control(char *opt) {
  char cmd[64] = "/usr/local/bin/power-util ";
  strcat(cmd, opt);
  if ( system(cmd) != 0 ) {
    syslog(LOG_WARNING, "Failed to do %s", cmd);
  }
}

static void
slot_rst_hndler(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  uint32_t slot_id;
  char pwr_cmd[32];
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gp);
  assert(cfg);
  sscanf(cfg->description, "GPIOF%u", &slot_id);
  slot_id += 1;
  sprintf(pwr_cmd, "slot%u reset", slot_id);
  if ( curr == GPIO_VALUE_LOW ) slot_power_control(pwr_cmd);
  log_gpio_change(gp, curr, 0, true);
}

static void
ocp_nic_hotplug_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  int ret = 0;
  uint8_t fru = 0;
  uint8_t bmc_location = 0;
  char key[MAX_KEY_LEN] = "to_blk_sled_cycle";
  char value[MAX_VALUE_LEN] = {0};
  bool is_log_asserted[MAX_NUM_FRUS + 1] = {false};
  bool is_fru_prsnt[MAX_NUM_FRUS + 1] = {false};
  int timeout = 0;
  int retry = MAX_SLED_CYCLE_RETRY;
  uint8_t num_of_safe_fru = 0;

  //AC-cycle after OCP NIC Inserted.
  if (curr == GPIO_VALUE_LOW) {
    syslog(LOG_CRIT, "OCP NIC Inserted");
    snprintf(value, sizeof(value), "%d", KEY_SET);
    if (kv_set(key, value, 0, 0) < 0) {
      syslog(LOG_WARNING, "%s() Fail to set the key \"%s\"", __func__, key);
      return;
    }
  } else if (curr == GPIO_VALUE_HIGH) {
    syslog(LOG_CRIT, "OCP NIC Removed");
    return;
  } else {
    syslog(LOG_CRIT, "OCP NIC present GPIO invalid value = %d", curr);
    return;
  }

  for (fru = 1; fru <= MAX_NUM_FRUS; fru++) {
    pal_is_fru_prsnt(fru, (uint8_t*)&is_fru_prsnt[fru]);
  }

  timeout = 0;
  do {

    num_of_safe_fru = 0;
    for (fru = 1; fru <= MAX_NUM_FRUS; fru++) {
      if ((is_fru_prsnt[fru] == true) && (pal_can_change_power(fru) == false)) {
        if(is_log_asserted[fru] == false) {
          syslog(LOG_CRIT, "%s() Postpone to do AC-cycle, FRU: %d is working", __func__, fru);
          is_log_asserted[fru] = true;
        }
      } else {
        num_of_safe_fru++;
      }
    }

    if (num_of_safe_fru == MAX_NUM_FRUS) {
      break;
    }

    sleep(3);
    timeout ++;

  } while (timeout < MAX_BLOCK_TIMEOUT);

  
  ret = fby35_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    goto exit;
  }

  switch(bmc_location) {
    case BB_BMC:
      syslog(LOG_CRIT, "SLED_CYCLE is starting due to NIC re-insertion.");
      pal_update_ts_sled();
      sync();
      sleep(2);
      for (retry = MAX_SLED_CYCLE_RETRY; retry > 0; retry--) {
        ret = pal_sled_cycle();
        if (ret == 0) {
          break;
        }
      }
      if (ret < 0) {
        syslog(LOG_WARNING, "SLED_CYCLE failed...");
        goto exit;
      } 
      break;
    case NIC_BMC:
      syslog(LOG_CRIT, "12V_CYCLE is starting due to NIC re-insertion.");
      for (retry = MAX_SLED_CYCLE_RETRY; retry > 0; retry--) {
        ret = pal_set_server_power(FRU_SLOT1, SERVER_12V_CYCLE); //class2 only have slot1
        if (ret == 0) {
          break;
        }
      }
      if (ret < 0) {
        syslog(LOG_WARNING, "12V_CYCLE failed...");
        goto exit;
      } 
      break;
    default:
      syslog(LOG_WARNING, "%s() Unknown location of BMC, location = %d", __func__, bmc_location);
      break;
  }
   
exit:
  snprintf(value, sizeof(value), "%d", KEY_CLEAR); 
  if (kv_set(key, value, 0, 0) < 0) {
    syslog(LOG_WARNING, "%s() Fail to clear the key \"%s\"", __func__, key);
    return;
  }
  return;
}

static void
ocp_nic_init(gpiopoll_pin_t *gp, gpio_value_t value) {
  if (value == GPIO_VALUE_HIGH)
    syslog(LOG_CRIT, "OCP NIC Removed");
}

static void
usb_hotplug_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  int ret = 0, i2cfd = 0, retry=0;
  uint8_t tbuf[1] = {0x08}, rbuf[1] = {0};
  uint8_t tlen = 1, rlen = 1;
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gp);

  assert(cfg);
  sscanf(cfg->description, "GPIOS2");
  log_gpio_change(gp, curr, 0, true);

  if (curr == GPIO_VALUE_LOW) {
    i2cfd = i2c_cdev_slave_open(BB_CPLD_BUS, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
    if ( i2cfd < 0 ) {
      syslog(LOG_WARNING, "Failed to open bus 12\n");
      return;
    }

    retry = 0;
    while (retry < MAX_READ_RETRY) {
      ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, tlen, rbuf, rlen);
      if ( ret < 0 ) {
        retry++;
        msleep(100);
      } else {
        break;
      }
    }
    if (retry == MAX_READ_RETRY) {
      syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    }
    if ( i2cfd > 0 ) close(i2cfd);

    // Do not handle in POC1
    if (rbuf[0] == POC1_BOARD_ID) {
      return;
    }

    gpio_set_value_by_shadow("USB_BMC_EN_R", GPIO_VALUE_LOW);
    msleep(250);
    gpio_set_value_by_shadow("USB_BMC_EN_R", GPIO_VALUE_HIGH);
  }
}

// GPIO table of the class 1
static struct gpiopoll_config g_class1_gpios[] = {
  // shadow, description, edge, handler, oneshot
  {"PRSNT_MB_BMC_SLOT1_BB_N",  "GPIOH4",   GPIO_EDGE_BOTH,     slot_hotplug_hndlr,       slot_present},
  {"PRSNT_MB_SLOT2_BB_N",      "GPIOH5",   GPIO_EDGE_BOTH,     slot_hotplug_hndlr,       slot_present},
  {"PRSNT_MB_BMC_SLOT3_BB_N",  "GPIOH6",   GPIO_EDGE_BOTH,     slot_hotplug_hndlr,       slot_present},
  {"PRSNT_MB_SLOT4_BB_N",      "GPIOH7",   GPIO_EDGE_BOTH,     slot_hotplug_hndlr,       slot_present},
  {"FM_RESBTN_SLOT1_BMC_N",    "GPIOF0",   GPIO_EDGE_BOTH,     slot_rst_hndler,          NULL},
  {"FM_RESBTN_SLOT2_N",        "GPIOF1",   GPIO_EDGE_BOTH,     slot_rst_hndler,          NULL},
  {"FM_RESBTN_SLOT3_BMC_N",    "GPIOF2",   GPIO_EDGE_BOTH,     slot_rst_hndler,          NULL},
  {"FM_RESBTN_SLOT4_N",        "GPIOF3",   GPIO_EDGE_BOTH,     slot_rst_hndler,          NULL},
  {"HSC_FAULT_BMC_SLOT1_N_R",  "GPION0",   GPIO_EDGE_BOTH,     slot_ocp_fault_hndlr,     NULL},
  {"HSC_FAULT_SLOT2_N",        "GPION1",   GPIO_EDGE_BOTH,     slot_ocp_fault_hndlr,     NULL},
  {"HSC_FAULT_BMC_SLOT3_N_R",  "GPION2",   GPIO_EDGE_BOTH,     slot_ocp_fault_hndlr,     NULL},
  {"HSC_FAULT_SLOT4_N",        "GPION3",   GPIO_EDGE_BOTH,     slot_ocp_fault_hndlr,     NULL},
  {"OCP_NIC_PRSNT_BMC_N",      "GPIOC0",   GPIO_EDGE_BOTH,     ocp_nic_hotplug_hndlr,    ocp_nic_init},
  {"P5V_USB_PG_BMC",           "GPIOS2",   GPIO_EDGE_BOTH,     usb_hotplug_hndlr,        NULL},
  {"FAST_PROCHOT_BMC_N_R",     "GPIOM4",   GPIO_EDGE_BOTH,     fast_prochot_hndlr,       fast_prochot_init},
};

// GPIO table of the class 2
static struct gpiopoll_config g_class2_gpios[] = {
  // shadow, description, edge, handler, oneshot
  {"OCP_NIC_PRSNT_BMC_N",      "GPIOC0",   GPIO_EDGE_BOTH,     ocp_nic_hotplug_hndlr,    ocp_nic_init}
};

static void
*ac_button_event() {
  unsigned int gpio_vals = 0x0;
  /*
  Baseboard AC button:
  - Press it more than 4s or will do nothing.

  Server board AC on/ off/ cycle button:
  - Pwr cycle: press it more than 4s
  - Pwr off: press it more than 8s
  - Pwr on: press it more than 1s and <= 4s
  */
  enum {
    BTN_BMC = 0x0,
    BTN_SLOT1,
    BTN_SLOT2,
    BTN_SLOT3,
    BTN_SLOT4,
    BTN_ARRAY_SIZE,

    /*Get cmd_str*/
    PWR_12V_OFF = 0x0,
    PWR_12V_ON,
    PWR_12V_CYCLE,
    SLED_CYCLE,
    UNKNOWN_PWR_ACTION,
  };
  const char *cmd_list[] = {
    "slot%c 12V-off",
    "slot%c 12V-on",
    "slot%c 12V-cycle",
    "sled%ccycle"
  };
  const char *shadows[] = {
    "BB_BUTTON_BMC_CO_N_R",
    "AC_ON_OFF_BTN_BMC_SLOT1_N_R",
    "AC_ON_OFF_BTN_SLOT2_N",
    "AC_ON_OFF_BTN_BMC_SLOT3_N_R",
    "AC_ON_OFF_BTN_SLOT4_N"
  };
  uint8_t action[BTN_ARRAY_SIZE] = {0};
  bool is_asserted[BTN_ARRAY_SIZE] = {0};
  int time_elapsed[BTN_ARRAY_SIZE] = {0};
  int i = 0;
  uint8_t pwr_sts = 0;

  pthread_detach(pthread_self());

  memset(action, UNKNOWN_PWR_ACTION, BTN_ARRAY_SIZE);
  memset(is_asserted, false, BTN_ARRAY_SIZE);

  //setup the default status
  if ( gpio_get_value_by_shadow_list(shadows, ARRAY_SIZE(shadows), &gpio_vals) < 0 ) {
    syslog(LOG_CRIT, "AC_ON_OFF buttons are not functional");
    pthread_exit(0);
  }

  //start to poll each pin
  while (1) {
    if ( gpio_get_value_by_shadow_list(shadows, ARRAY_SIZE(shadows), &gpio_vals) < 0 ) {
      syslog(LOG_WARNING, "Could not get the current values of all AC_ON_OFF buttons");
      sleep(1);
      continue;
    }

    for ( i = BTN_BMC; i < BTN_ARRAY_SIZE; i++ ) {
      if ( GETBIT(gpio_vals, i) == GPIO_VALUE_LOW ) {
        if ( is_asserted[i] == false ) {
          syslog(LOG_CRIT, "ASSERT: %s", shadows[i]);
          is_asserted[i] = true;
        }
        time_elapsed[i]++;
      } else if ( GETBIT(gpio_vals, i) == GPIO_VALUE_HIGH && time_elapsed[i] > 0 ) {
        if ( is_asserted[i] == true ) {
           syslog(LOG_CRIT, "DEASSERT: %s", shadows[i]);
           is_asserted[i] = false;
        }

        if ( i == BTN_BMC ) {
          if ( time_elapsed[i] > 4 ) {
            action[i] = SLED_CYCLE;
          }
        } else {
          pal_get_server_12v_power(i, &pwr_sts);
          if (pwr_sts == SERVER_12V_OFF && time_elapsed[i] >= 1) {
             //pwr 12V on since time_elpased >=1
             action[i] = PWR_12V_ON;
          } else if ( time_elapsed[i] > 4 ) {
            if ( time_elapsed[i] <= 8 ) {
              //pwr 12V cycle since 4 < time_elapsed <= 8
              action[i] = PWR_12V_CYCLE;
            } else {
              //pwr 12V off since time_elapsed > 8
              action[i] = PWR_12V_OFF;
            }
          }
        }

        if ( action[i] != UNKNOWN_PWR_ACTION ) {
          char opt[16] = {0};
          char c = 0;
          if ( action[i] == SLED_CYCLE ) {
            c = 0x2D; //2Dh is `-` in ASCII
          } else {
            c = 0x30; //30h is `1` in ASCII
          }
          snprintf(opt, sizeof(opt), cmd_list[action[i]], (c == 0x2D)?c:(c+i));
          slot_power_control(opt);
          action[i] = UNKNOWN_PWR_ACTION;
        }
        time_elapsed[i] = 0;
      }
    }

    //sleep periodically.
    sleep(1);
  }

  return NULL;
}

static void
init_class1_threads() {
  pthread_t tid_ac_button_event;

  //Create a thread to monitor the button
  if ( pthread_create(&tid_ac_button_event, NULL, ac_button_event, NULL) < 0 ) {
    syslog(LOG_WARNING, "Failed to create pthread_create for ac_button_event");
    exit(-1);
  }
}

static void
init_class2_threads() {
  //do nothing
}

int
main(int argc, char **argv) {
  int rc = 0;
  int pid_file = 0;
  uint8_t bmc_location = 0;
  struct gpiopoll_config *gpiop = NULL;
  size_t gpio_cnt = 0;
  gpiopoll_desc_t *polldesc = NULL;

  pid_file = open("/var/run/gpiointrd.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if (EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another gpiointrd instance is running...\n");
      exit(-1);
    }
  } else {

    if ( fby35_common_get_bmc_location(&bmc_location) < 0 ) {
      syslog(LOG_WARNING, "Failed to get the location of BMC!");
      exit(-1);
    }

    if ( bmc_location == BB_BMC ) {
      gpiop = g_class1_gpios;
      gpio_cnt = sizeof(g_class1_gpios)/sizeof(g_class1_gpios[0]);
      init_class1_threads();
    } else {
      gpiop = g_class2_gpios;
      gpio_cnt = sizeof(g_class2_gpios)/sizeof(g_class2_gpios[0]);
      init_class2_threads();
    }

    if ( gpio_cnt > 0 ) {
      openlog("gpiointrd", LOG_CONS, LOG_DAEMON);
      syslog(LOG_INFO, "gpiointrd: daemon started");

      // set flag to notice BMC gpiointrd is ready
      kv_set("flag_gpiointrd", STR_VALUE_1, 0, 0);

      // set flag to notice BMC gpiod fru_missing_monitor is ready
      kv_set("flag_gpiod_fru_miss", STR_VALUE_1, 0, 0);

      polldesc = gpio_poll_open(gpiop, gpio_cnt);
      if ( polldesc == NULL ) {
        syslog(LOG_CRIT, "Cannot start poll operation on GPIOs");
      } else {
        if (gpio_poll(polldesc, POLL_TIMEOUT)) {
          syslog(LOG_CRIT, "Poll returned error");
        }

        // clear the flag
        kv_set("flag_gpiointrd", STR_VALUE_0, 0, 0);

        gpio_poll_close(polldesc);
      }
    }
  }

  return 0;
}
